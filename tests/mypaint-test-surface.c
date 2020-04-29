/* libmypaint - The MyPaint Brush Library
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
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "mypaint-utils-stroke-player.h"
#include "mypaint-test-surface.h"
#include "mypaint-test-surface.h"
#include "testutils.h"
#include "mypaint-benchmark.h"

#ifndef LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
#define LIBMYPAINT_TESTING_ABS_TOP_SRCDIR ".."
#endif


typedef enum {
    SurfaceTransactionPerStrokeTo,
    SurfaceTransactionPerStroke
} SurfaceTransaction;

typedef struct {
    char *test_case_id;
    MyPaintTestsSurfaceFactory factory_function;
    gpointer factory_user_data;

    float brush_size;
    float scale;
    int iterations;
    const char *brush_file;
    SurfaceTransaction surface_transaction;
} SurfaceTestData;

int
test_surface_drawing(void *user_data)
{
    SurfaceTestData *data = (SurfaceTestData *)user_data;

    char * event_data = read_file(LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                                  "/tests/events/painting30sec.dat");
    char * brush_data = read_file(data->brush_file);

    assert(event_data);
    assert(brush_data);

    MyPaintSurface *surface = data->factory_function(data->factory_user_data);
    MyPaintBrush *brush = mypaint_brush_new();
    mypaint_brush_from_defaults(brush);
    MyPaintUtilsStrokePlayer *player = mypaint_utils_stroke_player_new();

    mypaint_brush_from_string(brush, brush_data);
    mypaint_brush_set_base_value(brush, MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, log(data->brush_size));

    mypaint_utils_stroke_player_set_brush(player, brush);
    mypaint_utils_stroke_player_set_surface(player, surface);
    mypaint_utils_stroke_player_set_source_data(player, event_data);
    mypaint_utils_stroke_player_set_scale(player, data->scale);

    if (data->surface_transaction == SurfaceTransactionPerStroke) {
        mypaint_utils_stroke_player_set_transactions_on_stroke_to(player, FALSE);
    }

    // Actually run benchmark
    mypaint_benchmark_start(data->test_case_id);
    for (int i=0; i<data->iterations; i++) {
        if (data->surface_transaction == SurfaceTransactionPerStroke) {
            mypaint_surface_begin_atomic(surface);
        }
        mypaint_utils_stroke_player_run_sync(player);

        if (data->surface_transaction == SurfaceTransactionPerStroke) {
            mypaint_surface_end_atomic(surface, NULL);
        }
    }
    int result = mypaint_benchmark_end();

    char *png_filename = malloc(snprintf(NULL, 0, "%s.png", data->test_case_id) + 1);
    sprintf(png_filename, "%s.png", data->test_case_id);

    //mypaint_surface_save_png(surface, png_filename, 0, 0, -1, 1);
    // FIXME: check the correctness of the outputted PNG

    free(png_filename);

    mypaint_brush_unref(brush);
    mypaint_surface_unref(surface);
    mypaint_utils_stroke_player_free(player);

    free(event_data);
    free(brush_data);

    return result;
}

char *
create_id(const char *templ, const char *title)
{
    char *id = malloc(snprintf(NULL, 0, templ, title) + 1);
    sprintf(id, templ, title);
    return id;
}

int
mypaint_test_surface_run(int argc, char **argv,
                      MyPaintTestsSurfaceFactory surface_factory,
                      gchar *title, gpointer user_data)
{
  gboolean correctness_only = TRUE;
  if (argc > 1 && strcmp(argv[1], "--full-benchmark") == 0) {
    correctness_only = FALSE;
  }

    printf("Running test: %s\n", title);
#define BRUSH_PATH(brushname) LIBMYPAINT_TESTING_ABS_TOP_SRCDIR "/tests/brushes/" brushname ".myb"

    const char* brush_paths[4] = {
      BRUSH_PATH("modelling"), BRUSH_PATH("charcoal"), BRUSH_PATH("coarse_bulk_2"), BRUSH_PATH("bulk")
    };
    const int num_brushes = TEST_CASES_NUMBER(brush_paths);

    float max_brush_radius[num_brushes];
    max_brush_radius[0] = correctness_only ? 256 : 512;
    max_brush_radius[1] = 512;
    max_brush_radius[2] = 256;
    max_brush_radius[3] = 512;

    int num_cases = 0;
    if (correctness_only) {
      num_cases = num_brushes * 2;
    } else {
      for (int i = 0; i < num_brushes; ++i) {
        num_cases += (int)log2(max_brush_radius[i]);
      }
    }

    SurfaceTestData test_data[num_cases];
    int max_id_length = 32;
    char test_ids[num_cases][max_id_length];
    int case_n = 0;

    // Generate test case parameters
    for (int brush = 0; brush < num_brushes; ++brush) {
      const int max_radius = max_brush_radius[brush];
      for (int radius = 2; radius <= max_radius; radius *= 2) {
         // For correctness tests, only run the first and the last radius/scale combo for each brush
        if (correctness_only && radius != 2 && radius*2 <= max_radius) {
          continue;
        }
        const float scale = powf(2, ((int)log2(radius)-1) / 3);
        snprintf(test_ids[case_n], max_id_length, "(b:%02d  r:%-3d s:%-3.1f)", brush, radius, scale);
        const int iterations = 1;
        SurfaceTransaction transaction = SurfaceTransactionPerStrokeTo;
        SurfaceTestData t_data = {
          test_ids[case_n], surface_factory, user_data, radius, scale, iterations, brush_paths[brush], transaction
        };
        test_data[case_n++] = t_data;
      }
    }

    // Generate test cases
     TestCase test_cases[num_cases];
     for (int i = 0; i < num_cases; ++i) {
         TestCase t;
         t.id = test_data[i].test_case_id;
         t.function = test_surface_drawing;
         t.user_data = (void*)&test_data[i];
         test_cases[i] = t;
     };

     // Run test cases
     return test_cases_run(argc, argv, test_cases, TEST_CASES_NUMBER(test_cases), TEST_CASE_BENCHMARK);

#undef BRUSH_PATH
}
