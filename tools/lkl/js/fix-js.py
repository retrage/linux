#!/usr/bin/env python

import sys

func_table = {
        "Module['printErr']('missing function: llvm_returnaddress'); abort(-1);":
            ["console.trace(); return 0;\n"],
        "Atomics_store(HEAP32,$53>>2,$54)|0; //@line 214 \"./include/linux/compiler.h\"":
            ["HEAP32[$53>>2] = $54;\n"],
            "Atomics_store(HEAP32,$58>>2,$59)|0; //@line 214 \"./include/linux/compiler.h\"":
            ["HEAP32[$58>>2] = $59;\n"],
            "Atomics_store(HEAP32,$47>>2,0)|0; //@line 5353 \"kernel/sched/core.c\"":
            ["HEAP32[$47>>2] = 0;\n"],
            "FUNCTION_TABLE_viii[$15 & 8191]($5,$41,$42); //@line 518 \"kernel/time/time.c\"":
            ["/* Do nothing */\n"],
            "$vararg_buffer9); //@line 3230 \"fs/namespace.c\"":
            ["console.trace();\n"],
            "Module['printErr']('missing function: __compiletime_assert_3990'); abort(-1);":
            ["console.trace();\n"],
            "Module['printErr']('missing function: __compiletime_assert_3991'); abort(-1);":
            ["console.trace();\n"],
            "$vararg_buffer1); //@line 3230 \"fs/namespace.c\"":
            ["console.trace();\n"],
        }

def main():
    if len(sys.argv) < 3:
        return -1

    fixed_js = []
    with open(sys.argv[1], "r") as fp:
        for line in fp:
            is_fixed = False
            for key, value in func_table.items():
                if key in line:
                    is_fixed = True
                    for fixed_line in value:
                        fixed_js.append(fixed_line)
            if not is_fixed:
                fixed_js.append(line)

    with open(sys.argv[2], "w") as fp:
        for line in fixed_js:
            fp.write(line)

    return 0

if __name__ == "__main__":
    sys.exit(main())
