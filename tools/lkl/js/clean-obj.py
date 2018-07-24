#!/usr/bin/env python

import sys
import os

filename = "objs"
ex_fn = ["lib/dump_stack.o"]

def main():
    with open(filename, "r") as fp:
        objs = fp.readline().split(" ")

    new_objs = []
    for obj in objs:
        if os.path.exists(obj) and not obj in new_objs and not obj in ex_fn:
            new_objs.append(obj)

    with open(filename, "w") as fp:
        for obj in new_objs:
            fp.write(obj + " ")

    return 0

if __name__ == "__main__":
    sys.exit(main())
