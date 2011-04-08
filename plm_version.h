/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _plm_version_h_
#define _plm_version_h_

#include "plm_config.h"

#if defined (PLM_CONFIG_VERSION_STRING)
  #define PLASTIMATCH_VERSION PLM_CONFIG_VERSION_STRING
#else
  #define PLASTIMATCH_VERSION "1.4-beta"
#endif

#if (PLASTIMATCH_HAVE_BUILD_NUMBER)
  #define PLASTIMATCH_VERSION_STRING  PLASTIMATCH_VERSION " (" PLASTIMATCH_BUILD_NUMBER ")"
#else
  #define PLASTIMATCH_VERSION_STRING  PLASTIMATCH_VERSION
#endif

#endif
