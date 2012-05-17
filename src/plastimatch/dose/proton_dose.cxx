/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information

   Algorithm c (modified Hong algorithm)
   -------------------------------------
   Stage 1(a): Compute pencil beam in standard grid
   (z direction is pdd in water)
   (x-y direction is scatter in water, or just 1-d with distance)

   Stage 1(b): Compute RPL in interpolated coordinate system
   (z axis is beam axis)
   (x-y perpendicular to beam, arbitrary v-up vector)

   Stage 2: For each voxel
   a) Look up primary in RPL grid
   b) Convolve to find scatter within x-y axis of primary grid (ignoring tilt)
   ----------------------------------------------------------------------- */
#include "plmdose_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plmbase.h"
#include "plmdose.h"
#include "plmsys.h"

#include "threading.h"
#include "plm_math.h"

#define VERBOSE 1
#define PROGRESS 1
//#define DEBUG_VOXEL 1
//#define DOSE_GAUSS 1

Proton_dose_parms::Proton_dose_parms ()
{
    this->aperture = new Aperture;
    this->beam = new Proton_Beam;

    this->threading = THREADING_CPU_OPENMP;
    this->flavor = 'a';

    this->debug = 0;
    this->detail = 0;
    this->ray_step = 1.0f;
    this->scale = 1.0f;
    this->input_fn[0] = '\0';
    this->output_fn[0] = '\0';
}

/* This function is used to rotate a point about a ray in an orbit
 * perpendicular to the ray.  It is assumed that the arbitrary axis of
 * rotation (ray) originates at the Cartesian origin.
 */
static void
rotate_about_ray (
    double *xyz_new,    /* rotated point */
    double *xyz,        /* point to rotate */
    double t,           /* angle of rotation */
    double *ray         /* axis of rotation */
)
{
    double u[3];
    double v[3];
    double w[3];
    double tmp[3] = {0.0, 0.0, 1.0};

    double M[12];

#if defined (commentout)
    double M[16];
#endif

    /* Generate coordinate system */
    vec3_copy (w, ray);
    vec3_normalize1 (w);
    vec3_cross (v, w, tmp);
    vec3_normalize1 (v);
    vec3_cross (u, v, w);

    /* Build the composite matrix
     *   -- Axis rotation: W coincident Z
     *   -- Rotates about Z by theta radians
     *   -- Undoes axis rotation (Z -> W)
     */
    M[4*0 + 0] = u[0]*u[0]*cos(t) + u[0]*v[0]*sin(t) - u[0]*v[0]*sin(t) + v[0]*v[0]*cos(t) + w[0]*w[0];
    M[4*0 + 1] = u[0]*u[1]*cos(t) + u[0]*v[1]*sin(t) - u[1]*v[0]*sin(t) + v[0]*v[1]*cos(t) + w[0]*w[1];
    M[4*0 + 2] = u[0]*u[2]*cos(t) + u[0]*v[2]*sin(t) - u[2]*v[0]*sin(t) + v[0]*v[2]*cos(t) + w[0]*w[2];
    M[4*0 + 3] = 0;

    M[4*1 + 0] = u[1]*u[0]*cos(t) + u[1]*v[0]*sin(t) - u[0]*v[1]*sin(t) + v[1]*v[0]*cos(t) + w[1]*w[0];
    M[4*1 + 1] = u[1]*u[1]*cos(t) + u[1]*v[1]*sin(t) - u[1]*v[1]*sin(t) + v[1]*v[1]*cos(t) + w[1]*w[1]; 
    M[4*1 + 2] = u[1]*u[2]*cos(t) + u[1]*v[2]*sin(t) - u[2]*v[1]*sin(t) + v[1]*v[2]*cos(t) + w[1]*w[2]; 
    M[4*1 + 3] = 0;

    M[4*2 + 0] = u[2]*u[0]*cos(t) + u[2]*v[0]*sin(t) - u[0]*v[2]*sin(t) + v[2]*v[0]*cos(t) + w[2]*w[0];
    M[4*2 + 1] = u[2]*u[1]*cos(t) + u[2]*v[1]*sin(t) - u[1]*v[2]*sin(t) + v[2]*v[1]*cos(t) + w[2]*w[1];
    M[4*2 + 2] = u[2]*u[2]*cos(t) + u[2]*v[2]*sin(t) - u[2]*v[2]*sin(t) + v[2]*v[2]*cos(t) + w[2]*w[2];
    M[4*2 + 3] = 0;

#if defined (commentout)
    M[4*3 + 0] = 0;
    M[4*3 + 1] = 0;
    M[4*3 + 2] = 0;
    M[4*3 + 3] = 1;
#endif

    /* Apply rotation transform */
    mat43_mult_vec3(xyz_new, M, xyz);
}

static inline void
display_progress (
    float is,
    float of
) 
{
#if defined (PROGRESS)
    printf (" [%3i%%]\b\b\b\b\b\b\b",
           (int)floorf((is/of)*100.0f));
    fflush (stdout);
#endif
}

#if defined (DEBUG_VOXEL)
static void 
debug_voxel (
    double r,             /* current radius */
    double t,             /* current angle */
    double rgdepth,       /* radiographic depth */
    double d,             /* dose from scatter_xyz */
    double* scatter_xyz,  /* space coordinates of scatterer */
    double* ct_xyz,       /* voxel receiving dose */
    double w,             /* gaussian weight (x) */
    double d0,            /* unweighted d */
    double sigma,         /* Gauss kernel stdev */
    double dose           /* aggr dose @ ct_xyz */
)
{
    FILE* fp;

    fp = fopen ("dump_voxel.txt", "a");

    fprintf (fp, "r,t: [%3.3f %1.3f] rgdepth: %2.3f d: %4.1f dist: %3.3f w: %1.4f d0: %4.3f sigma: %1.4f dose: %4.4f\n",
             r, t, rgdepth, d, vec3_dist(scatter_xyz, ct_xyz), w, d0, sigma, dose);

    fclose (fp);
}
#endif

/* This computes the Highland scattering radius due to Coulombic interactions.
 * 
 * This is valid only for an "infinitely thick" medium such as the patient.  A
 * different approximation is used to find the radial scattering in thin
 * degraders.
 */
static double
highland (
    double rgdepth,
    Proton_Beam* beam
)
{
#if defined (commentout)
    float rad_length = 1.0;     /* Radiation length of material (g/cm2) */
    float density    = 1.0;     /* Density of material (g/cm3)          */
    float p          = 0.0;     /* Proton momentum (passed in)          */
    float v          = 0.0;     /* Proton velocity (passed in)          */
    float sum        = 0.0;
    float i, tmp;

    for (i=0; i<rgdepth; i+=rgdepth/1000.0f) {
        tmp = ((rgdepth - i) / (p*v));
        sum += (tmp * tmp) * (density / rad_length) * (rgdepth/1000.0f);

        /* Update p and v here */
    }

    sum = sqrtf(sum);
//    printf ("%f\n", 14.1 * (1 + (1/9) * log10(rgdepth/rad_length)) * sum);

    return 14.1 * (1 + (1/9) * log10(rgdepth/rad_length)) * sum; 
#endif

    /* This is just a normalization I used to use instead
     * of the Highland approximation */
    return 3.0 * (rgdepth - beam->d_lut[0]) / (beam->dmax - beam->d_lut[0]);
}

static double
gaus_kernel (
    double* p,
    double* ct_xyz,
    double sigma
)
{
    double w1, w2;
    double denom;
    double sigma2;

    // kludge to prevent evaluations 
    // greater than 1
    if (sigma < 0.4) {
        sigma = 0.4;
    }

    sigma2 = sigma * sigma;

    denom = 2.0f * M_PI * sigma2;
    denom = sqrt (denom);
    denom = 1.0f / denom;

    w1 = denom * exp ( (-1.0*p[0]*p[0]) / (2.0f*sigma2) );
    w2 = denom * exp ( (-1.0*p[1]*p[1]) / (2.0f*sigma2) );

    return w1 * w2;
}


static float
lookup_energy (
    float depth,
    Proton_Beam* beam
)
{
    int i;
    float energy = 0.0f;

    /* Sanity check */
    if (depth < 0 || depth > beam->dmax) {
        return 0.0f;
    }

    /* Find index into profile arrays */
    for (i = 0; i < beam->num_samples; i++) {
        if (beam->d_lut[i] > depth) {
            i--;
            break;
        } else if (beam->d_lut[i] == depth) {
            return beam->e_lut[i];
        }
    }

    /* Use index to lookup and interpolate energy */
    if (i >= 0 || i < beam->num_samples) {
        // linear interpolation
        energy = beam->e_lut[i]
                 + (depth - beam->d_lut[i])
                 * ((beam->e_lut[i+1] - beam->e_lut[i]) 
                 / (beam->d_lut[i+1] - beam->d_lut[i]));
    } else {
        // this should never happen, failsafe
        energy = 0.0f;
    }

    return energy;   
}

/* This function should probably be marked for deletion once dose_scatter() is
 * working properly.  GCS: This funcion is useful for debugging.  Let's keep
 * it as flavor 'a'.
 */
static double
dose_direct (
    double* ct_xyz,             /* voxel to dose */
    Rpl_volume *rpl_vol,        /* radiographic depths */
    Proton_Beam* beam,          /* proton energy profile */
    int* ires,                  /* ray cast resolution */
    double* ap_ul,              /* aperture upper-left */
    double* incr_r,             /* ray row to row vector */
    double* incr_c,             /* ray col to col vector */
    Proton_dose_parms *parms
)
{
    double rgdepth;
    double dose;

    rgdepth = rpl_volume_get_rgdepth (
	rpl_vol,        /* volume of radiological path lengths */
	ct_xyz          /* voxel to find depth */
    );

    /* The voxel was not hit directly by the beam */
    if (rgdepth < 0.0f) {
        return 0.0f;
    }

    if (ct_xyz[1] > 0.0 && ct_xyz[1] < 2.0 
	&& ct_xyz[2] > 0.0 && ct_xyz[2] < 2.0) {
	printf ("(%f %f %f) %f\n", ct_xyz[0], ct_xyz[1], ct_xyz[2], 
	    rgdepth);
    }
#if defined (commentout)
#endif

    /* Lookup the dose at this radiographic depth */
    dose = lookup_energy (rgdepth, beam);
    
    return dose;
}

static double
dose_debug (
    double* ct_xyz,             /* voxel to dose */
    Rpl_volume *rpl_vol,        /* radiographic depths */
    Proton_Beam* beam,          /* proton energy profile */
    int* ires,                  /* ray cast resolution */
    double* ap_ul,              /* aperture upper-left */
    double* incr_r,             /* ray row to row vector */
    double* incr_c,             /* ray col to col vector */
    Proton_dose_parms *parms
)
{
    return rpl_volume_get_rgdepth (rpl_vol, ct_xyz);
}

/* Accounts for small angle scattering due to Columbic interactions */
static double
dose_scatter (
    double* ct_xyz,
    int* ct_ijk,            // DEBUG
    Rpl_volume *rpl_vol, 
    Proton_Beam* beam,
    int* ires,
    double* ap_ul,
    double* incr_r,
    double* incr_c,
    double* prt,
    double* pdn,
    Proton_dose_parms *parms
)
{
    double rgdepth;
    double sigma;

    double r, t;
    double r_step, t_step;
    double r_max;


    double sp_pos[3] = {0.0, 0.0, 0.0};
    double scatter_xyz[4] = {0.0, 0.0, 0.0, 1.0};
    double proj_xy[2] = {0.0, 0.0};
    double sctoct[3] = {0.0, 0.0, 0.0};
    double tmp[3] = {0.0, 0.0, 0.0};


    double d = 0.0f;
    double dose = 0.0f;
    double w;

    int debug = 0;

#if defined (DEBUG_VOXEL)
    double d0;

    //int watch_ijk[3] = {0, 255, 67};  // entry
    //int watch_ijk[3] = {134, 256, 67};  // bragg peak
    //int watch_ijk[3] = {23, 255, 67};    // "stem"

    int watch_ijk[3] = {20, 19, 19};

    if (ct_ijk[0] == watch_ijk[0] &&
        ct_ijk[1] == watch_ijk[1] &&
        ct_ijk[2] == watch_ijk[2]) {

        printf ("Watching voxel [%i %i %i]\n", watch_ijk[0], watch_ijk[1], watch_ijk[2]);
        debug = 1;
    }
#endif 

    /* Get approximation for scatterer search radius
     * NOTE: This is not used to define the Gaussian
     */
    rgdepth = rpl_volume_get_rgdepth (rpl_vol, ct_xyz);

    if (debug) {
//        printf ("rgdepth = %f\n", rgdepth);
    }

    /* If the voxel was not hit *directly* by the beam, there is still a
     * chance that it was hit by scatterers generated by a neighbor who *was*
     * * hit directly by the beam.  As a result, we cannot obtain a resonable
     * estimate, so we assume the largest scattering radius.
     */
    if (rgdepth < 0.0) {
        if (parms->detail == 0) {
            rgdepth = beam->dmax;
        }
        else if (parms->detail == 1) {
            /* User wants to ignore "scatter only" dose */
            if (debug) {
//                printf ("Voxel culled by detail flag\n");
            }
            return 0.0f;
        }
        else {
            rgdepth = beam->dmax;
        }
    }

    sigma = highland (rgdepth, beam);
    r_max = 3.0*sigma;

    r_step = 1.00;          // mm
    t_step = M_PI / 8.0f;   // radians

    if (debug) {
        printf ("sigma = %f\n", sigma);
        printf ("r_max = %f\n", r_max);
        printf ("r_step = %f\n", r_step);
        printf ("t_step = %f\n", t_step);
    }

    /* Step radius */
    for (r = 0; r < r_max; r += r_step) {
        vec3_copy (sp_pos, ct_xyz);
        vec3_scale3 (tmp, pdn, r);
        vec3_add2 (sp_pos, tmp);

        /* Step angle */
        for (t = 0.0f; t < 2.0*M_PI; t += t_step) {

            rotate_about_ray (
                scatter_xyz,  // O: new xyz coordinate
                sp_pos,       // I: init xyz coordinate
                t,            // I: angle of rotation
                ct_xyz);      // I: axis of rotation

            /* neighbor (or self) hit by proton beam? */
            rgdepth = rpl_volume_get_rgdepth (rpl_vol, scatter_xyz);

            if (rgdepth < 0.0f) {
                if (debug) {
                    printf ("Voxel culled by rgdepth\n");
                }
                continue;
            } else {
                d = lookup_energy (rgdepth, beam);
#if defined (DEBUG_VOXEL)
                d0 = d;
#endif
            }

            vec3_sub3 (sctoct, scatter_xyz, ct_xyz);

            proj_xy[0] = vec3_dot (sctoct, prt);
            proj_xy[1] = vec3_dot (sctoct, pdn);

            sigma = highland (rgdepth, beam);

            /* weight by gaussian kernel */
            w = gaus_kernel (proj_xy, ct_xyz, sigma);
            d *= w;

            /* Add to total dose for our target voxel */
            dose += d;

#if defined (DEBUG_VOXEL)
            if (debug) {
                debug_voxel (r, t, rgdepth, d, scatter_xyz, ct_xyz,
                             w, d0, sigma, dose);
            }
#endif

            /* Don't spin at the origin! */
            if (r == 0) {
                break;
            }

        }
    }

    return dose;    
}

static double
dose_hong (
    double* ct_xyz,
    int* ct_ijk,            // DEBUG
    Rpl_volume *rpl_vol, 
    Proton_Beam* beam,
    int* ires,
    double* ap_ul,
    double* incr_r,
    double* incr_c,
    double* prt,
    double* pdn,
    Proton_dose_parms *parms
)
{
    double rgdepth;
    double sigma;

    double r, t;
    double r_step, t_step;
    double r_max;

    double sp_pos[3] = {0.0, 0.0, 0.0};
    double scatter_xyz[4] = {0.0, 0.0, 0.0, 1.0};
    double proj_xy[2] = {0.0, 0.0};
    double sctoct[3] = {0.0, 0.0, 0.0};
    double tmp[3] = {0.0, 0.0, 0.0};

    double d = 0.0f;
    double dose = 0.0f;
    double w;

    int debug = 0;

#if defined (DEBUG_VOXEL)
    double d0;

    int watch_ijk[3] = {20, 19, 19};

    if (ct_ijk[0] == watch_ijk[0] &&
        ct_ijk[1] == watch_ijk[1] &&
        ct_ijk[2] == watch_ijk[2]) {

        printf ("Watching voxel [%i %i %i]\n", 
	    watch_ijk[0], watch_ijk[1], watch_ijk[2]);
        debug = 1;
    }
#endif 

    /* Get approximation for scatterer search radius
     * NOTE: This is not used to define the Gaussian
     */
    rgdepth = rpl_volume_get_rgdepth (rpl_vol, ct_xyz);

    if (debug) {
        printf ("rgdepth = %f\n", rgdepth);
    }

    /* If the voxel was not hit *directly* by the beam, there is still a
     * chance that it was hit by scatterers generated by a neighbor who
     * *was* * hit directly by the beam.  As a result, we cannot obtain
     * a resonable estimate, so we assume the largest scattering radius.
     */
    if (rgdepth < 0.0) {
        if (parms->detail == 0) {
            rgdepth = beam->dmax;
        }
        else if (parms->detail == 1) {
            /* User wants to ignore "scatter only" dose */
            if (debug) {
                printf ("Voxel culled by detail flag\n");
            }
            return 0.0f;
        }
        else {
            rgdepth = beam->dmax;
        }
    }

    sigma = highland (rgdepth, beam);
    r_max = 3.0*sigma;

    r_step = 1.00;          // mm
    t_step = M_PI / 8.0f;   // radians

    if (debug) {
        printf ("sigma = %f\n", sigma);
        printf ("r_max = %f\n", r_max);
        printf ("r_step = %f\n", r_step);
        printf ("t_step = %f\n", t_step);
    }

    /* Step radius */
    for (r = 0; r < r_max; r += r_step) {
        vec3_copy (sp_pos, ct_xyz);
        vec3_scale3 (tmp, pdn, r);
        vec3_add2 (sp_pos, tmp);

        /* Step angle */
        for (t = 0.0f; t < 2.0*M_PI; t += t_step) {

        rotate_about_ray (
                scatter_xyz,  // O: new xyz coordinate
                sp_pos,       // I: init xyz coordinate
                t,            // I: angle of rotation
                ct_xyz);      // I: axis of rotation

        /* neighbor (or self) hit by proton beam? */
        rgdepth = rpl_volume_get_rgdepth (rpl_vol, scatter_xyz);

            if (rgdepth < 0.0f) {
                if (debug) {
                    printf ("Voxel culled by rgdepth\n");
                }
                continue;
            } else {
                d = lookup_energy (rgdepth, beam);
#if defined (DEBUG_VOXEL)
                d0 = d;
#endif
            }

            vec3_sub3 (sctoct, scatter_xyz, ct_xyz);

            proj_xy[0] = vec3_dot (sctoct, prt);
            proj_xy[1] = vec3_dot (sctoct, pdn);

            sigma = highland (rgdepth, beam);

            /* weight by gaussian kernel */
            w = gaus_kernel (proj_xy, ct_xyz, sigma);
            d *= w;

            /* Add to total dose for our target voxel */
            dose += d;

#if defined (DEBUG_VOXEL)
            if (debug) {
                debug_voxel (r, t, rgdepth, d, scatter_xyz, ct_xyz,
                             w, d0, sigma, dose);
            }
#endif

            /* Don't spin at the origin! */
            if (r == 0) {
                break;
            }

        }
    }

    return dose;    

}

static void
dump_beam (Proton_Beam* beam)
{
    FILE* fp;

    int i;

    fp = fopen ("dump_beam.txt", "w");

    for (i = 0; i < beam->num_samples; i++) {
       fprintf (fp, "[%3.2f] %3.2f\n", beam->d_lut[i], beam->e_lut[i]);
    }

    fprintf (fp, "    dmax: %3.2f\n", beam->dmax);
    fprintf (fp, "num_samp: %i\n", beam->num_samples);

    fclose (fp);
}

void
proton_dose_compute (
    Volume *dose_vol,
    Volume *ct_vol,
    Proton_dose_parms *parms
)
{
    Proj_matrix *pmat;
    Rpl_volume* rpl_vol;

    int ct_ijk[3];
    double ct_xyz[4];
    double p1[3];
    double tmp[3];       

    Aperture* ap = parms->aperture;
    Proton_Beam* beam = parms->beam;
    float* dose_img = (float*) dose_vol->img;

    int idx = 0;

    double ps[2] = { 1., 1. };

    pmat = new Proj_matrix;
    proj_matrix_set (
            pmat,
            beam->src,
            beam->isocenter,
            ap->vup,
            ap->ap_offset,
            ap->ic,
            ps,
            ap->ires
    );

    proj_matrix_get_nrm (pmat, ap->nrm);
    proj_matrix_get_pdn (pmat, ap->pdn);
    proj_matrix_get_prt (pmat, ap->prt);

    /* Compute position of aperture in room coordinates */
    vec3_scale3 (tmp, ap->nrm, - pmat->sid);
    vec3_add3 (ap->ic_room, pmat->cam, tmp);

    /* Compute incremental change in 3d position for each change 
       in aperture row/column. */
    vec3_scale3 (ap->incr_c, ap->prt, ps[1]);
    vec3_scale3 (ap->incr_r, ap->pdn, ps[0]);

    /* Get position of upper left pixel on panel */
    vec3_copy (ap->ul_room, ap->ic_room);
    vec3_scale3 (tmp, ap->incr_r, - pmat->ic[0]);
    vec3_add2 (ap->ul_room, tmp);
    vec3_scale3 (tmp, ap->incr_c, - pmat->ic[1]);
    vec3_add2 (ap->ul_room, tmp);

    /* drr_trace_ray uses p1 & p2, p1 is the camera, p2 is in the 
       direction of the ray inside the aperture plane*/
    vec3_copy (p1, pmat->cam);

#if VERBOSE
    printf ("\n");
    printf ("PROTON SOURCE\n");
    printf ("  -- [POS] Location: %g %g %g\n", pmat->cam[0], pmat->cam[1], pmat->cam[2]);
    printf ("APERATURE\n");
    printf ("  -- [NUM] Res   : %i %i\n", ap->ires[0], ap->ires[1]);
    printf ("  -- [DIS] Offset: %g\n", ap->ap_offset);
    printf ("  -- [POS] Center: %g %g %g\n", ap->ic_room[0], ap->ic_room[1], ap->ic_room[2]);
    printf ("  -- [POS] UpLeft: %g %g %g\n", ap->ul_room[0], ap->ul_room[1], ap->ul_room[2]);
    printf ("  -- [VEC] Up    : %g %g %g\n", ap->vup[0], ap->vup[1], ap->vup[2]);
    printf ("  -- [VEC] Normal: %g %g %g\n", ap->nrm[0], ap->nrm[1], ap->nrm[2]);
    printf ("  -- [VEC] Right : %g %g %g\n", ap->prt[0], ap->prt[1], ap->prt[2]);
    printf ("  -- [VEC] Down  : %g %g %g\n", ap->pdn[0], ap->pdn[1], ap->pdn[2]);
    printf ("  -- [VEC] col++ : %g %g %g\n", ap->incr_c[0], ap->incr_c[1], ap->incr_c[2]);
    printf ("  -- [VEC] row++ : %g %g %g\n", ap->incr_r[0], ap->incr_r[1], ap->incr_r[2]);
#endif

    /* Create the depth volume */
    rpl_vol = rpl_volume_create (
            ct_vol,             /* CT volume */
            pmat,               /* from source to aperature  */
            ap->ires,           /* aperature dimension       */
            pmat->cam,          /* position of source        */
            ap->ul_room,        /* position of aperature     */
            ap->incr_r,         /* aperature row++ vector    */
            ap->incr_c,         /* aperature col++ vector    */
            parms->ray_step     /* step size along ray trace */
    );

    /* Scan through aperture to fill in rpl_volume */
    rpl_volume_compute (rpl_vol, ct_vol);

    if (parms->debug) {
        rpl_volume_save (rpl_vol, "depth_vol.mha");
        dump_beam (beam);
        proj_matrix_debug (pmat);
    }

    /* Scan through CT Volume */
    for (ct_ijk[2] = 0; ct_ijk[2] < ct_vol->dim[2]; ct_ijk[2]++) {
        for (ct_ijk[1] = 0; ct_ijk[1] < ct_vol->dim[1]; ct_ijk[1]++) {
            for (ct_ijk[0] = 0; ct_ijk[0] < ct_vol->dim[0]; ct_ijk[0]++) {
                double dose = 0.0;

                /* Transform vol index into space coords */
                ct_xyz[0] = (double) (ct_vol->offset[0] + ct_ijk[0] * ct_vol->spacing[0]);
                ct_xyz[1] = (double) (ct_vol->offset[1] + ct_ijk[1] * ct_vol->spacing[1]);
                ct_xyz[2] = (double) (ct_vol->offset[2] + ct_ijk[2] * ct_vol->spacing[2]);
                ct_xyz[3] = (double) 1.0;

                switch (parms->flavor) {
                case 'a':
                    dose = dose_direct (
                            ct_xyz,         /* voxel to dose */
                            rpl_vol,        /* radiographic depths */
                            beam,           /* proton energy profile */
                            ap->ires,       /* ray cast resolution */
                            ap->ul_room,    /* aperture upper-left */
                            ap->incr_r,     /* 3D ray row increment */
                            ap->incr_c,     /* 3D ray col increment */
                            parms);         /* parms->ray_step */
                    break;
                case 'b':
                    dose = dose_scatter (
                            ct_xyz,        /* voxel to dose */
                            ct_ijk,        /* index of voxel */
                            rpl_vol,       /* radiographic depths */
                            beam,          /* proton energy profile */
                            ap->ires,      /* ray cast resolution */
                            ap->ul_room,   /* aperture upper-left */
                            ap->incr_r,    /* 3D ray row increment */
                            ap->incr_c,    /* 3D ray col increment */
                            ap->prt,       /* x-dir in ap plane uv */
                            ap->pdn,       /* y-dir in ap plane uv */
                            parms);        /* parms->ray_step */
                    break;
                case 'c':
                    dose = dose_hong (
                            ct_xyz,        /* voxel to dose */
                            ct_ijk,        /* index of voxel */
                            rpl_vol,       /* radiographic depths */
                            beam,          /* proton energy profile */
                            ap->ires,      /* ray cast resolution */
                            ap->ul_room,   /* aperture upper-left */
                            ap->incr_r,    /* 3D ray row increment */
                            ap->incr_c,    /* 3D ray col increment */
                            ap->prt,       /* x-dir in ap plane uv */
                            ap->pdn,       /* y-dir in ap plane uv */
                            parms);        /* parms->ray_step */
                    break;
                case 'd':
                    dose = dose_debug (
                            ct_xyz,         /* voxel to dose */
                            rpl_vol,        /* radiographic depths */
                            beam,           /* proton energy profile */
                            ap->ires,       /* ray cast resolution */
                            ap->ul_room,    /* aperture upper-left */
                            ap->incr_r,     /* 3D ray row increment */
                            ap->incr_c,     /* 3D ray col increment */
                            parms);         /* parms->ray_step */
                    break;
                }

                /* Insert the dose into the dose volume */
                idx = INDEX_OF (ct_ijk, dose_vol->dim);
                dose_img[idx] = dose;

            }
        }
        display_progress ((float)idx, (float)ct_vol->npix);
    }

    delete beam;
    delete pmat;
    rpl_volume_destroy (rpl_vol);
}
