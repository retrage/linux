#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <lkl.h>
#include <lkl_host.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "test.h"

static void test_read_thread(void *data)
{
	int *pipe_fds = (int*) data;
	char tmp[LKL_PIPE_BUF+1];
	int ret;

        printf("read\n");
	ret = lkl_sys_read(pipe_fds[0], tmp, sizeof(tmp));
	if (ret < 0)
		lkl_test_logf("%s: %s\n", __func__, lkl_strerror(ret));
        printf("read\n");
}

static void test_write_thread(void *data)
{
	int *pipe_fds = (int*) data;
	char tmp[LKL_PIPE_BUF+1];
	long ret;

        printf("write\n");
	ret = lkl_sys_write(pipe_fds[1], tmp, sizeof(tmp));
	if (ret != sizeof(tmp)) {
		if (ret < 0)
			lkl_test_logf("write error: %s\n", lkl_strerror(ret));
		else
			lkl_test_logf("short write: %ld\n", ret);
		return;
	}
        printf("write\n");
}

static int lkl_test_syscall_thread(void)
{
	int pipe_fds[2];
	long ret;
	lkl_thread_t rtid, wtid;

        printf("pipe2\n");
	ret = lkl_sys_pipe2(pipe_fds, 0);
	if (ret) {
		lkl_test_logf("pipe2: %s\n", lkl_strerror(ret));
		return TEST_FAILURE;
	}
        printf("pipe2\n");

        printf("fcntl\n");
	ret = lkl_sys_fcntl(pipe_fds[0], LKL_F_SETPIPE_SZ, 1);
	if (ret < 0) {
		lkl_test_logf("fcntl setpipe_sz: %s\n", lkl_strerror(ret));
		return TEST_FAILURE;
	}
        printf("fcntl\n");

        printf("read thread_create\n");
	rtid = lkl_host_ops.thread_create(test_read_thread, pipe_fds);
	if (!rtid) {
		lkl_test_logf("failed to create thread\n");
		return TEST_FAILURE;
	}
        printf("read thread_create\n");

        printf("write thread_create\n");
	wtid = lkl_host_ops.thread_create(test_write_thread, pipe_fds);
	if (!wtid) {
		lkl_test_logf("failed to create thread\n");
		return TEST_FAILURE;
	}
        printf("write thread_create\n");

        printf("write thread_join\n");
	ret = lkl_host_ops.thread_join(wtid);
	if (ret) {
		lkl_test_logf("failed to join thread\n");
		return TEST_FAILURE;
	}
        printf("write thread_join\n");

        printf("read thread_join\n");
	ret = lkl_host_ops.thread_join(rtid);
	if (ret) {
		lkl_test_logf("failed to join thread\n");
		return TEST_FAILURE;
	}
        printf("read thread_join\n");

	return TEST_SUCCESS;
}

LKL_TEST_CALL(start_kernel, lkl_start_kernel, 0, &lkl_host_ops,
	     "mem=16M loglevel=8");
LKL_TEST_CALL(stop_kernel, lkl_sys_halt, 0);

struct lkl_test tests[] = {
	LKL_TEST(start_kernel),
	LKL_TEST(syscall_thread),
	LKL_TEST(stop_kernel),
};

int main(int argc, const char **argv)
{
	lkl_host_ops.print = lkl_test_log;

        lkl_thread_init();

	return lkl_test_run(tests, sizeof(tests)/sizeof(struct lkl_test),
			    "boot");
}
