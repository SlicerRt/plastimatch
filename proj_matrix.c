/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mathutil.h"
#include "MGHMtx_opts.h"
#include "proj_matrix.h"
#include "volume.h"

void set_image_parms (MGHMtx_Options * options);

#define DRR_PLANE_RAY_TOLERANCE 1e-8
#define DRR_STRIDE_TOLERANCE 1e-10
#define DRR_HUGE_DOUBLE 1e10
#define DRR_LEN_TOLERANCE 1e-6
#define DRR_TOPLANE_TOLERANCE 1e-7

#define MSD_NUM_BINS 60
#define LINELEN 128

// #define ULTRA_VERBOSE 1
// #define VERBOSE 1

#define PREPROCESS_ATTENUATION 0
#define IMGTYPE float

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#ifndef M_TWOPI
#define M_TWOPI         (M_PI * 2.0)
#endif

/* -----------------------------------------------------------------------
   Private functions
   ----------------------------------------------------------------------- */
/* -----------------------------------------------------------------------
   Public functions
   ----------------------------------------------------------------------- */
void
proj_matrix_init (Proj_matrix *pmat)
{
    memset (pmat, 0, sizeof(Proj_matrix));
}

Proj_matrix*
proj_matrix_create (void)
{
    Proj_matrix *pmat;
    
    pmat = (Proj_matrix*) malloc (sizeof(Proj_matrix));
    if (!pmat) return 0;

    proj_matrix_init (pmat);

    return pmat;
}

void
proj_matrix_destroy (Proj_matrix* pmat)
{
    free (pmat);
}

void
proj_matrix_save (
    Proj_matrix *pmat,
    const char *fn
)
{
    FILE *fp;

    fp = fopen (fn, "w");
    if (!fp) {
	fprintf (stderr, "Error opening %s for write\n", fn);
	exit (-1);
    }
    fprintf (fp, "%18.8e %18.8e\n", pmat->ic[0], pmat->ic[1]);
    fprintf (fp,
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n", 
	pmat->matrix[0], pmat->matrix[1], pmat->matrix[2], pmat->matrix[3],
	pmat->matrix[4], pmat->matrix[5], pmat->matrix[6], pmat->matrix[7],
	pmat->matrix[8], pmat->matrix[9], pmat->matrix[10], pmat->matrix[11]
    );
    fprintf (fp, "%18.8e\n%18.8e\n", pmat->sad, pmat->sid);

    /* NRM */
    //fprintf (fp, "%18.8e %18.8e %18.8e\n", nrm[0], nrm[1], nrm[2]);
    fprintf (fp, "%18.8e %18.8e %18.8e\n", pmat->extrinsic[8], 
	pmat->extrinsic[9], pmat->extrinsic[10]);

    fprintf (fp,
	"Extrinsic\n"
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n", 
	pmat->extrinsic[0], pmat->extrinsic[1], pmat->extrinsic[2], 
	pmat->extrinsic[3], pmat->extrinsic[4], pmat->extrinsic[5], 
	pmat->extrinsic[6], pmat->extrinsic[7], pmat->extrinsic[8], 
	pmat->extrinsic[9], pmat->extrinsic[10], pmat->extrinsic[11],
	pmat->extrinsic[12], pmat->extrinsic[13], pmat->extrinsic[14], 
	pmat->extrinsic[15]
    );
    fprintf (fp,
	"Intrinsic\n"
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n" 
	"%18.8e %18.8e %18.8e %18.8e\n", 
	pmat->intrinsic[0], pmat->intrinsic[1], 
	pmat->intrinsic[2], pmat->intrinsic[3],
	pmat->intrinsic[4], pmat->intrinsic[5], 
	pmat->intrinsic[6], pmat->intrinsic[7],
	pmat->intrinsic[8], pmat->intrinsic[9], 
	pmat->intrinsic[10], pmat->intrinsic[11]
    );
    fclose (fp);
}

void
proj_matrix_set (
    Proj_matrix *pmat,
    double* cam, 
    double* tgt, 
    double* vup, 
    double sid, 
    double* ic, 
    double* ps, 
    int* ires
)
{
    const int cols = 4;
    double nrm[3];       /* Panel normal */
    double prt[3];       /* Panel right (toward first column) */
    double pup[3];       /* Panel up (toward top row) */

    pmat->sid = sid;
    pmat->sad = vec3_len (cam);
    pmat->ic[0] = ic[0];
    pmat->ic[1] = ic[1];

    /* Compute imager coordinate sys (nrm,pup,prt) 
       ---------------
       nrm = cam - tgt
       prt = nrm x vup
       pup = prt x nrm
       ---------------
    */
    vec3_sub3 (nrm, cam, tgt);
    vec3_normalize1 (nrm);
    vec3_cross (prt, nrm, vup);
    vec3_normalize1 (prt);
    vec3_cross (pup, prt, nrm);
    vec3_normalize1 (pup);

#if defined (commentout)
    printf ("CAM = %g %g %g\n", cam[0], cam[1], cam[2]);
    printf ("TGT = %g %g %g\n", tgt[0], tgt[1], tgt[2]);
    printf ("NRM = %g %g %g\n", nrm[0], nrm[1], nrm[2]);
    printf ("PRT = %g %g %g\n", prt[0], prt[1], prt[2]);
    printf ("PUP = %g %g %g\n", pup[0], pup[1], pup[2]);
#endif

    /* Build extrinsic matrix - rotation part */
    vec_zero (pmat->extrinsic, 16);
    vec3_copy (&pmat->extrinsic[0], prt);
    vec3_copy (&pmat->extrinsic[4], pup);
    vec3_copy (&pmat->extrinsic[8], nrm);
    vec3_invert (&pmat->extrinsic[0]);
    vec3_invert (&pmat->extrinsic[4]);
    vec3_invert (&pmat->extrinsic[8]);
    m_idx (pmat->extrinsic,cols,3,3) = 1.0;

    /* Build extrinsic matrix - translation part */
    pmat->extrinsic[3] = vec3_dot (prt, tgt);
    pmat->extrinsic[7] = vec3_dot (pup, tgt);
    pmat->extrinsic[11] = vec3_dot (nrm, tgt) + pmat->sad;

#if defined (commentout)
    printf ("EXTRINSIC\n%g %g %g %g\n%g %g %g %g\n"
	"%g %g %g %g\n%g %g %g %g\n",
	pmat->extrinsic[0], pmat->extrinsic[1], 
	pmat->extrinsic[2], pmat->extrinsic[3],
	pmat->extrinsic[4], pmat->extrinsic[5], 
	pmat->extrinsic[6], pmat->extrinsic[7],
	pmat->extrinsic[8], pmat->extrinsic[9], 
	pmat->extrinsic[10], pmat->extrinsic[11],
	pmat->extrinsic[12], pmat->extrinsic[13], 
	pmat->extrinsic[14], pmat->extrinsic[15]);
#endif

    /* Build intrinsic matrix */
    vec_zero (pmat->intrinsic, 12);
    m_idx (pmat->intrinsic,cols,0,0) = 1 / ps[0];
    m_idx (pmat->intrinsic,cols,1,1) = 1 / ps[1];
    m_idx (pmat->intrinsic,cols,2,2) = 1 / sid;

#if defined (commentout)
    printf ("INTRINSIC\n%g %g %g %g\n%g %g %g %g\n%g %g %g %g\n",
	pmat->intrinsic[0], pmat->intrinsic[1], 
	pmat->intrinsic[2], pmat->intrinsic[3],
	pmat->intrinsic[4], pmat->intrinsic[5], 
	pmat->intrinsic[6], pmat->intrinsic[7],
	pmat->intrinsic[8], pmat->intrinsic[9], 
	pmat->intrinsic[10], pmat->intrinsic[11]);
#endif

    /* Build projection matrix */
    mat_mult_mat (pmat->matrix, pmat->intrinsic,3,4, pmat->extrinsic,4,4);
}

#if defined (commentout)
void
proj_matrix_write (double* cam, 
		   double* tgt, double* vup,
		   double sid, double* ic,
		   double* ps, int* ires,
		   int varian_mode, 
		   char* out_fn)
{
    double extrinsic[16];
    double intrinsic[12];
    double projection[12];
    const int cols = 4;
    double sad;

    double nrm[3];
    double vrt[3];
    double vup_tmp[3];  /* Don't overwrite vup */

    FILE* fp;

    vec_zero (extrinsic, 16);
    vec_zero (intrinsic, 12);

    /* Compute image coordinate sys (nrm,vup,vrt) relative to room coords.
       ---------------
       nrm = tgt - cam
       vrt = nrm x vup
       vup = vrt x nrm
       ---------------
    */
    vec3_sub3 (nrm, tgt, cam);
    vec3_normalize1 (nrm);
    vec3_cross (vrt, nrm, vup);
    vec3_normalize1 (vrt);
    vec3_cross (vup_tmp, vrt, nrm);
    vec3_normalize1 (vup_tmp);

    /* !!! But change nrm here to -nrm */
    vec3_scale2 (nrm, -1.0);

    /* Build extrinsic matrix */
    if (varian_mode) {
	vec3_scale2 (vrt, -1.0);
	vec3_copy (&extrinsic[0], nrm);
	vec3_copy (&extrinsic[4], vup_tmp);
	vec3_copy (&extrinsic[8], vrt);
    } else {
	vec3_copy (&extrinsic[0], vrt);
	vec3_copy (&extrinsic[4], vup_tmp);
	vec3_copy (&extrinsic[8], nrm);
    }

    sad = vec3_len (cam);
    m_idx(extrinsic,cols,2,3) = - sad;
    m_idx(extrinsic,cols,3,3) = 1.0;

    /* Build intrinsic matrix */

    m_idx(intrinsic,cols,0,1) = - 1 / ps[0];
    m_idx(intrinsic,cols,1,0) = 1 / ps[1];
    m_idx(intrinsic,cols,2,2) = - 1 / sid;
    //    m_idx(intrinsic,cols,0,3) = ic[0];
    //    m_idx(intrinsic,cols,1,3) = ic[1];

    mat_mult_mat (projection, intrinsic,3,4, extrinsic,4,4);

#if defined (VERBOSE)
    printf ("Extrinsic:\n");
    matrix_print_eol (stdout, extrinsic, 4, 4);
    printf ("Intrinsic:\n");
    matrix_print_eol (stdout, intrinsic, 3, 4);
    printf ("Projection:\n");
    matrix_print_eol (stdout, projection, 3, 4);
#endif

    fp = fopen (out_fn, "w");
    if (!fp) {
	fprintf (stderr, "Error opening %s for write\n", out_fn);
	exit (-1);
    }
    fprintf (fp, "%18.8e %18.8e\n", ic[0], ic[1]);
    fprintf (fp,
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n", 
	     projection[0], projection[1], projection[2], projection[3],
	     projection[4], projection[5], projection[6], projection[7],
	     projection[8], projection[9], projection[10], projection[11]
	    );
    fprintf (fp, "%18.8e\n%18.8e\n", sad, sid);
    fprintf (fp, "%18.8e %18.8e %18.8e\n", nrm[0], nrm[1], nrm[2]);
    fprintf (fp,
	     "Extrinsic\n"
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n"
	     "%18.8e %18.8e %18.8e %18.8e\n", 
	     extrinsic[0], extrinsic[1], extrinsic[2], extrinsic[3],
	     extrinsic[4], extrinsic[5], extrinsic[6], extrinsic[7],
	     extrinsic[8], extrinsic[9], extrinsic[10], extrinsic[11],
	     extrinsic[12], extrinsic[13], extrinsic[14], extrinsic[15]
	    );
    fprintf (fp,
	     "Intrinsic\n"
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n" 
	     "%18.8e %18.8e %18.8e %18.8e\n", 
	     intrinsic[0], intrinsic[1], intrinsic[2], intrinsic[3],
	     intrinsic[4], intrinsic[5], intrinsic[6], intrinsic[7],
	     intrinsic[8], intrinsic[9], intrinsic[10], intrinsic[11]
	    );
    fclose (fp);
}
#endif

int
read_ProjAngle(char *ProjAngle_file, float *ProjAngle)
{

    FILE *fp;
    char linebuf[LINELEN];
    int nProj=0;
    fp = fopen (ProjAngle_file,"rb");
    if (!fp) {
	fprintf (stderr, "File %s not found\n", ProjAngle_file);
	return 0;
    }
    while (fgets(linebuf,LINELEN,fp)) {
	sscanf (linebuf, "%f",&ProjAngle[nProj++]);
    }
    fclose(fp);
    return(nProj);
}

#if defined (commentout)
void
proj_matrix_write_varian_dir (MGHMtx_Options* options)
{
    int a;

    //    double cam_ap[3] = {0.0, -1.0, 0.0};
    //    double cam_lat[3] = {-1.0, 0.0, 0.0};
    //    double* cam = cam_ap;
    //    double* cam = cam_lat;

    double vup[3] = {0, 0, 1};
    double tgt[3] = {0.0, 0.0, 0.0};
    double nrm[3];
    double tmp[3];
    float ProjAngle[1000];
    int varian_mode = 1;

    /* Set source-to-axis distance */
    double sad = options->sad;

    /* Set source-to-image distance */
    double sid = options->sid;

    /* Set image resolution */
    int ires[2] = { options->image_resolution[0],
		    options->image_resolution[1] };

    /* Set physical size of imager in mm */
    //    int isize[2] = { 300, 400 };      /* Actual resolution */
    int isize[2] = { options->image_size[0],
		     options->image_size[1] };

    /* Set ic = image center (in pixels), and ps = pixel size (in mm)
       Note: pixels are numbered from 0 to ires-1 */
    double ic[2] = { options->image_center[0],
		     options->image_center[1] };

    /* Set pixel size in mm */
    double ps[2] = { (double)isize[0]/(double)ires[0], 
		     (double)isize[1]/(double)ires[1] };

    /* Loop through camera angles */
    //options->angle_diff=30.0f;
    int nProj=read_ProjAngle(options->ProjAngle_file,ProjAngle);

    //for (a = 0; a < options->num_angles; a++) {
    for (a = 0; a < nProj; a++) {
	double angle = ProjAngle[a];
	double cam[3];
	char out_fn[256];
	//char multispectral_fn[256];
	//angle=0;

	cam[0] = cos(angle/180.0*3.14159);
	cam[1] = sin(angle/180.0*3.14159);
	cam[2] = 0.0;

	//printf ("Rendering DRR %d\n", a);

	/* Place camera at distance "sad" from the volume isocenter */
	vec3_sub3 (nrm, tgt, cam);
	vec3_normalize1 (nrm);
	vec3_scale3 (tmp, nrm, sad);
	vec3_copy (cam, tgt);
	vec3_sub2 (cam, tmp);

	/* Some debugging info */
#if defined (VERBOSE)
	vec_set_fmt ("%12.4g ");
	printf ("cam: ");
	vec3_print_eol (stdout, cam);
	printf ("tgt: ");
	vec3_print_eol (stdout, tgt);
	printf ("ic:  %g %g\n", ic[0], ic[1]);
#endif
	sprintf (out_fn, "%s%04d.txt", options->output_prefix, a);
	proj_matrix_write (cam, tgt, vup, 
			   sid, ic, ps, ires, 
			   varian_mode, out_fn);
    }
}
#endif
