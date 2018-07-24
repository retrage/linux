#!/usr/bin/env python

import sys

blacklist = [
        "jit_init",
        "init_per_zone_wmark_min",
        "virtio_init",
        "init_user_reserve",
        "init_admin_reserve",
        "raid6_select_algo",
        "kswapd_init",
        "crypto_null_mod_init",
        "sha256_generic_mod_init",
        "aes_init",
        "crc32c_mod_init",
        "prng_mod_init",
        "drbg_init",
        "topology_sysfs_init",
        "virtio_net_driver",
        "virtio_crypto_driver_init",
        "init_oops_id",
        "deferred_probe_initcall",
        "spawn_ksoftirqd",
        "rand_initialize",
        ]

def main():
    if len(sys.argv) < 2:
        return -1

    INIT_LEVEL = 8
    SIG = "__initcall_"
    initcalls = [[] for i in range(INIT_LEVEL)]
    with open(sys.argv[1], "r") as fp:
        for line in fp:
            if SIG in line:
                symbol = line[:-1].split(" ")[2]
                try:
                    level = int(symbol[-1])
                    initcall = symbol[symbol.index(SIG)+len(SIG):len(symbol)-1]
                    initcalls[level].append(initcall)
                except ValueError:
                    pass

    for level, row in enumerate(initcalls):
        print("/* initcall{} */".format(level))
        print("EM_ASM({")
        for initcall in row:
            if initcall in blacklist:
                print("    /* _"+initcall+"(); */")
            else:
                print("    _"+initcall+"();")
        print("});")

    return 0

if __name__ == "__main__":
    sys.exit(main())
