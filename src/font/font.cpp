#include "font.h"

global Face *default_fonts[FONT_COUNT];

internal Face *load_font_face(String8 font_name, int font_height) {
    FT_Library ft_lib;
    int err = FT_Init_FreeType(&ft_lib);
    if (err) {
        printf("Error initializing freetype library: %d\n", err);
    }

    FT_Face ft_face;
    err = FT_New_Face(ft_lib, (const char *)font_name.data, 0, &ft_face);
    if (err == FT_Err_Unknown_File_Format) {
        printf("Format not supported\n");
    } else if (err) {
        printf("Font file could not be read\n");
    }

    err = FT_Set_Pixel_Sizes(ft_face, 0, font_height);
    //err = FT_Set_Pixel_Sizes(ft_face, 0, font_height);
    if (err) {
        printf("Error setting pixel sizes of font\n");
    }

    Face *font_face = (Face *)calloc(1, sizeof(Face));

    int bbox_ymax = FT_MulFix(ft_face->bbox.yMax, ft_face->size->metrics.y_scale) >> 6;
    int bbox_ymin = FT_MulFix(ft_face->bbox.yMin, ft_face->size->metrics.y_scale) >> 6;
    int height = bbox_ymax - bbox_ymin;
    float ascend = ft_face->size->metrics.ascender / 64.f;
    float descend = ft_face->size->metrics.descender / 64.f;
    float bbox_height = (float)(bbox_ymax - bbox_ymin);
    float glyph_height = (float)ft_face->size->metrics.height / 64.f;
    float glyph_width = (float)(ft_face->size->metrics.max_advance) / 64.f;

    int atlas_width = 0;
    int atlas_height = 0;
    int max_bmp_height = 0;
    for (u32 c = 0; c < 256; c++) {
        if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER)) {
            printf("Error loading char %c\n", c);
            continue;
        }

        atlas_width += ft_face->glyph->bitmap.width;
        if (atlas_height < (int)ft_face->glyph->bitmap.rows) {
            atlas_height = ft_face->glyph->bitmap.rows;
        }

        int bmp_height = ft_face->glyph->bitmap.rows + ft_face->glyph->bitmap_top;
        if (max_bmp_height < bmp_height) {
            max_bmp_height = bmp_height;
        }
    }

    FT_GlyphSlot slot = ft_face->glyph;

    // Pack glyph bitmaps
    int pixel_size = 4;
    int atlas_x = 0;
    unsigned char *bitmap = (unsigned char *)calloc(atlas_width * atlas_height + atlas_height, pixel_size);

    for (u32 c = 10; c < 128; c++) {
        err = FT_Load_Char(ft_face, c, FT_LOAD_DEFAULT);
        if (err) {
            printf("Error loading glyph '%c': %d\n", c, err);
        }

        err = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
        if (err) {
            printf("Error rendering glyph '%c': %d\n", c, err);
        }
        
        Glyph *glyph = &font_face->glyphs[c];
        glyph->ax = (float)(ft_face->glyph->advance.x >> 6);
        glyph->ay = (float)(ft_face->glyph->advance.y >> 6);
        glyph->bx = (float)ft_face->glyph->bitmap.width;
        glyph->by = (float)ft_face->glyph->bitmap.rows;
        glyph->bt = (float)ft_face->glyph->bitmap_top;
        glyph->bl = (float)ft_face->glyph->bitmap_left;
        glyph->to = (float)atlas_x / atlas_width;

        // Write glyph bitmap to atlas
        for (int y = 0; y < glyph->by; y++) {
            for (int x = 0; x < glyph->bx; x++) {
                unsigned char *dest = bitmap + (y * atlas_width + atlas_x + x) * pixel_size;
                unsigned char *source = ft_face->glyph->bitmap.buffer + y * ft_face->glyph->bitmap.width + x;
                memset(dest, *source, pixel_size);
            }
        }

        atlas_x += ft_face->glyph->bitmap.width;
    }
    
    font_face->width = atlas_width;
    font_face->height = atlas_height;
    font_face->max_bmp_height = max_bmp_height;
    font_face->ascend = ascend;
    font_face->descend = descend;
    font_face->bbox_height = height;
    font_face->glyph_width = glyph_width;
    font_face->glyph_height = glyph_height;
    font_face->texture = d3d11_create_texture_rgba(bitmap, atlas_width, atlas_height, atlas_width * pixel_size);
    
    free(bitmap);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_lib);
    return font_face;
}

