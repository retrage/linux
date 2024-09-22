// SPDX-License-Identifier: GPL-2.0
/*
 * svc-hook related code for hijack
 * Copyright (c) 2023 Hajime Tazaki
 * Copyright (c) 2024 Akira Moroo
 *
 * Author: Hajime Tazaki <thehajime@gmail.com>
 * Author: Akira Moroo <retrage01@gmail.com>
 *
 * Note: https://github.com/retrage/svc-hook
 */

/* svc-hook only works on aarch64 architecture */
#ifdef __aarch64__
#include <lkl.h>
#include <lkl_host.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>

#include "init.h"
#include "xlate.h"
#include "hijack.h"

#include <errno.h>

/* XXX: include <unistd.h> doesn't do the job.. */
extern __pid_t gettid(void) __THROW;

#define CALL_LKL_FD_SYSCALL(x)					\
{								\
	case __NR_##x:						\
	if (!is_lklfd(a1))					\
		ret = syscall(a7, a1, a2, a3, a4, a5, a6);	\
	else {							\
		long p[6] = {a1, a2, a3, a4, a5, a6};		\
		ret = lkl_syscall(__lkl__NR_##x, p);		\
	}							\
	break;							\
}


#define SVCHOOK_DEBUG 1
#define dprintf(fmt, ...)		       \
	do {						\
		if (SVCHOOK_DEBUG) {			\
			printf(fmt, ##__VA_ARGS__);	\
		}					\
	} while (0)

long svchook_lkl_hook(long a1, long a2, long a3, long a4, long a5, long a6,
                          long a7, long a8)
{
	int ret;

	dprintf("syscall %ld: tid: %d\n", a7, gettid());
	if (!lkl_running) {
		if (a7 == __NR_clone) {
			if (a2 & CLONE_VM) { // pthread creation
				/* push return address to the stack */
				a3 -= sizeof(uint64_t);
				*((uint64_t *) a3) = a7;
			}
		}
		return syscall(a7, a1, a2, a3, a4, a5);
	}

	switch (a7) {
		CALL_LKL_FD_SYSCALL(sendmsg);
		CALL_LKL_FD_SYSCALL(recvmsg);
		CALL_LKL_FD_SYSCALL(sendmmsg);
		CALL_LKL_FD_SYSCALL(recvmmsg);
		CALL_LKL_FD_SYSCALL(bind);
		CALL_LKL_FD_SYSCALL(connect);
		CALL_LKL_FD_SYSCALL(getsockopt);
		CALL_LKL_FD_SYSCALL(setsockopt);
		CALL_LKL_FD_SYSCALL(getsockname);
		CALL_LKL_FD_SYSCALL(sendto);
		CALL_LKL_FD_SYSCALL(recvfrom);
		CALL_LKL_FD_SYSCALL(listen);
		CALL_LKL_FD_SYSCALL(accept);
		CALL_LKL_FD_SYSCALL(close);
		CALL_LKL_FD_SYSCALL(ioctl);
		CALL_LKL_FD_SYSCALL(fcntl);
		CALL_LKL_FD_SYSCALL(read);
		CALL_LKL_FD_SYSCALL(write);
		CALL_LKL_FD_SYSCALL(pread64);
	case __NR_socket:
		ret = lkl_sys_socket(a7, a1, a2);
		if (ret < 0)
			syscall(a7, a1, a2, a3, a4, a5, a6);
		break;
	case __NR_openat:
		if (!lkl_running)
			ret = syscall(a7, a1, a2, a3, a4, a5, a6);
		else {
			ret = lkl_sys_open((char *)a2, a3, a4);
			/* open to host libraries should not hijack */
			if (ret < 0 && (strncmp((char *)a2, "/lib", 4) == 0))
				ret = syscall(a7, a1, a2, a3, a4, a5, a6);
		}
		break;
	case __NR_newfstatat:
		if (!lkl_running)
			ret = syscall(a7, a1, a2, a3, a4, a5, a6);
		else
			ret = lkl_sys_newfstatat(a1, (char *)a2, (void *)a3, a4);
		break;
	case __NR_epoll_create1:
		return hijack_epoll_create1(a1);
	case __NR_epoll_ctl:
		return hijack_epoll_ctl(a1, a2, a3, (void *)a4);
#if 0
	case __NR_epoll_wait:
		return hijack_epoll_wait(a1, (void *)a2, a3, a4);
	case __NR_poll:
		return hijack_poll((void *)a1, a2, a3);
	case __NR_select:
		return hijack_select(a1, (void *)a2, (void *)a3, (void *)a4, (void *)a5);
#endif
	case __NR_eventfd2:
		return hijack_eventfd(a1, a2);
	case __NR_futex:
		ret = syscall(a7, a1, a2, a3, a4, a5, a6);
		if (ret < 0)
			return -errno;
		break;
	default:
		return syscall(a7, a1, a2, a3, a4, a5, a6);
	}

	if (ret == LKL_ENOSYS)
		fprintf(stderr, "no syscall defined in LKL (syscall=%ld)\n", a7);

	return ret;
}

void  __attribute__((destructor))
hook_exit(void)
{
	__hijack_fini();
}

typedef long (*syscall_fn_t)(long, long, long, long, long, long, long, long);
int __hook_init(long placeholder __attribute__ ((__unused__)),
		void *default_hook)
{
	*((syscall_fn_t *) default_hook) = svchook_lkl_hook;

	/**
	 * XXX: this library is expected to be load via dlmopen of svc-hook, thus
	 * we need to patch a workaorund to handle thread specific data.
	 */
	lkl_change_tls_mode();

	__hijack_init();
	return 0;
}
#endif /* __aarch64__ */
