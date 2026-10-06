#pragma once

#include "config.h"

#include <iostream>
#include <sstream>
#include <random>
#include <fstream>
#include <string>
#include <cstdlib>

inline void randomize_voxels(Grid& grid, float min_temp, float max_temp){
    std::random_device rd;
    std::mt19937 gen(rd());

     std::uniform_real_distribution<float> distrib(min_temp, max_temp);

    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++){
            grid[y][x].temp = distrib(gen);
        }
    }
}

inline void init_hot_spot(Grid& grid, float background_temp, float hot_temp, size_t hot_radius){
    for (auto& row : grid)
        for (auto& voxel : row)
            voxel.temp = background_temp;

    size_t cy = ROWS / 2;
    size_t cx = COLS / 2;
    for (size_t y = cy - hot_radius; y <= cy + hot_radius; y++) {
        for (size_t x = cx - hot_radius; x <= cx + hot_radius; x++) {
            grid[y][x].temp = hot_temp;
        }
    }
}

inline void init_materials(Grid& grid, float conductivity, float density, float specific_heat){
    for (auto& row : grid) {
        for (auto& voxel : row) {
            voxel.conductivity = conductivity;
            voxel.density = density;
            voxel.specific_heat = specific_heat;
        }
    }
}

inline bool get_voxel(const std::string& line, Voxel& voxel)
{
    std::stringstream ss(line);
    std::string value;

    try
    {
        // temp
        if (!std::getline(ss, value, ','))
            return false;
        voxel.temp = std::stof(value);

        // conductivity
        if (!std::getline(ss, value, ','))
            return false;
        voxel.conductivity = std::stof(value);

        // density
        if (!std::getline(ss, value, ','))
            return false;
        voxel.density = std::stof(value);

        // specific heat
        if (!std::getline(ss, value, ','))
            return false;
        voxel.specific_heat = std::stof(value);
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

inline bool load_scene(const std::string& filename, Grid& grid)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Could not open scene file: " << filename << std::endl;
        return false;
    }

    std::string line;

    //skip CSV header
    if (!std::getline(file, line))
    {
        std::cerr << "Scene file is empty." << std::endl;
        return false;
    }

    size_t voxel_index = 0;

    while (std::getline(file, line))
    {
        //ignore empty lines
        if (line.empty())
            continue;

        if (voxel_index >= ROWS * COLS)
        {
            std::cerr << "Scene file contains too many voxels." << std::endl;
            return false;
        }

        size_t y = voxel_index / COLS;
        size_t x = voxel_index % COLS;

        if (!get_voxel(line, grid[y][x]))
        {
            std::cerr << "Invalid voxel data on CSV row "
                      << voxel_index + 2 << std::endl;
            return false;
        }

        voxel_index++;
    }

    if (voxel_index != ROWS * COLS)
    {
        std::cerr << "Scene file contains " << voxel_index
                  << " voxels, but the grid requires "
                  << ROWS * COLS << "." << std::endl;
        return false;
    }

    return true;
}

inline void reset_sim(
    Grid& grid,
    double& sim_accumulator,
    double& sim_time,
    const std::string& filename
)
{
    sim_accumulator = 0.0;
    sim_time = 0.0;

    if (!load_scene(filename, grid))
    {
        std::cerr << "Failed to load scene. Exiting." << std::endl;
        std::exit(1);
    }
}
