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

int dnet_init(int argc, char **argv) {
	int err = 0, res = 0;
	if ((err = dnet_crypt_init(DNET_VERSION))) {
		sleep(3); printf("Password incorrect.\n");
		return err;
	}
	return res;
}
