/* dnet: main file; T11.231-T13.789; $DVS:time$ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#include "dnet_crypt.h"
#include "dnet_main.h"
#include "./utils/log.h"

#if defined (__MACOS__) || defined (__APPLE__)
#define SIGPOLL SIGIO
#endif

//#define NO_DNET_FORK

extern int getdtablesize(void);

static void daemonize(void) {
#if !defined(_WIN32) && !defined(_WIN64) && !defined(QDNET) && !defined(NO_DNET_FORK)
	int i;

	if (getppid() == 1) exit(0); /* already a daemon */
	i = fork();
	if (i < 0) exit(1); /* fork error */
	if (i > 0) exit(0); /* parent exits */

	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i = getdtablesize(); i >= 0; --i) close(i); /* close all descriptors */
	i = open("/dev/null", O_RDWR); dup(i); dup(i); /* handle standard I/O */

	/* first instance continues */
#if 0
	signal(SIGCHLD, SIG_IGN); /* ignore child */
#endif
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGPOLL, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGVTALRM, SIG_IGN);
	signal(SIGPROF, SIG_IGN);
#endif
}

static void angelize(void) {
#if !defined(__LDuS__) && !defined(QDNET) && !defined(_WIN32) && !defined(_WIN64) && !defined(NO_DNET_FORK)
    int stat = 0;
    pid_t childpid;
	while ((childpid = fork())) {
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
		if (childpid > 0) while (waitpid(childpid, &stat, 0) == -1) {
			if (errno != EINTR) {
				abort();
			}
		}
		if (stat >= 0 && stat <= 5) {
			exit(stat);
		}
		sleep(10);
    }
#endif
}

int dnet_init(int argc, char **argv) {
	int i = 0, err = 0, res = 0, is_daemon = 0, is_server = 0;

	for (i = 1; i < argc + 2; ++i) {
		if (i == 1) {
#if !defined(_WIN32) && !defined(_WIN64)
			if (i < argc && !strcmp(argv[i], "-d")) is_daemon = 1;
#endif
			printf("%s %s%s.\n", argv[0], DNET_VERSION, (is_daemon ? ", running as daemon" : ""));
			if ((err = dnet_crypt_init(DNET_VERSION))) {
				sleep(3); printf("Password incorrect.\n");
				return err;
			}
		work:
			if (is_daemon) daemonize();
			angelize();
			xdag_mess("%s %s%s.\n", argv[0], DNET_VERSION, (is_daemon ? ", running as daemon" : ""));
			if (is_daemon) continue;
		}
		if (i < argc && !strcmp(argv[i], "-s")) {
			is_server = 1;
			continue;
		}
		if (i < argc - 1 && !strcmp(argv[i], "-c")) {
			i++;
			continue;
		}
	}
	return res;
}
