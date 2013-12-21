/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _process_parms_h_
#define _process_parms_h_

#include "plmregister_config.h"
#include <list>
#include <string>
#include <ctype.h>
#include <stdlib.h>

#include "smart_pointer.h"

class Process_parms_private;

class PLMREGISTER_API Process_parms {
public:
    SMART_POINTER_SUPPORT (Process_parms);
    Process_parms_private *d_ptr;
public:
    Process_parms ();
    Process_parms (const Process_parms& s);
    ~Process_parms ();
public:
    void set_action (const std::string& action);
    void set_parms (const std::string& parms);
    void set_images (const std::string& images);
};

#endif
