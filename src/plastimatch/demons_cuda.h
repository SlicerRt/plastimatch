/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _demons_cuda_h_
#define _demons_cuda_h_

#include "plm_config.h"

#include "demons.h"

class Demons_state;
class Volume;

plmcuda_EXPORT (
Volume* demons_cuda,
    Demons_state *demons_state,
    Volume* fixed,
    Volume* moving,
    Volume* moving_grad,
    Volume* vf_init,
    DEMONS_Parms* parms
);

#endif
