#!/usr/bin/env python

import sys
import os

filename = "objs"

def main():
    if not os.path.exists(filename):
        with open(filename, "w") as fp:
            pass

    objs = []
    for i, arg in enumerate(sys.argv):
        if ".o" in arg and not "built-in" in arg and i > 2:
            objs.append(arg)

    with open(filename, "aw") as fp:
        for obj in objs:
            if not obj is "":
                fp.write(obj + " ")

    return 0

if __name__ == "__main__":
    sys.exit(main())
