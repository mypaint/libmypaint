#ifndef MYPAINTTILEDSURFACE_H
#define MYPAINTTILEDSURFACE_H

#include <stdint.h>
#include "mypaint-surface.h"
#include "mypaint-symmetry.h"
#include "mypaint-config.h"

G_BEGIN_DECLS

typedef struct MyPaintTiledSurface MyPaintTiledSurface;
typedef struct MyPaintTiledSurface2 MyPaintTiledSurface2;

/*!
 * Tile request used by MyPaintTiledSurface and MyPaintTiledSurface2
 *
 * Request for tile data for a given tile coordinate (tx, ty), also acting
 * as the response by defining fields to be populated by the receiver of
 * the request.
 *
 */
typedef struct {
    /*! The x-coordinate of the requested tile */
    int tx;
    /*! The y-coordinate of the requested tile */
    int ty;
    /*! Whether the tile data should be considered read-only */
    gboolean readonly;
    /*! Pointer to the tile buffer (set by requestee)*/
    guint16 *buffer;
    /*! Additional data to be used by surface implementations __(unused)__.*/
    gpointer context; /* Only to be used by the surface implementations. */
    /*! Identifier of the thread from which the request is made.*/
    int thread_id;
    /*! The mipmap level for which to fetch the tile __(unused)__.*/
    int mipmap_level;
} MyPaintTileRequest;

/*!
 * Initiatilze a tile request
 *
 * @memberof MyPaintTileRequest
 */
void
mypaint_tile_request_init(MyPaintTileRequest *data, int level,
                          int tx, int ty, gboolean readonly);

typedef void (*MyPaintTileRequestStartFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);
typedef void (*MyPaintTileRequestEndFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);

/*!
 * Tile-backed implementation of MyPaintSurface
 *
 * Interface and convenience class for implementing a MyPaintSurface backed by
 * a tile store.
 *
 * The size of the surface is infinite, and consumers need just implement two
 * vfuncs.
 */
struct MyPaintTiledSurface {
    MyPaintSurface parent;
    /* private: */
    MyPaintTileRequestStartFunction tile_request_start;
    MyPaintTileRequestEndFunction tile_request_end;
    gboolean surface_do_symmetry;
    float surface_center_x;
    struct OperationQueue *operation_queue;
    MyPaintRectangle dirty_bbox;
    gboolean threadsafe_tile_requests;
    int tile_size;
};

/*!
 * Initialize the surface by providing the tile request implementations.
 *
 * Allocates the resources necessary for the surface to function.
 * @sa mypaint_tiled_surface_destroy
 *
 * @memberof MyPaintTiledSurface
 */
void
mypaint_tiled_surface_init(MyPaintTiledSurface *self,
                           MyPaintTileRequestStartFunction tile_request_start,
                           MyPaintTileRequestEndFunction tile_request_end);


/*!
 * Free the resources used by the surface, and the surface itself
 *
 * Frees up the resources allocated in mypaint_tiled_surface_init.
 * @sa mypaint_tiled_surface_init
 *
 * @memberof MyPaintTiledSurface
 */
void
mypaint_tiled_surface_destroy(MyPaintTiledSurface *self);


/*!
 * Set the symmetry state of the surface.
 *
 * When the symmetry is active, for each dab drawn with
 * @ref mypaint_surface_draw_dab, reflected horizontally across the
 * vertical line defined by MyPaintTiledSurface.surface_center_x.
 *
 * @param active Whether symmetry should be used or not.
 * @param center_x The x-coordinate of the vertical line to reflect the dabs across
 *
 * @memberof MyPaintTiledSurface
 */
void
mypaint_tiled_surface_set_symmetry_state(MyPaintTiledSurface *self, gboolean active, float center_x);

/*!
 * Get the average alpha value of pixels covered by a standard dab.
 *
 * Equivalent to @ref mypaint_surface_get_alpha
 * (this function should probably not have been made public).
 *
 * @memberof MyPaintTiledSurface
 */
float
mypaint_tiled_surface_get_alpha (MyPaintTiledSurface *self, float x, float y, float radius);

/*!
 * Fetch a tile out from the underlying tile store.
 *
 * When successful, request->data will be set to point to the fetched tile.
 * Consumers must *always* call mypaint_tiled_surface_tile_request_end with the same
 * request to complete the transaction.
 *
 * @memberof MyPaintTiledSurface
 */
void mypaint_tiled_surface_tile_request_start(MyPaintTiledSurface *self, MyPaintTileRequest *request);

/*!
 * Put a (potentially modified) tile back into the underlying tile store.
 *
 * Consumers must *always* call mypaint_tiled_surface_tile_request_start() with the same
 * request to start the transaction before calling this function.
 *
 * @memberof MyPaintTiledSurface
 */
void mypaint_tiled_surface_tile_request_end(MyPaintTiledSurface *self, MyPaintTileRequest *request);

/*!
 * Implementation of MyPaintSurface::begin_atomic
 * Note: Only intended to be used from MyPaintTiledSurface subclasses,
 * which should chain up to this if overriding MyPaintSurface::begin_atomic.
 * Application code should only use @ref mypaint_surface_begin_atomic
 *
 * @memberof MyPaintTiledSurface
 */
void mypaint_tiled_surface_begin_atomic(MyPaintTiledSurface *self);
void mypaint_tiled_surface_end_atomic(MyPaintTiledSurface *self, MyPaintRectangle *roi);


/* -- Extended interface -- */

typedef void (*MyPaintTileRequestStartFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);
typedef void (*MyPaintTileRequestEndFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

/*!
  * Tile-backed implementation of MyPaintSurface2
  *
  * Apart from the additional calls of MyPaintSurface2, this implementation
  * supports additional symmetry types, and the ability to adjust the symmetry
  * angle - it is otherwise identical to MyPaintTiledSurface.
  *
  */
struct MyPaintTiledSurface2 {
  MyPaintSurface2 parent;
  MyPaintTileRequestStartFunction2 tile_request_start;
  MyPaintTileRequestEndFunction2 tile_request_end;
  struct OperationQueue *operation_queue;
  gboolean threadsafe_tile_requests;
  int tile_size;
  MyPaintSymmetryData symmetry_data;
  int num_bboxes;
  int num_bboxes_dirtied;
  MyPaintRectangle* bboxes;
};

/*!
 * Initialize the surface by providing the tile request implementations.
 *
 * Allocates the resources necessary for the surface to function.
 * @sa mypaint_tiled_surface2_destroy
 *
 * @memberof MyPaintTiledSurface2
 */
void
mypaint_tiled_surface2_init(
  MyPaintTiledSurface2 *self,
  MyPaintTileRequestStartFunction2 tile_request_start,
  MyPaintTileRequestEndFunction2 tile_request_end
  );

void mypaint_tiled_surface2_begin_atomic(MyPaintTiledSurface2 *self);
void mypaint_tiled_surface2_end_atomic(MyPaintTiledSurface2 *self, MyPaintRectangles *roi);

void mypaint_tiled_surface2_tile_request_start(MyPaintTiledSurface2 *self, MyPaintTileRequest *request);
void mypaint_tiled_surface2_tile_request_end(MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

void
mypaint_tiled_surface2_destroy(MyPaintTiledSurface2 *self);

void
mypaint_tiled_surface2_set_symmetry_state(MyPaintTiledSurface2 *self, gboolean active,
                                         float center_x, float center_y,
                                         float symmetry_angle,
                                         MyPaintSymmetryType symmetry_type,
                                         int rot_symmetry_lines);

G_END_DECLS

#endif // MYPAINTTILEDSURFACE_H
