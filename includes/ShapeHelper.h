#pragma once
//#include "rwd3d9.h"
#include "CRGBA.h"
#include "CSprite2d.h"

class ShapeHelper {
public:
    static float mSinTable[360];
    static float mCosTable[360];

    static void InitSinCosTable();
    static void AddOneVertToBuffer(RwIm2DVertex *verts, unsigned int vertIndex, float x, float y, float z, float rhw, float u, float v, unsigned int color);
    static void DrawCircleSectorTextured(float tex_width, float tex_height, float tex_u_offset, float tex_v_offset, float tex_u_size, float tex_v_size, float center_x, float center_y, float width, float height, float start, float end, CSprite2d* sprite = nullptr);
    static void RotateVertices(RwIm2DVertex *verts, unsigned int num, float center_x, float center_y, float angle);
    static void SetVertices(RwIm2DVertex* verts, CRect const& rect, CRGBA const& color, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4);
    static void DrawTexturedRectangle(CRect const& rect, CRGBA const& color, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, CSprite2d* sprite = nullptr);
    static void DrawRotatedTexturedRectangle(CRect const& rect, float center_x, float center_y, float angle, CRGBA const& color, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4, CSprite2d* sprite = nullptr);
};