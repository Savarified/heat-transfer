#pragma once

#include "config.h"

#include <algorithm>

inline float effective_conductivity(float kA, float kB){ //harmonic mean of conductivity
    if (kA + kB == 0.0f)
        return 0.0f;

    return (2.0f * kA * kB) / (kA + kB);
}

inline float calc_resistance(const Voxel& a, const Voxel& b, float contact_area){
    return VOX_SIZE_X / (effective_conductivity(a.conductivity, b.conductivity) * contact_area);
}

inline float calc_heat_flow(const Voxel& a, const Voxel& b, float contact_area){
    float resistance = calc_resistance(a, b, contact_area);

    return (a.temp - b.temp) / resistance; //a=>b in watts
}

inline float temp_change(double energy, const Voxel& voxel){
    float mass = voxel.density * VOX_VOLUME;
    return energy / (mass * voxel.specific_heat);
}

inline void simulate_step(Grid& grid, EnergyBuffer& energy_change){
    for (auto& row : energy_change)
        row.fill(0.0f);

    //horizontal pairs: (x, y) and (x+1, y)
    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x + 1 < COLS; x++) {
            const Voxel& a = grid[y][x];
            const Voxel& b = grid[y][x + 1];

            double energy = calc_heat_flow(a, b, CONTACT_AREA_X) * dt; //J moved per dt

            energy_change[y][x]     -= energy; //a donated
            energy_change[y][x + 1] += energy; //b received
        }
    }

    //vertical pairs: (x, y) and (x, y+1)
    for (size_t y = 0; y + 1 < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            const Voxel& a = grid[y][x];
            const Voxel& b = grid[y + 1][x];

            double energy = calc_heat_flow(a, b, CONTACT_AREA_Y) * dt;

            energy_change[y][x]     -= energy;
            energy_change[y + 1][x] += energy;
        }
    }

    //apply change in energy
    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            grid[y][x].temp += temp_change(energy_change[y][x], grid[y][x]);
        }
    }
}

struct GridStats { float min_temp, max_temp, avg_temp; };

inline GridStats compute_stats(const Grid& grid){
    float min_t = grid[0][0].temp, max_t = grid[0][0].temp;
    double sum = 0.0;

    for (size_t y = 0; y < ROWS; y++) {
        for (size_t x = 0; x < COLS; x++) {
            float t = grid[y][x].temp;
            min_t = std::min(min_t, t);
            max_t = std::max(max_t, t);

            sum += t;
        }
    }

    return { min_t, max_t, float(sum / (ROWS * COLS)) };
}
