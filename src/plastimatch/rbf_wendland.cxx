/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vnl/vnl_matrix_fixed.h"
#include "vnl/vnl_matrix.h"
#include "vnl/vnl_vector.h"
#include "vnl/vnl_vector_fixed.h"
#include "vnl/algo/vnl_svd.h"
#include "vnl/vnl_sample.h"

#include "bspline.h"
#include "bspline_landmarks.h"
#include "bspline_opts.h"
#include "landmark_warp.h"
#include "logfile.h"
#include "math_util.h"
#include "print_and_exit.h"
#include "rbf_wendland.h"
#include "vf.h"
#include "volume.h"
#include "volume_macros.h"
#include "rbf_cluster.h"

typedef struct rbf_params Rbf_parms;
struct rbf_params { // used to pass information to bspline_rbf_score
    float radius;    // radius of the RBF
    Bspline_parms *bparms;
    Volume *vector_field;
};

static float 
rbf_wendland_value (float *rbf_center, float *loc, float radius)
{
    float val, r, dx, dy, dz;

    dx = rbf_center[0]-loc[0];
    dy = rbf_center[1]-loc[1];
    dz = rbf_center[2]-loc[2];
    r = sqrt (dx*dx + dy*dy + dz*dz);
    r = r / radius;

    if (r>1) return 0.;
    val = (1-r)*(1-r)*(1-r)*(1-r)*(4*r+1.); // Wendland
    return val;
}

// find RBF coeff by solving the linear equations using ITK's SVD routine
// Output:
// parms->blm->rbf_coeff contains RBF coefficients
static void 
bspline_rbf_find_coeffs_noreg (
    float *coeff,                  /* Output */
    Landmark_warp *lw              /* Input */
)
{
    float rbfv;
    int i, j, d;
    int num_landmarks = lw->m_fixed_landmarks->num_points;

    typedef vnl_matrix <double> Vnl_matrix;
    typedef vnl_svd <double> SVDSolverType;
    Vnl_matrix A, b;

    //printf("Finding RBF coeffs, radius %.2f, Young modulus %e\n", 
	//lw->rbf_radius, lw->young_modulus);

    A.set_size (3 * num_landmarks, 3 * num_landmarks);
    A.fill(0.);

    b.set_size (3 * num_landmarks, 1);
    b.fill (0.0);

    // right-hand side
    for(i=0;i<num_landmarks;i++) {
	for(d=0;d<3;d++) {
	    b (3*i +d, 0) = 
		-(lw->m_fixed_landmarks->points[3*i+d] 
		    - lw->m_moving_landmarks->points[3*i+d]);
	}
    }

    // matrix
    for(i=0;i<num_landmarks;i++) {
	for(j=0;j<num_landmarks;j++) {

    	    float rbf_center[3];
	    for (d=0; d<3; d++) {
	        rbf_center[d] = lw->m_fixed_landmarks->points[3*j+d];
	    }

	    rbfv = rbf_wendland_value( rbf_center, 
		    &lw->m_fixed_landmarks->points[3*i], 
		    lw->adapt_radius[j] );

	    for(d=0;d<3;d++) A(3*i+d, 3*j+d) = rbfv ;

	}
    }

    //    A.print (std::cout);
    //    b.print (std::cout);

    SVDSolverType svd (A, 1e-6);
    Vnl_matrix x = svd.solve (b);

    //    x.print (std::cout);

    for (i=0; i<3*num_landmarks; i++) {
	coeff[i] = x(i,0);
    }

}

// find RBF coeff by solving the linear equations using ITK's SVD routine
// Output:
// parms->blm->rbf_coeff contains RBF coefficients
static void 
bspline_rbf_find_coeffs (
    float *coeff,                   /* Output */
    Landmark_warp *lw               /* Input */
)
{
// Regularization for Wendland RBF not yet implemented
//    bspline_rbf_find_coeffs_reg (coeff, lw);
    bspline_rbf_find_coeffs_noreg( coeff, lw);

    int i;
    for (i=0; i < lw->m_fixed_landmarks->num_points; i++) {
	printf("coeff %4d  %.4f %.4f %.4f\n",  i,
	    coeff[3*i+0],
	    coeff[3*i+1],
	    coeff[3*i+2]);
    }

}

/*
Adds RBF contributions to the vector field
landmark_dxyz is not updated by this function
Version without truncation: scan over the entire vf
and add up all RBFs in each voxel
*/
void
rbf_wendland_update_vf (
    Volume *vf,                  /* Modified */
    Landmark_warp *lw,           /* Input */
    float *coeff                 /* Input */
)
{
    int lidx, d, v;
    int ijk[3];
    float fxyz[3];
    float *vf_img;
    float rbf;
    int num_landmarks = lw->m_fixed_landmarks->num_points;

    printf("Wendland RBF, updating the vector field\n");

    if (vf->pix_type != PT_VF_FLOAT_INTERLEAVED )
	print_and_exit("Sorry, this type of vector field is not supported\n");

    vf_img = (float*) vf->img;

    for (v = 0, LOOP_Z (ijk, fxyz, vf)) {
	for (LOOP_Y (ijk, fxyz, vf)) {
	    for (LOOP_X (ijk, fxyz, vf), v++) {

		for (lidx=0; lidx < num_landmarks; lidx++) {
			
		    rbf = rbf_wendland_value (
			&lw->m_fixed_landmarks->points[3*lidx], 
			fxyz, 
			lw->adapt_radius[lidx]);

		    for (d=0; d<3; d++) {
			vf_img[3*v+d] += coeff[3*lidx+d] * rbf;
#if defined (commentout)
			printf ("Adding: %d (%d %d %d) (%g * %g) %g\n", 
			    lidx, 
			    ijk[0], ijk[1], ijk[2],
			    coeff[3*lidx+d], rbf, 
			    coeff[3*lidx+d] * rbf);
#endif
		    }
		}
	    }
	}
    }
}

void
rbf_wendland_warp (Landmark_warp *lw)
{
    float *coeff;
    float origin[3], spacing[3];
    int dim[3];
	float direction_cosines[9];
    int i;
    Volume *moving, *vf_out, *warped_out;

    //printf ("Wendland Radial basis functions requested, radius %.2f\n", lw->rbf_radius);

    lw->adapt_radius = (float *)malloc(lw->m_fixed_landmarks->num_points * sizeof(float));
    lw->cluster_id = (int *)malloc(lw->m_fixed_landmarks->num_points * sizeof(int));

    if (lw->num_clusters > 0) {
    rbf_cluster_kmeans_plusplus( lw ); // cluster the landmarks; result in lw->cluster_id
    rbf_cluster_find_adapt_radius( lw ); // using cluster_id, fill in lw->adapt_radius
    }
    else { // use the specified radius
	for(i = 0; i < lw->m_fixed_landmarks->num_points; i++) 
		lw->adapt_radius[i]=lw->rbf_radius;
    }


	for(i = 0; i < lw->m_fixed_landmarks->num_points; i++){ 
	lw->adapt_radius[i]*=2;
	printf("%f\n", lw->adapt_radius[i]);
	}


	/* Solve for RBF weights */
    coeff = (float*) malloc (
	3 * lw->m_fixed_landmarks->num_points * sizeof(float));
    bspline_rbf_find_coeffs (coeff, lw);

    /* Create output vector field */
    printf ("Creating output vf\n");
    lw->m_pih.get_origin (origin);
    lw->m_pih.get_spacing (spacing);
    lw->m_pih.get_dim (dim);
	lw->m_pih.get_direction_cosines (direction_cosines);
    vf_out = volume_create (dim, origin, spacing, direction_cosines, 
	PT_VF_FLOAT_INTERLEAVED, 3, 0);

    printf ("Rendering vector field\n");
    rbf_wendland_update_vf (vf_out, lw, coeff);

    /* Create output (warped) image */
    printf ("Converting volume to float\n");
    moving = lw->m_input_img->gpuit_float ();

    printf ("Creating output vol\n");
    warped_out = volume_create (dim, origin, spacing, direction_cosines, 
	PT_FLOAT, 1, 0);

    printf ("Warping image\n");
    vf_warp (warped_out, moving, vf_out);

    printf ("Freeing coeff\n");
    free (coeff);

    /* Copy outputs to lw structure */
    lw->m_vf = new Xform;
    lw->m_vf->set_gpuit_vf (vf_out);
    lw->m_warped_img = new Plm_image;
    lw->m_warped_img->set_gpuit (warped_out);

    printf ("Done with rbf_wendland_warp\n");
}
