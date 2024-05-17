/*
* Copyright 2023 Ezurio
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided  that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
*  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
*  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
*  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
*  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*--------------------------------------------------------------------------
*/
/*
 * Utility to load the fips provider and stay resident.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <err.h>

#define VERSION_STR "1.0"
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/provider.h>
#include <openssl/core_names.h>
#endif /* OpenSSL version >= 3.0 */

static char usage[] =
	"ossl-fipsload " VERSION_STR "\n"
	"\n"
	"usage:\n"
	"  ossl-fipsload [-B]\n"
	"\n"
	"options:\n"
	"  -B = run daemon in background\n"
	"  -h = show this help text\n";

static OSSL_PROVIDER *openssl_fips_provider = NULL;

// wait for signal to exit
static void wait_for_exit_signal(void) {
	int                      sfd;
	ssize_t                  s;
	sigset_t                 mask;
	struct signalfd_siginfo  fdsi;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		err(EXIT_FAILURE, "sigprocmask");

	sfd = signalfd(-1, &mask, 0);
	if (sfd == -1)
		err(EXIT_FAILURE, "signalfd");

	for (;;) {
		s = read(sfd, &fdsi, sizeof(fdsi));
		if (s != sizeof(fdsi))
			err(EXIT_FAILURE, "read");
		break;
	}
}

static void cleanup(void)
{
	if (openssl_fips_provider) {
		OSSL_PROVIDER_unload(openssl_fips_provider);
	}
}

int main(int argc, char *argv[])
{
	OSSL_PROVIDER *openssl_fips_provider = NULL;
	int opt;
	int resident = 0;
	atexit(cleanup);
	while ((opt = getopt(argc, argv, "Bh")) != -1) {
		switch (opt) {
		case 'B':
			resident = 1;
			break;
		case 'h':
			printf("%s", usage);
			exit(EXIT_SUCCESS);
			break;
		default: /* '?' */
			printf("%s", usage);
			exit(EXIT_FAILURE);
		}
	}

	openssl_fips_provider = OSSL_PROVIDER_load(NULL, "fips");
	if (!openssl_fips_provider) {
		fprintf(stderr, "ERROR: could not load fips provider\n");
		exit(EXIT_FAILURE);
	}
	if (resident) {
		// remain resident to keep fips provider loaded
		if (daemon(0,0) < 0) {
			fprintf(stderr, "ERROR: daemon() failed\n");
			exit(EXIT_FAILURE);
		}
		wait_for_exit_signal();
	}
	exit(EXIT_SUCCESS);
	return 0;
}
