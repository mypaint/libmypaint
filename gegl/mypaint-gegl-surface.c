/* brushlib - The MyPaint Brush Library
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

#include <stdlib.h>
#include <assert.h>

#include "mypaint-gegl-surface.h"
#include <gegl-utils.h>

typedef struct _MyPaintGeglTiledSurface {
    MyPaintTiledSurface parent;

    GeglRectangle extent_rect; // TODO: remove, just use the extent of the buffer
    GeglBuffer *buffer;
    const Babl *format;
} MyPaintGeglTiledSurface;

#include <glib/mypaint-gegl-glib.c>

void free_gegl_tiledsurf(MyPaintSurface *surface);

static gboolean
buffer_is_native(MyPaintGeglTiledSurface *self) {
    const int tile_size = self->parent.tile_size;

    int tile_height = -1;
    int tile_width = -1;
    g_object_get(self->buffer, "tile-width", &tile_width, "tile-height", &tile_height, NULL);
    g_assert(tile_height != -1);
    g_assert(tile_width != -1);

    const gboolean correct_format = gegl_buffer_get_format(self->buffer) == self->format;
    const gboolean correct_tile_size = tile_height == tile_size && tile_width == tile_size;

    return correct_format && correct_tile_size;
}

void *
alloc_for_format(const Babl *format, int pixels) {
    const size_t bytes = babl_format_get_bytes_per_pixel(format)*pixels;
    return gegl_malloc(bytes);
}

static void
tile_request_start(MyPaintTiledSurface *tiled_surface, MyPaintTileRequest *request)
{
    MyPaintGeglTiledSurface *self = (MyPaintGeglTiledSurface *)tiled_surface;

    const int tile_size = tiled_surface->tile_size;
    GeglRectangle tile_bbox;
    gegl_rectangle_set(&tile_bbox, request->tx * tile_size, request->ty * tile_size, tile_size, tile_size);

    int read_write_flags;

    if (request->readonly) {
        read_write_flags = GEGL_BUFFER_READ;
    } else {
        read_write_flags = GEGL_BUFFER_READWRITE;

        // Extend the bounding box
        gegl_rectangle_bounding_box(&self->extent_rect, &self->extent_rect, &tile_bbox);
        gboolean success = gegl_buffer_set_extent(self->buffer, &self->extent_rect);
        g_assert(success);
    }

    if (buffer_is_native(self)) {
        GeglBufferIterator *iterator = gegl_buffer_iterator_new(self->buffer, &tile_bbox, 0, self->format,
                                      read_write_flags, GEGL_ABYSS_NONE);

        // Read out
        gboolean completed = gegl_buffer_iterator_next(iterator);
        g_assert(completed);

        if (iterator->length != tile_size*tile_size) {
            g_critical("Unable to get tile aligned access to GeglBuffer");
            request->buffer = NULL;
        } else {
            request->buffer = (uint16_t *)(iterator->data[0]);
        }

        // So we can finish the iterator in tile_request_end()
        request->context = (void *)iterator;
    } else {
        // Extract a linear rectangular chunk of appropriate BablFormat,
        // potentially triggering copying and color conversions
        request->buffer = alloc_for_format(self->format, tile_size*tile_size);
        gegl_buffer_get(self->buffer, &tile_bbox, 1, self->format,
                        request->buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        g_assert(request->buffer);
    }
}

static void
tile_request_end(MyPaintTiledSurface *tiled_surface, MyPaintTileRequest *request)
{
    MyPaintGeglTiledSurface *self = (MyPaintGeglTiledSurface *)tiled_surface;

    if (buffer_is_native(self)) {
        GeglBufferIterator *iterator = (GeglBufferIterator *)request->context;

        if (iterator) {
            gegl_buffer_iterator_next(iterator);
            request->context = NULL;
        }
    } else {
        // Push our linear buffer back into the GeglBuffer
        const int tile_size = tiled_surface->tile_size;
        GeglRectangle tile_bbox;

        g_assert(request->buffer);
        gegl_rectangle_set(&tile_bbox, request->tx*tile_size, request->ty*tile_size, tile_size, tile_size);
        gegl_buffer_set(self->buffer, &tile_bbox, 0, self->format,
                        request->buffer, GEGL_AUTO_ROWSTRIDE);
        gegl_free(request->buffer);
    }

}

void
save_png(MyPaintSurface *surface, const char *path,
         int x, int y, int width, int height)
{
    MyPaintGeglTiledSurface *self = (MyPaintGeglTiledSurface *)surface;
    GeglNode *graph, *save, *source;

    graph = gegl_node_new();
    source = gegl_node_new_child(graph, "operation", "gegl:buffer-source",
                                 "buffer", mypaint_gegl_tiled_surface_get_buffer(self), NULL);
    save = gegl_node_new_child(graph, "operation", "gegl:png-save", "path", path, NULL);
    gegl_node_link(source, save);

    gegl_node_process(save);
    g_object_unref(graph);
}

/**
 * mypaint_gegl_tiled_surface_get_buffer:
 *
 * Returns: (transfer none): The buffer this surface is backed by.
 */
GeglBuffer *
mypaint_gegl_tiled_surface_get_buffer(MyPaintGeglTiledSurface *self)
{
    return self->buffer;
}

/**
 * mypaint_gegl_tiled_surface_set_buffer:
 * @buffer: (transfer full): The buffer which shall back this surface.
 *
 */
void
mypaint_gegl_tiled_surface_set_buffer(MyPaintGeglTiledSurface *self, GeglBuffer *buffer)
{
    if (buffer && self->buffer == buffer) {
        return;
    }

    if (self->buffer) {
        g_object_unref(self->buffer);
    }

    if (buffer) {
        g_return_if_fail(GEGL_IS_BUFFER(buffer));
        g_object_ref(buffer);
        self->buffer = buffer;
    } else {
        // Using GeglBuffer with aligned tiles for zero-copy access
        self->buffer = GEGL_BUFFER(g_object_new(GEGL_TYPE_BUFFER,
                          "x", self->extent_rect.x, "y", self->extent_rect.y,
                          "width", self->extent_rect.width, "height", self->extent_rect.height,
                          "format", self->format,
                          "tile-width", self->parent.tile_size, "tile-height", self->parent.tile_size,
                          NULL));
    }
    g_assert(GEGL_IS_BUFFER(self->buffer));


    self->parent.threadsafe_tile_requests = buffer_is_native(self);
}

MyPaintGeglTiledSurface *
mypaint_gegl_tiled_surface_new(void)
{
    MyPaintGeglTiledSurface *self = (MyPaintGeglTiledSurface *)malloc(sizeof(MyPaintGeglTiledSurface));

    mypaint_tiled_surface_init(&self->parent, tile_request_start, tile_request_end);

    // MyPaintSurface vfuncs
    self->parent.parent.destroy = free_gegl_tiledsurf;
    self->parent.parent.save_png = save_png;

    self->parent.threadsafe_tile_requests = TRUE;

    self->buffer = NULL;

    gegl_rectangle_set(&self->extent_rect, 0, 0, 0, 0);

    self->format = babl_format_new(babl_model ("R'aG'aB'aA"), babl_type ("u15"),
                             babl_component("R'a"), babl_component("G'a"), babl_component("B'a"), babl_component("A"),
                             NULL);
    g_assert(self->format);

    mypaint_gegl_tiled_surface_set_buffer(self, NULL);

    return self;
}

void free_gegl_tiledsurf(MyPaintSurface *surface)
{
    MyPaintGeglTiledSurface *self = (MyPaintGeglTiledSurface *)surface;

    mypaint_tiled_surface_destroy(&self->parent);
    g_object_unref(self->buffer);

    free(self);
}

MyPaintSurface *
mypaint_gegl_tiled_surface_interface(MyPaintGeglTiledSurface *self)
{
    return (MyPaintSurface *)self;
}
