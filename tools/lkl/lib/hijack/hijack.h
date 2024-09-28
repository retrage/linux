/* SPDX-License-Identifier: GPL-2.0 */

#include <sys/socket.h>
#include <sys/epoll.h>
#include <poll.h>


int is_lklfd(int fd);
int hijack_setsockopt(int fd, int level, int optname, const void *optval,
		      socklen_t optlen);
int hijack_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);
int hijack_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int hijack_ppoll(struct pollfd *fds, nfds_t nfds,
		  const struct timespec *timeout_ts, const sigset_t *sigmask);
int hijack_select(int nfds, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout);
int hijack_pselect(int nfds, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, const struct timespec *timeout,
		  const sigset_t *sigmask);
int hijack_eventfd(unsigned int count, int flags);
int hijack_epoll_create(int size);
int hijack_epoll_create1(int flags);
int hijack_epoll_ctl(int epollfd, int op, int fd, struct epoll_event *event);
int hijack_epoll_wait(int epfd, struct epoll_event *events,
	      int maxevents, int timeout);
int hijack_epoll_pwait(int epfd, struct epoll_event *events,
				int maxevents, int timeout,
				const sigset_t *sigmask);
int hijack_eventfd_read(int fd, uint64_t *value);
int hijack_eventfd_write(int fd, uint64_t value);
