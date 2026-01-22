/**********************************************************************
* Filename : PowerMain
*
* Kurzbeschreibung : Wrapper fuer PowerWaldmeister mit wenigen, einfachen
*                    Optionen
*
* Autoren : Andreas Jaeger
*
* 08.02.98: Erstellung
*
* Benoetigt folgende Module:
*   SpeicherVerwaltung.h
* Benutzte Globale Variablen aus anderen Modulen:
*   zur Zeit keine
*
**********************************************************************/

#if !defined COMP_POWER || COMP_POWER == 1
/* the following define is needed by at least SunOS 5.5 */
#define _XOPEN_SOURCE 1
#include "antiwarnings.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "version.h"
#include "WaldmeisterII.h"
#include "SpeicherVerwaltung.h"

/* Name of program (pointer to argv[0]) */
char *wm_name;

typedef enum
{
   OPT_DETAILS, OPT_HELP, OPT_EXT_HELP, 
   OPT_VERSION,
   OPT_ADD, OPT_MIX,
   OPT_DB, OPT_OUTPUT,
   OPT_TEX, OPT_PROLOG, OPT_PCL,
   OPT_PCL_NO, OPT_ASCII, OPT_EXPERT, OPT_AUTO,
   OPT_END
} optart_t;

typedef enum
{
   NO_ARGS, REQUIRES_ARG
} args_t;



typedef struct 
{
   const char *flag; /* with leading minus */
   optart_t optart;
   args_t args;
} opt_struct;



static opt_struct options [] =
{
   {"--details", OPT_DETAILS, NO_ARGS},
   {"--help", OPT_HELP, NO_ARGS},
   {"-h", OPT_HELP, NO_ARGS},
   {"--man", OPT_EXT_HELP, NO_ARGS},
   {"--version", OPT_VERSION, NO_ARGS},
   {"-V", OPT_VERSION, NO_ARGS},
   {"--add", OPT_ADD, NO_ARGS},
   {"--mix", OPT_MIX, NO_ARGS},
   {"--db", OPT_DB, REQUIRES_ARG},
   {"--output", OPT_OUTPUT, REQUIRES_ARG},
   {"-o", OPT_OUTPUT, REQUIRES_ARG},
   {"--tex", OPT_TEX, NO_ARGS},
   {"--prolog", OPT_PROLOG, NO_ARGS},
   {"--pcl", OPT_PCL, NO_ARGS},
   {"--noproof", OPT_PCL_NO, NO_ARGS},
   {"--ascii", OPT_ASCII, NO_ARGS},
   {"--auto", OPT_AUTO, NO_ARGS},
   {"--expert", OPT_EXPERT, NO_ARGS}, /* has to come after the filename and other options ! */
   {"", OPT_END, NO_ARGS}
};


typedef enum {pcl_tex, pcl_pcl, pcl_prolog, pcl_ascii, pcl_no} pcl_output_type;
typedef enum {weight_add, weight_mix} weight_type;

static void
print_version (void)
{
      printf ("\
                                                                 \n\
WALDMEISTER                                                      \n\
                                                                 \n\
     Copyright (C) 1994-2002                                     \n\
     by A. Buch, Th. Hillenbrand, A. Jaeger, B. Loechner         \n\
              <waldmeister@informatik.uni-kl.de>                 \n\
     Version: %s                                                 \n\
                                                                 \n\
", wm_release);
}


static void
usage (int status)
{
  if (status)
    {
       switch (status) {
       case 1 :
          fprintf (stderr, "Use only one option of --add and --mix.\n");
          break;
       case 2:
          fprintf (stderr, "Use only one option of --tex, --prolog, --pcl, --ascii and --noproof.\n");
          break;
       case 3:
          fprintf (stderr, "Use --db=FILE not more than once.\n");
          break;
       case 4:
          fprintf (stderr, "Use --output=FILE/-o FILE not more than once.\n");
          break;
       case 6:
          fprintf (stderr, "Option --auto was given more than once.\n");
          break;
       case 7:
          fprintf (stderr, "Option --auto cannot be used together with --add or --mix.\n");
          break;
       default:
          break;
       }
       fprintf (stderr, "Try `%s --help' or `--man' for more information.\n", wm_name);
       exit (1);
    }
  else
    {
      print_version ();
      printf ("\
Usage: %s [OPTION] SPECIFICATION                                             \n\
  -h, --help                display this help and exit                       \n\
      --man                 print \"man page\"-like documentation and exit   \n\
  -V, --version             output version information and exit              \n\
  Options controlling the prover                                             \n\
      --add                 use add-weight heuristic                         \n\
      --mix                 use mix-weight heuristic (default)               \n\
      --auto                activate automated control component             \n\
      --db=FILE             employ database FILE                             \n\
  Options for proof presentation                                             \n\
  -o, --output=FILE         output file is FILE                              \n\
      --details             include more details in proof                    \n\
      --tex                 produce proof in LaTeX notation                  \n\
      --prolog              produce proof in Prolog notation                 \n\
      --pcl                 produce proof in PCL notation                    \n\
      --ascii               produce proof as plain text (default)            \n\
      --noproof             produce no proof presentation                    \n\
"
, wm_name);
    }
  exit (0);
}


static void
manual (void)
{
  print_version ();
  printf ("\
SYNOPSIS                                                         \n\
     %s [OPTION ...] FILE                                        \n\
                                                                 \n\
DESCRIPTION                                                      \n\
     Waldmeister   is a  theorem   prover  for   first-order unit\n\
     equational logic.  It is  based  on unfailing   Knuth-Bendix\n\
     completion employed  as  proof procedure. Waldmeister's main\n\
     advantage  is that efficiency  has been  reached in terms of\n\
     time  as  well as of space.   Within that scope,  a complete\n\
     proof object is constructed at run-time.                    \n\
                                                                 \n\
     Options for online help:                                    \n\
     -h, --help                                                  \n\
          Print a short summary of options.                      \n\
     --man                                                       \n\
          Display this \"man page\"-like documentation.          \n\
     -V, --version                                               \n\
          Output version information.                            \n\
                                                                 \n\
     Options determining the heuristic for the proof search:     \n\
     There  are  two  different functions for weighting  critical\n\
     pairs, each realizing well-behaved general purpose top-level\n\
     heuristic. Only one may be chosen.                          \n\
     --add                                                       \n\
          Add-weight heuristic: measures size of terms           \n\
     --mix                                                       \n\
          Mix-weight heuristic: based on size of terms and orien-\n\
          tability (default)                                     \n\
                                                                 \n\
     A search parameter of perhaps even greater importance is the\n\
     reduction ordering:   A  proof that  is   impossible to find\n\
     within hours may be found  within milliseconds when changing\n\
     from KBO to LPO or vice versa, or changing the operator pre-\n\
     cedence in case of LPO! This parameter  has to be set in the\n\
     specification file.                                         \n\
                                                                 \n\
     Further options influencing the proof search:               \n\
     --auto                                                      \n\
          Activate  automated  control component, including  con-\n\
          struction of a  reduction  ordering  and selection of a\n\
          weighting function, hence not combinable with the above\n\
          options. Invokes a number of  optimizations on  the le-\n\
          vel of the  inference machine  that are  often advanta-\n\
          geous in proof mode,  but may lead to loss of complete-\n\
          ness.                                                  \n\
     --db=FILE                                                   \n\
          Specify name of a database file. See  the ShortDocumen-\n\
          tation for a description.                              \n\
                                                                 \n\
     Options specifying the proof presentation format:           \n\
     Four  different  output formats  are available.             \n\
     --ascii                                                     \n\
          Processed proof as plain text (default)                \n\
     --tex                                                       \n\
          The same as LaTeX source file                          \n\
     --prolog                                                    \n\
          The same in a PROLOG-like notation                     \n\
     --pcl                                                       \n\
          Step-wise internal proof object                        \n\
     --noproof                                                   \n\
          Turns output of proof object of                        \n\
     --slowproof                                                 \n\
          Generates proof object in a slower way                 \n\
          (avoids a rarely occuring bug)                         \n\
                                                                 \n\
     More options controlling the proof presentation:            \n\
     --details                                                   \n\
          Enrich the  proof with more detailed  information, such\n\
          as variable instantiations in lemma applications. Works\n\
          only for ASCII and LaTeX output formats.               \n\
     -o FILE , --output=FILE                                     \n\
          Redirect output to file.                               \n\
                                                                 \n\
SPECIFICATION FORMAT                                             \n\
     This example deals with the standard group axiomatization.  \n\
                                                                 \n\
     NAME            group %% just a comment                     \n\
     MODE            PROOF                                       \n\
                     %% COMPLETION                               \n\
     SORTS           ANY                                         \n\
                     %% ANY1 ANY2                                \n\
     SIGNATURE       e: -> ANY                                   \n\
                     i: ANY -> ANY                               \n\
                     f: ANY ANY -> ANY                           \n\
     ORDERING        LPO                                         \n\
                     i > f > e                                   \n\
                     %% alternatively:                           \n\
                     %% KBO                                      \n\
                     %% i=0, f=1, e=1                            \n\
                     %% i > f > e                                \n\
     VARIABLES       x,y,z : ANY                                 \n\
     EQUATIONS       f(x,e) = x                                  \n\
                     f(x,i(x)) = e                               \n\
                     f(f(x,y),z) = f(x,f(y,z))                   \n\
     CONCLUSION      f(x,i(x)) = f(i(x),x)                       \n\
                                                                 \n\
     Note: The termpairs in  section EQUATIONS are considered  as\n\
     implicitly  universally  quantified    over their variables,\n\
     whereas existantial quantifications have to be expressed via\n\
     skolemization.  Vice versa,  in section CONCLUSION variables\n\
     are implicitly existentially quantified, and universal quan-\n\
     tification must be expressed via  skolemization. Existential\n\
     quantification  is  allowed in proof  mode only, and for not\n\
     more than one hypothesis.                                   \n\
                                                                 \n\
MORE INFORMATION                                                 \n\
     See also the ShortDocumentation.                            \n\
                                                                 \n\
     If you have  any comments  or  suggestions, if you find  any\n\
     bugs, or  if your proof tasks  seem to be invincible, please\n\
     send an e-mail to waldmeister@informatik.uni-kl.de - we will\n\
     try to help. See also the web page of our prover, located at\n\
     http://www.uni-kl.de/AG-AvenhausMadlener/waldmeister.html . \n\
                                                                 \n\
"
, wm_name);

  exit (0);
}


/* check that file `filename' exists and is readable */
static void
check_file (char *filename)
{
   int fd;
      
   fd = open (filename, O_RDONLY);
   if (fd < 0) {
#if SUNOS5 || LINUX
      char *err_str;
      err_str = strerror (errno);
      if (err_str) {
         fprintf (stderr, "Couldn't open file `%s' for reading: %s\n",
                  filename, err_str);
      }
      else
	{
         fprintf (stderr, "Couldn't open file `%s' for reading.\n",
                  filename);
	}
#else
      fprintf (stderr, "Couldn't open file `%s' for reading.\n",
                  filename);
#endif      
      exit (1);
   }
   else 
     {
       close (fd);
     }
   
}

/*
  like strcat but with dynamically enlarging dest,
  notice that dest is of type char ** unlike strcat
*/
static void
my_strcat (char **dest, const char *source, size_t *len_dest)
{
  if (source[0] == '\0')
    return;
  if (strlen (*dest) + strlen (source) + 1 > *len_dest) {
    *len_dest *= 2;
    *dest = our_realloc (*dest, *len_dest, "my_strcat failed.");
  }
  if (*dest[0] != '\0') {
     strcat (*dest, " ");
  }
  strcat (*dest, source);
}

/*
  call Waldmeister
*/
static void
call_wm (char *opt_str, char *pcl_temp)
{
  int count;
  char **new_argv;
  
  BuildArgv (opt_str, wm_name, &count, &new_argv);

#if 0
  {
    int i = 0;
    printf("\nARGV: ");
    for (i=0; i < count; i++)
      printf(" %s ",  new_argv[i]);
    printf("\n");
  }
#endif

  /* call waldmeister main */
  /* Return == 0 heisst, dass Beweise gefunden wurden und weitergearbeitet werden soll */
  /* Return != 0 bedeutet, dass keine Beweise gefunden wurden und somit proof nicht aufgerufen */
  /* werden soll */
  if (WM_main (count, new_argv)) {
    /* Clean up after ourselves - remove tmporary file */
     
    if (pcl_temp && !strcmp (pcl_temp, "/dev/null"))
      unlink (pcl_temp);
    exit (0);
  }
}

/*
  Call proof tool with desired options.
  proof is an external program that is either found via $PATH or
  at the same place that Waldmeister lives
*/
static void
call_proof (int detail_flag, int redirect_output, char * output_file,
            pcl_output_type pcl_output, char *pcl_temp)
{
   char *new_argv[12];
   char *new_path;
   char *last_sep;
   int new_argc = 0;
   
   if (!detail_flag) {
      new_argv [++new_argc] = "-nosubst";
      new_argv [++new_argc] = "-noplace";
   }
   if (redirect_output) {
      new_argv [++new_argc] = "-o";
      new_argv [++new_argc] = output_file;
   }

  switch (pcl_output) {
  case pcl_tex:
     new_argv [++new_argc] = "-latex";
     break;
  case pcl_pcl:
  case pcl_no:
     break;
  case pcl_prolog:
     new_argv [++new_argc] = "-prolog";
     break;
  case pcl_ascii:
     break;
  }
  new_argv [++new_argc] = "-nobrackets";
  new_argv [++new_argc] = "-delete";
  new_argv [++new_argc] = pcl_temp;
  new_argv [++new_argc] = NULL;

  printf ("\nCalling proof presentation program...\n");
  /* fflush sollte ausgefuehrt werden, da sonst Ausgabeumlenkung nicht
     klappt :-( */
  fflush (stdout);
  fflush (stderr);

  new_path = our_alloc (strlen (wm_name) + 5);
  strcpy (new_path, wm_name);

  last_sep = strrchr (new_path, '/');
  if (last_sep) {
     *last_sep = '\0';
     strcat (last_sep, "/proof");
     new_argv [0] = new_path;
     execvp (new_path, new_argv);
  }
  
  new_argv[0] = "proof";
  execvp ("proof", new_argv);

  perror ("Execution of `proof' failed");
  fprintf (stderr, "Please check that `proof' is in the same directory as\n"
                   "the Waldmeister executable, or can be found via $PATH.\n");

  /* Clean up after ourselves - remove tmporary file */
  unlink (pcl_temp);
  /* Exit with failure */
  exit (1);
}


int
main (int argc,  char **argv)
{
  int            detail_flag = 0;
  int            redirect_output = 0;
  char           *output_file = NULL;
  int            use_db = 0;
  char           *db_file = NULL;
  int            use_auto = 0;
  pcl_output_type pcl_output = pcl_ascii;
  weight_type    weight = weight_mix;
  char           *input_file = NULL;
  char           *internal_options;
  size_t         len_internal_options;
  int 		 pos = 1;
  int            i;
  char           *arg_to_option;
  int            heuristic_set = 0;
  int            pcl_set = 0;
  char           *pcl_temp = NULL;
  char           *pcl_str;
  int            expert_pos = 0;

  wm_name = argv[0];
  
  while (pos < argc && expert_pos == 0) {
     if (argv[pos][0] != '-') {
        /* input file */
        if (input_file) {
           fprintf (stderr, "%s needs the file name of exactly one specification as argument.\n",
                    wm_name);
           usage (99);
        }
        input_file = argv[pos];
        ++pos;
        continue;
     }
     arg_to_option = NULL;
     for (i = 0; options[i].optart != OPT_END; i++) {
        if (strcmp (options[i].flag, argv[pos]) == 0) {
           if (options[i].args == REQUIRES_ARG) {
              if (pos + 1 < argc) {
                 arg_to_option = argv[pos + 1];
                 ++pos;
                 break;
              } else {
                 fprintf (stderr, "Option %s needs a parameter\n", argv[pos]);
                 usage (99);
              }
           }
           break;
        }
        /* Allow --db=filename or -o=filename */
        if (options[i].args == REQUIRES_ARG) {
           if ((strncmp (options[i].flag, argv[pos], strlen (options[i].flag)) == 0)
               && (argv[pos][strlen(options[i].flag)] == '='))
              {
                 arg_to_option = &argv[pos][strlen(options[i].flag) + 1];
                 break;
              }
        }
     }
     if (options[i].optart == OPT_END) {
        fprintf (stderr, "Unknown option %s\n", argv[pos]);
        usage (99);
     }
     switch (options[i].optart)
        {
        case OPT_ASCII:
           if (pcl_set) {
              usage (2);
           }
           pcl_set = 1;
           pcl_output = pcl_ascii;
           break;
           
	case OPT_ADD:
           if (heuristic_set) {
              usage (1);
           }
           heuristic_set = 1;
           weight = weight_add;
           break;
           
	case OPT_DB:
           if (use_db) {
              usage (3);
           }
           use_db = 1;
           db_file = arg_to_option;
           break;

        case OPT_EXT_HELP:
           manual ();
           break;

        case OPT_HELP:
           usage (0);
           break;
           
	case OPT_AUTO:
           if (use_auto) {
              usage (6);
           }
           use_auto = 1;
           break;

	case OPT_MIX:
           if (heuristic_set) {
              usage (1);
           }
           heuristic_set = 1;
           weight = weight_mix;
           break;
           
        case OPT_PCL_NO:
           if (pcl_set) {
              usage (2);
           }
           pcl_set = 1;
           pcl_output = pcl_no;
           break;
           
        case OPT_OUTPUT:
           if (redirect_output) {
              usage (4);
           }
           redirect_output = 1;
           output_file = arg_to_option;
           break;
           
	case OPT_PROLOG:
           if (pcl_set) {
              usage (2);
           }
           pcl_set = 1;
           pcl_output = pcl_prolog;
           break;
           
	case OPT_PCL:
           if (pcl_set) {
              usage (2);
           }
           pcl_set = 1;
           pcl_output = pcl_pcl;
	  break;
          
	case OPT_TEX:
           if (pcl_set) {
              usage (2);
           }
           pcl_set = 1;
           pcl_output = pcl_tex;
           break;
           
        case OPT_VERSION:
           print_version ();
           exit (0);
           break;
           
        case OPT_DETAILS:
           detail_flag = 1;
           break;

        case OPT_EXPERT:
           if (!input_file){
              fprintf (stderr, "%s needs the file name of one specification as argument before --expert.\n",
	       wm_name);
              usage (99);
           }
           expert_pos = pos + 1; /* next argument is first expert option */
           break;
           
        case OPT_END: /* to keep gcc happy */
           break;
           
        default:
           abort ();
           break;
        }
     ++pos;
  }

  /* Check for one file argument */
  if (!input_file)
    {
      fprintf (stderr, "%s needs the file name of one specification as argument.\n",
	       wm_name);
      usage(99);
    }
  if (use_auto && heuristic_set)
    usage (7);
  /* check that all files exists and are readable */
  check_file (input_file);
  if (use_db)
    check_file (db_file);
  
  /* Create temporary filename */
  if (pcl_output != pcl_no) {
     if (pcl_output != pcl_pcl) {
        pcl_temp = tempnam ("/tmp", "wm");
        if (!pcl_temp) {
           perror ("tempnam failed");
           exit (1);
        }
     } else if (redirect_output) { /* impliziert pcl_output == pcl_pcl */
        pcl_temp = output_file;
     }
  }
  
  
  /* Let's construct our own internal options */
  /* start with a hopefully high enough initial value */
  len_internal_options = 1024;
  internal_options = our_alloc (len_internal_options);
  internal_options[0] = '\0';

  if ((pcl_output != pcl_no)
      && ((pcl_output != pcl_pcl) || redirect_output)) {
    pcl_str = our_alloc (strlen (pcl_temp) + strlen ("-pcl On -pclOutput ") + 1);
    strcpy (pcl_str, "-pcl On -pclOutput ");
    strcat (pcl_str, pcl_temp);
    my_strcat (&internal_options, pcl_str, &len_internal_options);
    our_dealloc (pcl_str);
  }
  
  if (use_db) {
    my_strcat (&internal_options, "-db", &len_internal_options);
    my_strcat (&internal_options, db_file, &len_internal_options);
  }

  if (use_auto) {
    my_strcat (&internal_options,
               "-auto",
	       &len_internal_options);
  } else {
    switch (weight) {
    case weight_add:
      my_strcat (&internal_options, "-clas heuristic=addweight",
                 &len_internal_options);
      break;
    case weight_mix: /* default */
      my_strcat (&internal_options, "-clas heuristic=mixweight",
                 &len_internal_options);
      break;
    }
    my_strcat (&internal_options,
               "-zb -:continue:rnf"
               " -kg -:r:e:s"
               " -symbnorm off",
	       &len_internal_options);
  }
  /* append expert options without further processing */
  if (expert_pos) {
     for (pos = expert_pos; pos < argc; pos++)
        my_strcat (&internal_options, argv [pos], &len_internal_options);
  }
  

          
  /* Finally the input file */
  /* This has to be the very last parameter due to the old WM interface (see FileMenue.c)! */
  my_strcat (&internal_options, input_file, &len_internal_options);

  call_wm (internal_options, pcl_temp);

  if (pcl_output != pcl_pcl && pcl_output != pcl_no) {
     call_proof (detail_flag, redirect_output, output_file, pcl_output,
                 pcl_temp);
  }
  
  exit (0);
}

#endif
