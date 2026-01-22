/* 
   Diese Datei kapselt das Suchen nach Dateinamen.

   Die folgenden Endungen der Dateien sind fest einkodiert:
   Spezifikationsdateien: pr,cp,cf,cv,tm,rd

   Neugeschrieben von Andreas Jaeger, 18.9.98
   */

#include "antiwarnings.h"
#include "Ausgaben.h"
#include "general.h"
#include "FileMenue.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
# define PATH_MAX _POSIX_PATH_MAX
#endif

#if defined (SUNOS4)
#define SPEZ_PFAD "/usr/progress/indexing/Specifications"
#else
#define SPEZ_PFAD "/home/indexing/Specifications"
#endif
#define TPTP_PFAD "TPTP250.WMdir"

/* ----------------------------------------------------------------------------------------------------- */
/* ------------------------ Hilfsprozeduren ------------------------------------------------------------ */
/* ----------------------------------------------------------------------------------------------------- */

#if !COMP_POWER && !COMP_COMP
static BOOLEAN IsWMSpecification(const char* String)
{
   char *letzterPunkt = strrchr(String, '.');

   if (!letzterPunkt) return FALSE;
   
   return (strcmp (letzterPunkt, ".pr") == 0)
      || (strcmp (letzterPunkt, ".cp") == 0)
      || (strcmp (letzterPunkt, ".cf") == 0)
      || (strcmp (letzterPunkt, ".cv") == 0)
      || (strcmp (letzterPunkt, ".tm") == 0)
      || (strcmp (letzterPunkt, ".rd") == 0);
}

static BOOLEAN IsTPTPSpecification (const char *name)
{
   int len = strlen (name);
   BOOLEAN result;
   
   result = (len == 11 || len == 12);
   result = result &&
      (isupper ((int)*(name++)) && isupper ((int)*(name++)) && isupper((int)*(name++))
       && isdigit ((int)*(name++)) && isdigit ((int)*(name++)) && isdigit ((int)*(name++))
       && (*(name++) == '-')
       && isdigit ((int)*(name++)));
   if (len == 12)
      result = result && isdigit ((int)*(name++));
   return result && (strcmp (name, ".pr") == 0);
}


static BOOLEAN IsWMdir(const char* String)
{
   char *letzterPunkt = strrchr(String, '.');

   if (!letzterPunkt) return FALSE;
   
   return (strcmp (letzterPunkt, ".WMdir") == 0);
}

static char *RekursiveSuche (char *aktueller_pfad, char *name)
{
   DIR *directory;
   struct dirent *dir_eintrag;
   struct stat stat_buf;
   char *gefunden = NULL;
   char kompl_pfad[PATH_MAX]; /* vollstaendiger Pfad, Zwischenspeicher */
   
   directory = opendir (aktueller_pfad);
   if (directory == NULL)
      /* Fehler aufgetreten */
      return NULL;
   
   while ((dir_eintrag = readdir (directory)) != NULL) {
      if ((strcmp (dir_eintrag->d_name, ".") != 0)
          && strcmp (dir_eintrag->d_name, "..") != 0) {
         strcpy (kompl_pfad, aktueller_pfad);
         strcat (kompl_pfad, "/");
         strcat (kompl_pfad, dir_eintrag->d_name);
         if (strcmp (dir_eintrag->d_name, name) == 0) {
            gefunden = our_strdup (kompl_pfad);
            break;
         }
         /* Directories mit .WMdir am Ende rekursiv durchsuchen */
         if (IsWMdir (dir_eintrag->d_name)
	     && (stat (kompl_pfad, &stat_buf) == 0)
             && S_ISDIR(stat_buf.st_mode)) {
            gefunden = RekursiveSuche (kompl_pfad, name);
            if (gefunden)
               break;
         }
      }
   }
   closedir (directory);
   return gefunden;
}
#endif /* !COMP_POWER && !COMP_COMP */

static char *SearchFileName (char *name)
{
   /* Falls Datei existiert, dann braucht nicht gesucht werden */
   if (access (name, R_OK) == 0)
      return name;
#if COMP_POWER || COMP_COMP
   return NULL;
#else   
   /* Enthaelt Datei '/'?  Wenn nein, dann Abbruch */
   if (strchr (name, '/'))
      return NULL;

   if (IsTPTPSpecification (name)) {
      char pfad[PATH_MAX];
      
      strcpy (pfad, SPEZ_PFAD "/" TPTP_PFAD "/");
      strncat (pfad, name, 3);
      strcat (pfad, ".WMdir/");
      strcat (pfad, name);
      if (access (pfad, R_OK) == 0)
         return our_strdup (pfad);
   }
   
   /* rekursiv den Dateibaum nach diesem Namen suchen */
   return RekursiveSuche (SPEZ_PFAD, name);
#endif
}

/****************************************************************
  Exportierte Routinen
  ***************************************************************/

char *FM_SpezifikationErkannt(int argc, char *argv[])
/* String fn zu komplettem Dateinamen machen.
   1. Als Datei suchen
   2. In Beispielverzeichnissen suchen
   Rueckgabewert ist der komplette Pfad oder NULL, falls Suche erfolglos war.
*/
{
   char *fn;
   int i;

#if COMP_POWER || COMP_COMP
   i = argc-1;                                    /* Beim Power-Waldmeister ist der Dateiname das letzte Argument. */
#else
   for (i=1; i<argc; i++)                                                    /* Filenamen in Argument-Liste suchen */
     if (argv[i] && IsWMSpecification(argv[i]))
       break;
   if (i >= argc)
     our_error ("No specification specified!");
#endif
   fn = argv[i];                 /* Wenn kein Spezifikationsname gefunden, ist i=argcs und folglich argv[i]==NULL. */
   argv[i] = NULL;

   return SearchFileName(fn);
}
