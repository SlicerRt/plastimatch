/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _tps_interp_h_
#define _tps_interp_h_

#include "plm_config.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImage.h"
#include "itk_image.h"
#include "itk_warp.h"
#include "itkThinPlateSplineKernelTransform.h"
#include "itkPoint.h"
#include "itkPointSet.h"
#include "xform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUFLEN 2048

typedef struct tps_parms TPS_parms;
struct tps_parms {
    char* reference;
	char* target;
	char* original;
	char* warped;
	char* vf;
};

template<class T>
void do_tps(TPS_parms* parms,typename itk::Image<T,3>::Pointer img_in,T);
#endif
