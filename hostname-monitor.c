/* cc -g -Wall -O0 -I$(pwd)/build/soletta_sysroot/usr/include/soletta -lsoletta -L$(pwd)/build/soletta_sysroot/usr/lib -Wl,-rpath $(pwd)/build/soletta_sysroot/usr/lib hostname-monitor.c -o hostmon */

#include <stdio.h>
#include <sol-platform.h>
#include <sol-mainloop.h>

static void localeHasChanged(void *nothing, enum sol_platform_locale_category category, const char *locale) {
	printf("%d, %s\n", category, locale);
}

static void startup() {
	sol_platform_add_locale_monitor(localeHasChanged, NULL);
}

static void shutdown() {
	sol_platform_del_locale_monitor(localeHasChanged, NULL);
}

SOL_MAIN_DEFAULT(startup, shutdown);
