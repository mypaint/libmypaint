#ifndef MYPAINTMATRIX_H
#define MYPAINTMATRIX_H

/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2019 The MyPaint Team
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

typedef struct {
  float rows[3][3];
} MyPaintTransform;

MyPaintTransform mypaint_transform_unit();
MyPaintTransform mypaint_transform_rotate_cw(const MyPaintTransform transform, const float angle_radians);
MyPaintTransform mypaint_transform_rotate_ccw(const MyPaintTransform transform, const float angle_radians);
MyPaintTransform mypaint_transform_reflect(const MyPaintTransform transform, const float angle_radians);
MyPaintTransform mypaint_transform_translate(const MyPaintTransform transform, const float x, const float y);

void mypaint_transform_point(const MyPaintTransform* const t, float x, float y, float* x_out, float* y_out);

#endif
