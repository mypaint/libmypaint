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

/*!
 * @brief 3x3 matrix of floats, row-major order.
 *
 * Only used in MyPaintTiledSurface2 for symmetry calculations, not
 * required for any calls to public functions.
 *
 * _The containing header should probably not have been made public._
 *
 */
typedef struct {
  float rows[3][3];
} MyPaintTransform;

/*!
 * @brief 3x3 unit matrix
 *
 * <pre>
 * 1 0 0
 * 0 1 0
 * 0 0 1
 * </pre>
 *
 * @memberof MyPaintTransform
 */
MyPaintTransform mypaint_transform_unit();

/*!
 * @brief Create a new transform by rotating the input transform clockwise.
 *
 * @param transform input transform
 * @param angle_radians The amount of rotation to apply, in radians.
 * @return a 3x3 unit matrix
 *
 * @memberof MyPaintTransform
 */
MyPaintTransform mypaint_transform_rotate_cw(const MyPaintTransform transform, const float angle_radians);

/*!
 * @brief Create a new transform by rotating the input transform counterclockwise.
 *
 * @param transform input transform
 * @param angle_radians The amount of rotation to apply, in radians.
 * @return the resulting transformation matrix
 *
 * @memberof MyPaintTransform
 */
MyPaintTransform mypaint_transform_rotate_ccw(const MyPaintTransform transform, const float angle_radians);

/*!
 * @brief Create a new transform by reflecting the input transform across a line crossing 0,0.
 *
 * @param transform input transform
 * @param angle_radians the angle of the line to reflect across, in radians
 * @return the resulting transformation matrix
 *
 * @memberof MyPaintTransform
 */
MyPaintTransform mypaint_transform_reflect(const MyPaintTransform transform, const float angle_radians);

/*!
 * @brief Create a new transform by reflecting the input transform across a line crossing 0,0.
 *
 * @param transform Input transform
 * @param x, y The translation to apply
 * @return the resulting transformation matrix
 *
 * @memberof MyPaintTransform
 */
MyPaintTransform mypaint_transform_translate(const MyPaintTransform transform, const float x, const float y);

/*!
 * @brief Apply transformation to a (x, y) point, writing the resulting coordinates to out parameters
 *
 * @param transform Transform to apply
 * @param x x coordinate to transform
 * @param y y coordinate to transform
 * @param[out] x_out pointer to x coordinate of the transformed point
 * @param[out] y_out pointer to y coordinate of the transformed point
 *
 * @memberof MyPaintTransform
 */
void mypaint_transform_point(const MyPaintTransform* const transform, float x, float y, float* x_out, float* y_out);

#endif
