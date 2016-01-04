/* cc -g -Wall -O0 -I$(pwd)/build/soletta_sysroot/usr/include/soletta -lsoletta -L$(pwd)/build/soletta_sysroot/usr/lib -Wl,-rpath $(pwd)/build/soletta_sysroot/usr/lib hostname-monitor.c -o hostmon */

#include <stdio.h>
#include <sol-platform.h>
#include <sol-mainloop.h>

static void hostnameHasChanged(void *nothing, const char *hostname) {
	printf("%s\n", hostname);
}

static void startup() {
	sol_platform_add_hostname_monitor(hostnameHasChanged, NULL);
}

static void shutdown() {
	sol_platform_del_hostname_monitor(hostnameHasChanged, NULL);
}

SOL_MAIN_DEFAULT(startup, shutdown);
