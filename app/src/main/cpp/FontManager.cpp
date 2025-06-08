//
// Created by carlo on 07/06/2025.
//

#include "FontManager.h"

constexpr int GLYPH_W = 16;
constexpr int GLYPH_H = 16;
constexpr int ATLAS_W = 256;
constexpr int ATLAS_H = 256;
constexpr int GLYPHS_PER_ROW = 16;

inline std::pair<int, int> glyphCoord(char c);

// Returns the (col, row) in the atlas for a given char code (0–255)
inline std::pair<int, int> glyphCoord(char c) {
    int idx = (unsigned char) c;
    int row = idx / GLYPHS_PER_ROW;
    int col = idx % GLYPHS_PER_ROW;
    return {col, row};
}

// Helper to split string by '\n'
std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Helper to compute pixel width of a line
float measureLineWidth(const std::string& line, float scale, const FontAtlasMetrics& atlas) {
    float width = 0.0f;
    int firstChar = atlas.firstChar;
    for (char c : line) {
        int idx = (unsigned char)c - firstChar;
        if (idx < 0 || idx >= (int)atlas.glyphs.size()) continue;
        width += atlas.glyphs[idx].advance * scale ;
    }
    return width;
}

// Helper: find glyph bounds and advance in a grid atlas
void FontManager::autoPackFontAtlas(const std::vector<uint8_t> &image, int imgW, int imgH, int cellW,
                               int cellH, int cols, int rows, int firstChar, int lastChar,
                               float pad) {

    fontAtlasMetrics_.imgW = imgW;
    fontAtlasMetrics_.imgH = imgH;
    fontAtlasMetrics_.cellW = cellW;
    fontAtlasMetrics_.cellH = cellH;
    fontAtlasMetrics_.cols = cols;
    fontAtlasMetrics_.rows = rows;
    fontAtlasMetrics_.firstChar = firstChar;
    fontAtlasMetrics_.lastChar = lastChar;
    fontAtlasMetrics_.glyphs.resize(lastChar - firstChar + 1);

    for (int code = firstChar; code <= lastChar; ++code) {
        int idx = code - firstChar;
        int glyphIdx = idx;
        int col = glyphIdx % cols;
        int row = glyphIdx / cols;
        int cellX = col * cellW;
        int cellY = row * cellH;

        int minX = cellW, maxX = -1;
        int minY = cellH, maxY = -1;
        for (int y = 0; y < cellH; ++y) {
            for (int x = 0; x < cellW; ++x) {
                int px = cellX + x;
                int py = cellY + y;
                int pidx = (py * imgW + px) * 4;
                uint8_t a = image[pidx + 3];
                if (a > 16) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }
        FontGlyphMetrics &gm = fontAtlasMetrics_.glyphs[idx];
        gm.cellX = cellX;
        gm.cellY = cellY;
        gm.cellW = cellW;
        gm.cellH = cellH;
        gm.minX = (maxX >= minX) ? minX : cellW / 2; // Centered if blank
        gm.maxX = (maxX >= minX) ? maxX : cellW / 2;
        gm.minY = (maxY >= minY) ? minY : cellH / 2;
        gm.maxY = (maxY >= minY) ? maxY : cellH / 2;
        gm.advance = (maxX >= minX)
                     ? (maxX - minX + 1 + pad) // visual width + extra
                     : (cellW * 0.35f);        // fallback for space/blank


    }
}

std::vector<Vertex>
FontManager::buildTextVertices(const std::string &text, float x, float y, float z, float scale, bool center) {
    std::vector<Vertex> outVerts;

    float charSpacing = -0.04f;
    float penY = y;
    float lineHeight = fontAtlasMetrics_.cellH * scale * 0.5f; // 1.15 is line spacing
    auto lines = splitLines(text);

    for (const auto& line : lines) {
        float penX = x;
        if (center) {
            float width = measureLineWidth(line, scale, fontAtlasMetrics_);
            penX = x - width * 0.5f;
        }
        for (char c : line) {
            int idx = (unsigned char)c - fontAtlasMetrics_.firstChar;
            if (idx < 0 || idx >= (int)fontAtlasMetrics_.glyphs.size()) continue;
            const auto& g = fontAtlasMetrics_.glyphs[idx];
            // ... (same glyph drawing code as before)
            if (g.maxX < g.minX || g.maxY < g.minY) {
                penX += g.advance * scale + charSpacing;
                continue;
            }
            float gw = (g.maxX - g.minX + 1) * scale;
            float gh = (g.maxY - g.minY + 1) * scale;
            float x0 = penX;
            float y0 = penY;
            float x1 = penX + gw;
            float y1 = penY + gh;

            // UVs, vertices (same as before)
            float u0 = (g.cellX + g.minX) / (float)fontAtlasMetrics_.imgW;
            float v0 = (g.cellY + g.minY) / (float)fontAtlasMetrics_.imgH;
            float u1 = (g.cellX + g.maxX + 1) / (float)fontAtlasMetrics_.imgW;
            float v1 = (g.cellY + g.maxY + 1) / (float)fontAtlasMetrics_.imgH;

            Vertex vtx[6] = {
                    {{x0, y0, z},{1.0f,1.0f,0.0f}, {u0, v0}},
                    {{x1, y0, z},{1.0f,1.0f,0.0f}, {u1, v0}},
                    {{x0, y1, z},{0.0f,1.0f,0.0f}, {u0, v1}},

                    {{x1, y0, z},{1.0f,1.0f,0.0f}, {u1, v0}},
                    {{x1, y1, z},{1.0f,1.0f,0.0f}, {u1, v1}},
                    {{x0, y1, z},{0.0f,1.0f,1.0f}, {u0, v1}},
            };
            outVerts.insert(outVerts.end(), vtx, vtx+6);
            penX += g.advance * scale + charSpacing;
        }
        penY += lineHeight; // Next line lower down
    }

    return outVerts;

}

FontManager::FontManager() {

}

FontManager::~FontManager() {

}
