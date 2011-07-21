/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _pcmd_compose_h_
#define _pcmd_compose_h_

#include "plm_config.h"
#include <stdlib.h>
#include <string.h>
#include "plm_image_type.h"
#include "pstring.h"

class Compose_parms {
public:
    Pstring xf_in_1_fn;
    Pstring xf_in_2_fn;
    Pstring xf_out_fn;
    bool negate_mask;
    float mask_value;
    bool output_dicom;
    Plm_image_type output_type;
public:
    Compose_parms () {
	negate_mask = false;
	mask_value = 0.;
	output_dicom = false;
	output_type = PLM_IMG_TYPE_UNDEFINED;
    }
};

void do_command_compose (int argc, char *argv[]);

#endif
