/*=================================================================================================================
  =                                                                                                               =
  =  Achtzigstel.h                                                                                                =
  =                                                                                                               =
  =  Eine kleine Muenchhausiade.                                                                                  =
  =                                                                                                               =
  =  08.02.99 Thomas.                                                                                             =
  =================================================================================================================*/

#ifndef ACHTZIGSTEL_H
#define ACHTZIGSTEL_H


#include "Ausgaben.h"

#if IO_AUSGABEN

extern unsigned int AZ_d,
                    AZ_N,
                    AZ_Div,
                    AZ_Mod;
#define AZ_S 80
#define AZ_80 "Eightieths"

#define AZ_DruckeMeldungStart(/*char **/ whom, /*char **/ strength, /*unsigned long int*/ toGo)                 \
{                                                                                                               \
  AZ_d = 0;                                                                                                     \
  if ((AZ_N = toGo)) {                                                                                          \
    AZ_Div = AZ_S / AZ_N,                                                                                       \
    AZ_Mod = AZ_S % AZ_N;                                                                                       \
  }                                                                                                             \
  if (IO_level2() || IO_level3() || IO_level4())                                                                      \
    printf("reprocessing %s (%s, %ld to go)%s\n", whom, strength, toGo, AZ_N ? ". "AZ_80":" : "");   \
}

#define AZ_DruckeMeldungEnde(/*void*/)                                                                          \
{                                                                                                               \
  if (IO_level2() || IO_level3() || IO_level4())                                                                      \
    printf("\nreprocessing finished\n");                                                             \
}

#define AZ_DruckeMeldungItem(/*void*/)                                                                          \
{                                                                                                               \
  if (IO_level2() || IO_level3() || IO_level4()) {                                                                    \
    unsigned int s;                                                                                             \
    s = AZ_Div + ((AZ_d+=AZ_Mod)>=AZ_N ? (AZ_d-=AZ_N,1) : 0);                                                   \
    if (s) {                                                                                                    \
      do                                                                                                        \
        putc('.',stdout);                                                                                     \
      while (--s);                                                                                              \
      fflush(stdout);                                                                                         \
    }                                                                                                           \
  }                                                                                                             \
} 



#else

#define AZ_DruckeMeldungStart(whom,strength,nr)
#define AZ_DruckeMeldungEnde()
#define AZ_DruckeMeldungItem()

#endif

#endif
