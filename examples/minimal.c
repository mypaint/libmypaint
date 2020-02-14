#include "libmypaint.c"
#include "mypaint-fixed-tiled-surface.h"

void
stroke_to(MyPaintBrush *brush, MyPaintSurface *surf, float x, float y)
{
    float viewzoom = 1.0, viewrotation = 0.0, barrel_rotation = 0.0;
    float pressure = 1.0, ytilt = 0.0, xtilt = 0.0, dtime = 1.0/10;
    mypaint_brush_stroke_to
      (brush, surf, x, y, pressure, xtilt, ytilt, dtime);
}

int
main(int argc, char argv[]) {

    MyPaintBrush *brush = mypaint_brush_new();
    int w = 300;
    int h = 150;
    float wq = (float)w / 5;
    float hq = (float)h / 5;

    /* Create an instance of the simple and naive fixed_tile_surface to draw on. */
    MyPaintFixedTiledSurface *surface = mypaint_fixed_tiled_surface_new(w, h);

    /* Create a brush with default settings for all parameters, then set its color to red. */
    mypaint_brush_from_defaults(brush);
    mypaint_brush_set_base_value(brush, MYPAINT_BRUSH_SETTING_COLOR_H, 0.0);
    mypaint_brush_set_base_value(brush, MYPAINT_BRUSH_SETTING_COLOR_S, 1.0);
    mypaint_brush_set_base_value(brush, MYPAINT_BRUSH_SETTING_COLOR_V, 1.0);

    /* Draw a rectangle on the surface using the brush */
    mypaint_surface_begin_atomic((MyPaintSurface*)surface);
    stroke_to(brush, (MyPaintSurface*)surface, 0, 0);
    stroke_to(brush, (MyPaintSurface*)surface, wq, hq);
    stroke_to(brush, (MyPaintSurface*)surface, 4 * wq, hq);
    stroke_to(brush, (MyPaintSurface*)surface, 4 * wq, 4 * hq);
    stroke_to(brush, (MyPaintSurface*)surface, wq, 4 * hq);
    stroke_to(brush, (MyPaintSurface*)surface, wq, hq);

    /* Finalize the surface operation, passing one or more invalidation
       rectangles to get information about which areas were affected by
       the operations between ``surface_begin_atomic`` and ``surface_end_atomic.``
    */
    MyPaintRectangle roi;
    mypaint_surface_end_atomic((MyPaintSurface *)surface, &roi);

    /* Write the surface pixels to a ppm image file */
    fprintf(stdout, "Writing output\n");
    write_ppm(surface, "output.ppm");

    mypaint_brush_unref(brush);
    mypaint_surface_unref((MyPaintSurface *)surface);
}
