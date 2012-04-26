/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "plmsys_config.h"

#include "plmsys.h"

#if (PANTHEIOS_FOUND)
#include <pantheios/pantheios.h>        /* The root header for Panthieos when using the C-API. */
#include <pantheios/frontends/stock.h>  /* Declares the process identity symbol PANTHEIOS_FE_PROCESS_IDENTITY */
#include <pantheios/implicit_link/fe.simple.h>
#endif
#include "plm_version.h"

#define LOGFILE_ECHO_ON 1

#if (PANTHEIOS_FOUND)
const char PANTHEIOS_FE_PROCESS_IDENTITY[] = "plastimatch";

void
logfile_open (FILE** log_fp, char* log_fn)
{
    int res = pantheios_init();
    if (!res) {
        fprintf (stderr, 
	    "Failed to initialise the Pantheios libraries: %s\n", 
	    pantheios_getInitCodeString(res));
	return;
    }
}

void
logfile_printf (FILE* log_fp, char* fmt, ...)
{
//    pantheios_logprintf (PANTHEIOS_SEV_DEBUG, fmt, ...);
}

void
logfile_close (FILE** log_fp, char* log_fn)
{
    int res = pantheios_init();
    if (!res) {
        fprintf (stderr, 
	    "Failed to initialise the Pantheios libraries: %s\n", 
	    pantheios_getInitCodeString(res));
	return;
    }
}

#else

FILE* log_fp = 0;

void
logfile_open (char* log_fn)
{
    if (!log_fn[0]) return;
    if (!(log_fp)) {
	log_fp = fopen (log_fn, "w");
	if (!log_fp) {
	    /* If failure (e.g. bad path), do nothing */	
	}
    } else {
	/* Already open? */
    }
    logfile_printf ("Plastimatch " 
		     PLASTIMATCH_VERSION_STRING
		     "\n");
}

void
logfile_close (void)
{
    if (log_fp) {
	fclose (log_fp);
	log_fp = 0;
    }
}

void
logfile_printf (const char* fmt, ...)
{
    va_list argptr;
    va_start (argptr, fmt);

    /* Write to console */
    if (LOGFILE_ECHO_ON) {
	vprintf (fmt, argptr);
	fflush (stdout);
    }

    if (!log_fp) {
	va_end (argptr);
	return;
    }

    /* Write to file */
    vfprintf (log_fp, fmt, argptr);
    fflush (log_fp);

    va_end (argptr);
}
#endif
