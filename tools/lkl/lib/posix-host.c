#include <emscripten.h>
#ifdef __FIBER__
#include <lk/kernel/semaphore.h>
#include <lk/kernel/mutex.h>
#include <lk/kernel/thread.h>
#include <lk/kernel/event.h>
#include <lk/kernel/timer.h>
#else
#include <pthread.h>
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <poll.h>
#include <lkl_host.h>
#include "iomem.h"
#include "jmp_buf.h"

/* Let's see if the host has semaphore.h */
#include <unistd.h>

#ifndef __FIBER__
#ifdef _POSIX_SEMAPHORES
#include <semaphore.h>
/* TODO(pscollins): We don't support fork() for now, but maybe one day
 * we will? */
#define SHARE_SEM 0
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */

static void print(const char *str, int len)
{
	int ret __attribute__((unused));

	ret = write(STDOUT_FILENO, str, len);
}

struct lkl_mutex {
#ifdef __FIBER__
       int recursive;
       mutex_t mutex;
       semaphore_t sem;
#else
       pthread_mutex_t mutex;
#endif /* __FIBER__ */
};

struct lkl_sem {
#ifdef __FIBER__
        semaphore_t sem;
#else
#ifdef _POSIX_SEMAPHORES
	sem_t sem;
#else
	pthread_mutex_t lock;
	int count;
	pthread_cond_t cond;
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */
};

struct lkl_tls_key {
#ifdef __FIBER__
        uint key;
#else
	pthread_key_t key;
#endif /* __FIBER__ */
};

#define WARN_UNLESS(exp) do {						\
		if (exp < 0)						\
			lkl_printf("%s: %s\n", #exp, strerror(errno));	\
	} while (0)

#ifndef __FIBER__
static int _warn_pthread(int ret, char *str_exp)
{
	if (ret > 0)
		lkl_printf("%s: %s\n", str_exp, strerror(ret));

	return ret;
}


/* pthread_* functions use the reverse convention */
#define WARN_PTHREAD(exp) _warn_pthread(exp, #exp)
#endif /* __FIBER__ */

static struct lkl_sem *lkl_sem_alloc(int count)
{
	struct lkl_sem *sem;

	sem = malloc(sizeof(*sem));
	if (!sem)
		return NULL;

#ifdef __FIBER__
        sem_init(&sem->sem, count);
#else
#ifdef _POSIX_SEMAPHORES
	if (sem_init(&sem->sem, SHARE_SEM, count) < 0) {
		lkl_printf("sem_init: %s\n", strerror(errno));
		free(sem);
		return NULL;
	}
#else
	pthread_mutex_init(&sem->lock, NULL);
	sem->count = count;
	WARN_PTHREAD(pthread_cond_init(&sem->cond, NULL));
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */

	return sem;
}

static void lkl_sem_free(struct lkl_sem *sem)
{
#ifdef __FIBER__
        sem_destroy(&sem->sem);
#else
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_destroy(&sem->sem));
#else
	WARN_PTHREAD(pthread_cond_destroy(&sem->cond));
	WARN_PTHREAD(pthread_mutex_destroy(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */
	free(sem);
}

static void lkl_sem_up(struct lkl_sem *sem)
{
#ifdef __FIBER__
        sem_post(&sem->sem, 1);
#else
#ifdef _POSIX_SEMAPHORES
	WARN_UNLESS(sem_post(&sem->sem));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	sem->count++;
	if (sem->count > 0)
		WARN_PTHREAD(pthread_cond_signal(&sem->cond));
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */

}

static void lkl_sem_down(struct lkl_sem *sem)
{
#ifdef __FIBER__
        int err;
        do {
                thread_yield();
                err = sem_wait(&sem->sem);
        } while (err < 0);
#else
#ifdef _POSIX_SEMAPHORES
	int err;

	do {
		err = sem_wait(&sem->sem);
	} while (err < 0 && errno == EINTR);
	if (err < 0 && errno != EINTR)
		lkl_printf("sem_wait: %s\n", strerror(errno));
#else
	WARN_PTHREAD(pthread_mutex_lock(&sem->lock));
	while (sem->count <= 0)
		WARN_PTHREAD(pthread_cond_wait(&sem->cond, &sem->lock));
	sem->count--;
	WARN_PTHREAD(pthread_mutex_unlock(&sem->lock));
#endif /* _POSIX_SEMAPHORES */
#endif /* __FIBER__ */
}

static struct lkl_mutex *lkl_mutex_alloc(int recursive)
{
	struct lkl_mutex *_mutex = malloc(sizeof(struct lkl_mutex));
#ifdef __FIBER__
        if (!_mutex)
                return NULL;

        if (recursive)
                mutex_init(&_mutex->mutex);
        else
                sem_init(&_mutex->sem, 1);
        _mutex->recursive = recursive;
#else 
	pthread_mutex_t *mutex = NULL;
	pthread_mutexattr_t attr;

	if (!_mutex)
		return NULL;

	mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutexattr_init(&attr));

	/* PTHREAD_MUTEX_ERRORCHECK is *very* useful for debugging,
	 * but has some overhead, so we provide an option to turn it
	 * off. */
#ifdef DEBUG
	if (!recursive)
		WARN_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
#endif /* DEBUG */

	if (recursive)
		WARN_PTHREAD(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));

	WARN_PTHREAD(pthread_mutex_init(mutex, &attr));

#endif /* __FIBER__ */
	return _mutex;
}

static void lkl_mutex_lock(struct lkl_mutex *mutex)
{
#ifdef __FIBER__
        int err;

        if (mutex->recursive) {
                if (!is_mutex_held(&mutex->mutex))
                        mutex_acquire(&mutex->mutex);
        } else {
                do {
                        thread_yield();
                        err = sem_wait(&mutex->sem);
                } while (err < 0);
        }
#else
	WARN_PTHREAD(pthread_mutex_lock(&mutex->mutex));
#endif /* __FIBER__ */
}

static void lkl_mutex_unlock(struct lkl_mutex *_mutex)
{
#ifdef __FIBER__
        if (_mutex->recursive) {
                if (!is_mutex_held(&_mutex->mutex))
                        mutex_release(&_mutex->mutex);
        } else
                sem_post(&_mutex->sem, 1);
#else
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_unlock(mutex));
#endif /* __FIBER__ */
}

static void lkl_mutex_free(struct lkl_mutex *_mutex)
{
#ifdef __FIBER__
        if (_mutex->recursive)
                mutex_destroy(&_mutex->mutex);
        else
                sem_destroy(&_mutex->sem);
#else
	pthread_mutex_t *mutex = &_mutex->mutex;
	WARN_PTHREAD(pthread_mutex_destroy(mutex));
#endif /* __FIBER__ */
	free(_mutex);
}

#ifdef __FIBER__
/*
 * Most of the code comes from
 * http://linux-biyori.sakura.ne.jp/program/pr_signal02.php
 */
static struct sigaction sigact;
static struct sigevent sigevp;
static struct itimerspec ispec;
static timer_t timerid = 0;
static volatile lk_time_t ticks = 0;

static void lkl_timer_callback(int signum, siginfo_t *info, void *ctx)
{
        ticks += 10;
        if (thread_timer_tick()==INT_RESCHEDULE)
                thread_preempt();
}

lk_time_t current_time(void)
{
        return ticks;
}

lk_bigtime_t current_time_hires(void)
{
        return (lk_bigtime_t)ticks * 1000;
}

void lkl_thread_init(void)
{
        thread_init_early();
        thread_init();
        timer_init();
        thread_create_idle();
        thread_set_priority(DEFAULT_PRIORITY);

        sigact.sa_sigaction = lkl_timer_callback;
        sigact.sa_flags = SA_SIGINFO | SA_RESTART;
        sigemptyset(&sigact.sa_mask);
        if (sigaction(SIGRTMIN + 1, &sigact, NULL) < 0) {
                perror("sigaction error");
                exit(1);
        }

        sigevp.sigev_notify = SIGEV_SIGNAL;
        sigevp.sigev_signo = SIGRTMIN + 1;
        if (timer_create(CLOCK_REALTIME, &sigevp, &timerid) < 0) {
                perror("timer_create error");
                exit(1);
        }

        ispec.it_interval.tv_sec = 0;
        ispec.it_interval.tv_nsec = 10000000;
        ispec.it_value.tv_sec = 0;
        ispec.it_value.tv_nsec = 0;
        if (timer_settime(timerid, 0, &ispec, NULL) < 0) {
                perror("timer_settime error");
                exit(1);
        }
}
#endif

static lkl_thread_t lkl_thread_create(void (*fn)(void *), void *arg)
{
#ifdef __FIBER__
        thread_t *thread = thread_create("lkl", (int (*)(void *))fn, arg, DEFAULT_PRIORITY, 2*1024*1024);
        if (!thread) {
                return 0;
        } else {
                thread_resume(thread);
                return (lkl_thread_t) thread;
        }
#else
	pthread_t thread;
	if (WARN_PTHREAD(pthread_create(&thread, NULL, (void* (*)(void *))fn, arg)))
		return 0;
	else
		return (lkl_thread_t) thread;
#endif /* __FIBER__ */
}

static void lkl_thread_detach(void)
{
#ifdef __FIBER__
        thread_detach(get_current_thread());
#else
	WARN_PTHREAD(pthread_detach(pthread_self()));
#endif /* __FIBER__ */
}

static void lkl_thread_exit(void)
{
#ifdef __FIBER__
        thread_exit(0);
#else
	pthread_exit(NULL);
#endif /* __FIBER__ */
}

static int lkl_thread_join(lkl_thread_t tid)
{
#ifdef __FIBER__
        if (thread_join((thread_t *)tid, NULL, INFINITE_TIME))
#else
	if (WARN_PTHREAD(pthread_join((pthread_t)tid, NULL)))
#endif /* __FIBER__ */
		return -1;
	else
		return 0;
}

static lkl_thread_t lkl_thread_self(void)
{
#ifdef __FIBER__
        return (lkl_thread_t)get_current_thread();
#else
	return (lkl_thread_t)pthread_self();
#endif /* __FIBER__ */
}

static int lkl_thread_equal(lkl_thread_t a, lkl_thread_t b)
{
#ifdef __FIBER__
        return a==b;
#else
	return pthread_equal((pthread_t)a, (pthread_t)b);
#endif /* __FIBER__ */
}

static struct lkl_tls_key *tls_alloc(void (*destructor)(void *))
{
	struct lkl_tls_key *ret = malloc(sizeof(struct lkl_tls_key));
#ifdef __FIBER__
        get_current_thread()->tls[ret->key] = (uintptr_t)ret;
#else
	if (WARN_PTHREAD(pthread_key_create(&ret->key, destructor))) {
		free(ret);
		return NULL;
	}
#endif /* __FIBER__ */
	return ret;
}

static void tls_free(struct lkl_tls_key *key)
{
#ifdef __FIBER__
        get_current_thread()->tls[key->key] = (uintptr_t)NULL;
#else
	WARN_PTHREAD(pthread_key_delete(key->key));
#endif /* __FIBER__ */
	free(key);
}

static int tls_set(struct lkl_tls_key *key, void *data)
{
#ifdef __FIBER__
        get_current_thread()->tls[key->key] = (uintptr_t)data;
#else
	if (WARN_PTHREAD(pthread_setspecific(key->key, data)))
		return -1;
#endif /* __FIBER__ */
	return 0;
}

static void *tls_get(struct lkl_tls_key *key)
{
#ifdef __FIBER__
        return (void *)get_current_thread()->tls[key->key];
#else
	return pthread_getspecific(key->key);
#endif /* __FIBER__ */
}

static unsigned long long time_ns(void)
{
#ifdef __FIBER__
        return current_time()*1000000UL;
#else
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return 1e9*ts.tv_sec + ts.tv_nsec;
#endif /* __FIBER__ */
}

static void *lkl_timer_alloc(void (*fn)(void *), void *arg)
{
#ifdef __FIBER__
        lk_timer_t *timer = malloc(sizeof(lk_timer_t));

        if (!timer) {
                lkl_printf("malloc: %s\n", strerror(errno));
                return NULL;
        }

        timer_initialize(timer);

        timer->callback = fn;
        timer->arg = arg;

        return (void *)timer;
#else
	int err;
	struct sigevent se =  {
		.sigev_notify = SIGEV_THREAD,
		.sigev_value = {
			.sival_ptr = arg,
		},
		.sigev_notify_function = (void (*)(union sigval))fn,
	};
	timer_t timer;

	err = timer_create(CLOCK_REALTIME, &se, &timer);
	if (err)
		return NULL;

	return (void *)(long)timer;
#endif /* __FIBER__ */
}

static int lkl_timer_set_oneshot(void *_timer, unsigned long ns)
{
#ifdef __FIBER__
        lk_timer_t *timer = _timer;
        lk_time_t delay = ns / 1000000;

        timer_set_oneshot(timer, delay, timer->callback, timer->arg);

        return 0;
#else
        struct itimerspec ts = {
                .it_value = {
                        .tv_sec = ns / 1000000000,
                        .tv_nsec = ns % 1000000000,
                },
        };
	timer_t timer = (timer_t)(long)_timer;

	return timer_settime(timer, 0, &ts, NULL);
#endif /* __FIBER__ */
}

static void lkl_timer_free(void *_timer)
{
#ifdef __FIBER__
        lk_timer_t *timer = _timer;

        timer_cancel(timer);
#else
	timer_t timer = (timer_t)(long)_timer;

	timer_delete(timer);
#endif /* __FIBER__ */
}

static void lkl_panic(void)
{
	assert(0);
}

static long _gettid(void)
{
#ifdef __FIBER__
        return (long)get_current_thread();
#else
#ifdef	__FreeBSD__
	return (long)pthread_self();
#elif EMSCRIPTEN
        return 0;
#else
	return syscall(SYS_gettid);
#endif
#endif /* __FIBER__ */
}

struct lkl_host_operations lkl_host_ops = {
	.panic = lkl_panic,
	.thread_create = lkl_thread_create,
	.thread_detach = lkl_thread_detach,
	.thread_exit = lkl_thread_exit,
	.thread_join = lkl_thread_join,
	.thread_self = lkl_thread_self,
	.thread_equal = lkl_thread_equal,
	.sem_alloc = lkl_sem_alloc,
	.sem_free = lkl_sem_free,
	.sem_up = lkl_sem_up,
	.sem_down = lkl_sem_down,
	.mutex_alloc = lkl_mutex_alloc,
	.mutex_free = lkl_mutex_free,
	.mutex_lock = lkl_mutex_lock,
	.mutex_unlock = lkl_mutex_unlock,
	.tls_alloc = tls_alloc,
	.tls_free = tls_free,
	.tls_set = tls_set,
	.tls_get = tls_get,
	.time = time_ns,
	.timer_alloc = lkl_timer_alloc,
	.timer_set_oneshot = lkl_timer_set_oneshot,
	.timer_free = lkl_timer_free,
	.print = print,
	.mem_alloc = malloc,
	.mem_free = free,
	.ioremap = lkl_ioremap,
	.iomem_access = lkl_iomem_access,
	.virtio_devices = lkl_virtio_devs,
	.gettid = _gettid,
	.jmp_buf_set = jmp_buf_set,
	.jmp_buf_longjmp = jmp_buf_longjmp,
};

static int fd_get_capacity(struct lkl_disk disk, unsigned long long *res)
{
	off_t off;

	off = lseek(disk.fd, 0, SEEK_END);
	if (off < 0)
		return -1;

	*res = off;
	return 0;
}

static int do_rw(ssize_t (*fn)(), struct lkl_disk disk, struct lkl_blk_req *req)
{
	off_t off = req->sector * 512;
	void *addr;
	int len;
	int i;
	int ret = 0;

	for (i = 0; i < req->count; i++) {

		addr = req->buf[i].iov_base;
		len = req->buf[i].iov_len;

		do {
			ret = fn(disk.fd, addr, len, off);

			if (ret <= 0) {
				ret = -1;
				goto out;
			}

			addr += ret;
			len -= ret;
			off += ret;

		} while (len);
	}

out:
	return ret;
}

static int blk_request(struct lkl_disk disk, struct lkl_blk_req *req)
{
	int err = 0;

	switch (req->type) {
	case LKL_DEV_BLK_TYPE_READ:
		err = do_rw(pread, disk, req);
		break;
	case LKL_DEV_BLK_TYPE_WRITE:
		err = do_rw(pwrite, disk, req);
		break;
	case LKL_DEV_BLK_TYPE_FLUSH:
	case LKL_DEV_BLK_TYPE_FLUSH_OUT:
#ifdef __linux__
		err = fdatasync(disk.fd);
#else
		err = fsync(disk.fd);
#endif
		break;
	default:
		return LKL_DEV_BLK_STATUS_UNSUP;
	}

	if (err < 0)
		return LKL_DEV_BLK_STATUS_IOERR;

	return LKL_DEV_BLK_STATUS_OK;
}

struct lkl_dev_blk_ops lkl_dev_blk_ops = {
	.get_capacity = fd_get_capacity,
	.request = blk_request,
};

