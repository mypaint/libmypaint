#ifndef MYPAINTTILEDSURFACE_H
#define MYPAINTTILEDSURFACE_H

#include <stdint.h>
#include "mypaint-surface.h"
#include "mypaint-symmetry.h"
#include "mypaint-config.h"

G_BEGIN_DECLS

typedef struct MyPaintTiledSurface MyPaintTiledSurface;
typedef struct MyPaintTiledSurface2 MyPaintTiledSurface2;

typedef struct {
    int tx;
    int ty;
    gboolean readonly;
    guint16 *buffer;
    gpointer context; /* Only to be used by the surface implementations. */
    int thread_id;
    int mipmap_level;
} MyPaintTileRequest;

void
mypaint_tile_request_init(MyPaintTileRequest *data, int level,
                          int tx, int ty, gboolean readonly);

typedef void (*MyPaintTileRequestStartFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);
typedef void (*MyPaintTileRequestEndFunction) (MyPaintTiledSurface *self, MyPaintTileRequest *request);

/**
  * MyPaintTiledSurface:
  *
  * Interface and convenience class for implementing a #MyPaintSurface backed by a tile store.
  *
  * The size of the surface is infinite, and consumers need just implement two vfuncs.
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

void
mypaint_tiled_surface_init(MyPaintTiledSurface *self,
                           MyPaintTileRequestStartFunction tile_request_start,
                           MyPaintTileRequestEndFunction tile_request_end);

void
mypaint_tiled_surface_destroy(MyPaintTiledSurface *self);

void
mypaint_tiled_surface_set_symmetry_state(MyPaintTiledSurface *self, gboolean active, float center_x);

float
mypaint_tiled_surface_get_alpha (MyPaintTiledSurface *self, float x, float y, float radius);

void mypaint_tiled_surface_tile_request_start(MyPaintTiledSurface *self, MyPaintTileRequest *request);
void mypaint_tiled_surface_tile_request_end(MyPaintTiledSurface *self, MyPaintTileRequest *request);

void mypaint_tiled_surface_begin_atomic(MyPaintTiledSurface *self);
void mypaint_tiled_surface_end_atomic(MyPaintTiledSurface *self, MyPaintRectangle *roi);


/* -- Extended interface -- */

typedef void (*MyPaintTileRequestStartFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);
typedef void (*MyPaintTileRequestEndFunction2) (MyPaintTiledSurface2 *self, MyPaintTileRequest *request);

/**
  * MyPaintTiledSurface2: (skip)
  *
  * Tiled surface supporting the MyPaintSurface2 interface.
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
