/* Datei: antiwarnings.h
 * Zweck: Definition von Symbolen, die eigentlich def. sein muessten
 * Autor: Bernd Loechner
 * Stand: 06.01.94
 * Ersterstellung: 20.12.93
 * Aenderungen:
 * - strncasecmp ergaenzt, B.L., 06.01.94
 * - atoi erg"anzt, A.B., 11.01.94
 */

#ifndef ANTIWARNINGS_H
#define ANTIWARNINGS_H

#if defined (SUNOS5)
# define __EXTENSIONS__ 1
#endif

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#if defined (SUNOS4) && !defined (__APOGEE__)
extern int   fclose(FILE *stream);
extern int   printf(const char *format, ...);
extern int   fprintf(FILE *stream, const char *format, ...);
extern char  *sprintf(char *s, const char *format, ...);
extern int   scanf(const char *format, ...);
extern int   fscanf(FILE *stream, const char *format, ...);
extern int   sscanf(const char *s, const char *format, ...);
extern int   fseek(FILE *, long, int);
extern int   fflush(FILE *stream);
extern int   fputc(int, FILE *stream);
extern int   fgetc(FILE *stream);
extern int   ungetc(int, FILE *stream);
extern int   puts(const char *s);
extern int   fputs(const char *s, FILE *stream);
extern int   fread (void *ptr, __SIZE_TYPE__ size, __SIZE_TYPE__ nobj, FILE *stream);
extern int   toupper(int);
extern int   strcasecmp(const char *, const char *);
extern int   strncasecmp(const char *s1, const char *s2, int n);
extern void  perror(const char *s);
extern int   gettimeofday(struct timeval *tp, struct timezone *tzp);
extern int   getrusage(int who, struct rusage *rusage);
extern int   getrlimit(int resource, struct rlimit *rlp);
extern int   setrlimit(int resource, struct rlimit *rlp);
extern int   getpagesize(void);
extern void  rewind(FILE *stream);
extern __SIZE_TYPE__   fwrite(const void *ptr, __SIZE_TYPE__ size, __SIZE_TYPE__ nitems, FILE *stream);
extern int   _filbuf(FILE *);
extern int   _flsbuf(unsigned int, FILE *);
extern int   setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
extern int   fork(void);
extern pid_t getpid(void);
extern int   munmap(caddr_t addr, int len);
extern int   mprotect(caddr_t addr, int len, int prot);
extern char  *sys_errlist[];
extern int   truncate(char *path, off_t length);
/* unsigned long int strtoul (char *nptr, char **endptr, int base) */
#ifndef __cplusplus
#define strtoul(nptr, endptr, base) \
      atol (nptr)
#endif
     
#endif /* SUNOS4 && !__APOGEE__ */

#if defined (SUNOS5)     
extern int   getrusage(int who, struct rusage *rusage);
extern int   getpagesize(void);
extern char  *sys_errlist[];
#endif     

#if defined __GLIBC__ && __GLIBC__ >= 2
#define SignalHandler __sighandler_t
#endif

#if defined(DARWIN)
/* macOS / Apple Silicon support */
#include <signal.h>
typedef void (*SignalHandler)(int);
#endif

#if defined(SUNOS4)
/* Wir brauchen uintptr_t */
typedef unsigned int uintptr_t;
#else
# include <inttypes.h>
#endif

#endif
