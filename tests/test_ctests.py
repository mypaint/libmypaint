
from __future__ import absolute_import, division, print_function

import os
import ctests

"""Nosetest compatible wrapper, runs the libmypaint plain-C tests."""

tests_dir = ctests.tests_dir


def is_ctest(fn):
    return fn.startswith('test-') and not os.path.splitext(fn)[1]


def test_libmypaint():
    c_tests = [
        os.path.abspath(os.path.join(tests_dir, fn))
        for fn in sorted(os.listdir(tests_dir))
        if is_ctest(fn)
    ]

    for executable in c_tests:
        yield ctests.run_ctest, executable
