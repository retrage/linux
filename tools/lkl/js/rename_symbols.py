#!/usr/bin/env python

import sys

lib_paths = ["js/dlmalloc.ll", "js/libc.ll"]

def is_in_literal(line, word):
    if line.count('"') < 2:
        return False
    if not word in line:
        return False
    fore = line.index('"')
    back = len(line) - line[::-1].index('"') - 1
    if fore < line.index(word) and line.index(word)+len(word) < back:
        return True
    else:
        return False

def main():
    if len(sys.argv) < 3:
        return -1

    lib_val_defs = []
    lib_fnc_defs = []
    for path in lib_paths:
        with open(path, "r") as fp:
            for line in fp:
                if line[0] == "@" and " = " in line:
                    lib_val = line[1:line.index(" = ")]
                    lib_val_defs.append(lib_val)
                elif line[:6] == "define":
                    lib_fnc = line[line.index("@")+1:line.index("(")]
                    lib_fnc_defs.append(lib_fnc)

    app = []
    dup_val_defs = ["in6addr_any", "in6addr_loopback"]
    dup_fnc_defs = []
    with open(sys.argv[1], "r") as fp:
        for line in fp:
            app.append(line)
            if line[:6] == "define":
                app_fnc = line[line.index("@")+1:line.index("(")]
                if app_fnc in lib_fnc_defs:
                    dup_fnc_defs.append(app_fnc)

    dup_fnc_defs.sort()
    print(dup_val_defs)
    print(dup_fnc_defs)

    asm = 'call i32 asm "", "=r,0"(i32 '
    ems = 'call i32 (i8*, ...) @emscripten_asm_const_int(i8* getelementptr inbounds ([1 x i8], [1 x i8]* @em_code, i32 0, i32 0))'
    renamed_app = []
    for line in app:
        for val in dup_val_defs:
            if not is_in_literal(line, val):
                line = line.replace(val, "kernel_"+val)
        for fnc in dup_fnc_defs:
            if not is_in_literal(line, fnc):
                line = line.replace(fnc, "kernel_"+fnc)
        if asm in line:
            s = line[line.index(asm):line.index(')')+1]
            line = line.replace(s, ems)
        renamed_app.append(line)

    em_code = "@em_code = private unnamed_addr constant [1 x i8] zeroinitializer, align 1"

    renamed_app.append("\n")
    renamed_app.append(em_code+"\n")

    renamed_app2 = []
    for line in renamed_app:
        line = line.replace("c\"kernel_strlcat\\00\"", "c\"strlcat\\00\"")
        line = line.replace("c\"kernel_getrusage\\00\"", "c\"getrusage\\00\"")
        renamed_app2.append(line)

    del renamed_app

    with open(sys.argv[2], "w") as fp:
        for line in renamed_app2:
            fp.write(line)

    return 0

if __name__ == "__main__":
    sys.exit(main())
