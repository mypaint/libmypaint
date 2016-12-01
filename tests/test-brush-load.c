
#include <mypaint-brush.h>

#include "testutils.h"

#include <stddef.h> // For NULL

int
test_brush_load_succeeds(void *user_data)
{
    char *input_json = read_file((char *) user_data);
    MyPaintBrush *brush = mypaint_brush_new();
    int result = mypaint_brush_from_string(brush, input_json);
    mypaint_brush_unref(brush);
    return result;
}

int
test_brush_load_fails(void *user_data)
{
    char *input_json = read_file((char *) user_data);
    MyPaintBrush *brush = mypaint_brush_new();
    int result = mypaint_brush_from_string(brush, input_json);
    mypaint_brush_unref(brush);
    return ! result;
}

int
main(int argc, char **argv)
{
    TestCase test_cases[] = {

        // Mostly or completely OK brushes.
        // Expect these to load with minimal warnings.
        {
            "/brush/load/good",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/impressionism.myb"
        },
        {
            "/brush/load/bad/some_unknown_settings",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/some_unknown_settings.myb"
        },

        // Irrecoverably pathological brush data, missing global stuff
        // or entirely useless. We expect these to fail.
        {
            "/brush/load/bad/entirely_unknown_settings",
            test_brush_load_fails,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/entirely_unknown_settings.bad-myb"
        },
        {
            "/brush/load/bad/missing_settings",
            test_brush_load_fails,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/missing_settings.bad-myb"
        },
        {
            "/brush/load/bad/missing_version",
            test_brush_load_fails,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/missing_version.bad-myb"
        },
        {
            "/brush/load/bad/truncated",
            test_brush_load_fails,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/truncated.bad-myb"
        },
        {
            "/brush/load/bad/empty",
            test_brush_load_fails,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/empty.bad-myb"
        },

        // More individual bad settings. Covers dereferencing NULLs
        // deep down, bad data types, and any other potential segfaults
        // and gotchas we can think of.
        //
        // These should in general load, albeit with warnings.
        {
            "/brush/load/bad/bad_setting_types_1",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/bad_setting_types_1.myb"
        },
        {
            "/brush/load/bad/bad_setting_types_2",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/bad_setting_types_2.myb"
        },
        {
            "/brush/load/bad/bad_setting_types_3",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/bad_setting_types_3.myb"
        },
        {
            "/brush/load/bad/bad_setting_types_4",
            test_brush_load_succeeds,
            LIBMYPAINT_TESTING_ABS_TOP_SRCDIR
                "/tests/brushes/bad/bad_setting_types_4.myb"
        },
    };

    return test_cases_run(argc, argv, test_cases, TEST_CASES_NUMBER(test_cases), 0);
}
