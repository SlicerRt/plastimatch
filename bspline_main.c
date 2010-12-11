/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
/* -------------------------------------------------------------------------
    REFS:
    http://en.wikipedia.org/wiki/B-spline
    http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/surface/bspline-construct.html
    http://graphics.idav.ucdavis.edu/education/CAGDNotes/Quadratic-B-Spline-Surface-Refinement/Quadratic-B-Spline-Surface-Refinement.html
    ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bspline.h"
#if defined (commentout)
#include "bspline_landmarks.h"
#include "bspline_rbf.h"
#endif
#if defined (HAVE_F2C_LIBRARY)
#include "bspline_optimize_lbfgsb.h"
#endif
#include "bspline_opts.h"
#if (CUDA_FOUND)
#include "bspline_cuda.h"
#include "cuda_util.h"
#include "delayload.h"
#endif
#include "mha_io.h"
#include "vf.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

int
main (int argc, char* argv[])
{
    BSPLINE_Options options;
    Bspline_parms *parms = &options.parms;
    Bspline_xform *bxf;
    Volume *moving, *fixed, *moving_grad;
    Volume *vector_field = 0;
    Volume *moving_warped = 0;
    int roi_offset[3];

    bspline_opts_parse_args (&options, argc, argv);

#if (CUDA_FOUND)
    if (parms->threading == BTHR_CUDA) {
        if (!delayload_cuda()) { exit(0); }
        LOAD_LIBRARY (libplmcuda);
        LOAD_SYMBOL (CUDA_selectgpu, libplmcuda);
        CUDA_selectgpu (parms->gpuid);
        UNLOAD_LIBRARY (libplmcuda);
    }
#endif

    fixed = read_mha (options.fixed_fn);
    if (!fixed) exit (-1);
    moving = read_mha (options.moving_fn);
    if (!moving) exit (-1);

    volume_convert_to_float (moving);
    volume_convert_to_float (fixed);

    printf ("Making gradient\n");
    moving_grad = volume_make_gradient (moving);

#if defined (commentout)
    /* Load and adjust landmarks */
    if (options.fixed_landmarks && options.moving_landmarks) {
	parms->landmarks = bspline_landmarks_load (
	    options.fixed_landmarks, options.moving_landmarks);
	bspline_landmarks_adjust (parms->landmarks, fixed, moving);
    }
#endif

    /* Debug */
    //write_mha ("moving_grad.mha", moving_grad);

    /* Allocate memory and build lookup tables */
    printf ("Allocating lookup tables\n");
    memset (roi_offset, 0, 3*sizeof(int));
    if (options.input_xf_fn) {
	bxf = read_bxf (options.input_xf_fn);
    } else {
	bxf = (Bspline_xform*) malloc (sizeof (Bspline_xform));
	bspline_xform_initialize (
	    bxf,
	    fixed->offset,
	    fixed->pix_spacing,
	    fixed->dim,
	    roi_offset,
	    fixed->dim,
	    options.vox_per_rgn
	);
    }

    /* Run the optimization */
    printf ("Running optimization.\n");
    bspline_run_optimization (bxf, 0, parms, fixed, moving, moving_grad);
    printf ("Done running optimization.\n");

    /* Save output transform */
    if (options.output_xf_fn) {
	write_bxf (options.output_xf_fn, bxf);
    }

    /* Create vector field from bspline coefficients and save */
    if (options.output_vf_fn 
	|| options.output_warped_fn 
#if defined (commentout)
	|| (options.warped_landmarks && options.fixed_landmarks 
	    && options.moving_landmarks)
#endif
    )
    {
	printf ("Creating vector field.\n");
	vector_field = volume_create (fixed->dim, fixed->offset, 
	    fixed->pix_spacing,
	    PT_VF_FLOAT_INTERLEAVED, 
	    fixed->direction_cosines, 0);
	if (parms->threading == BTHR_CUDA) {
#if (CUDA_FOUND)
	    LOAD_LIBRARY (libplmcuda);
	    LOAD_SYMBOL (CUDA_bspline_interpolate_vf, libplmcuda);
	    CUDA_bspline_interpolate_vf (vector_field, bxf);
	    UNLOAD_LIBRARY (libplmcuda);
#else
	    bspline_interpolate_vf (vector_field, bxf);
#endif
	} else {
	    bspline_interpolate_vf (vector_field, bxf);
	}
    }

    /* Create warped output image and save */
    if (options.output_warped_fn) {
	printf ("Warping image.\n");
	moving_warped = vf_warp (0, moving, vector_field);
	if (moving_warped) {
	    printf ("Writing warped image.\n");
	    write_mha (options.output_warped_fn, moving_warped);
	} else {
	    printf ("Sorry, couldn't create warped image.\n");
	}
    }

    /* Write the vector field */
    if (options.output_vf_fn) {
	printf ("Writing vector field.\n");
	write_mha (options.output_vf_fn, vector_field);
    }

    /* Free memory */
    printf ("Done warping images.\n");
    bspline_parms_free (parms);
    bspline_xform_free (bxf);
    free (bxf);
    volume_destroy (fixed);
    volume_destroy (moving);
    volume_destroy (moving_grad);
    volume_destroy (moving_warped);
    volume_destroy (vector_field);

    printf ("Done freeing memory\n");

    return 0;
}
