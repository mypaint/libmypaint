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
    /*! Pointer to the tile buffer, set by receiver of the request */
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

/*!
 * Function for beginning a tile request from the surface backend
 *
 * @memberof MyPaintTiledSurface
 * @sa MyPaintTileRequest, MyPaintTileRequestEndFunction
 */
typedef void (*MyPaintTileRequestStartFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);

/*!
 * Function for ending a tile request from the surface backend
 *
 * @memberof MyPaintTiledSurface
 * @sa MyPaintTileRequest, MyPaintTileRequestStartFunction
 */
typedef void (*MyPaintTileRequestEndFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);

/*!
 * Tile-backed implementation of MyPaintSurface
 *
 * Interface and convenience class for implementing a MyPaintSurface backed by
 * a tile store.
 *
 * The size of the surface is infinite, and consumers only need to provide
 * implementations for #tile_request_start and #tile_request_end
 *
 * @sa MyPaintTiledSurface2
 */
struct MyPaintTiledSurface {
    /*! Surface interface */
    MyPaintSurface parent;
    /* "private": */
    /*! See #MyPaintTileRequestStartFunction */
    MyPaintTileRequestStartFunction tile_request_start;
    /*! See #MyPaintTileRequestEndFunction */
    MyPaintTileRequestEndFunction tile_request_end;
    /*! Whether vertical-line symmetry is enabled or not */
    gboolean surface_do_symmetry;
    /*! The x-coordinate of the vertical symmetry line */
    float surface_center_x;
    /*! Per-tile queue of pending dab operations */
    struct OperationQueue *operation_queue;
    /*!
     * Invalidation rectangle recording areas changed between the calls to
     * #parent%'s MyPaintSurface::begin_atomic and MyPaintSurface::end_atomic
     */
    MyPaintRectangle dirty_bbox;
    /*! Whether tile requests shuold be considered thread-safe or not */
    gboolean threadsafe_tile_requests;
    /*! The side length of the (square) tiles */
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
 * ::mypaint_surface_draw_dab, reflected horizontally across the
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
 * Equivalent to ::mypaint_surface_get_alpha
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
 * Application code should only use #mypaint_surface_begin_atomic
 *
 * @memberof MyPaintTiledSurface
 */
void mypaint_tiled_surface_begin_atomic(MyPaintTiledSurface *self);
void mypaint_tiled_surface_end_atomic(MyPaintTiledSurface *self, MyPaintRectangle *roi);


/* -- Extended interface -- */

/*! Functionally equivalent to #MyPaintTileRequestStartFunction
 * @memberof MyPaintTiledSurface2
 */
typedef void (*MyPaintTileRequestStartFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);
/*! Functionally equivalent to #MyPaintTileRequestEndFunction
 * @memberof MyPaintTiledSurface2
 */
typedef void (*MyPaintTileRequestEndFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

/*!
  * Tile-backed implementation of MyPaintSurface2
  *
  * Apart from the additional calls of MyPaintSurface2, this implementation
  * supports additional symmetry types, and the ability to adjust the symmetry
  * angle - it is otherwise identical to MyPaintTiledSurface.
  *
  * @sa MyPaintTiledSurface
  */
struct MyPaintTiledSurface2 {
  /*! Parent interface */
  MyPaintSurface2 parent;
  /*! See #MyPaintTileRequestStartFunction2 */
  MyPaintTileRequestStartFunction2 tile_request_start;
  /*! See #MyPaintTileRequestEndFunction2 */
  MyPaintTileRequestEndFunction2 tile_request_end;
  /*! Per-tile queue of pending dab operations */
  struct OperationQueue *operation_queue;
  /*! Whether tile requests shuold be considered thread-safe or not */
  gboolean threadsafe_tile_requests;
  int tile_size;
  /*! The symmetry data used
   *
   * See MyPaintSymmetryData for details.
   */
  MyPaintSymmetryData symmetry_data;
  /*! Length of #bboxes */
  int num_bboxes;
  /*! The number of #bboxes that have been modified since they were last reset */
  int num_bboxes_dirtied;
  /*! Pointer to an array of invalidation rectangles
   *
   * Records multiple invalidation rectangles when symmetry is enabled.
   */
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

/*!
 * Prepare the surface for handling a set of dab operations.
 *
 * @memberof MyPaintTiledSurface2
 */
void mypaint_tiled_surface2_begin_atomic(MyPaintTiledSurface2 *self);

/*!
 * Finalize any pending dab operations and set the resulting invalidation rectangles.
 *
 * @memberof MyPaintTiledSurface2
 */
void mypaint_tiled_surface2_end_atomic(MyPaintTiledSurface2 *self, MyPaintRectangles *roi);

/*!
 * Finalize any pending dab operations and set the resulting invalidation rectangles.
 *
 * @memberof MyPaintTiledSurface2
 */
void mypaint_tiled_surface2_tile_request_start(MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

/*!
 * Finalize any pending dab operations and set the resulting invalidation rectangles.
 *
 * @memberof MyPaintTiledSurface2
 */
void mypaint_tiled_surface2_tile_request_end(MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

/*!
 * Deallocate all resources used by the surface struct, and the struct itself.
 *
 * @memberof MyPaintTiledSurface2
 */
void
mypaint_tiled_surface2_destroy(MyPaintTiledSurface2 *self);

/*!
 * Set new #symmetry_data values and mark it for update
 *
 * @memberof MyPaintTiledSurface2
 * @sa MyPaintSymmetryData, MyPaintSymmetryState
 */
void
mypaint_tiled_surface2_set_symmetry_state(MyPaintTiledSurface2 *self, gboolean active,
                                         float center_x, float center_y,
                                         float symmetry_angle,
                                         MyPaintSymmetryType symmetry_type,
                                         int rot_symmetry_lines);

G_END_DECLS

#endif // MYPAINTTILEDSURFACE_H
