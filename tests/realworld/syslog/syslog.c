/* This is a special version of syslog.c that has been modified to
** always use network sockets, so that it can be called from within
** a chroot() tree.  It has also been made more portable.  -JP
*/

/*
 * Copyright (c) 1983, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef __alpha
#define _XOPEN_SOURCE_EXTENDED
#endif

/* Always use a network socket.  This lets us run in a chroot() tree. */
#define USE_INET

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslog.c    8.9 (Berkeley) 9/4/95";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#ifdef USE_INET
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <syslog.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif
#ifndef _PATH_CONSOLE
# define _PATH_CONSOLE  "/dev/console"
#endif
#ifndef _PATH_LOG
# define _PATH_LOG      "/dev/log"
#endif

#include <stdarg.h>

#ifndef HAVE_STRERROR
char *strerror(int);
#endif

#ifndef LOG_PRI
# define LOG_PRI(p)     ((p) & LOG_PRIMASK)
#endif

#ifndef LOG_PERROR
# define LOG_PERROR     0
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#define BUFSLOP         1024    /* overflow space */

static int      LogFile = -1;           /* fd for log */
static int      connected;              /* have done connect */
static int      LogStat = 0;            /* status bits, set by openlog() */
static const char *LogTag = NULL;       /* string to tag the entry with */
static int      LogFacility = LOG_USER; /* default facility code */
static int      LogMask = 0xff;         /* mask of priorities to be logged */
#ifdef HAVE__PROGNAME
extern char     *__progname;            /* Program name, from crt0. */
#else
char            *__progname = NULL;
#endif

void syslog(int pri, const char *fmt, ...);
void openlog(const char *ident, int logstat, int logfac);
void closelog(void);

/*
 * syslog --
 *      print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
	register int cnt;
	register char ch, *p, *t;
	time_t now;
	int fd, saved_errno;
#ifndef HAVE_VSNPRINTF
	int panic = 0;
#endif
	static int maxsend = BUFSIZ;
	char *stdp, fmt_cpy[1024], tbuf[BUFSIZ + BUFSLOP];
	va_list ap;

#define INTERNALLOG     LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		syslog(INTERNALLOG,
		    "syslog: unknown facility/priority: %x", pri);
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if (!(LOG_MASK(LOG_PRI(pri)) & LogMask))
		return;

	saved_errno = errno;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* Get connected. */
	if (!connected)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* Build the message. */
	(void)time(&now);
	sprintf(tbuf, "<%d>", pri);
	p = tbuf + strlen(tbuf);
	strftime(p, sizeof (tbuf) - (p - tbuf), "%h %e %T ", localtime(&now));
	p += strlen(p);
	stdp = p;
	if (LogTag == NULL)
		LogTag = __progname;
	if (LogTag != NULL) {
		char *q = strrchr(LogTag, '/');

		if (q != NULL)
			LogTag = ++q;
		sprintf(p, "%.20s", LogTag);
		p += strlen(p);
	}
	if (LogStat & LOG_PID) {
		sprintf(p, "[%ld]", (long) getpid());
		p += strlen(p);
	}
	if (LogTag != NULL) {
		*p++ = ':';
		*p++ = ' ';
	}

	/* Substitute error message for %m. */
	for (t = fmt_cpy; (ch = *fmt) != '\0'; ++fmt)
		if (ch == '%' && fmt[1] == 'm') {
			++fmt;
			strcpy(t, strerror(saved_errno));
			t += strlen(t);
		} else
			*t++ = ch;
	*t = '\0';

	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	cnt = maxsend - (p - tbuf) + 1;
	p += vsnprintf(p, cnt, fmt_cpy, ap);
	cnt = p - tbuf;
#else
	vsprintf(p, fmt_cpy, ap);
	p += strlen(p);
	cnt = p - tbuf;
	if (cnt > sizeof tbuf) {
		/* Panic condition. */
		panic = 1;
	}
	if (cnt > maxsend)
		cnt = maxsend;
#endif
	va_end(ap);

	/* Output to stderr if requested. */
	if (LogStat & LOG_PERROR) {
		struct iovec iov[2];
		register struct iovec *v = iov;

		v->iov_base = stdp;
		v->iov_len = cnt - (stdp - tbuf);
		++v;
		v->iov_base = "\n";
		v->iov_len = 1;
		(void)writev(STDERR_FILENO, iov, 2);
	}

	/* Output the message to the local logger. */
	for (;;) {
		if (send(LogFile, tbuf, cnt, 0) >= 0)
			goto done;
		if (errno != EMSGSIZE || maxsend <= 256)
			break;

		/* Message was too large -- back it off. */
		do {
			maxsend -= 128;
		} while (cnt < maxsend && maxsend > 256);
		cnt = maxsend;
	}

	/*
	 * Output the message to the console; don't worry about blocking,
	 * if console blocks everything will.  Make sure the error reported
	 * is the one from the syslogd failure.
	 */
	if (LogStat & LOG_CONS &&
	    (fd = open(_PATH_CONSOLE, O_WRONLY, 0)) >= 0) {
		(void)strcat(tbuf, "\r\n");
		cnt += 2;
		p = strchr(tbuf, '>') + 1;
		(void)write(fd, p, cnt - (p - tbuf));
		(void)close(fd);
	}

  done: ;
#ifndef HAVE_VSNPRINTF
	/*
	 * If we had a buffer overrun, log a panic and abort.
	 * We can't return because our stack is probably toast.
	 */
	if (panic) {
		syslog(LOG_EMERG, "SYSLOG BUFFER OVERRUN -- EXITING");
		abort();
	}
#endif
}

static struct sockaddr SyslogAddr;      /* AF_UNIX address of local logger */

void
openlog(const char *ident, int logstat, int logfac)
{
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		LogFacility = logfac;

	if (LogFile == -1) {
#ifndef USE_INET
		SyslogAddr.sa_family = AF_UNIX;
		(void)strncpy(SyslogAddr.sa_data, _PATH_LOG,
		    sizeof(SyslogAddr.sa_data));
		if (LogStat & LOG_NDELAY) {
			if ((LogFile = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
				return;
			(void)fcntl(LogFile, F_SETFD, 1);
		}
#else
#define satosin(sa)     ((struct sockaddr_in *)(sa))

		satosin(&SyslogAddr)->sin_family = AF_INET;
		satosin(&SyslogAddr)->sin_port = 514;
		satosin(&SyslogAddr)->sin_addr.s_addr = inet_addr("127.0.0.1");
		if (LogStat & LOG_NDELAY) {
			if ((LogFile = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
				return;
			(void)fcntl(LogFile, F_SETFD, 1);
			if (connect(LogFile, &SyslogAddr,
			    sizeof(SyslogAddr)) < 0) {
				closelog();
				return;
			}
		}
#endif
	}
	if (LogFile != -1 && !connected) {
		if (connect(LogFile, &SyslogAddr, sizeof(SyslogAddr)) == -1) {
			(void)close(LogFile);
			LogFile = -1;
		} else
			connected = 1;
	}
}

void
closelog(void)
{
	(void)close(LogFile);
	LogFile = -1;
	connected = 0;
}

#ifdef notdef
/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
	int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
#endif
