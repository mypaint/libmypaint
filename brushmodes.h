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

void draw_dab_pixels_BlendMode_Normal (float * mask,
                                       float * rgba_buffer,
                                       DabBounds * b,
                                       float color_r,
                                       float color_g,
                                       float color_b,
                                       float opacity);
void
draw_dab_pixels_BlendMode_Color (uint16_t * mask,
                                 uint16_t * rgba_buffer, // b=bottom, premult
                                 DabBounds *b,
                                 uint16_t color_r,  // }
                                 uint16_t color_g,  // }-- a=top, !premult
                                 uint16_t color_b,  // }
                                 uint16_t opacity);
void draw_dab_pixels_BlendMode_Normal_and_Eraser (float * mask,
                                                  float * rgba,
                                                  DabBounds *b,
                                                  float color_r,
                                                  float color_g,
                                                  float color_b,
                                                  float color_a,
                                                  float opacity);
void draw_dab_pixels_BlendMode_LockAlpha (float *mask,
                                          float *rgba,
                                          DabBounds *b,
                                          float color_r,
                                          float color_g,
                                          float color_b,
                                          float opacity);
void get_color_pixels_accumulate (float * mask,
                                  float * rgba,
                                  DabBounds *b,
                                  float * sum_weight,
                                  float * sum_r,
                                  float * sum_g,
                                  float * sum_b,
                                  float * sum_a
                                  );



#endif // BRUSHMODES_H
