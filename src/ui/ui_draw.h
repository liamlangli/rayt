#pragma once

#include "ui/ui.h"

enum UI_PRIMITIVE_TYPE {
    UI_PRIMITIVE_TYPE_RECTANGLE = (1 << 26),
    UI_PRIMITIVE_TYPE_TRIANGLE = (2 << 26),
    UI_PRIMITIVE_TYPE_TRIANGLE_ADVANCED = (3 << 26)
};

enum UI_TRIANGLE_TYPE {
    TRIANGLE_SOLID = 2,
    TRIANGLE_DASH,
    TRIANGLE_ICON,
    TRIANGLE_SCREEN,
    TRIANGLE_ENTITY,
    TRIANGLE_ATLAS
};

/**
 * vertex_id uint32 index
 * | 0 00000|     00| 00000000 00000000 00000000|
 * |  6 bits| 2 bits|                     24 bit|
 * |    type| corner|           primitive offset|
 */
inline u32 encode_vertex_id(u32 type, u32 corner, u32 offset) {
    return type | corner | (offset >> 2);
}

/**
 * glyph_id uint32 index
 * |     1|         00000|     00| 00000000 00000000 00000000|
 * | 1 bit|        5 bits| 2 bits|                     24bits|
 * |  flag| header offset| corner|           primitive offset|
 */
inline u32 encode_glyph_id(u32 header_offset, u32 corner, u32 offset) {
    return 0x80000000 | (header_offset << 26) | corner | (offset >> 2);
}

inline u32 decode_vertex_type(u32 i) {
    return (i >> 26) & 0x3f;
}

inline u32 decode_corner_id(u32 i) {
    return (i >> 24) & 0x3;
}

inline u32 decode_vertex_offset(u32 i) {
    return (i & 0xffffff) << 2;
}


// ui draw commands
void fill_rect(ui_layer *layer, ui_style style, ui_rect rect, u32 clip);
void fill_round_rect(ui_layer *layer, ui_style style, ui_rect rect, f32 radius, u32 clip, u32 triangle_type);
void fill_round_rect_pre_corner(ui_layer *layer, ui_style style, ui_rect rect, float4 radiusese, u32 clip, u32 triangle_type);

void stroke_rect(ui_layer *layer, ui_style style, ui_rect rect, u32 clip);
void stroke_round_rect(ui_layer *layer, ui_style style, ui_rect rect, f32 radius, u32 clip, u32 triangle_type);
void stroke_round_rect_pre_corner(ui_layer *layer, ui_style style, ui_rect rect, float4 radiusese, u32 clip, u32 triangle_type);

void draw_glyph(ui_layer *layer, float2 origin, ui_font *font, ustring text, u32 clip, f32 scale, ui_style style);