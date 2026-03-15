
#pragma once

#include <vector>
#include <unordered_map>
#include "Particle.h"


class SpatialGrid
{
public: 
    SpatialGrid(float cellsize)
        : m_cellSize(cellsize){};

    void clear()
    {
        m_cells.clear();
    }

    void insert(Particle* p)
    {
        auto key=cellKey(p->position);
        m_cells[key].push_back(p);
    }

    std::vector<Particle*>& getCell(int x, int y)
    {
        return m_cells[{x,y}];
    }

    std::pair<int,int> cellCoords(const Vec2& pos)
    {
        int x = static_cast<int>(pos.x / m_cellSize);
        int y = static_cast<int>(pos.y / m_cellSize);

        return {x, y};
    }
    
private:
    struct pair_hash
    {
        size_t operator()(const std::pair<int, int>& p) const
        {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };

    std::pair<int, int> cellKey(const Vec2& pos)
    {
        return cellCoords(pos);
    }

    float m_cellSize;

    std::unordered_map<
        std::pair<int, int>,
        std::vector<Particle*>,
        pair_hash
    > m_cells;
};