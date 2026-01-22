/**********************************************************************
* Filename : compiler-extra.h
*
* Kurzbeschreibung : Macros fuer bestimmte Compilerfeatures
*
* Autoren : Andreas Jaeger
*
* 25.10.96: Erstellung
*
**********************************************************************/

#ifndef COMPILER_EXTRA_H
#define COMPILER_EXTRA_H

/* Folgende Makros werden deklariert:
   INLINE: Bei der Funktionsdefinition wird angegeben:
           INLINE void func (void);
   MAYBE_UNUSEDPARAM: Fuer Funktionensvariablen:
           void func (int a MAYBE_UNUSEDPARAM);
   MAYBE_UNUSEDFUNC: Fuer Funktionen in der Deklaration:
           void func (void) MAYBE_UNUSEDFUNC;
   NEVER_RETURN: Fuer Funktionen in der Deklaration:
           void func (void) NEVER_RETURN;
*/

#if defined(__clang__) || (defined (__GNUC__) && (((__GNUC__ > 1) && (__GNUC_MINOR__ > 6)) || (__GNUC__ > 2)))
# if defined (__cplusplus)
#  define INLINE inline
#  define MAYBE_UNUSEDFUNC
#  define MAYBE_UNUSEDPARAM
#  define NEVER_RETURN
# else
#  define INLINE __inline__
#  define MAYBE_UNUSEDFUNC __attribute__((__unused__))
#  define MAYBE_UNUSEDPARAM __attribute__((__unused__))
#  define NEVER_RETURN __attribute__((__noreturn__))
# endif /* __cplusplus */
#elif defined(__APOGEE__)
# define INLINE 
# define MAYBE_UNUSEDFUNC
# define MAYBE_UNUSEDPARAM
# define NEVER_RETURN
#else
# define INLINE
# define MAYBE_UNUSEDFUNC
# define MAYBE_UNUSEDPARAM
# define NEVER_RETURN
#endif

#endif
