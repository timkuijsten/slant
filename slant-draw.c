#include <sys/queue.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <assert.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"
#include "slant.h"

struct	buf {
	char	*buf;
	size_t	 bufsz;
};

static void
buf_appendv(struct buf *, const char *, ...)
	__attribute__((format(printf, 2, 3)));

static void
buf_appendc(struct buf *b, char c)
{

	b->buf = realloc(b->buf, b->bufsz + 2);
	if (NULL == b->buf) 
		err(EXIT_FAILURE, "realloc");

	b->buf[b->bufsz] = c;
	b->buf[b->bufsz + 1] = '\0';
	b->bufsz++;
}

static void
buf_appendv(struct buf *b, const char *fmt, ...)
{
	int	 len;
	va_list	 ap;
	
	va_start(ap, fmt);

	/* Get length of variable string. */

	if ((len = vsnprintf(NULL, 0, fmt, ap)) < 0)
		err(EXIT_FAILURE, "vsnprintf");

	va_end(ap);
	va_start(ap, fmt);

	/* Reallocate and fill to new buffer + NUL. */

	b->buf = realloc(b->buf, b->bufsz + len + 1);
	if (NULL == b->buf) 
		err(EXIT_FAILURE, "realloc");
	vsnprintf(b->buf + b->bufsz, len + 1, fmt, ap);
	b->bufsz += len;
	va_end(ap);

}

void
draw(const struct node *n, size_t nsz)
{
	size_t	 	 i, j, sz, maxhostsz, maxipsz;
	struct buf	 b;
	double		 v, vv;

	maxhostsz = strlen("hostname");
	for (i = 0; i < nsz; i++) {
		sz = strlen(n[i].host);
		if (sz > maxhostsz)
			maxhostsz = sz;
	}

	memset(&b, 0, sizeof(struct buf));

	/*
	 * hostname                          (maxhostsz)
	 * [|||||||||| xxx.x%|xxx.x%|xxx.x%] ([10 6|6|6]=31)
	 * hh:mm:ss o
	 */

	for (i = 0; i < nsz; i++) {
		buf_appendv(&b, "%*s [", 
			(int)maxhostsz, n[i].host);
		if (NULL == n[i].recs) {
			buf_appendv(&b, "%31s", "]\n");
			continue;
		} 
		
		if (n[i].recs->byqminsz &&
	  	    n[i].recs->byqmin[0].entries) {
			vv = n[i].recs->byqmin[0].cpu /
				n[i].recs->byqmin[0].entries;
			for (j = 1; j <= 10; j++) {
				v = j * 10.0;
				if (v > vv)
					break;
				buf_appendc(&b, '|');
			}
			for ( ; j <= 10; j++)
				buf_appendc(&b, ' ');
			buf_appendv(&b, " %4.1f%%|", vv);
		} else
			buf_appendv(&b, "%17s", "|");

		if (n[i].recs->byminsz &&
	  	    n[i].recs->bymin[0].entries) {
			vv = n[i].recs->bymin[0].cpu /
				n[i].recs->bymin[0].entries;
			buf_appendv(&b, " %4.1f%%|", vv);
		} else
			buf_appendv(&b, "%7s", "|");

		if (n[i].recs->byhoursz &&
	  	    n[i].recs->byhour[0].entries) {
			vv = n[i].recs->byhour[0].cpu /
				n[i].recs->byhour[0].entries;
			buf_appendv(&b, " %4.1f%%]", vv);
		} else
			buf_appendv(&b, "%7s", "]");

		buf_appendc(&b, '\n');
	}

	printf("%s", b.buf);
	free(b.buf);
}
