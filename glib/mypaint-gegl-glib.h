#ifndef MYPAINTGEGLGLIB_H
#define MYPAINTGEGLGLIB_H

#include "mypaint-config.h"

#include <glib-object.h>
#define MYPAINT_GEGL_TYPE_TILED_SURFACE (mypaint_gegl_tiled_surface_get_type ())
#define MYPAINT_GEGL_VALUE_HOLDS_TILED_SURFACE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), MYPAINT_GEGL_TYPE_TILED_SURFACE))
GType mypaint_gegl_tiled_surface_get_type(void);

#endif // MYPAINTGEGLGLIB_H
