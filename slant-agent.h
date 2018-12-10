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
#ifndef SLANT_AGENT_H
#define SLANT_AGENT_H

struct 	ifcount {
	u_int64_t	ifc_ib;			/* input bytes */
	u_int64_t	ifc_ip;			/* input packets */
	u_int64_t	ifc_ie;			/* input errors */
	u_int64_t	ifc_ob;			/* output bytes */
	u_int64_t	ifc_op;			/* output packets */
	u_int64_t	ifc_oe;			/* output errors */
	u_int64_t	ifc_co;			/* collisions */
	int		ifc_flags;		/* up / down */
	int		ifc_state;		/* link state */
};

struct	ifstat {
	struct ifcount	ifs_cur;
	struct ifcount	ifs_old;
	struct ifcount	ifs_now;
	char		ifs_flag;
};

/*
 * Define pagetok in terms of pageshift.
 */
#define PAGETOK(size, pageshift) ((size) << (pageshift))

struct	sysinfo {
	size_t		 sample; /* sample number */
	int		 pageshift; /* used for memory pages */
	double		 mem_avg; /* average memory */
	double		 nproc_pct; /* nprocs percent */
	double		 nfile_pct; /* nfiles percent */
	int64_t         *cpu_states; /* used for cpu compute */
	double		 cpu_avg; /* average cpu */
	int64_t        **cp_time; /* used for cpu compute */
	int64_t        **cp_old; /* used for cpu compute */
	int64_t        **cp_diff; /* used for cpu compute */
	size_t		 ncpu; /* number cpus */
	double		 rproc_pct; /* pct command (by name) found */
	struct ifstat	*ifstats; /* used for inet compute */
	size_t		 ifstatsz; /* used for inet compute */
	struct ifcount	 ifsum; /* average inet */
	u_int64_t	 disc_rbytes; /* last disc total read */
	u_int64_t	 disc_wbytes; /* last disc total write */
	int64_t	 	 disc_ravg; /* average reads/sec */
	int64_t	 	 disc_wavg; /* average reads/sec */
	time_t		 boottime; /* time booted */
};

/*
 * Configuration of things we're going to look for.
 */
struct	syscfg {
	char	**discs; /* disc devices (e.g., sd0) */
	size_t	  discsz;
	char	**cmds; /* commands (e.g., httpd) */
	size_t	  cmdsz;
};

__BEGIN_DECLS

struct sysinfo	*sysinfo_alloc(void);
int 		 sysinfo_update(const struct syscfg *, struct sysinfo *);
void 		 sysinfo_free(struct sysinfo *);

__END_DECLS

#endif /* ! SLANT_SLANT_AGENT_H */
