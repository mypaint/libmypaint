#!/usr/bin/env python3

"""
The `generate.py` script will pass location info as a translator note.
This script converts it to a proper location comment.
"""

import re
import sys

LOCATION_PATTERN = re.compile(r"^#\. (: \.\./brushsettings.json:.*)", re.MULTILINE)

match sys.argv:
    case [_, input_path, output_path]:
        pass
    case _:
        print("usage: fix-po-location.py <input> <output>", file=sys.stderr)
        sys.exit(1)

with open(input_path) as po_in, open(output_path, "w") as po_out:
    po_out.write(LOCATION_PATTERN.sub(r"#\1", po_in.read()))
