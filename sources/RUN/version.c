/**********************************************************************
* Filename : version.c
*
* Kurzbeschreibung : Versionsidentifizierung
*
* Autoren : Andreas Jaeger
**********************************************************************/


#include "compile.h"
#include "version.h"

#define WM_RELEASE	"July 99"

const char *wm_release = WM_RELEASE;

const char *wm_banner = 
	"WaldmeisterII (" WM_RELEASE ") (" WM_COMPILE_BY "@"
	WM_COMPILE_HOST "." WM_COMPILE_DOMAIN ") (" WM_COMPILER ") ("WM_COMPILE_DATE") (" BUILD_VERSION ")" ;

const char *configuration_banner =
        "WaldmeisterII Version "WM_RELEASE " ("BUILD_VERSION ") " WM_COMPILE_DATE
        " compiled by " WM_COMPILE_BY "@" WM_COMPILE_HOST
        " with "WM_COMPILER "\n";




