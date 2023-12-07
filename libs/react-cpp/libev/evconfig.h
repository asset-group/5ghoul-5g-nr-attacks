#ifndef EV_CONFIG_H_
#define EV_CONFIG_H_

#define HAVE_SYS_INOTIFY_H 1
#define HAVE_SYS_EPOLL_H 1
/* #undef HAVE_SYS_EVENT_H */
/* #undef HAVE_PORT_H */
#define HAVE_POLL_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_EVENTFD_H 1
#define HAVE_SYS_SIGNALFD_H 1

#define HAVE_INOTIFY_INIT 1
#define HAVE_EPOLL_CTL 1
/* #undef HAVE_KQUEUE */
/* #undef HAVE_PORT_CREATE */
#define HAVE_POLL 1
#define HAVE_SELECT 1
#define HAVE_EVENTFD 1
#define HAVE_SIGNALFD 1

#define HAVE_CLOCK_GETTIME 1
#define HAVE_CLOCK_SYSCALL 1
#define HAVE_NANOSLEEP 1
#define HAVE_FLOOR 1

#define EV_USE_REALTIME 1

#define PACKAGE "wdissector"
#define VERSION "4.33"

#endif // EV_CONFIG_H_
