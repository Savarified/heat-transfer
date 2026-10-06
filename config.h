#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <array>
#include <cstddef>

extern SDL_Renderer* renderer;
extern SDL_Window* window;
extern TTF_Font* font;

//grid render area
extern int GRID_WIDTH;
extern int GRID_HEIGHT;
extern int voxel_width;
extern int voxel_height;

//side panel for colorbar + stats
constexpr int PANEL_WIDTH = 210;
constexpr int WINDOW_WIDTH = 400 + PANEL_WIDTH;
constexpr int WINDOW_HEIGHT = 400;

constexpr const char* FONT_PATH = "C:/Windows/Fonts/consola.ttf";
constexpr int FONT_SIZE = 14;

constexpr size_t ROWS = 100;
constexpr size_t COLS = 100;

//real dimensions
constexpr float SIM_WIDTH = 0.1f; //10cm
constexpr float SIM_HEIGHT = 0.1f; //10cm
constexpr float SIM_DEPTH = 0.01f; //1cm
constexpr float VOX_SIZE_X = SIM_WIDTH / COLS;
constexpr float VOX_SIZE_Y = SIM_HEIGHT / ROWS;
constexpr float VOX_VOLUME = VOX_SIZE_X * VOX_SIZE_Y * SIM_DEPTH; //m^3
constexpr float CONTACT_AREA_X = VOX_SIZE_Y * SIM_DEPTH; //face between horizontal neighbors
constexpr float CONTACT_AREA_Y = VOX_SIZE_X * SIM_DEPTH; //face between vertical neighbors

constexpr float dt = 0.0015f; //seconds

constexpr double TARGET_FPS = 60.0;
constexpr double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;//seconds per rendered frame

//fixed color scale bounds
constexpr float FIXED_MIN_TEMP = 20.0f;
constexpr float FIXED_MAX_TEMP = 40.0f;

struct Voxel
{
    float temp; //C
    float conductivity; //W/(m*K)
    float density; //kg/m^3
    float specific_heat; //J/(kg*K)
};

using Grid = std::array<std::array<Voxel, COLS>, ROWS>;
using EnergyBuffer = std::array<std::array<double, COLS>, ROWS>;