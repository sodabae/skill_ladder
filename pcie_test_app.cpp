// pcie_test_app.cpp
//
// Simple userspace test application for custom_pcie_driver.c.
//
// Flow:
//   1. Open the control device (/dev/custom_pcie_ctrl).
//   2. ioctl(PCIE_IOC_ALLOC_BUF)      -> allocate a DMA buffer in the driver.
//   3. ioctl(PCIE_IOC_GET_BUF_ADDR)   -> get its 64-bit bus address.
//   4. mmap offset 0                  -> map BAR0 (FPGA registers).
//   5. mmap offset 0xc0000000         -> map the DMA buffer itself.
//   6. Write the buffer's bus address into the FPGA's "DMA target
//      address" register at BAR offset 0x1600, telling the FPGA where
//      to write its data.
//   7. Open the interrupt channel device (e.g. /dev/fpga_irq0) and
//      block on read() until the FPGA raises its MSI/MSI-X interrupt
//      (signaling "DMA write to the buffer is complete").
//   8. Read back the data the FPGA wrote into the mmap'ed buffer.
//
// Build:
//   g++ -O2 -Wall -o pcie_test_app pcie_test_app.cpp
//
// Run (as root, or with appropriate udev permissions on the device nodes):
//   ./pcie_test_app

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

// ---- Must match the definitions in custom_pcie_driver.c --------------
#define PCIE_IOC_MAGIC          'F'
#define PCIE_IOC_ALLOC_BUF      _IOW(PCIE_IOC_MAGIC, 1, uint32_t)
#define PCIE_IOC_GET_BUF_ADDR   _IOR(PCIE_IOC_MAGIC, 2, uint64_t)

static const off_t MMAP_BUFFER_OFFSET = 0xc0000000L;
// ------------------------------------------------------------------------

// ---- Adjust to your setup ----------------------------------------------
static const char *CTRL_DEV_PATH   = "/dev/custom_pcie_ctrl";
static const char *IRQ_DEV_PATH    = "/dev/fpga_irq0"; // must match a "names" entry in the DT
static const size_t BAR_MAP_SIZE   = 0x10000;   // BAR window size to map (adjust to your BAR length)
static const uint32_t DMA_BUF_SIZE = 4096;      // size to request from PCIE_IOC_ALLOC_BUF
static const off_t FPGA_DMA_ADDR_REG = 0x1600;  // FPGA register: DMA target address
// --------------------------------------------------------------------------

static void die(const char *what)
{
	std::fprintf(stderr, "%s: %s\n", what, std::strerror(errno));
	std::exit(EXIT_FAILURE);
}

int main()
{
	// 1. Open control device
	int ctrl_fd = open(CTRL_DEV_PATH, O_RDWR);
	if (ctrl_fd < 0)
		die("open(ctrl)");

	// 2. Allocate the DMA buffer
	uint32_t alloc_size = DMA_BUF_SIZE;
	if (ioctl(ctrl_fd, PCIE_IOC_ALLOC_BUF, &alloc_size) < 0)
		die("ioctl(PCIE_IOC_ALLOC_BUF)");
	std::printf("Allocated DMA buffer: %u bytes\n", alloc_size);

	// 3. Get the buffer's bus address
	uint64_t buf_addr = 0;
	if (ioctl(ctrl_fd, PCIE_IOC_GET_BUF_ADDR, &buf_addr) < 0)
		die("ioctl(PCIE_IOC_GET_BUF_ADDR)");
	std::printf("DMA buffer bus address: 0x%016llx\n",
		    static_cast<unsigned long long>(buf_addr));

	// 4. mmap BAR0 (offset 0) -> FPGA register space
	void *bar_map = mmap(nullptr, BAR_MAP_SIZE, PROT_READ | PROT_WRITE,
			      MAP_SHARED, ctrl_fd, 0);
	if (bar_map == MAP_FAILED)
		die("mmap(BAR0)");
	volatile uint8_t *bar = static_cast<volatile uint8_t *>(bar_map);

	// 5. mmap the DMA buffer itself (offset 0xc0000000)
	void *buf_map = mmap(nullptr, alloc_size, PROT_READ | PROT_WRITE,
			      MAP_SHARED, ctrl_fd, MMAP_BUFFER_OFFSET);
	if (buf_map == MAP_FAILED)
		die("mmap(DMA buffer)");
	volatile uint8_t *dma_buf = static_cast<volatile uint8_t *>(buf_map);

	// Zero the buffer so we can clearly see the FPGA's write later.
	std::memset(const_cast<uint8_t *>(dma_buf), 0, alloc_size);

	// 6. Tell the FPGA where to DMA its data: write the 64-bit bus
	// address into its "DMA target address" register at BAR offset
	// 0x1600. Adjust width/endianness to match your FPGA's register
	// definition -- this example assumes a single 64-bit little-endian
	// register.
	volatile uint64_t *dma_addr_reg =
		reinterpret_cast<volatile uint64_t *>(bar + FPGA_DMA_ADDR_REG);
	*dma_addr_reg = buf_addr;
	std::printf("Wrote buffer address 0x%016llx to FPGA reg @0x%llx\n",
		    static_cast<unsigned long long>(buf_addr),
		    static_cast<unsigned long long>(FPGA_DMA_ADDR_REG));

	// (If your FPGA needs an explicit "go"/doorbell register write to
	// kick off the DMA after the address is programmed, add that here,
	// e.g.: *reinterpret_cast<volatile uint32_t*>(bar + 0x1604) = 1;)

	// 7. Wait for the FPGA's interrupt telling us the DMA write is done.
	int irq_fd = open(IRQ_DEV_PATH, O_RDONLY);
	if (irq_fd < 0)
		die("open(irq channel)");

	std::printf("Waiting for interrupt on %s ...\n", IRQ_DEV_PATH);
	uint32_t irq_count = 0;
	ssize_t n = read(irq_fd, &irq_count, sizeof(irq_count));
	if (n < 0)
		die("read(irq channel)");
	if (n != sizeof(irq_count)) {
		std::fprintf(stderr, "short read from irq channel: %zd bytes\n", n);
		return EXIT_FAILURE;
	}
	std::printf("Interrupt received (count=%u)\n", irq_count);

	// 8. Read back the data the FPGA wrote into the DMA buffer.
	// Example: dump the first 32 bytes.
	std::printf("Buffer contents (first 32 bytes):\n");
	for (int i = 0; i < 32 && static_cast<uint32_t>(i) < alloc_size; i++) {
		std::printf("%02x ", dma_buf[i]);
		if ((i + 1) % 16 == 0)
			std::printf("\n");
	}
	std::printf("\n");

	// Cleanup
	munmap(buf_map, alloc_size);
	munmap(bar_map, BAR_MAP_SIZE);
	close(irq_fd);
	close(ctrl_fd);

	return EXIT_SUCCESS;
}
