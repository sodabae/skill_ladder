// SPDX-License-Identifier: GPL-2.0
/*
 * custom_pcie_driver.c
 *
 * Reference/template PCIe driver for NVIDIA Jetson Orin NX
 * (Ubuntu 20.04 "focal" L4T, kernel ~5.10/5.11-tegra).
 *
 * What it does
 * ------------
 *  - Binds to a PCIe endpoint (e.g. an FPGA) by vendor/device ID. The
 *    same endpoint is expected to be described in the device tree as a
 *    child node of the PCIe controller/root-port, carrying:
 *        vendor-id = <...>;
 *        device-id = <...>;
 *        names = "chan0", "chan1", ...;   // up to 32 strings
 *    The kernel's PCI-OF core (drivers/pci/of.c) automatically attaches
 *    that DT node to the matching struct pci_dev as dev->of_node when
 *    the node's "reg" property encodes the device's PCI b:d:f, so this
 *    driver can read "vendor-id"/"device-id"/"names" straight out of
 *    pdev->dev.of_node in probe().
 *
 *  - Creates ONE control character device (e.g. /dev/custom_pcie_ctrl)
 *    exposing two ioctls:
 *        PCIE_IOC_ALLOC_BUF     (in:  __u32 size)
 *            Allocates a DMA-coherent buffer of "size" bytes. Any
 *            previously allocated buffer is freed first.
 *        PCIE_IOC_GET_BUF_ADDR  (out: __u64 addr)
 *            Returns the bus/physical address of the buffer allocated
 *            above, suitable for programming into the endpoint as a
 *            DMA target address.
 *
 *  - mmap() on the control device:
 *        offset == 0            -> maps BAR0 of the PCIe endpoint
 *                                   (device register space)
 *        offset == 0xc0000000   -> maps the DMA buffer allocated via
 *                                   PCIE_IOC_ALLOC_BUF
 *
 *  - Creates up to 32 additional character devices, one per MSI/MSI-X
 *    vector, named after the DT "names" string-list. A blocking
 *    read(2) on one of these devices returns (as a little-endian
 *    uint32_t) the number of interrupts seen on that vector since the
 *    last read.
 *
 * IMPORTANT
 * ---------
 * This is a starting template, not a drop-in binary. You MUST:
 *   1. Set CUSTOM_PCIE_VENDOR_ID / CUSTOM_PCIE_DEVICE_ID (or rely purely
 *      on the DT-based check -- see probe()) to match your endpoint.
 *   2. Confirm which BAR your device registers live on (BAR_INDEX).
 *   3. Confirm your DMA addressing width / dma_set_mask() value.
 *   4. Build against your actual L4T kernel headers
 *      (apt: nvidia-l4t-kernel-headers, or a matching kernel source
 *      tree) -- symbol signatures (e.g. class_create()) are version
 *      sensitive and this file targets the ~5.10/5.11 API shape.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/poll.h>

#define DRV_NAME		"custom_pcie"
#define CTRL_DEV_NAME		"custom_pcie_ctrl"

/* ---- Adjust these to your actual endpoint ---------------------------- */
#define CUSTOM_PCIE_VENDOR_ID	0x10EE	/* placeholder: e.g. Xilinx */
#define CUSTOM_PCIE_DEVICE_ID	0x903F	/* placeholder */
#define BAR_INDEX		0
#define DMA_ADDR_BITS		32	/* adjust to your endpoint's DMA capability */
/* ----------------------------------------------------------------------- */

#define MAX_IRQ_CHANS		32
#define MMAP_BUFFER_OFFSET	0xc0000000UL

/* ---- ioctl interface (also duplicated in the userspace app) ---------- */
#define PCIE_IOC_MAGIC		'F'
#define PCIE_IOC_ALLOC_BUF	_IOW(PCIE_IOC_MAGIC, 1, __u32)
#define PCIE_IOC_GET_BUF_ADDR	_IOR(PCIE_IOC_MAGIC, 2, __u64)
/* ----------------------------------------------------------------------- */

struct custom_pcie_dev;

struct irq_chan {
	struct custom_pcie_dev *cdrv;
	struct cdev		cdev;
	struct device		*device;
	dev_t			devt;
	char			name[32];
	int			irq;
	int			vector_idx;
	wait_queue_head_t	wq;
	atomic_t		irq_count;
	bool			registered;
};

struct custom_pcie_dev {
	struct pci_dev		*pdev;

	void __iomem		*bar;
	resource_size_t		bar_phys;
	resource_size_t		bar_len;

	/* control (ioctl/mmap) char device */
	struct cdev		ctrl_cdev;
	struct device		*ctrl_device;
	dev_t			ctrl_devt;

	/* DMA buffer created by PCIE_IOC_ALLOC_BUF */
	struct mutex		buf_lock;
	void			*buf_virt;
	dma_addr_t		buf_dma;
	size_t			buf_size;

	/* MSI/MSI-X interrupt channels */
	int			num_irqs;
	struct irq_chan		chans[MAX_IRQ_CHANS];

	bool			msix_used;
};

/* Shared class + base devt for all minors: minor 0 = ctrl device,
 * minors 1..MAX_IRQ_CHANS = interrupt channel devices.
 * This template supports a single bound PCIe device instance. */
static struct class *g_class;
static dev_t g_devt_base;
static bool g_bound; /* guards against a 2nd device instance */

/* ------------------------------------------------------------------- */
/* IRQ channel char device fops                                        */
/* ------------------------------------------------------------------- */

static int chan_open(struct inode *inode, struct file *filp)
{
	struct irq_chan *chan = container_of(inode->i_cdev, struct irq_chan, cdev);
	filp->private_data = chan;
	return 0;
}

static ssize_t chan_read(struct file *filp, char __user *ubuf, size_t len, loff_t *off)
{
	struct irq_chan *chan = filp->private_data;
	__u32 count;
	int ret;

	if (len < sizeof(count))
		return -EINVAL;

	if (atomic_read(&chan->irq_count) == 0) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		ret = wait_event_interruptible(chan->wq, atomic_read(&chan->irq_count) != 0);
		if (ret)
			return ret; /* interrupted by a signal */
	}

	count = atomic_xchg(&chan->irq_count, 0);

	if (copy_to_user(ubuf, &count, sizeof(count)))
		return -EFAULT;

	return sizeof(count);
}

static __poll_t chan_poll(struct file *filp, poll_table *wait)
{
	struct irq_chan *chan = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &chan->wq, wait);
	if (atomic_read(&chan->irq_count) != 0)
		mask |= EPOLLIN | EPOLLRDNORM;

	return mask;
}

static const struct file_operations chan_fops = {
	.owner	= THIS_MODULE,
	.open	= chan_open,
	.read	= chan_read,
	.poll	= chan_poll,
	.llseek	= no_llseek,
};

static irqreturn_t custom_pcie_irq_handler(int irq, void *data)
{
	struct irq_chan *chan = data;

	atomic_inc(&chan->irq_count);
	wake_up_interruptible(&chan->wq);

	return IRQ_HANDLED;
}

/* ------------------------------------------------------------------- */
/* Control device fops (ioctl / mmap)                                  */
/* ------------------------------------------------------------------- */

static int ctrl_open(struct inode *inode, struct file *filp)
{
	struct custom_pcie_dev *cdrv = container_of(inode->i_cdev, struct custom_pcie_dev, ctrl_cdev);
	filp->private_data = cdrv;
	return 0;
}

static long ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct custom_pcie_dev *cdrv = filp->private_data;
	void __user *uarg = (void __user *)arg;
	__u32 size;
	__u64 addr;
	int ret = 0;

	switch (cmd) {
	case PCIE_IOC_ALLOC_BUF:
		if (copy_from_user(&size, uarg, sizeof(size)))
			return -EFAULT;
		if (size == 0 || size > (64u * 1024u * 1024u)) /* sane upper bound */
			return -EINVAL;

		mutex_lock(&cdrv->buf_lock);
		if (cdrv->buf_virt) {
			dma_free_coherent(&cdrv->pdev->dev, cdrv->buf_size,
					   cdrv->buf_virt, cdrv->buf_dma);
			cdrv->buf_virt = NULL;
			cdrv->buf_size = 0;
		}

		cdrv->buf_virt = dma_alloc_coherent(&cdrv->pdev->dev, size,
						     &cdrv->buf_dma, GFP_KERNEL);
		if (!cdrv->buf_virt) {
			mutex_unlock(&cdrv->buf_lock);
			return -ENOMEM;
		}
		cdrv->buf_size = size;
		mutex_unlock(&cdrv->buf_lock);
		break;

	case PCIE_IOC_GET_BUF_ADDR:
		mutex_lock(&cdrv->buf_lock);
		if (!cdrv->buf_virt) {
			mutex_unlock(&cdrv->buf_lock);
			return -ENODEV; /* alloc ioctl hasn't been called yet */
		}
		addr = (__u64)cdrv->buf_dma;
		mutex_unlock(&cdrv->buf_lock);

		if (copy_to_user(uarg, &addr, sizeof(addr)))
			return -EFAULT;
		break;

	default:
		ret = -ENOTTY;
	}

	return ret;
}

static int ctrl_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct custom_pcie_dev *cdrv = filp->private_data;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;
	int ret;

	if (offset == MMAP_BUFFER_OFFSET) {
		/* Map the DMA buffer allocated by PCIE_IOC_ALLOC_BUF */
		mutex_lock(&cdrv->buf_lock);
		if (!cdrv->buf_virt) {
			mutex_unlock(&cdrv->buf_lock);
			return -ENODEV;
		}
		if (size > cdrv->buf_size) {
			mutex_unlock(&cdrv->buf_lock);
			return -EINVAL;
		}

		/* dma_mmap_coherent() expects vm_pgoff relative to the start
		 * of the buffer, not the 0xc0000000 sentinel offset we used
		 * to select this path. */
		vma->vm_pgoff = 0;
		ret = dma_mmap_coherent(&cdrv->pdev->dev, vma,
					 cdrv->buf_virt, cdrv->buf_dma, size);
		mutex_unlock(&cdrv->buf_lock);
		return ret;
	}

	/* Default: map BAR_INDEX of the PCIe endpoint (device registers) */
	if (offset != 0)
		return -EINVAL;
	if (size > cdrv->bar_len)
		return -EINVAL;

	vma->vm_pgoff = cdrv->bar_phys >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;

	return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				   size, vma->vm_page_prot);
}

static int ctrl_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations ctrl_fops = {
	.owner		= THIS_MODULE,
	.open		= ctrl_open,
	.release	= ctrl_release,
	.unlocked_ioctl	= ctrl_ioctl,
	.compat_ioctl	= ctrl_ioctl,
	.mmap		= ctrl_mmap,
	.llseek		= no_llseek,
};

/* ------------------------------------------------------------------- */
/* probe / remove                                                      */
/* ------------------------------------------------------------------- */

static void teardown_irq_chans(struct custom_pcie_dev *cdrv)
{
	int i;

	for (i = 0; i < cdrv->num_irqs; i++) {
		struct irq_chan *chan = &cdrv->chans[i];

		if (chan->irq > 0)
			free_irq(chan->irq, chan);
		if (chan->device)
			device_destroy(g_class, chan->devt);
		if (chan->registered)
			cdev_del(&chan->cdev);
	}

	if (cdrv->num_irqs > 0)
		pci_free_irq_vectors(cdrv->pdev);
}

static int setup_irq_chans(struct custom_pcie_dev *cdrv, struct device_node *np)
{
	int num_names, num_vectors, i, ret;
	const char *name;

	num_names = of_property_count_strings(np, "names");
	if (num_names <= 0) {
		dev_warn(&cdrv->pdev->dev,
			 "no valid 'names' string-list in DT node, skipping IRQ channel setup\n");
		return 0;
	}
	if (num_names > MAX_IRQ_CHANS)
		num_names = MAX_IRQ_CHANS;

	num_vectors = pci_alloc_irq_vectors(cdrv->pdev, 1, num_names,
					     PCI_IRQ_MSIX | PCI_IRQ_MSI);
	if (num_vectors < 0) {
		dev_err(&cdrv->pdev->dev, "failed to allocate MSI/MSI-X vectors: %d\n",
			num_vectors);
		return num_vectors;
	}
	cdrv->msix_used = cdrv->pdev->msix_enabled;

	for (i = 0; i < num_vectors; i++) {
		struct irq_chan *chan = &cdrv->chans[i];

		ret = of_property_read_string_index(np, "names", i, &name);
		if (ret) {
			dev_err(&cdrv->pdev->dev, "failed to read names[%d]\n", i);
			goto err;
		}
		strscpy(chan->name, name, sizeof(chan->name));

		chan->cdrv = cdrv;
		chan->vector_idx = i;
		atomic_set(&chan->irq_count, 0);
		init_waitqueue_head(&chan->wq);

		chan->irq = pci_irq_vector(cdrv->pdev, i);
		if (chan->irq < 0) {
			ret = chan->irq;
			goto err;
		}

		chan->devt = MKDEV(MAJOR(g_devt_base), MINOR(g_devt_base) + 1 + i);
		cdev_init(&chan->cdev, &chan_fops);
		chan->cdev.owner = THIS_MODULE;
		ret = cdev_add(&chan->cdev, chan->devt, 1);
		if (ret)
			goto err;
		chan->registered = true;

		chan->device = device_create(g_class, &cdrv->pdev->dev, chan->devt,
					      chan, "%s", chan->name);
		if (IS_ERR(chan->device)) {
			ret = PTR_ERR(chan->device);
			chan->device = NULL;
			goto err;
		}

		ret = request_irq(chan->irq, custom_pcie_irq_handler, 0,
				   chan->name, chan);
		if (ret) {
			dev_err(&cdrv->pdev->dev, "request_irq failed for %s: %d\n",
				chan->name, ret);
			goto err;
		}

		cdrv->num_irqs = i + 1;
	}

	return 0;

err:
	teardown_irq_chans(cdrv);
	cdrv->num_irqs = 0;
	return ret;
}

static int custom_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct custom_pcie_dev *cdrv;
	struct device_node *np = pdev->dev.of_node;
	u32 dt_vendor = 0, dt_device = 0;
	int ret;

	if (g_bound) {
		dev_err(&pdev->dev, "this template supports only one bound device instance\n");
		return -EBUSY;
	}

	/* Cross-check against the device-tree node's vendor-id/device-id,
	 * if present, in addition to the compiled-in pci_device_id match. */
	if (np) {
		of_property_read_u32(np, "vendor-id", &dt_vendor);
		of_property_read_u32(np, "device-id", &dt_device);
		if (dt_vendor && dt_device &&
		    (dt_vendor != pdev->vendor || dt_device != pdev->device)) {
			dev_err(&pdev->dev,
				"DT vendor/device-id (%04x:%04x) does not match PCI id (%04x:%04x)\n",
				dt_vendor, dt_device, pdev->vendor, pdev->device);
			return -ENODEV;
		}
	} else {
		dev_warn(&pdev->dev,
			 "no device-tree node associated with this PCI device; "
			 "IRQ channel names will be unavailable\n");
	}

	cdrv = devm_kzalloc(&pdev->dev, sizeof(*cdrv), GFP_KERNEL);
	if (!cdrv)
		return -ENOMEM;

	cdrv->pdev = pdev;
	mutex_init(&cdrv->buf_lock);
	pci_set_drvdata(pdev, cdrv);

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);

	ret = pci_request_regions(pdev, DRV_NAME);
	if (ret)
		goto err_disable;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(DMA_ADDR_BITS));
	if (ret) {
		dev_err(&pdev->dev, "failed to set DMA mask: %d\n", ret);
		goto err_release_regions;
	}

	cdrv->bar_phys = pci_resource_start(pdev, BAR_INDEX);
	cdrv->bar_len = pci_resource_len(pdev, BAR_INDEX);
	cdrv->bar = pci_iomap(pdev, BAR_INDEX, cdrv->bar_len);
	if (!cdrv->bar) {
		ret = -ENOMEM;
		goto err_release_regions;
	}

	/* Control device: minor 0 */
	cdrv->ctrl_devt = MKDEV(MAJOR(g_devt_base), MINOR(g_devt_base));
	cdev_init(&cdrv->ctrl_cdev, &ctrl_fops);
	cdrv->ctrl_cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdrv->ctrl_cdev, cdrv->ctrl_devt, 1);
	if (ret)
		goto err_unmap;

	cdrv->ctrl_device = device_create(g_class, &pdev->dev, cdrv->ctrl_devt,
					   cdrv, CTRL_DEV_NAME);
	if (IS_ERR(cdrv->ctrl_device)) {
		ret = PTR_ERR(cdrv->ctrl_device);
		goto err_cdev_del;
	}

	if (np) {
		ret = setup_irq_chans(cdrv, np);
		if (ret)
			goto err_ctrl_device_destroy;
	}

	g_bound = true;
	dev_info(&pdev->dev, "%s bound (vendor=%04x device=%04x, %d irq channel(s))\n",
		 DRV_NAME, pdev->vendor, pdev->device, cdrv->num_irqs);

	return 0;

err_ctrl_device_destroy:
	device_destroy(g_class, cdrv->ctrl_devt);
err_cdev_del:
	cdev_del(&cdrv->ctrl_cdev);
err_unmap:
	pci_iounmap(pdev, cdrv->bar);
err_release_regions:
	pci_release_regions(pdev);
err_disable:
	pci_disable_device(pdev);
	return ret;
}

static void custom_pcie_remove(struct pci_dev *pdev)
{
	struct custom_pcie_dev *cdrv = pci_get_drvdata(pdev);

	if (!cdrv)
		return;

	teardown_irq_chans(cdrv);

	device_destroy(g_class, cdrv->ctrl_devt);
	cdev_del(&cdrv->ctrl_cdev);

	mutex_lock(&cdrv->buf_lock);
	if (cdrv->buf_virt) {
		dma_free_coherent(&pdev->dev, cdrv->buf_size, cdrv->buf_virt, cdrv->buf_dma);
		cdrv->buf_virt = NULL;
	}
	mutex_unlock(&cdrv->buf_lock);

	pci_iounmap(pdev, cdrv->bar);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

	g_bound = false;
}

static const struct pci_device_id custom_pcie_ids[] = {
	{ PCI_DEVICE(CUSTOM_PCIE_VENDOR_ID, CUSTOM_PCIE_DEVICE_ID) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, custom_pcie_ids);

static struct pci_driver custom_pcie_driver = {
	.name		= DRV_NAME,
	.id_table	= custom_pcie_ids,
	.probe		= custom_pcie_probe,
	.remove		= custom_pcie_remove,
};

/* ------------------------------------------------------------------- */
/* module init/exit                                                    */
/* ------------------------------------------------------------------- */

static int __init custom_pcie_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&g_devt_base, 0, 1 + MAX_IRQ_CHANS, DRV_NAME);
	if (ret)
		return ret;

	/* NOTE: class_create() takes (owner, name) on the ~5.10/5.11 kernel
	 * API targeted here. On kernel >= 6.4 this becomes class_create(name).
	 * Adjust if you build against a newer kernel tree. */
	g_class = class_create(THIS_MODULE, DRV_NAME);
	if (IS_ERR(g_class)) {
		ret = PTR_ERR(g_class);
		goto err_chrdev;
	}

	ret = pci_register_driver(&custom_pcie_driver);
	if (ret)
		goto err_class;

	return 0;

err_class:
	class_destroy(g_class);
err_chrdev:
	unregister_chrdev_region(g_devt_base, 1 + MAX_IRQ_CHANS);
	return ret;
}

static void __exit custom_pcie_exit(void)
{
	pci_unregister_driver(&custom_pcie_driver);
	class_destroy(g_class);
	unregister_chrdev_region(g_devt_base, 1 + MAX_IRQ_CHANS);
}

module_init(custom_pcie_init);
module_exit(custom_pcie_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Template PCIe driver: DMA buffer ioctl/mmap + per-vector MSI/MSI-X char devices");
