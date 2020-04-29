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

/*!
  * Enumeration of different kinds of symmetry
  *
  * @see MyPaintSymmetryState, MyPaintSymmetryData
  */
typedef enum {
    /*!
     * reflection across the (vertical) y-axis
     */
    MYPAINT_SYMMETRY_TYPE_VERTICAL,
    /*!
     * reflection across the (horizontal) x-axis
     */
    MYPAINT_SYMMETRY_TYPE_HORIZONTAL,
    /*!
     * reflection across both the x-axis and the y-axis
     */
    MYPAINT_SYMMETRY_TYPE_VERTHORZ,
    /*!
     * rotational symmetry
     */
    MYPAINT_SYMMETRY_TYPE_ROTATIONAL,
    /*!
     * rotational symmetry and its reflection
     */
    MYPAINT_SYMMETRY_TYPE_SNOWFLAKE,
    /*!
     * number of available symmetry types (only use to enumerate)
     */
    MYPAINT_SYMMETRY_TYPES_COUNT
} MyPaintSymmetryType;


/*!
  * Contains the basis for symmetry calculations
  *
  * The data in this structure is used to calculate the matrices that are
  * used for the actual symmetry calculations, and to determine when those
  * matrices need to be recalculated.
  *
  * @see MyPaintSymmetryData
  *
  */
typedef struct {
  /*!
   * The type of symmetry to use
   */
  MyPaintSymmetryType type;
  /*!
   * The x coordinate of the symmetry center
   */
  float center_x;
  /*!
   * The y coordinate of the symmetry center
   */
  float center_y;
  /*!
   * The angle of the symmetry, in radians
   */
  float angle;
  /*!
   * The number of symmetry lines to use, only relevant when #type is one of
   * @ref MYPAINT_SYMMETRY_TYPE_ROTATIONAL or @ref MYPAINT_SYMMETRY_TYPE_SNOWFLAKE
  */
  float num_lines;
} MyPaintSymmetryState;

/*!
  * Contains data used for symmetry calculations
  *
  * Instances contain a current and pending symmetry basis, and the
  * matrices used for the actual symmetry transforms. When the pending
  * state is modified, the "pending_changes" flag should be set.
  * Matrix recalculation should not be performed during draw operations.
  *
  * @see MyPaintTiledSurface2
  */
typedef struct {
  /*!
   * The current symmetry state. This is the data used for symmetry calculations
   * if #active is TRUE.
   */
  MyPaintSymmetryState state_current;
  /*!
   * The pending symmetry state. This is copied to #state_current when the
   * #symmetry_matrices are recalculated, and used to check whether the matrices
   * need to be recalculated.
   */
  MyPaintSymmetryState state_pending;
  /*!
   * Flag used to check if #state_pending needs to be compared
   * against #state_current (does not necessarily mean that the
   * #symmetry_matrices need to be recalculated.
   */
  gboolean pending_changes;
  /*!
   * Whether symmetry is used or not
   */
  gboolean active;
  /*!
   * The size of #symmetry_matrices, depends on __type__ and __num_lines__ of #state_current
   **/
  int num_symmetry_matrices;
  /*!
   * The matrices used for the actual symmetry calculations
   */
  MyPaintTransform *symmetry_matrices;
} MyPaintSymmetryData;

/*!
 * If necessary, recalculate #symmetry_matrices
 *
 * @memberof MyPaintSymmetryData
 */
void mypaint_update_symmetry_state(MyPaintSymmetryData * const symmetry_data);


/*!
 * Create a default symmetry data instance
 *
 * Creates a symmetry data object in an inactive state. Also attempts to
 * allocate space for an initial fixed number of matrices. If the allocation
 * is successful, the data is initialized, otherwise #symmetry_matrices is
 * NULL, and the object is left uninitialized.
 *
 * @memberof MyPaintSymmetryData
 */
MyPaintSymmetryData mypaint_default_symmetry_data();


/*!
 * Destroy resources used by the data struct, and the struct itself.
 *
 * @memberof MyPaintSymmetryData
 */
void mypaint_symmetry_data_destroy(MyPaintSymmetryData *);

/*!
 * Update #state_pending and #active and set #pending_changes to TRUE.
 *
 * Apart from __active__, the arguments correspond to the fields of
 * MyPaintSymmetryState
 *
 * @memberof MyPaintSymmetryData
 */
void mypaint_symmetry_set_pending(
    MyPaintSymmetryData* data, gboolean active, float center_x, float center_y,
    float symmetry_angle, MyPaintSymmetryType symmetry_type, int rot_symmetry_lines);

#endif
