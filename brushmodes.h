#ifndef BRUSHMODES_H
#define BRUSHMODES_H

// FIXME: must be defined only ONE place
#define TILE_SIZE 64

typedef struct {
    int x0; // Bounding box for the dab.
    int y0; // Top left corner: (x0, y0)
    int x1; // Top right corner: (x1, y1)
    int y1; // Cordinates are pixel offsets into a tile
} DabBounds;

void draw_dab_pixels_BlendMode_Normal (uint16_t * mask,
                                       uint16_t * rgba_buffer,
                                       DabBounds * b,
                                       uint16_t color_r,
                                       uint16_t color_g,
                                       uint16_t color_b,
                                       uint16_t opacity);
void
draw_dab_pixels_BlendMode_Color (uint16_t * mask,
                                 uint16_t * rgba_buffer, // b=bottom, premult
                                 DabBounds *b,
                                 uint16_t color_r,  // }
                                 uint16_t color_g,  // }-- a=top, !premult
                                 uint16_t color_b,  // }
                                 uint16_t opacity);
void draw_dab_pixels_BlendMode_Normal_and_Eraser (uint16_t * mask,
                                                  uint16_t * rgba,
                                                  DabBounds *b,
                                                  uint16_t color_r,
                                                  uint16_t color_g,
                                                  uint16_t color_b,
                                                  uint16_t color_a,
                                                  uint16_t opacity);
void draw_dab_pixels_BlendMode_LockAlpha (uint16_t * mask,
                                          uint16_t * rgba,
                                          DabBounds *b,
                                          uint16_t color_r,
                                          uint16_t color_g,
                                          uint16_t color_b,
                                          uint16_t opacity);
void get_color_pixels_accumulate (uint16_t * mask,
                                  uint16_t * rgba,
                                  DabBounds *b,
                                  float * sum_weight,
                                  float * sum_r,
                                  float * sum_g,
                                  float * sum_b,
                                  float * sum_a
                                  );



#endif // BRUSHMODES_H
