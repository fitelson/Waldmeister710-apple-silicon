/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - InfoStrings.h                                                                                              */
/* - *************                                                                                              */
/* -                                                                                                            */
/* - Parameter-Uebergabe als String, in dem die einzelnen Daten durch ':' getrennt werden.                      */
/* -                                                                                                            */
/* - 14.03.96 Arnim. Angelegt.                                                                                  */
/* -                                                                                                            */
/* -                                                                                                            */
/* -------------------------------------------------------------------------------------------------------------*/

#include "InfoStrings.h"
#include "general.h"
#include "SpeicherVerwaltung.h"
#include <stdarg.h>
#include <string.h>
#include "compiler-extra.h"


/* erzeugt neue Kopie von start. Ende der Kopie ist Stringende von Start oder
   naechstes '=' oder ':' */
static char * get_value (char * start) 
{
   char * end = strpbrk (start, "=:");
   char * new_str;
   
   if (!end) {
      end = &start[strlen (start)];
   }
   
   new_str = our_alloc (end - start + 1);
   strncpy (new_str, start, (end - start));
   new_str[end - start] = '\0';

   return new_str;
   
}


/* Liefert val, falls is = ...:str=val:..., und NULL sonst. */
char *IS_ValueOf (const char *str, InfoStringT is) 
{
   size_t str_len = strlen (str);
   char * iStr;
   
   /* entweder steht str am Anfang, oder aber direkt nach einem Doppelpunkt */
   /* hinter str muss ein '=' kommen */
   
   if ((strncmp (is, str, str_len) == 0) && (is[str_len] == '=')) 
      return get_value (&is[str_len+1]);

   
   for (iStr = is; *iStr; iStr++) {
      if ((*iStr == ':') && (strncmp ( &iStr[1], str, str_len) == 0)
           && (iStr[str_len + 1] == '='))
         return get_value (&iStr[str_len + 2]);
   }
   
   return NULL;
}


/* str ist ein Teil von is, also is = ...:str:.. oder is = ...:str=...:...*/
BOOLEAN IS_IsIn (const char *str, InfoStringT is)
{
   size_t str_len = strlen (str);
   char * iStr;
   
   /* entweder steht str am Anfang, oder aber direkt nach einem Doppelpunkt */
   /* str muss mit '=', ':' aufhoeren oder am Stringende sein */

   if ((strncmp (is, str, str_len) == 0)
       && ((is[str_len] == '=') || (is[str_len] == ':')
           || (is[str_len] == '\0')))
      return TRUE;

   for (iStr=is; *iStr; iStr++) {
      if ((*iStr == ':') && (strncmp ( &iStr[1], str, str_len) == 0)
          /* + 1, da iStr ':' enthaelt */
          && ((iStr[str_len + 1] == '=') || (iStr[str_len + 1] == ':')
              || (iStr[str_len + 1] == '\0')))
         return TRUE;
   }
   
   return FALSE;
}


int IS_FindOption(InfoStringT defaultIS, InfoStringT userIS, int n, ...)
/* sucht zunaechst in userIS, dann in defaultIS nach den n angegebenen searchStrings.
   Die Nummer des gefundenen searchStrings wird zurueckgeliefert (beginnend bei 0).
*/
{
   va_list ap;
   char *strVal;
   int i;
   BOOLEAN found = FALSE;

   if (!(defaultIS || userIS)) return -1;

   va_start(ap, n);
   for (i=0; i<n; i++){
      strVal = va_arg(ap, char *);
      if (IS_IsIn(strVal, userIS)) {found = TRUE; break;}
   }
   va_end(ap);

   if (found) return i;

   va_start(ap, n);
   for (i=0; i<n; i++){
      strVal = va_arg(ap, char *);
      if (IS_IsIn(strVal, defaultIS)) {found = TRUE; break;}
   }
   va_end(ap);
   if (found) return i;
   else return -1;
}
   
int IS_FindValuedOption(InfoStringT defaultIS, InfoStringT userIS, int n, ...)
/* sucht zunaechst in userIS, dann in defaultIS nach den n angegebenen searchStrings.
   Die Nummer des gefundenen searchStrings wird zurueckgeliefert (beginnend bei 0).
*/
{
   va_list ap;
   char *strVal;
   int i;
   int found = 0;

   if (!(defaultIS || userIS)) return -1;

   va_start(ap, n);
   for (i=0; i<n; i++){
      strVal = va_arg(ap, char *);
      if (IS_ValueOf(strVal, userIS)) {found = 1; break;}
   }
   va_end(ap);

   if (found) return i;

   va_start(ap, n);
   for (i=0; i<n; i++){
      strVal = va_arg(ap, char *);
      if (IS_ValueOf(strVal, defaultIS)) {found = 1; break;}
   }
   va_end(ap);
   if (found) return i;
   else return -1;
}

char *IS_ValueOfOption(InfoStringT defaultIS, InfoStringT userIS, const char *str)
/* Gibt den Wert der zuvor mit IS_FindValuedOption identifizierten gueltigen Option zurueck. */
{
   char *retVal;
   
   if (!(defaultIS || userIS)) return NULL;

   retVal = IS_ValueOf(str, userIS);
   if (!retVal) return IS_ValueOf(str, defaultIS);
   else return retVal;
}
