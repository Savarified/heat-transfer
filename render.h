#pragma once

#include "config.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>

inline int init_SDL(){
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() != 0){
        std::cout << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    window = SDL_CreateWindow(
        "Heat Transfer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr){
        std::cout << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (renderer == nullptr){
        std::cout << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (font == nullptr)
    {
        std::cout << "TTF_OpenFont Error (check FONT_PATH): " << TTF_GetError() << std::endl;
    }
    return 0;
}

inline SDL_Color temp_to_col(float temp, float min_temp, float max_temp){
    if (max_temp == min_temp)
        return {255, 0, 0, 255};

    float luma = (temp - min_temp) / (max_temp - min_temp); //normalize luma
    luma = std::clamp(luma, 0.0f, 1.0f);

    //gradient stops
    static const SDL_Color stops[] = {
        {0, 0, 55, 255}, //blue
        {0, 255, 255, 255}, //cyan
        {0, 255, 0, 255}, //green
        {255, 255, 0, 255}, //yellow
        {255, 50, 50, 255}  //red 
    };
    constexpr int num_stops = sizeof(stops) / sizeof(stops[0]);
    constexpr int num_segments = num_stops - 1;

    //determine segment
    float scaled = luma * num_segments;
    int seg = std::min(static_cast<int>(scaled), num_segments - 1);
    float t = scaled - seg; //t within segment

    const SDL_Color& c0 = stops[seg];
    const SDL_Color& c1 = stops[seg + 1];

    SDL_Color temp_color;
    temp_color.r = static_cast<Uint8>(c0.r + t * (c1.r - c0.r));
    temp_color.g = static_cast<Uint8>(c0.g + t * (c1.g - c0.g));
    temp_color.b = static_cast<Uint8>(c0.b + t * (c1.b - c0.b));
    temp_color.a = 255;

    return temp_color;
}

inline void render_text(const std::string& text, int x, int y, SDL_Color color, int* out_w = nullptr, int* out_h = nullptr){
    if (font == nullptr || text.empty())
    {
        if (out_w) *out_w = 0;
        if (out_h) *out_h = 0;
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (surface == nullptr) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst{ x, y, surface->w, surface->h };
    if (out_w) *out_w = surface->w;
    if (out_h) *out_h = surface->h;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

inline void draw_voxels(const Grid& grid, float min_temp, float max_temp){
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++){
            SDL_Rect rect{
                .x = x * voxel_width,
                .y = y * voxel_height,
                .w = voxel_width,
                .h = voxel_height
            };

            SDL_Color temp_color = temp_to_col(grid[y][x].temp, min_temp, max_temp);

            SDL_SetRenderDrawColor(
                renderer,
                temp_color.r,
                temp_color.g,
                temp_color.b,
                temp_color.a
            );
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

//vertical gradient with ticks
inline void draw_colorbar(int x, int y, int w, int h, float min_temp, float max_temp){
    for (int row = 0; row < h; row++)
    {
        float t = max_temp - (float(row) / float(h - 1)) * (max_temp - min_temp);
        SDL_Color c = temp_to_col(t, min_temp, max_temp);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        SDL_RenderDrawLine(renderer, x, y + row, x + w, y + row);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect border{ x, y, w, h };
    SDL_RenderDrawRect(renderer, &border);

    const int ticks = 5;
    for (int i = 0; i < ticks; i++)
    {
        float frac = float(i) / float(ticks - 1);
        int ty = y + int(frac * h);
        float temp = max_temp - frac * (max_temp - min_temp);

        SDL_RenderDrawLine(renderer, x + w, ty, x + w + 5, ty);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << temp << "C";
        render_text(oss.str(), x + w + 8, ty - 7, {255,255,255,255});
    }
}
