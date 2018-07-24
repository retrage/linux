#!/usr/bin/env python

from __future__ import print_function

import sys

def main():
    symbols = []
    for line in sys.stdin:
        symbol = line[line.index("@")+1:line.index("(")]
        symbols.append(symbol)
    
    symbols.sort()

    print("[", end="")
    for symbol in symbols:
        print("\"_{}\"".format(symbol), end="")
        if not symbol is symbols[-1]:
            print(",", end="")
    print("]", end="")

    return 0

if __name__ == "__main__":
    sys.exit(main())
