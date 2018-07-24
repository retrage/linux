#!/usr/bin/env python

import sys

obj_filename = "objs"
sh_filename = "link-vmlinux.sh"

def main():
    with open(obj_filename, "r") as fp:
        objs = fp.readline().split(" ")
    
    with open(sh_filename, "w") as fp:
        fp.write("#!/usr/bin/env bash\n")
        fp.write("llvm-link -o vmlinux")
        for obj in objs:
            fp.write(" "+obj)

    return 0

if __name__ == "__main__":
    sys.exit(main())
