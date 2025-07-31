//
// Created by carlo on 07/06/2025.
//

#ifndef SPACEINVADERS3D_FONTMANAGER_H
#define SPACEINVADERS3D_FONTMANAGER_H


#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cstdio> // For printf, optional
#include <sstream>
#include "GameObjectData.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)

struct FontGlyphMetrics {
    int minX, maxX;      // Left/right of nontransparent region (relative to cell)
    int minY, maxY;      // Top/bottom of nontransparent region (relative to cell)
    float advance;       // Recommended horizontal advance (pixels, incl. extra spacing)
    int cellX, cellY;    // Top-left in atlas (pixels)
    int cellW, cellH;    // Cell scale (pixels)
};

struct FontAtlasMetrics {
    int imgW, imgH;         // Atlas scale in pixels
    int cellW, cellH;       // Each cell scale in pixels
    int cols, rows;         // How many glyphs in X/Y
    int firstChar, lastChar;// ASCII code (typically 32..127)
    std::vector<FontGlyphMetrics> glyphs; // Indexed by (code - firstChar)
};


class FontManager {
public:
    FontManager();
    ~FontManager();


    void autoPackFontAtlas(
            const std::vector<uint8_t> &image, int imgW, int imgH,
            int cellW, int cellH, int cols, int rows,
            int firstChar = 0, int lastChar = 255, // ASCII
            float pad = 1.0f // Extra pixels between glyphs (for spacing)
    );

    std::vector<Vertex> buildTextVertices(
            const std::string &text,
            float x, float y, float z,
            float scale,bool center = false);

private:
    FontAtlasMetrics fontAtlasMetrics_;
};


#endif //SPACEINVADERS3D_FONTMANAGER_H
