/*
 * Copyright (c) 2018 Tim Kuijsten
 * Copyright (c) 2018 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "slant-agent.h"

static	sig_atomic_t	doexit = 0;

static void
sig(int sig)
{

	doexit = 1;
}

static void
printifcount(FILE *fp, const struct ifcount *ifc)
{
	fprintf(fp, "%llu|%llu|%llu|%llu|%llu|%llu|%llu|%d|%d|",
	    ifc->ifc_ib,	/* input bytes */
	    ifc->ifc_ip,	/* input packets */
	    ifc->ifc_ie,	/* input errors */
	    ifc->ifc_ob,	/* output bytes */
	    ifc->ifc_op,	/* output packets */
	    ifc->ifc_oe,	/* output errors */
	    ifc->ifc_co,	/* collisions */
	    ifc->ifc_flags,	/* up / down */
	    ifc->ifc_state);	/* link state */
}

static void
printifstat(FILE *fp, const struct ifstat *ifs)
{
	printifcount(fp, &ifs->ifs_cur);
	printifcount(fp, &ifs->ifs_old);
	printifcount(fp, &ifs->ifs_now);

	fprintf(fp, "%u|", ifs->ifs_flag);
}

static void
printsysinfo(FILE *fp, const struct syscfg *cfg, const struct sysinfo *p)
{
	size_t i;
	time_t t;

	if (time(&t) == -1)
		err(1, "time");

	fprintf(fp, "%zu|%lld|%d|%f|%f|%f|%lld|%f|%lld|%lld|%lld|%zu|%f|%zu|"
	    "%llu|%llu|%lld|%lld|%lld|",
	    p->sample,	/* sample number */
	    t,
	    p->pageshift,	/* used for memory pages */
	    p->mem_avg,	/* average memory */
	    p->nproc_pct,	/* nprocs percent */
	    p->nfile_pct,	/* nfiles percent */
	    *p->cpu_states,	/* used for cpu compute */
	    p->cpu_avg,	/* average cpu */
	    **p->cp_time,	/* used for cpu compute */
	    **p->cp_old,	/* used for cpu compute */
	    **p->cp_diff,	/* used for cpu compute */
	    p->ncpu,	/* number cpus */
	    p->rproc_pct,	/* pct command (by name) found */
	    p->ifstatsz,	/* used for inet compute */
	    p->disc_rbytes,	/* last disc total read */
	    p->disc_wbytes,	/* last disc total write */
	    p->disc_ravg,	/* average reads/sec */
	    p->disc_wavg,	/* average reads/sec */
	    p->boottime);	/* time booted */

	printifstat(fp, p->ifstats);	/* used for inet compute */
	printifcount(fp, &p->ifsum);	/* average inet */

	for (i = 0; i < cfg->discsz; i++) {
		if (i > 0)
			fprintf(fp, ",");
		fprintf(fp, "%s", cfg->discs[i]);
	}

	fprintf(fp, "|");

	for (i = 0; i < cfg->cmdsz; i++) {
		if (i > 0)
			fprintf(fp, ",");
		fprintf(fp, "%s", cfg->cmds[i]);
	}

	fprintf(fp, "|\n");

	if (fflush(fp) == -1)
		err(1, "fflush");
}

/*
 * Test if a string is suitable for use in printsysinfo.
 *
 * Return 0 if "name" is not printable, 1 if it is.
 */
int
isprintable(const char *name)
{
	size_t i;

	for (i = 0; name[i]; i++) {
		if (!isprint(name[i]))
			return 0;
		if (name[i] == '|')
			return 0;
		if (name[i] == ',')
			return 0;
	}

	return 1;
}

static void
cfg_free(struct syscfg *cfg)
{
	size_t	 i;

	for (i = 0; i < cfg->discsz; i++)
		free(cfg->discs[i]);
	for (i = 0; i < cfg->cmdsz; i++)
		free(cfg->cmds[i]);

	free(cfg->discs);
	free(cfg->cmds);
}

int
main(int argc, char *argv[])
{
	struct sysinfo	*info;
	int		 c, rc = 0, noop = 0, verb = 0;
	char		*d, *discs = NULL, *procs = NULL;
	struct syscfg	 cfg;

	memset(&cfg, 0, sizeof(struct syscfg));

	while (-1 != (c = getopt(argc, argv, "d:nv:p:")))
		switch (c) {
		case 'd':
			discs = optarg;
			break;
		case 'n':
			noop = 1;
			break;
		case 'p':
			procs = optarg;
			break;
		case 'v':
			verb = 1;
			break;
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	/*
	 * From here on our, use the "out" label for bailing on errors,
	 * which will clean up behind us.
	 * Let SIGINT and SIGTERM trigger us into exiting safely.
	 */

	if (NULL != discs)
		while (NULL != (d = strsep(&discs, ","))) {
			if ('\0' == d[0])
				continue;
			if (!isprintable(d))
				errx(1, "%s contains illegal characters", d);
			cfg.discs = reallocarray
				(cfg.discs, 
				 cfg.discsz + 1,
				 sizeof(char *));
			if (NULL == cfg.discs)
				err(EXIT_FAILURE, NULL);
			cfg.discs[cfg.discsz] = strdup(d);
			if (NULL == cfg.discs[cfg.discsz]) 
				err(EXIT_FAILURE, NULL);
			cfg.discsz++;
		}

	if (NULL != procs)
		while (NULL != (d = strsep(&procs, ","))) {
			if ('\0' == d[0])
				continue;
			if (!isprintable(d))
				errx(1, "%s contains illegal characters", d);
			cfg.cmds = reallocarray
				(cfg.cmds, 
				 cfg.cmdsz + 1,
				 sizeof(char *));
			if (NULL == cfg.cmds)
				err(EXIT_FAILURE, NULL);
			cfg.cmds[cfg.cmdsz] = strdup(d);
			if (NULL == cfg.cmds[cfg.cmdsz]) 
				err(EXIT_FAILURE, NULL);
			cfg.cmdsz++;
		}

	if (NULL == (info = sysinfo_alloc()))
		goto out;

	if (SIG_ERR == signal(SIGINT, sig) ||
	    SIG_ERR == signal(SIGTERM, sig)) {
		warn("signal");
		goto out;
	}

	/*
	 * Now enter our main loop.
	 * The body will run every 15 seconds.
	 * Start each iteration by grabbing the current system state
	 * using sysctl(3).
	 * Then grab what we have in the database.
	 * Lastly, modify the database state given our current.
	 */

	if (-1 == chdir("/"))
		err(EXIT_FAILURE, "chdir");

	if (-1 == unveil("/var/empty", ""))
		err(EXIT_FAILURE, "unveil");

	if (-1 == unveil(NULL, NULL))
		err(EXIT_FAILURE, "unveil");

	while ( ! doexit) {
		if ( ! sysinfo_update(&cfg, info))
			goto out;
		printsysinfo(stdout, &cfg, info);
		if (sleep(15))
			break;
	}

	rc = 1;
out:
	cfg_free(&cfg);
	sysinfo_free(info);
	return rc ? EXIT_SUCCESS : EXIT_FAILURE;
usage:
	fprintf(stderr, "usage: %s "
		"[-nv] "
		"[-d discs] "
		"[-f dbfile]\n", getprogname());
	return EXIT_FAILURE;
}
