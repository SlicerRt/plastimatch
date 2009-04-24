/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _demons_h_
#define _demons_h_

#include "volume.h"

typedef struct DEMONS_Parms_struct DEMONS_Parms;
struct DEMONS_Parms_struct {
    float denominator_eps;
    float homog;
    float accel;
    int filter_width[3];
    int max_its;
    float filter_std;
};

#if defined __cplusplus
extern "C" {
#endif
void demons_default_parms (DEMONS_Parms* parms);
Volume* demons (Volume* fixed, Volume* moving, Volume* moving_grad, Volume* vf_init, char* method, DEMONS_Parms* parms);
Volume* demons_c (Volume* fixed, Volume* moving, Volume* moving_grad, Volume* vf_init, DEMONS_Parms* parms);
Volume* demons_brook (Volume* fixed, Volume* moving, Volume* moving_grad, Volume* vf_init, DEMONS_Parms* parms);
#if defined __cplusplus
}
#endif

#endif
