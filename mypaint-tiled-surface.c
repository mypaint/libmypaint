/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2007-2014 Martin Renold <martinxyz@gmx.ch> et. al.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "mypaint-config.h"
#include "mypaint-tiled-surface.h"
#include "tiled-surface-private.h"
#include "helpers.h"
#include "brushmodes.h"
#include "operationqueue.h"

#define NUM_BBOXES_DEFAULT 32

void tiled_surface_process_tile(MyPaintTiledSurface *self, int tx, int ty);

void process_tile_internal(
    void* tiled_surface, void (*request_start)(void*, void*), void (*request_end)(void*, void*),
    OperationQueue* op_queue, int tx, int ty);

static void
begin_atomic_default(MyPaintSurface *surface)
{
    mypaint_tiled_surface_begin_atomic((MyPaintTiledSurface *)surface);
}

static void
end_atomic_default(MyPaintSurface *surface, MyPaintRectangle *roi)
{
    mypaint_tiled_surface_end_atomic((MyPaintTiledSurface *)surface, roi);
}

/**
 * mypaint_tiled_surface_begin_atomic: (skip)
 *
 * Implementation of #MyPaintSurface::being_atomic vfunc
 * Note: Only intended to be used from #MyPaintTiledSurface subclasses, which should chain up to this
 * if implementing their own #MyPaintSurface::begin_atomic vfunc.
 * Application code should only use mypaint_surface_being_atomic()
 */
void
mypaint_tiled_surface_begin_atomic(MyPaintTiledSurface *self)
{
  self->dirty_bbox.x = 0;
  self->dirty_bbox.y = 0;
  self->dirty_bbox.width = 0;
  self->dirty_bbox.height = 0;
}

/**
 * mypaint_tiled_surface_end_atomic: (skip)
 *
 * Implementation of #MyPaintSurface::end_atomic vfunc
 * Note: Only intended to be used from #MyPaintTiledSurface subclasses, which should chain up to this
 * if implementing their own #MyPaintSurface::end_atomic vfunc.
 * Application code should only use mypaint_surface_end_atomic().
 */
void
mypaint_tiled_surface_end_atomic(MyPaintTiledSurface *self, MyPaintRectangle *roi)
{
    // Process tiles
    TileIndex *tiles;
    int tiles_n = operation_queue_get_dirty_tiles(self->operation_queue, &tiles);

    #pragma omp parallel for schedule(static) if(self->threadsafe_tile_requests && tiles_n > 3)
    for (int i = 0; i < tiles_n; i++) {
      tiled_surface_process_tile(self, tiles[i].x, tiles[i].y);
    }

    operation_queue_clear_dirty_tiles(self->operation_queue);

    if (roi) {
        *roi = self->dirty_bbox;
    }
}


/**
 * mypaint_tiled_surface_tile_request_start:
 *
 * Fetch a tile out from the underlying tile store.
 * When successful, request->data will be set to point to the fetched tile.
 * Consumers must *always* call mypaint_tiled_surface_tile_request_end() with the same
 * request to complete the transaction.
 */
void mypaint_tiled_surface_tile_request_start(MyPaintTiledSurface *self, MyPaintTileRequest *request)
{
    assert(self->tile_request_start);
    self->tile_request_start(self, request);
}

/**
 * mypaint_tiled_surface_tile_request_end:
 *
 * Put a (potentially modified) tile back into the underlying tile store.
 *
 * Consumers must *always* call mypaint_tiled_surface_tile_request_start() with the same
 * request to start the transaction before calling this function.
 */
void mypaint_tiled_surface_tile_request_end(MyPaintTiledSurface *self, MyPaintTileRequest *request)
{
    assert(self->tile_request_end);
    self->tile_request_end(self, request);
}

/**
 * mypaint_tiled_surface_set_symmetry_state:
 * @active: TRUE to enable, FALSE to disable.
 * @center_x: X axis to mirror events across.
 *
 * Enable/Disable symmetric brush painting across an X axis.
 */
void
mypaint_tiled_surface_set_symmetry_state(MyPaintTiledSurface *self, gboolean active, float center_x)
{
    self->surface_do_symmetry = active;
    self->surface_center_x = center_x;
}

/**
 * mypaint_tile_request_init:
 *
 * Initialize a request for use with mypaint_tiled_surface_tile_request_start()
 * and mypaint_tiled_surface_tile_request_end()
 */
void
mypaint_tile_request_init(MyPaintTileRequest *data, int level,
                          int tx, int ty, gboolean readonly)
{
    data->tx = tx;
    data->ty = ty;
    data->readonly = readonly;
    data->buffer = NULL;
    data->context = NULL;
#ifdef _OPENMP
    data->thread_id = omp_get_thread_num();
#else
    data->thread_id = -1;
#endif
    data->mipmap_level = level;
}

// Must be threadsafe
static inline float
calculate_r_sample(float x, float y, float aspect_ratio,
                      float sn, float cs)
{
    const float yyr=(y*cs-x*sn)*aspect_ratio;
    const float xxr=y*sn+x*cs;
    const float r = (yyr*yyr + xxr*xxr);
    return r;
}

static inline float
calculate_rr(int xp, int yp, float x, float y, float aspect_ratio,
                      float sn, float cs, float one_over_radius2)
{
    // code duplication, see brush::count_dabs_to()
    const float yy = (yp + 0.5f - y);
    const float xx = (xp + 0.5f - x);
    const float yyr=(yy*cs-xx*sn)*aspect_ratio;
    const float xxr=yy*sn+xx*cs;
    const float rr = (yyr*yyr + xxr*xxr) * one_over_radius2;
    // rr is in range 0.0..1.0*sqrt(2)
    return rr;
}

static inline float
sign_point_in_line( float px, float py, float vx, float vy )
{
    return (px - vx) * (-vy) - (vx) * (py - vy);
}

static inline void
closest_point_to_line( float lx, float ly, float px, float py, float *ox, float *oy )
{
    const float l2 = lx*lx + ly*ly;
    const float ltp_dot = px*lx + py*ly;
    const float t = ltp_dot / l2;
    *ox = lx * t;
    *oy = ly * t;
}

// Must be threadsafe
//
// This works by taking the visibility at the nearest point
// and dividing by 1.0 + delta.
//
// - nearest point: point where the dab has more influence
// - farthest point: point at a fixed distance away from
//                   the nearest point
// - delta: how much occluded is the farthest point relative
//          to the nearest point
static inline float
calculate_rr_antialiased(int xp, int yp, float x, float y, float aspect_ratio,
                      float sn, float cs, float one_over_radius2,
                      float r_aa_start)
{
    // calculate pixel position and borders in a way
    // that the dab's center is always at zero
    float pixel_right = x - (float)xp;
    float pixel_bottom = y - (float)yp;
    float pixel_center_x = pixel_right - 0.5f;
    float pixel_center_y = pixel_bottom - 0.5f;
    float pixel_left = pixel_right - 1.0f;
    float pixel_top = pixel_bottom - 1.0f;

    float nearest_x, nearest_y; // nearest to origin, but still inside pixel
    float farthest_x, farthest_y; // farthest from origin, but still inside pixel
    float r_near, r_far, rr_near, rr_far;
    // Dab's center is inside pixel?
    if( pixel_left<0 && pixel_right>0 &&
        pixel_top<0 && pixel_bottom>0 )
    {
        nearest_x = 0;
        nearest_y = 0;
        r_near = rr_near = 0;
    }
    else
    {
        closest_point_to_line( cs, sn, pixel_center_x, pixel_center_y, &nearest_x, &nearest_y );
        nearest_x = CLAMP( nearest_x, pixel_left, pixel_right );
        nearest_y = CLAMP( nearest_y, pixel_top, pixel_bottom );
        // XXX: precision of "nearest" values could be improved
        // by intersecting the line that goes from nearest_x/Y to 0
        // with the pixel's borders here, however the improvements
        // would probably not justify the perdormance cost.
        r_near = calculate_r_sample( nearest_x, nearest_y, aspect_ratio, sn, cs );
        rr_near = r_near * one_over_radius2;
    }

    // out of dab's reach?
    if( rr_near > 1.0f )
        return rr_near;

    // check on which side of the dab's line is the pixel center
    float center_sign = sign_point_in_line( pixel_center_x, pixel_center_y, cs, -sn );

    // radius of a circle with area=1
    //   A = pi * r * r
    //   r = sqrt(1/pi)
    const float rad_area_1 = sqrtf( 1.0f / M_PI );

    // center is below dab
    if( center_sign < 0 )
    {
        farthest_x = nearest_x - sn*rad_area_1;
        farthest_y = nearest_y + cs*rad_area_1;
    }
    // above dab
    else
    {
        farthest_x = nearest_x + sn*rad_area_1;
        farthest_y = nearest_y - cs*rad_area_1;
    }

    r_far = calculate_r_sample( farthest_x, farthest_y, aspect_ratio, sn, cs );
    rr_far = r_far * one_over_radius2;

    // check if we can skip heavier AA
    if( r_far < r_aa_start )
        return (rr_far+rr_near) * 0.5f;

    // calculate AA approximate
    float visibilityNear = 1.0f - rr_near;
    float delta = rr_far - rr_near;
    float delta2 = 1.0f + delta;
    visibilityNear /= delta2;

    return 1.0f - visibilityNear;
}

static inline float
calculate_opa(float rr, float hardness,
              float segment1_offset, float segment1_slope,
              float segment2_offset, float segment2_slope) {

    const float fac = rr <= hardness ? segment1_slope : segment2_slope;
    float opa = rr <= hardness ? segment1_offset : segment2_offset;
    opa += rr*fac;

    if (rr > 1.0f) {
        opa = 0.0f;
    }
    #ifdef HEAVY_DEBUG
    assert(isfinite(opa));
    assert(opa >= 0.0f && opa <= 1.0f);
    #endif
    return opa;
}

// Must be threadsafe
void render_dab_mask (uint16_t * mask,
                        float x, float y,
                        float radius,
                        float hardness,
                        float aspect_ratio, float angle
                        )
{

    hardness = CLAMP(hardness, 0.0, 1.0);
    if (aspect_ratio<1.0) aspect_ratio=1.0;
    assert(hardness != 0.0); // assured by caller

    // For a graphical explanation, see:
    // http://wiki.mypaint.info/Development/Documentation/Brushlib
    //
    // The hardness calculation is explained below:
    //
    // Dab opacity gradually fades out from the center (rr=0) to
    // fringe (rr=1) of the dab. How exactly depends on the hardness.
    // We use two linear segments, for which we pre-calculate slope
    // and offset here.
    //
    // opa
    // ^
    // *   .
    // |        *
    // |          .
    // +-----------*> rr = (distance_from_center/radius)^2
    // 0           1
    //
    float segment1_offset = 1.0f;
    float segment1_slope  = -(1.0f/hardness - 1.0f);
    float segment2_offset = hardness/(1.0f-hardness);
    float segment2_slope  = -hardness/(1.0f-hardness);
    // for hardness == 1.0, segment2 will never be used

    float angle_rad=angle/360*2*M_PI;
    float cs=cos(angle_rad);
    float sn=sin(angle_rad);

    const float r_fringe = radius + 1.0f; // +1.0 should not be required, only to be sure
    int x0 = floor (x - r_fringe);
    int y0 = floor (y - r_fringe);
    int x1 = floor (x + r_fringe);
    int y1 = floor (y + r_fringe);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > MYPAINT_TILE_SIZE-1) x1 = MYPAINT_TILE_SIZE-1;
    if (y1 > MYPAINT_TILE_SIZE-1) y1 = MYPAINT_TILE_SIZE-1;
    const float one_over_radius2 = 1.0f/(radius*radius);

    // Pre-calculate rr and put it in the mask.
    // This an optimization that makes use of auto-vectorization
    // OPTIMIZE: if using floats for the brush engine, store these directly in the mask
    float rr_mask[MYPAINT_TILE_SIZE*MYPAINT_TILE_SIZE+2*MYPAINT_TILE_SIZE];

    if (radius < 3.0f)
    {
      const float aa_border = 1.0f;
      float r_aa_start = ((radius>aa_border) ? (radius-aa_border) : 0);
      r_aa_start *= r_aa_start / aspect_ratio;

      for (int yp = y0; yp <= y1; yp++) {
        for (int xp = x0; xp <= x1; xp++) {
          const float rr = calculate_rr_antialiased(xp, yp,
                                  x, y, aspect_ratio,
                                  sn, cs, one_over_radius2,
                                  r_aa_start);
          rr_mask[(yp*MYPAINT_TILE_SIZE)+xp] = rr;
        }
      }
    }
    else
    {
      for (int yp = y0; yp <= y1; yp++) {
        for (int xp = x0; xp <= x1; xp++) {
          const float rr = calculate_rr(xp, yp,
                                  x, y, aspect_ratio,
                                  sn, cs, one_over_radius2);
          rr_mask[(yp*MYPAINT_TILE_SIZE)+xp] = rr;
        }
      }
    }

    // we do run length encoding: if opacity is zero, the next
    // value in the mask is the number of pixels that can be skipped.
    uint16_t * mask_p = mask;
    int skip=0;

    skip += y0*MYPAINT_TILE_SIZE;
    for (int yp = y0; yp <= y1; yp++) {
      skip += x0;

      int xp;
      for (xp = x0; xp <= x1; xp++) {
        const float rr = rr_mask[(yp*MYPAINT_TILE_SIZE)+xp];
        const float opa = calculate_opa(rr, hardness,
                                  segment1_offset, segment1_slope,
                                  segment2_offset, segment2_slope);
        const uint16_t opa_ = opa * (1<<15);
        if (!opa_) {
          skip++;
        } else {
          if (skip) {
            *mask_p++ = 0;
            *mask_p++ = skip*4;
            skip = 0;
          }
          *mask_p++ = opa_;
        }
      }
      skip += MYPAINT_TILE_SIZE-xp;
    }
    *mask_p++ = 0;
    *mask_p++ = 0;
  }

// Must be threadsafe
void
process_op(uint16_t *rgba_p, uint16_t *mask,
           int tx, int ty, OperationDataDrawDab *op)
{

    // first, we calculate the mask (opacity for each pixel)
    render_dab_mask(mask,
                    op->x - tx*MYPAINT_TILE_SIZE,
                    op->y - ty*MYPAINT_TILE_SIZE,
                    op->radius,
                    op->hardness,
                    op->aspect_ratio, op->angle
                    );

    // second, we use the mask to stamp a dab for each activated blend mode
    if (op->paint < 1.0) {
      if (op->normal) {
        if (op->color_a == 1.0) {
          draw_dab_pixels_BlendMode_Normal(mask, rgba_p,
                                           op->color_r, op->color_g, op->color_b, op->normal*op->opaque*(1 - op->paint)*(1<<15));
        } else {
          // normal case for brushes that use smudging (eg. watercolor)
          draw_dab_pixels_BlendMode_Normal_and_Eraser(mask, rgba_p,
                                                      op->color_r, op->color_g, op->color_b, op->color_a*(1<<15),
                                                      op->normal*op->opaque*(1 - op->paint)*(1<<15));
        }
      }

      if (op->lock_alpha && op->color_a != 0) {
        draw_dab_pixels_BlendMode_LockAlpha(mask, rgba_p,
                                            op->color_r, op->color_g, op->color_b,
                                            op->lock_alpha*op->opaque*(1 - op->colorize)*(1 - op->posterize)*(1 - op->paint)*(1<<15));
      }
    }

    if (op->paint > 0.0) {
      if (op->normal) {
        if (op->color_a == 1.0) {
          draw_dab_pixels_BlendMode_Normal_Paint(mask, rgba_p,
                                           op->color_r, op->color_g, op->color_b, op->normal*op->opaque*op->paint*(1<<15));
        } else {
          // normal case for brushes that use smudging (eg. watercolor)
          draw_dab_pixels_BlendMode_Normal_and_Eraser_Paint(mask, rgba_p,
                                                      op->color_r, op->color_g, op->color_b, op->color_a*(1<<15),
                                                      op->normal*op->opaque*op->paint*(1<<15));
        }
      }

      if (op->lock_alpha && op->color_a != 0) {
        draw_dab_pixels_BlendMode_LockAlpha_Paint(mask, rgba_p,
                                            op->color_r, op->color_g, op->color_b,
                                            op->lock_alpha*op->opaque*(1 - op->colorize)*(1 - op->posterize)*op->paint*(1<<15));
      }
    }

    if (op->colorize) {
      draw_dab_pixels_BlendMode_Color(mask, rgba_p,
                                      op->color_r, op->color_g, op->color_b,
                                      op->colorize*op->opaque*(1<<15));
    }
    if (op->posterize) {
      draw_dab_pixels_BlendMode_Posterize(mask, rgba_p,
                                      op->posterize*op->opaque*(1<<15),
                                      op->posterize_num);
    }
}

// Must be threadsafe
void
process_tile_internal(
  void *tiled_surface,
  void (*request_start) (void*, void*),
  void (*request_end) (void*, void*),
  OperationQueue* op_queue, int tx, int ty)
{
    TileIndex tile_index = {tx, ty};
    OperationDataDrawDab *op = operation_queue_pop(op_queue, tile_index);
    if (!op) {
        return;
    }

    MyPaintTileRequest request_data;
    const int mipmap_level = 0;
    mypaint_tile_request_init(&request_data, mipmap_level, tx, ty, FALSE);

    request_start(tiled_surface, &request_data);
    uint16_t * rgba_p = request_data.buffer;
    if (!rgba_p) {
        printf("Warning: Unable to get tile!\n");
        return;
    }

    uint16_t mask[MYPAINT_TILE_SIZE*MYPAINT_TILE_SIZE+2*MYPAINT_TILE_SIZE];

    while (op) {
        process_op(rgba_p, mask, tile_index.x, tile_index.y, op);
        free(op);
        op = operation_queue_pop(op_queue, tile_index);
    }
    request_end(tiled_surface, &request_data);
}

void
update_dirty_bbox(MyPaintRectangle *bbox, OperationDataDrawDab *op)
{
    int bb_x, bb_y, bb_w, bb_h;
    float r_fringe = op->radius + 1.0f; // +1.0 should not be required, only to be sure
    bb_x = floor (op->x - r_fringe);
    bb_y = floor (op->y - r_fringe);
    bb_w = floor (op->x + r_fringe) - bb_x + 1;
    bb_h = floor (op->y + r_fringe) - bb_y + 1;

    mypaint_rectangle_expand_to_include_point(bbox, bb_x, bb_y);
    mypaint_rectangle_expand_to_include_point(bbox, bb_x+bb_w-1, bb_y+bb_h-1);
}

// returns TRUE if the surface was modified
gboolean draw_dab_internal (
  OperationQueue *op_queue, float x, float y,
  float radius,
  float color_r, float color_g, float color_b,
  float opaque, float hardness,
  float color_a,
  float aspect_ratio, float angle,
  float lock_alpha,
  float colorize,
  float posterize,
  float posterize_num,
  float paint,
  MyPaintRectangle *bbox
  )

{
    OperationDataDrawDab op_struct;
    OperationDataDrawDab *op = &op_struct;

    op->x = x;
    op->y = y;
    op->radius = radius;
    op->aspect_ratio = aspect_ratio;
    op->angle = angle;
    op->opaque = CLAMP(opaque, 0.0f, 1.0f);
    op->hardness = CLAMP(hardness, 0.0f, 1.0f);
    op->lock_alpha = CLAMP(lock_alpha, 0.0f, 1.0f);
    op->colorize = CLAMP(colorize, 0.0f, 1.0f);
    op->posterize = CLAMP(posterize, 0.0f, 1.0f);
    op->posterize_num= CLAMP(ROUND(posterize_num * 100.0), 1, 128);
    op->paint = CLAMP(paint, 0.0f, 1.0f);
    if (op->radius < 0.1f) return FALSE; // don't bother with dabs smaller than 0.1 pixel
    if (op->hardness == 0.0f) return FALSE; // infintly small center point, fully transparent outside
    if (op->opaque == 0.0f) return FALSE;

    color_r = CLAMP(color_r, 0.0f, 1.0f);
    color_g = CLAMP(color_g, 0.0f, 1.0f);
    color_b = CLAMP(color_b, 0.0f, 1.0f);
    color_a = CLAMP(color_a, 0.0f, 1.0f);

    op->color_r = color_r * (1<<15);
    op->color_g = color_g * (1<<15);
    op->color_b = color_b * (1<<15);
    op->color_a = color_a;

    // blending mode preparation
    op->normal = 1.0f;

    op->normal *= 1.0f-op->lock_alpha;
    op->normal *= 1.0f-op->colorize;
    op->normal *= 1.0f-op->posterize;

    if (op->aspect_ratio<1.0f) op->aspect_ratio=1.0f;

    // Determine the tiles influenced by operation, and queue it for processing for each tile
    float r_fringe = radius + 1.0f; // +1.0 should not be required, only to be sure

    int tx1 = floor(floor(x - r_fringe) / MYPAINT_TILE_SIZE);
    int tx2 = floor(floor(x + r_fringe) / MYPAINT_TILE_SIZE);
    int ty1 = floor(floor(y - r_fringe) / MYPAINT_TILE_SIZE);
    int ty2 = floor(floor(y + r_fringe) / MYPAINT_TILE_SIZE);

    for (int ty = ty1; ty <= ty2; ty++) {
        for (int tx = tx1; tx <= tx2; tx++) {
            const TileIndex tile_index = {tx, ty};
            OperationDataDrawDab *op_copy = (OperationDataDrawDab *)malloc(sizeof(OperationDataDrawDab));
            *op_copy = *op;
            operation_queue_add(op_queue, tile_index, op_copy);
        }
    }

    update_dirty_bbox(bbox, op);

    return TRUE;
}

// returns TRUE if the surface was modified
int draw_dab (MyPaintSurface *surface, float x, float y,
               float radius,
               float r, float g, float b,
               float opaque, float hardness,
               float color_a,
               float aspect_ratio, float angle,
               float lock_alpha,
               float colorize)
{
    MyPaintTiledSurface* self = (MyPaintTiledSurface*)surface;
    // Normal pass
    gboolean surface_modified = (draw_dab_internal(
        self->operation_queue, x, y, radius, r, g, b, opaque, hardness, color_a, aspect_ratio, angle, lock_alpha,
        colorize, 0.0, 0.0, 0.0, &self->dirty_bbox));
    // Symmetry pass
    if (surface_modified && self->surface_do_symmetry) {
        const float symm_x = self->surface_center_x + (self->surface_center_x - x);
        draw_dab_internal(
            self->operation_queue, symm_x, y, radius, r, g, b, opaque, hardness, color_a, aspect_ratio, -angle,
            lock_alpha, colorize, 0.0, 0.0, 0.0, &self->dirty_bbox);
    }
    return surface_modified;
}


void get_color_internal
(
 void *tiled_surface,
 void (*request_start) (void*, void*),
 void (*request_end) (void*, void*),
 gboolean threadsafe_tile_requests,
 OperationQueue *op_queue,
 float x, float y,
 float radius,
 float * color_r, float * color_g, float * color_b, float * color_a,
 float paint
  )
{
    if (radius < 1.0f) radius = 1.0f;
    const float hardness = 0.5f;
    const float aspect_ratio = 1.0f;
    const float angle = 0.0f;

    float sum_weight, sum_r, sum_g, sum_b, sum_a;
    sum_weight = sum_r = sum_g = sum_b = sum_a = 0.0f;

    // in case we return with an error
    *color_r = 0.0f;
    *color_g = 1.0f;
    *color_b = 0.0f;

    // WARNING: some code duplication with draw_dab

    float r_fringe = radius + 1.0f; // +1 should not be required, only to be sure

    int tx1 = floor(floor(x - r_fringe) / MYPAINT_TILE_SIZE);
    int tx2 = floor(floor(x + r_fringe) / MYPAINT_TILE_SIZE);
    int ty1 = floor(floor(y - r_fringe) / MYPAINT_TILE_SIZE);
    int ty2 = floor(floor(y + r_fringe) / MYPAINT_TILE_SIZE);
    #ifdef _OPENMP
    int tiles_n = (tx2 - tx1) * (ty2 - ty1);
    #endif

    // Calculate the `guaranteed sample` interval and
    // the percentage of pixels to sample for the dab.
    // The basic idea is to have larger intervals and
    // lower percentages for really large dabs, to
    // avoid accumulated rounding errors and heavier
    // calculations.
    //
    // The values are set so that the number of pixels
    // sampled is _bounded_ linearly by the radius.
    //
    // The constant factor 7 is chosen through manual
    // evaluation of results and gives us a total sample
    // rate bounded by '1/(r * 3.5)'
    // Other models may have better properties, some
    // more thinking needed here.
    //
    // For really small radii we'll sample every pixel
    // in the dab to avoid biasing.
    const int sample_interval = radius <= 2.0f ? 1 : (int)(radius * 7);
    const float random_sample_rate = 1.0f / (7 * radius);

    #ifdef _OPENMP
    #pragma omp parallel for schedule(static) if(threadsafe_tile_requests && tiles_n > 3)
    #endif
    for (int ty = ty1; ty <= ty2; ty++) {
      for (int tx = tx1; tx <= tx2; tx++) {

        // Flush queued draw_dab operations
        process_tile_internal(tiled_surface, request_start, request_end, op_queue, tx, ty);

        MyPaintTileRequest request_data;
        const int mipmap_level = 0;
        mypaint_tile_request_init(&request_data, mipmap_level, tx, ty, TRUE);
        request_start(tiled_surface, &request_data);
        uint16_t * rgba_p = request_data.buffer;
        if (!rgba_p) {
          printf("Warning: Unable to get tile!\n");
          break;
        }

        // first, we calculate the mask (opacity for each pixel)
        uint16_t mask[MYPAINT_TILE_SIZE*MYPAINT_TILE_SIZE+2*MYPAINT_TILE_SIZE];

        render_dab_mask(mask,
                        x - tx*MYPAINT_TILE_SIZE,
                        y - ty*MYPAINT_TILE_SIZE,
                        radius,
                        hardness,
                        aspect_ratio, angle
                        );

        // TODO: try atomic operations instead
        #pragma omp critical
        {
        get_color_pixels_accumulate (
          mask, rgba_p, &sum_weight, &sum_r, &sum_g, &sum_b, &sum_a, paint,
          sample_interval, random_sample_rate);
        }

        request_end(tiled_surface, &request_data);
      }
    }

    assert(sum_weight > 0.0f);
    sum_a /= sum_weight;

    // For legacy sampling, we need to divide
    // by the total after the accumulation.
    if (paint < 0.0) {
        sum_r /= sum_weight;
        sum_g /= sum_weight;
        sum_b /= sum_weight;
    }

    *color_a = CLAMP(sum_a, 0.0f, 1.0f);
    if (sum_a > 0.0f) {
      // Straighten the color channels if using legacy sampling.
      // Clamp to guard against rounding errors.
      const float demul = paint < 0.0 ? sum_a : 1.0;
      *color_r = CLAMP(sum_r / demul, 0.0f, 1.0f);
      *color_g = CLAMP(sum_g / demul, 0.0f, 1.0f);
      *color_b = CLAMP(sum_b / demul, 0.0f, 1.0f);
    } else {
      // it is all transparent, so don't care about the colors
      // (let's make them ugly so bugs will be visible)
      *color_r = 0.0f;
      *color_g = 1.0f;
      *color_b = 0.0f;
    }
}

/* Go-betweens for more clarity  */
void tsf1_request_start(void* surface, void* request) {
  MyPaintTiledSurface *self = (MyPaintTiledSurface*) surface;
  self->tile_request_start(self, (MyPaintTileRequest*) request);
}
void tsf1_request_end(void* surface, void* request) {
  MyPaintTiledSurface *self = (MyPaintTiledSurface*) surface;
  self->tile_request_end(self, (MyPaintTileRequest*) request);
}

void
get_color(
    MyPaintSurface* surface, float x, float y, float radius, float* color_r, float* color_g, float* color_b,
    float* color_a)
{
    MyPaintTiledSurface* self = (MyPaintTiledSurface*)surface;
    return get_color_internal(
        surface, tsf1_request_start, tsf1_request_end, self->threadsafe_tile_requests, self->operation_queue, x, y,
        radius, color_r, color_g, color_b, color_a, -1.0);
}

void tiled_surface_process_tile(MyPaintTiledSurface *self, int tx, int ty) {
  process_tile_internal(self, tsf1_request_start, tsf1_request_end, self->operation_queue, tx, ty);
}

/**
 * mypaint_tiled_surface_init: (skip)
 *
 * Initialize the surface, passing in implementations of the tile backend.
 * Note: Only intended to be called from subclasses of #MyPaintTiledSurface
 **/
void
mypaint_tiled_surface_init(MyPaintTiledSurface *self,
                           MyPaintTileRequestStartFunction tile_request_start,
                           MyPaintTileRequestEndFunction tile_request_end)
{
    mypaint_surface_init(&self->parent);
    self->parent.draw_dab = draw_dab;
    self->parent.get_color = get_color;
    self->parent.begin_atomic = begin_atomic_default;
    self->parent.end_atomic = end_atomic_default;

    self->tile_request_end = tile_request_end;
    self->tile_request_start = tile_request_start;

    self->tile_size = MYPAINT_TILE_SIZE;
    self->threadsafe_tile_requests = FALSE;

    self->dirty_bbox.x = 0;
    self->dirty_bbox.y = 0;
    self->dirty_bbox.width = 0;
    self->dirty_bbox.height = 0;
    self->surface_do_symmetry = FALSE;
    self->surface_center_x = 0.0f;
    self->operation_queue = operation_queue_new();
}


/**
 * mypaint_tiled_surface_destroy: (skip)
 *
 * Deallocate resources set up by mypaint_tiled_surface_init()
 * Does not free the #MyPaintTiledSurface itself.
 * Note: Only intended to be called from subclasses of #MyPaintTiledSurface
 */
void
mypaint_tiled_surface_destroy(MyPaintTiledSurface *self)
{
    operation_queue_free(self->operation_queue);
}

/* -- Extended interface -- */

/* Go-betweens for more clarity  */
void tsf2_request_start(void* surface, void* request) {
  MyPaintTiledSurface2 *self = (MyPaintTiledSurface2*) surface;
  self->tile_request_start(self, (MyPaintTileRequest*) request);
}

void tsf2_request_end(void* surface, void* request) {
  MyPaintTiledSurface2 *self = (MyPaintTiledSurface2*) surface;
  self->tile_request_end(self, (MyPaintTileRequest*) request);
}

void tiled_surface2_process_tile(MyPaintTiledSurface2 *self, int tx, int ty) {
  process_tile_internal(self, tsf2_request_start, tsf2_request_end, self->operation_queue, tx, ty);
}

void
get_color_pigment(
    MyPaintSurface2* surface, float x, float y, float radius, float* color_r, float* color_g, float* color_b,
    float* color_a, float paint)
{
    MyPaintTiledSurface2* self = (MyPaintTiledSurface2*)surface;
    return get_color_internal(
        surface, tsf2_request_start, tsf2_request_end, self->threadsafe_tile_requests, self->operation_queue, x, y,
        radius, color_r, color_g, color_b, color_a, paint);
}

static void
begin_atomic_default_2(MyPaintSurface *surface)
{
  mypaint_tiled_surface2_begin_atomic((MyPaintTiledSurface2 *)surface);
}

static void
end_atomic_default_2(MyPaintSurface2 *surface, MyPaintRectangles *roi)
{
    mypaint_tiled_surface2_end_atomic((MyPaintTiledSurface2 *)surface, roi);
}

void
prepare_bounding_boxes(MyPaintTiledSurface2 *self) {
    MyPaintSymmetryState symm_state = self->symmetry_data.state_current;
    const gboolean snowflake = symm_state.type == MYPAINT_SYMMETRY_TYPE_SNOWFLAKE;
    const int num_bboxes_desired = symm_state.num_lines * (snowflake ? 2 : 1);
    // If the bounding box array cannot fit one rectangle per symmetry dab,
    // try to allocate enough space for that to be possible.
    // Failure is ok, as the bounding box assignments will be functional anyway.
    if (num_bboxes_desired > self->num_bboxes) {
        const int margin = 10; // Add margin to avoid unnecessary reallocations.
        const int num_to_allocate = num_bboxes_desired + margin;
        int bytes_to_allocate = num_to_allocate * sizeof(MyPaintRectangle);
        MyPaintRectangle* new_bboxes = malloc(bytes_to_allocate);
        if (new_bboxes) {
            free(self->bboxes);
            // Initialize memory
            memset(new_bboxes, 0, bytes_to_allocate);
            self->bboxes = new_bboxes;
            self->num_bboxes = num_to_allocate;
            // No need to clear anything after the memset, so reset counter
            self->num_bboxes_dirtied = 0;
        }
    }
    // Clean up any previously populated bounding boxes and reset the counter
    for (int i = 0; i < MIN(self->num_bboxes, self->num_bboxes_dirtied); ++i) {
        self->bboxes[i].height = 0;
        self->bboxes[i].width = 0;
        self->bboxes[i].x = 0;
        self->bboxes[i].y = 0;
    }
    self->num_bboxes_dirtied = 0;
}

// returns TRUE if the surface was modified
int draw_dab_2 (MyPaintSurface2 *surface, float x, float y,
               float radius,
               float color_r, float color_g, float color_b,
               float opaque, float hardness,
               float color_a,
               float aspect_ratio, float angle,
               float lock_alpha,
               float colorize,
               float posterize,
               float posterize_num,
               float paint)
{
    MyPaintTiledSurface2* self = (MyPaintTiledSurface2*)surface;

    // These calls are repeated enough to warrant a local macro, for both readability and correctness.
#define DDI(x, y, angle, bb_idx) (draw_dab_internal(\
        self->operation_queue, (x), (y), radius, color_r, color_g, color_b, opaque, \
        hardness, color_a, aspect_ratio, (angle), \
        lock_alpha, colorize, posterize, posterize_num, paint, &self->bboxes[(bb_idx)]))

    // Normal pass
    gboolean surface_modified = DDI(x, y, angle, 0);

    int num_bboxes_used = surface_modified ? 1 : 0;

    // Symmetry pass

    // OPTIMIZATION: skip the symmetry pass if surface was not modified by the initial dab;
    // at current if the initial dab does not modify the surface, none of the symmetry dabs
    // will either. If/when selection masks are added, this optimization _must_ be removed,
    // and `surface_modified` must be or'ed with the result of each call to draw_dab_internal.
    MyPaintSymmetryData *symm_data = &self->symmetry_data;
    if (surface_modified && symm_data->active && symm_data->num_symmetry_matrices) {
        const MyPaintSymmetryState symm = symm_data->state_current;
        const int num_bboxes = self->num_bboxes;
        const float rot_angle = 360.0 / symm.num_lines;
        const MyPaintTransform* const matrices = symm_data->symmetry_matrices;
        float x_out, y_out;

        switch (symm.type) {
        case MYPAINT_SYMMETRY_TYPE_VERTICAL: {
            mypaint_transform_point(&matrices[0], x, y, &x_out, &y_out);
            DDI(x_out, y_out, -2.0 * (90 + symm.angle) - angle, 1);
            num_bboxes_used = 2;
            break;
        }
        case MYPAINT_SYMMETRY_TYPE_HORIZONTAL: {
            mypaint_transform_point(&matrices[0], x, y, &x_out, &y_out);
            DDI(x_out, y_out, -2.0 * symm.angle - angle, 1);
            num_bboxes_used = 2;
            break;
        }
        case MYPAINT_SYMMETRY_TYPE_VERTHORZ: {
            // Reflect across horizontal line
            mypaint_transform_point(&matrices[0], x, y, &x_out, &y_out);
            DDI(x_out, y_out, -2.0 * symm.angle - angle, 1);
            // Then across the vertical line (diagonal)
            mypaint_transform_point(&matrices[1], x, y, &x_out, &y_out);
            DDI(x_out, y_out, angle, 2);
            // Then back across the horizontal line
            mypaint_transform_point(&matrices[2], x, y, &x_out, &y_out);
            DDI(x_out, y_out, -2.0 * symm.angle - angle, 3);
            num_bboxes_used = 4;
            break;
        }
        case MYPAINT_SYMMETRY_TYPE_SNOWFLAKE: {

            // These dabs will occupy the bboxes after the last bbox used by the rotational dabs.
            const int offset = MIN(num_bboxes / 2, symm.num_lines);
            const float dabs_per_bbox = MAX(1, (float)symm.num_lines * 2.0 / num_bboxes);
            const int base_idx = symm.num_lines - 1;
            const float base_angle = -2 * symm.angle - angle;
            // draw snowflake dabs for _all_ symmetry lines as we need to reflect the initial dab.
            for (int dab_count = 0; dab_count < symm.num_lines; dab_count++) {
                // If the number of bboxes cannot fit all snowflake dabs, use half for the rotational dabs
                // and the other half for the reflected dabs. This is not always optimal, but seldom bad.
                const int bbox_idx = offset + MIN(roundf(dab_count / dabs_per_bbox), num_bboxes - 1);
                mypaint_transform_point(&matrices[base_idx + dab_count], x, y, &x_out, &y_out);
                DDI(x_out, y_out, base_angle - dab_count * rot_angle, bbox_idx);
            }
            num_bboxes_used = MIN(self->num_bboxes, symm.num_lines * 2);
            // fall through to rotational to finish the process
        }
        case MYPAINT_SYMMETRY_TYPE_ROTATIONAL: {

            // Set the dab bbox distribution factor based on whether the pass is only
            // rotational, or following a snowflake pass. For the latter, we compress
            // the available range (unimportant if there are enough bboxes to go around).
            const gboolean snowflake = symm.type == MYPAINT_SYMMETRY_TYPE_SNOWFLAKE;
            float dabs_per_bbox = MAX(1, (float)(symm.num_lines * (snowflake ? 2 : 1)) / num_bboxes);

            // draw self->rot_symmetry_lines - 1 rotational dabs since initial pass handles the first dab
            for (int dab_count = 1; dab_count < symm.num_lines; dab_count++) {
                const int bbox_index = MIN(roundf(dab_count / dabs_per_bbox), num_bboxes - 1);
                mypaint_transform_point(&matrices[dab_count - 1], x, y, &x_out, &y_out);
                DDI(x_out, y_out, angle - dab_count * rot_angle, bbox_index);
            }

            // Use existing (larger) number of bboxes if it was set (in a snowflake pass)
            num_bboxes_used = MIN(self->num_bboxes, MAX(symm.num_lines, num_bboxes_used));
            break;
        }
        default:
            fprintf(stderr, "Warning: Unhandled symmetry type: %d\n", symm.type);
            break;
        }
    }
    self->num_bboxes_dirtied = MIN(self->num_bboxes, num_bboxes_used);
    return surface_modified;
#undef DDI
}

int
draw_dab_wrapper(
    MyPaintSurface* surface, float x, float y, float radius, float r, float g, float b, float opaque, float hardness,
    float color_a, float aspect_ratio, float angle, float lock_alpha, float colorize)
{
    const float posterize = 0.0;
    const float posterize_num = 1.0;
    const float pigment = 0.0;
    return draw_dab_2(
        (MyPaintSurface2*)surface, x, y, radius, r, g, b, opaque, hardness, color_a, aspect_ratio, angle, lock_alpha,
        colorize, posterize, posterize_num, pigment);
}

void
get_color_wrapper(
    MyPaintSurface* surface, float x, float y, float radius, float* color_r, float* color_g, float* color_b,
    float* color_a)
{
    MyPaintTiledSurface2* self = (MyPaintTiledSurface2*)surface;
    return get_color_internal(
        surface, tsf2_request_start, tsf2_request_end, self->threadsafe_tile_requests, self->operation_queue, x, y,
        radius, color_r, color_g, color_b, color_a, -1.0);
}

static void
end_atomic_wrapper(MyPaintSurface *surface, MyPaintRectangle *roi)
{
  MyPaintRectangles rois = {1, roi};
  mypaint_tiled_surface2_end_atomic((MyPaintTiledSurface2*)surface, &rois);
}

/**
 * mypaint_tiled_surface2_init: (skip)
 *
 * Initialize the surface, passing in implementations of the tile backend.
 * Note: Only intended to be called from subclasses of #MyPaintTiledSurface
 **/
void
mypaint_tiled_surface2_init(MyPaintTiledSurface2 *self,
                           MyPaintTileRequestStartFunction2 tile_request_start,
                           MyPaintTileRequestEndFunction2 tile_request_end)
{
    mypaint_surface_init(&self->parent.parent);

    self->tile_request_end = tile_request_end;
    self->tile_request_start = tile_request_start;
    self->tile_size = MYPAINT_TILE_SIZE;
    self->threadsafe_tile_requests = FALSE;
    self->operation_queue = operation_queue_new();

    MyPaintSurface2 *s = &self->parent;

    s->draw_dab_pigment = draw_dab_2;
    s->get_color_pigment = get_color_pigment;
    s->end_atomic_multi = end_atomic_default_2;
    s->parent.begin_atomic = begin_atomic_default_2;

    // Adapters supporting the base interface
    s->parent.draw_dab = draw_dab_wrapper;
    s->parent.get_color = get_color_wrapper;
    s->parent.end_atomic = end_atomic_wrapper;

    self->num_bboxes = NUM_BBOXES_DEFAULT;
    self->bboxes = malloc(sizeof(MyPaintRectangle) * NUM_BBOXES_DEFAULT);
    memset(self->bboxes, 0, sizeof(MyPaintRectangle) * NUM_BBOXES_DEFAULT);
    self->symmetry_data = mypaint_default_symmetry_data();
}

void
mypaint_tiled_surface2_begin_atomic(MyPaintTiledSurface2 *self)
{
    mypaint_update_symmetry_state(&self->symmetry_data);
    prepare_bounding_boxes(self);
}

/**
 * mypaint_tiled_surface_end_atomic_2: (skip)
 *
 * Implementation of #MyPaintSurface::end_atomic vfunc
 * Note: Only intended to be used from #MyPaintTiledSurface subclasses, which should chain up to this
 * if implementing their own #MyPaintSurface::end_atomic vfunc.
 * Application code should only use mypaint_surface_end_atomic().
 */
void
mypaint_tiled_surface2_end_atomic(MyPaintTiledSurface2 *self, MyPaintRectangles *roi)
{
    // Process tiles
    TileIndex *tiles;
    int tiles_n = operation_queue_get_dirty_tiles(self->operation_queue, &tiles);

    #pragma omp parallel for schedule(static) if(self->threadsafe_tile_requests && tiles_n > 3)
    for (int i = 0; i < tiles_n; i++) {
      tiled_surface2_process_tile(self, tiles[i].x, tiles[i].y);
    }

    operation_queue_clear_dirty_tiles(self->operation_queue);

    if (roi) {
        const int roi_rects = roi->num_rectangles;
        const int num_dirty = self->num_bboxes_dirtied;
        // Clear out the input rectangles that will be overwritten
        for (int i = 0; i < MIN(roi_rects, num_dirty); ++i) {
            roi->rectangles[i].x = 0;
            roi->rectangles[i].y = 0;
            roi->rectangles[i].width = 0;
            roi->rectangles[i].height = 0;
        }
        // Write bounding box rectangles to the output array
        const float bboxes_per_output = MAX(1, (float)num_dirty / roi_rects);
        for (int i = 0; i < num_dirty; ++i) {
            int out_index;
            // If there is not enough space for all rectangles in the output,
            // merge some of the rectangles with their list-adjacent neighbours.
            if (num_dirty > roi_rects) {
                out_index = (int)MIN(roi_rects - 1, roundf((float)i / bboxes_per_output));
            } else {
                out_index = i;
            }
            mypaint_rectangle_expand_to_include_rect(&(roi->rectangles[out_index]), &(self->bboxes[i]));
        }
        // Set the number of rectangles written to, so the caller knows which ones to act on.
        roi->num_rectangles = MIN(roi_rects, num_dirty);
    }
}

/**
 * mypaint_tiled_surface_set_symmetry_state_2: (skip)
 * @active: TRUE to enable, FALSE to disable.
 * @center_x: X axis to mirror events across.
 * @center_y: Y axis to mirror events across.
 * @symmetry_angle: Angle to rotate the symmetry lines
 * @symmetry_type: Symmetry type to activate.
 * @rot_symmetry_lines: Number of rotational symmetry lines.
 *
 * Enable/Disable symmetric brush painting across an X axis.
 *
 */
void
mypaint_tiled_surface2_set_symmetry_state(MyPaintTiledSurface2 *self, gboolean active,
                                         float center_x, float center_y,
                                         float symmetry_angle,
                                         MyPaintSymmetryType symmetry_type,
                                         int rot_symmetry_lines)
{
    mypaint_symmetry_set_pending( // Only write to the pending new state, nothing gets recalculated here
        &self->symmetry_data, active, center_x, center_y, symmetry_angle, symmetry_type, rot_symmetry_lines);
}

/**
 * mypaint_tiled_surface2_destroy: (skip)
 *
 * Deallocate resources set up by mypaint_tiled_surface2_init()
 * Does not free the #MyPaintTiledSurface itself.
 * Note: Only intended to be called from subclasses of #MyPaintTiledSurface
 */
void
mypaint_tiled_surface2_destroy(MyPaintTiledSurface2 *self)
{
    operation_queue_free(self->operation_queue);
    free(self->bboxes);
    mypaint_symmetry_data_destroy(&self->symmetry_data);
}
