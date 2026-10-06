#define SDL_MAIN_HANDLED
#include "config.h"
#include "physics.h"
#include "render.h"
#include "scene.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>

// g++ main.cpp -o main.exe -I/ucrt64/include/SDL2 -L/ucrt64/lib -lSDL2 -lSDL2_ttf

SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font* font;

//grid render area
int GRID_WIDTH = 400;
int GRID_HEIGHT = 400;
int voxel_width;
int voxel_height;

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: main.exe <scene.csv>" << std::endl;
        return 1;
    }

    const std::string scene_filename = argv[1];

    init_SDL();

    Grid grid = {}; //all voxels
    EnergyBuffer energy_change{}; //temp change buffer

    voxel_width = GRID_WIDTH / COLS;
    voxel_height = GRID_HEIGHT / ROWS;

    bool running = true;
    bool paused = false;
    bool auto_scale = false;
    SDL_Event event;

    //fps calc
    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();
    double sim_accumulator = 0.0;
    double sim_time = 0.0; //total simulated seconds elapsed

    bool step_requested = false; //. key step

    reset_sim(grid, sim_accumulator, sim_time, scene_filename);

    while (running)
    {
        Uint64 frame_start = SDL_GetPerformanceCounter();

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_a)
                    auto_scale = !auto_scale;
                else if (event.key.keysym.sym == SDLK_SPACE)
                    paused = !paused;
                else if (event.key.keysym.sym == SDLK_PERIOD)
                    step_requested = true;
                else if (event.key.keysym.sym == SDLK_r)
                    reset_sim(grid, sim_accumulator, sim_time, scene_filename);
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = double(now - last_counter) / double(perf_freq);
        last_counter = now;

        //ignore late frames
        if (elapsed > 0.25)
            elapsed = 0.25;

        if (!paused)
        {
            sim_accumulator += elapsed;
            while (sim_accumulator >= dt)
            {
                simulate_step(grid, energy_change);
                sim_accumulator -= dt;
                sim_time += dt;
            }
        }

        if (step_requested)
        {
            simulate_step(grid, energy_change);
            sim_time += dt;
            step_requested = false;
        }

        GridStats stats = compute_stats(grid);
        float min_temp = auto_scale ? stats.min_temp : FIXED_MIN_TEMP;
        float max_temp = auto_scale ? stats.max_temp : FIXED_MAX_TEMP;

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        draw_voxels(grid, min_temp, max_temp);

        //mouse hover readout
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        bool hovering = mouse_x >= 0 && mouse_x < GRID_WIDTH && mouse_y >= 0 && mouse_y < GRID_HEIGHT;
        int hover_col = -1, hover_row = -1;
        if (hovering)
        {
            hover_col = mouse_x / voxel_width;
            hover_row = mouse_y / voxel_height;
            hover_col = std::clamp(hover_col, 0, int(COLS) - 1);
            hover_row = std::clamp(hover_row, 0, int(ROWS) - 1);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
            SDL_Rect highlight{ hover_col * voxel_width, hover_row * voxel_height, voxel_width, voxel_height };
            SDL_RenderDrawRect(renderer, &highlight);
        }

        //panel background
        SDL_SetRenderDrawColor(renderer, 3, 3, 3, 255);
        SDL_Rect panel{ GRID_WIDTH, 0, PANEL_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(renderer, &panel);

        int panel_x = GRID_WIDTH + 15;
        int text_y = 15;
        SDL_Color white{255,255,255,255};
        SDL_Color gray{170,170,170,255};

        std::ostringstream oss;

        oss << std::fixed << std::setprecision(2) << sim_time << " s";
        render_text(oss.str(), panel_x, text_y, white); text_y += 20; oss.str("");

        render_text(paused ? "PAUSED" : "running", panel_x, text_y, paused ? SDL_Color{255,200,80,255} : gray);
        text_y += 20;

        oss << "min: " << std::fixed << std::setprecision(1) << stats.min_temp << "C";
        render_text(oss.str(), panel_x, text_y, white); text_y += 20; oss.str("");

        oss << "max: " << std::fixed << std::setprecision(1) << stats.max_temp << "C";
        render_text(oss.str(), panel_x, text_y, white); text_y += 20; oss.str("");

        oss << "avg: " << std::fixed << std::setprecision(1) << stats.avg_temp << "C";
        render_text(oss.str(), panel_x, text_y, white); text_y += 25; oss.str("");

        if (hovering)
        {
            oss << "cell (" << hover_col << "," << hover_row << ")";
            render_text(oss.str(), panel_x, text_y, gray); text_y += 20; oss.str("");

            oss << "temp: " << std::fixed << std::setprecision(2) << grid[hover_row][hover_col].temp << "C";
            render_text(oss.str(), panel_x, text_y, white); text_y += 20; oss.str("");
        }
        else
        {
            render_text("hover grid for", panel_x, text_y, gray); text_y += 16;
            render_text("cell readout", panel_x, text_y, gray); text_y += 20;
        }

        render_text(auto_scale ? "scale: AUTO" : "scale: FIXED", panel_x, text_y, gray); text_y += 16;
        render_text("[a] scale [space] pause", panel_x, text_y, gray); text_y += 14;
        render_text("[.] step   [r] reset", panel_x, text_y, gray); text_y += 22;

        int colorbar_height = std::min(130, WINDOW_HEIGHT - text_y - 20);
        draw_colorbar(panel_x, text_y+20, 20, colorbar_height, min_temp, max_temp);

        SDL_RenderPresent(renderer);

        //cap fps
        Uint64 frame_end = SDL_GetPerformanceCounter();
        double frame_time = double(frame_end - frame_start) / double(perf_freq);
        if (frame_time < TARGET_FRAME_TIME)
        {
            SDL_Delay(Uint32((TARGET_FRAME_TIME - frame_time) * 1000.0));
        }
    }

    if (font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}