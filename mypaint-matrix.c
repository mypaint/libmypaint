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

#include "mypaint-matrix.h"
#include <math.h>

MyPaintTransform
mypaint_matrix_multiply(const MyPaintTransform m1, const MyPaintTransform m2)
{
    MyPaintTransform result;
    for (int row = 0; row < 3; ++row) {
      for (int col = 0; col < 3; ++col) {
	  result.rows[row][col] =
	    m1.rows[0][col] * m2.rows[row][0] +
	    m1.rows[1][col] * m2.rows[row][1] +
	    m1.rows[2][col] * m2.rows[row][2];
        }
    }
    return result;
}

MyPaintTransform
mypaint_transform_unit()
{
    MyPaintTransform m = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return m;
}

MyPaintTransform
mypaint_transform_rotate_cw(const MyPaintTransform transform, const float angle_radians)
{
    const float a = angle_radians;
    MyPaintTransform factor = {{{cos(a), sin(a), 0}, {-sin(a), cos(a), 0}, {0, 0, 1}}};
    return mypaint_matrix_multiply(transform, factor);
}

MyPaintTransform
mypaint_transform_rotate_ccw(const MyPaintTransform transform, const float angle_radians)
{
    const float a = angle_radians;
    MyPaintTransform factor = {{
        {cos(a), -sin(a), 0},
        {sin(a), cos(a), 0},
        {0, 0, 1},
    }};
    return mypaint_matrix_multiply(transform, factor);
}

MyPaintTransform
mypaint_transform_reflect(const MyPaintTransform transform, const float angle_radians)
{
    float x = cos(angle_radians);
    float y = sin(angle_radians);
    MyPaintTransform factor = {{
        {x * x - y * y, 2.0 * x * y, 0},
        {2.0 * x * y, y * y - x * x, 0},
        {0, 0, 1},
    }};
    return mypaint_matrix_multiply(transform, factor);
}


MyPaintTransform
mypaint_transform_translate(const MyPaintTransform transform, const float x, const float y)
{
    MyPaintTransform factor = {{
	{1, 0, x},
	{0, 1, y},
	{0, 0, 1},
      }};
    return mypaint_matrix_multiply(transform, factor);
}

void
mypaint_transform_point(const MyPaintTransform* const t, float x, float y, float* xout, float* yout)
{
    *xout = t->rows[0][0] * x + t->rows[0][1] * y + t->rows[0][2];
    *yout = t->rows[1][0] * x + t->rows[1][1] * y + t->rows[1][2];
}
