#include "parse-util.h"
#include "wm-parse.tab.h"
#include "wrapper.h"

#include <stdio.h>
#include <errno.h>

extern int yydebug;
extern FILE *yyin;

#include <signal.h>
#include <sys/types.h>

#if defined (LINUX)
# if !defined __GLIBC__
#  include <asm/sigcontext.h>
#  include <signal.h>
#  define sigcontext sigcontext_struct
# endif
   typedef __sighandler_t sighandler_t;
#elif defined (SUNOS5)
#include <sys/siginfo.h>
#elif defined (SUNOS4)
#else
#error "Signalbehandlung nur fuer SunOS4, SunOS5 und Linux definiert"
#endif

#if   defined (SUNOS4)
 typedef void (*WM_SignalHandlerT)(int Signal, int Code, struct sigcontext *Kontext, char *Adresse);
 static void Kollisionsbehandlung(int Signal, int Code, struct sigcontext *Kontext, char *Adresse);
#elif defined (SUNOS5)
  static void Kollisionsbehandlung(int Signal, siginfo_t *Kontext, void *Adresse);
#elif defined (LINUX)
  typedef void (*WM_SignalHandlerT) (int Signal, struct sigcontext Kontext);
  static void Kollisionsbehandlung(int Signal, struct sigcontext Kontext);
#endif
static struct sigaction AlterSignalbehandler;

static void LastRessortSignalHandler(void)  /* Wenn kein Signalbehandler gesetzt ist, hilft nur eins: raus! Nichts */
{       /* mehr ausgeben, da es sonst zu einer Endlosschleife kommen kann (auf Linux). Prozedur wurde ausgelagert, */
#if defined SUNOS4 || defined SUNOS5                               /* um einen Breakpoint darauf setzen zu koennen */
  fputs("Unrecoverable Segmentation Fault\n",stderr);                              /* mal austesten, ob das klappt */
#endif
  abort ();                                                                        /* weniger Aufwand als exit(-1) */
}

#define TestAlterSignalbehandler(Signalbehandler)         /* pruefen, ob ueberhaupt eigener Behandler gesetzt war */\
  if ((void *)Signalbehandler==(void *)SIG_DFL) {  /* Diese Abfrage wird vom Apogee bemeckert mit Warnung invalid */\
    LastRessortSignalHandler(); /* type conversion. Es ist aber die einzige Moeglichkeit, den GCC ruhigzustellen. */\
  }

#if defined (SUNOS4)
#define SonstigerFehler() (Code!=SEGV_PROT)
#define AlteBehandlung()                                                                                            \
{ TestAlterSignalbehandler(AlterSignalbehandler.sa_handler);                                                        \
  (*AlterSignalbehandler.sa_handler)(Signal,Code,Kontext,Adresse); }
#elif defined (SUNOS5)
#define SonstigerFehler() (Info->si_code!=SEGV_ACCERR)
#define AlteBehandlung()                                                                                            \
{ if ((AlterSignalbehandler.sa_flags) && SA_SIGINFO) {                                                              \
    TestAlterSignalbehandler(AlterSignalbehandler.sa_sigaction);                                                    \
    (*(AlterSignalbehandler.sa_sigaction))(Signal, Info, Kontext);                                                  \
  }                                                                                                                 \
  else {                                                                                                            \
    TestAlterSignalbehandler(AlterSignalbehandler.sa_handler);                                                      \
    (*(AlterSignalbehandler.sa_handler))(Signal);                                                                   \
  } }
#elif defined (LINUX)
#define SonstigerFehler() (Signal!=SIGSEGV)
#define AlteBehandlung()                                                                                            \
{ TestAlterSignalbehandler (AlterSignalbehandler.sa_handler);                                                       \
  (*(WM_SignalHandlerT)AlterSignalbehandler.sa_handler)(Signal,Kontext); }
#endif

static void Kollisionsbehandlung(int Signal, 
#if   defined (SUNOS4)
                                             int Code, struct sigcontext *Kontext, char *Adresse)
#elif defined (SUNOS5)    
                                             siginfo_t *Info, void *Kontext)
#elif defined (LINUX)    
                                             struct sigcontext Kontext)
#endif
{
    AlteBehandlung()
}

void install_sighandler(void)
{
    struct sigaction sigact;
#if defined (SUNOS4)
    sigact.sa_handler = &Kollisionsbehandlung;
    sigact.sa_flags =  0;
#elif defined (SUNOS5)
    sigact.sa_handler = NULL;
    sigact.sa_sigaction  = &Kollisionsbehandlung;
    sigact.sa_flags =  SA_SIGINFO|SA_RESTART;
#elif defined (LINUX)
    sigact.sa_handler = (sighandler_t)&Kollisionsbehandlung;
    sigact.sa_flags =  SA_RESTART;
#endif                                   
    sigemptyset(&sigact.sa_mask);
    if (sigaction(SIGSEGV, &sigact, &AlterSignalbehandler))
      our_error("Signalbehandler fuer SIGSEGV kann nicht gesetzt werden.");
}


int
#if DO_OMEGA
    C_main
#else
    main
#endif
           (int argc, char *argv[])
{
  install_sighandler();

  if (argc == 2) {
     /*
       printf ("Reading from file `%s'.\n", argv[1]);
      */
    errno = 0;
    yyin = fopen (argv[1], "r");
    if (yyin == NULL) {
      perror ("opening input file");
      exit (1);
    }
  } else {
     if(0)printf ("Let's start!\n\n");
  }
  init_apriori();
  fflush(stdout);
 yydebug = 0;
  yyparse ();
  
  /*  print_symbols();*/
}
