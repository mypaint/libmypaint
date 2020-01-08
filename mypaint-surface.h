#ifndef MYPAINTSURFACE_H
#define MYPAINTSURFACE_H

/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2008 Martin Renold <martinxyz@gmx.ch>
 * Copyright (C) 2012 Jon Nordby <jononor@gmail.com>
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

#include "mypaint-config.h"
#include "mypaint-rectangle.h"

G_BEGIN_DECLS

typedef struct MyPaintSurface MyPaintSurface;

typedef void (*MyPaintSurfaceGetColorFunction) (MyPaintSurface *self,
                                                float x, float y,
                                                float radius,
                                                float * color_r, float * color_g, float * color_b, float * color_a
                                                );

typedef int (*MyPaintSurfaceDrawDabFunction) (MyPaintSurface *self,
                       float x, float y,
                       float radius,
                       float color_r, float color_g, float color_b,
                       float opaque, float hardness,
                       float alpha_eraser,
                       float aspect_ratio, float angle,
                       float lock_alpha,
                       float colorize);

typedef void (*MyPaintSurfaceDestroyFunction) (MyPaintSurface *self);

typedef void (*MyPaintSurfaceSavePngFunction) (MyPaintSurface *self, const char *path, int x, int y, int width, int height);

typedef void (*MyPaintSurfaceBeginAtomicFunction) (MyPaintSurface *self);

typedef void (*MyPaintSurfaceEndAtomicFunction) (MyPaintSurface *self, MyPaintRectangle *roi);

/**
  * MyPaintSurface:
  *
  * Abstract surface type for the MyPaint brush engine. The surface interface
  * lets the brush engine specify dabs to render, and to pick color.
  */
struct MyPaintSurface {
    MyPaintSurfaceDrawDabFunction draw_dab;
    MyPaintSurfaceGetColorFunction get_color;
    MyPaintSurfaceBeginAtomicFunction begin_atomic;
    MyPaintSurfaceEndAtomicFunction end_atomic;
    MyPaintSurfaceDestroyFunction destroy;
    MyPaintSurfaceSavePngFunction save_png;
    int refcount;
};

/**
  * mypaint_surface_draw_dab:
  *
  * Draw a dab onto the surface.
  */
int mypaint_surface_draw_dab(
    MyPaintSurface* self, float x, float y, float radius, float color_r, float color_g, float color_b, float opaque,
    float hardness, float alpha_eraser, float aspect_ratio, float angle, float lock_alpha, float colorize);

void mypaint_surface_get_color(
    MyPaintSurface* self, float x, float y, float radius, float* color_r, float* color_g, float* color_b,
    float* color_a);

float mypaint_surface_get_alpha(MyPaintSurface* self, float x, float y, float radius);

void
mypaint_surface_save_png(MyPaintSurface *self, const char *path, int x, int y, int width, int height);

void mypaint_surface_begin_atomic(MyPaintSurface *self);

void mypaint_surface_end_atomic(MyPaintSurface *self, MyPaintRectangle *roi);

void mypaint_surface_init(MyPaintSurface *self);
void mypaint_surface_ref(MyPaintSurface *self);
void mypaint_surface_unref(MyPaintSurface *self);


/* Extended interface */

typedef struct MyPaintSurface2 MyPaintSurface2;


typedef int (*MyPaintSurfaceDrawDabFunction2) (
  MyPaintSurface2 *self,
  float x, float y,
  float radius,
  float color_r, float color_g, float color_b,
  float opaque, float hardness,
  float alpha_eraser,
  float aspect_ratio, float angle,
  float lock_alpha,
  float colorize,
  float posterize,
  float posterize_num,
  float paint);

typedef void (*MyPaintSurfaceGetColorFunction2) (
  MyPaintSurface2 *self,
  float x, float y,
  float radius,
  float * color_r, float * color_g, float * color_b, float * color_a,
  float paint
  );


typedef void (*MyPaintSurfaceEndAtomicFunction2) (MyPaintSurface2 *self, MyPaintRectangles *roi);

/**
 * MyPaintSurface2: (skip)
 *
 * This extends the regular MyPaintSurface with three new
 * functions that support the use of spectral blending,
 * for drawing dabs and getting color, and additionally
 * supports using multiple invalidation rectangles when
 * finalizing drawing operations.
 *
 * The interface functions for MyPaintSurface can be called
 * with instances of MyPaintSurface2 via the use of
 * mypaint_surface2_to_surface (or just casting). Concrete
 * implementations of MypaintSurface2 should support this.
 *
 */
struct MyPaintSurface2 {
    MyPaintSurface parent;
    MyPaintSurfaceDrawDabFunction2 draw_dab_pigment;
    MyPaintSurfaceGetColorFunction2 get_color_pigment;
    MyPaintSurfaceEndAtomicFunction2 end_atomic_multi;
};

MyPaintSurface* mypaint_surface2_to_surface(MyPaintSurface2 *self);

void mypaint_surface2_get_color(
    MyPaintSurface2* self, float x, float y, float radius, float* color_r, float* color_g, float* color_b,
    float* color_a, float paint);

void mypaint_surface2_end_atomic(MyPaintSurface2 *self, MyPaintRectangles *roi);

/**
  * mypaint_surface_draw_dab_2:
  *
  * Draw a dab onto the surface, with support for posterize/pigment
  */
int mypaint_surface2_draw_dab(
    MyPaintSurface2* self, float x, float y, float radius, float color_r, float color_g, float color_b, float opaque,
    float hardness, float alpha_eraser, float aspect_ratio, float angle, float lock_alpha, float colorize,
    float posterize, float posterize_num, float paint);

G_END_DECLS

#endif // MYPAINTSURFACE_H

