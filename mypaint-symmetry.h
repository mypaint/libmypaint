#ifndef MYPAINTSYMMETRY_H
#define MYPAINTSYMMETRY_H
/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2017-2019 The MyPaint Team
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
#include "mypaint-glib-compat.h"

/**
  * MyPaintSymmetryType: Enumeration of different kinds of symmetry
  *
  * Prefix = 'MYPAINT_SYMMETRY_TYPE_'
  * VERTICAL: reflection across the y-axis
  * HORIZONTAL: reflection across the x-axis
  * VERTHORZ: reflection across x-axis and y-axis, special case of SNOWFLAKE
  * ROTATIONAL: rotational symmetry by N symmetry lines around a point
  * SNOWFLAKE: rotational symmetry w. reflection across the N symmetry lines
  */
typedef enum {
    MYPAINT_SYMMETRY_TYPE_VERTICAL,
    MYPAINT_SYMMETRY_TYPE_HORIZONTAL,
    MYPAINT_SYMMETRY_TYPE_VERTHORZ,
    MYPAINT_SYMMETRY_TYPE_ROTATIONAL,
    MYPAINT_SYMMETRY_TYPE_SNOWFLAKE,
    MYPAINT_SYMMETRY_TYPES_COUNT
} MyPaintSymmetryType;


/**
  * MyPaintSymmetryState: Contains the basis for symmetry calculations
  *
  * This is used to calculate the matrices that are
  * used for the actual symmetry calculations, and to
  * determine whether the matrices need to be recalculated.
  */
typedef struct {
  MyPaintSymmetryType type;
  float center_x;
  float center_y;
  float angle;
  float num_lines;
} MyPaintSymmetryState;

/**
  * MyPaintSymmetryData: Contains data used for symmetry calculations
  *
  * Instances contain a current and pending symmetry basis, and the
  * matrices used for the actual symmetry transforms. When the pending
  * state is modified, the "pending_changes" flag should be set.
  * Matrix recalculation should not be performed during draw operations.
  */
typedef struct {
  MyPaintSymmetryState state_current;
  MyPaintSymmetryState state_pending;
  gboolean pending_changes;
  gboolean active;
  int num_symmetry_matrices;
  MyPaintTransform *symmetry_matrices;
} MyPaintSymmetryData;

void mypaint_update_symmetry_state(MyPaintSymmetryData * const symmetry_data);

MyPaintSymmetryData mypaint_default_symmetry_data();

void mypaint_symmetry_data_destroy(MyPaintSymmetryData *);

void mypaint_symmetry_set_pending(
    MyPaintSymmetryData* data, gboolean active, float center_x, float center_y,
    float symmetry_angle, MyPaintSymmetryType symmetry_type, int rot_symmetry_lines);

#endif
