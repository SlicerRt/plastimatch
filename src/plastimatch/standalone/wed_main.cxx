/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "aperture.h"
#include "plm_image.h"
#include "plm_math.h"
#include "proj_volume.h"
#include "proton_beam.h"
#include "proton_scene.h"
#include "ray_trace_probe.h"
#include "rpl_volume.h"
#include "volume.h"
#include "volume_limit.h"
#include "wed_parms.h"

typedef struct callback_data Callback_data;
struct callback_data {
    Volume* wed_vol;   /* Water equiv depth volume */
    int* ires;         /* Aperture Dimensions */
    int ap_idx;        /* Current Aperture Coord */
};


#if 0
static plm_long
get_wed_volume_depth (Wed_Parms* parms)
{
    Rpl_volume* rpl_vol = parms->scene->rpl_vol;
    float* rpl_img = (float*) rpl_vol->vol->img;


    plm_long N = rpl_vol->vol->dim[0]
               * rpl_vol->vol->dim[1]
               * rpl_vol->vol->dim[2];

    double largest = 0.0;
    for (int i=0; i<N; i++) {
        if (rpl_img[i] > largest) {
            largest = rpl_img[i];
        }
    }
    return (plm_long) floorf (largest+1);
}
#endif


static Volume*
create_wed_volume (Wed_Parms* parms, Proton_Scene *scene)
{
    Rpl_volume* rpl_vol = scene->rpl_vol;

    float wed_off[3] = {0.0f, 0.0f, 0.0f};
    float wed_ps[3] = {1.0f, 1.0f, 1.0f};

    /* water equivalent depth volume has the same x,y dimensions as the rpl
     * volume. Note: this means the wed x,y dimensions are equal to the
     * aperture dimensions and the z-dimension is equal to the sampling
     * resolution chosen for the rpl */
    plm_long wed_dims[3];
    Volume *vol = rpl_vol->get_volume ();
    wed_dims[0] = vol->dim[0];
    wed_dims[1] = vol->dim[1];
    wed_dims[2] = vol->dim[2];

    return new Volume (wed_dims, wed_off, wed_ps, NULL, PT_FLOAT, 1);
}


static void
wed_ray_trace_callback (
    void *callback_data, 
    size_t vox_index, 
    double vox_len, 
    float vox_value
)
{
    Callback_data *cd = (Callback_data *) callback_data;
    Volume *wed_vol = cd->wed_vol;
    float *wed_img = (float*) wed_vol->img;
    int ap_idx = cd->ap_idx;
    int ap_area = cd->ires[0] * cd->ires[1];
    int step_num = vox_index;

    wed_img[ap_area*step_num + ap_idx] = vox_value;
}


static void
wed_dose_ray_trace (
    Volume* wed_vol,             /* O: water equiv depth vol */
    Rpl_volume *rpl_vol,         /* I: radiographic depths */
    Volume *ct_vol,              /* I: CT volume */
    Volume_limit *vol_limit,     /* I: CT bounding region */
    const double *p1,                  /* I: @ source */
    const double *p2,                  /* I: @ aperture */
    int* ires,                   /* I: ray cast resolution */
    int ap_idx                   /* I: linear index of ray in ap */
)
{
    Callback_data cd;
    double ray[3];
    double ip1[3];
    double ip2[3];

    float* rpl_img = (float*) rpl_vol->get_volume()->img;

    /* Define unit vector in ray direction */
    vec3_sub3 (ray, p2, p1);
    vec3_normalize1 (ray);

    /* Test if ray intersects volume and create intersection points */
    if (!volume_limit_clip_ray (vol_limit, ip1, ip2, p1, ray)) {
        return;
    }

#if VERBOSE
    printf ("ap_idx: %d\n", ap_idx);
    printf ("P1: %g %g %g\n", p1[0], p1[1], p1[2]);
    printf ("P2: %g %g %g\n", p2[0], p2[1], p2[2]);

    printf ("ip1 = %g %g %g\n", ip1[0], ip1[1], ip1[2]);
    printf ("ip2 = %g %g %g\n", ip2[0], ip2[1], ip2[2]);
    printf ("ray = %g %g %g\n", ray[0], ray[1], ray[2]);
#endif

    /* init callback data for this ray */
    cd.ires = ires;
    cd.wed_vol = wed_vol;
    cd.ap_idx = ap_idx;

    /* useful things to have */
    int ap_area = ires[0] * ires[1];

    float ray_depth = 0.0;
    for (int i=0; i<wed_vol->dim[2]; i++) {

        ray_depth = rpl_img[ap_area*i + ap_idx];

        ray_trace_probe (
            ct_vol,                  // INPUT: CT volume
            vol_limit,               // INPUT: CT volume bounding box
            &wed_ray_trace_callback, // INPUT: step action cbFunction
            &cd,                     // INPUT: callback data
            ip1,                     // INPUT: ray starting point
            ip2,                     // INPUT: ray ending point
            ray_depth,               // INPUT: depth along ray
            i);                      // INPUT: index along ray
     }
}

static void
wed_volume_populate (
    Volume* wed_vol,
    Volume* ct_vol,
    Rpl_volume* rpl_vol
)
{
    int r;
    int ires[2];
    Volume_limit ct_limit;

    /* A couple of abbreviations */
    Proj_volume *proj_vol = rpl_vol->get_proj_volume();
    ires[0] = rpl_vol->get_volume()->dim[0];
    ires[1] = rpl_vol->get_volume()->dim[1];

    /* Compute volume boundary box */
    volume_limit_set (&ct_limit, ct_vol);

    /* Scan through the aperture */
    for (r = 0; r < ires[0]; r++) {
        int c;
        double r_tgt[3];
        double tmp[3];
        double p2[3];

        vec3_copy (r_tgt, proj_vol->get_ul_room());
        vec3_scale3 (tmp, proj_vol->get_incr_r(), (double) r);
        vec3_add2 (r_tgt, tmp);

        for (c = 0; c < ires[1]; c++) {
            int ap_idx;

            /* Compute index of aperture pixel */
            ap_idx = c * ires[0] + r;

            /* Compute p2 = 3d coordinates of point on aperture */
            vec3_scale3 (tmp, proj_vol->get_incr_c(), (double) c);
            vec3_add3 (p2, r_tgt, tmp);

            wed_dose_ray_trace (
                wed_vol,      /* O: wed volume */
                rpl_vol,      /* I: radiographic depths */
                ct_vol,       /* I: CT volume */
                &ct_limit,    /* I: CT bounding region */
                proj_vol->get_src(), /* I: @ source */
                p2,           /* I: @ aperture */
                ires,         /* I: ray cast resolution */
                ap_idx        /* I: linear index of ray in ap */
            );
        }
    }
}

void
wed_ct_compute (
    const char* out_fn,
    Wed_Parms* parms,
    Plm_image *ct_vol,
    Proton_Scene *scene
)
{
    Volume* wed_vol;
    Rpl_volume* rpl_vol = scene->rpl_vol;

#if defined (commentout)
    wed_vol = create_wed_volume (parms, scene);
    wed_volume_populate (wed_vol, ct_vol->gpuit_float(), rpl_vol);
    plm_image_save_vol (out_fn, wed_vol);
#endif

    wed_vol = create_wed_volume (parms, scene);
    rpl_vol->compute_wed_volume (wed_vol, ct_vol->gpuit_float());
    plm_image_save_vol (out_fn, wed_vol);
}

int
main (int argc, char* argv[])
{
    Wed_Parms parms;

    if (!parms.parse_args (argc, argv)) {
        exit (0);
    }

    Plm_image* ct_vol;
    Plm_image* dose_vol = 0;
    Proton_Scene scene;

    /* load the patient and insert into the scene */
    ct_vol = plm_image_load (parms.input_ct_fn, PLM_IMG_TYPE_ITK_FLOAT);
    if (!ct_vol) {
        fprintf (stderr, "\n** ERROR: Unable to load patient volume.\n");
        return -1;
    }
    scene.set_patient (ct_vol);

    if (parms.input_dose_fn != "" && parms.output_dose_fn != "") {
        dose_vol = plm_image_load (parms.input_dose_fn.c_str(), 
            PLM_IMG_TYPE_ITK_FLOAT);
    }

    /* set scene parameters */
    scene.beam->set_source_position (parms.src);
    scene.beam->set_isocenter_position (parms.isocenter);

    scene.ap->set_distance (parms.ap_offset);
    scene.ap->set_dim (parms.ires);
    scene.ap->set_spacing (parms.ap_spacing);
    if (parms.have_ic) {
        scene.ap->set_center (parms.ic);
    }

    scene.set_step_length(parms.ray_step);

    /* try to setup the scene with the provided parameters */
    if (!scene.init ()) {
        fprintf (stderr, "ERROR: Unable to initilize scene.\n");
        return -1;
    }
    scene.debug ();

    if (parms.rpl_vol_fn != "") {
        scene.rpl_vol->save (parms.rpl_vol_fn);
    }

    printf ("Working...\n");
    fflush(stdout);

    wed_ct_compute (parms.output_ct_fn, &parms, ct_vol, &scene);
    printf ("done.\n");

    if (parms.input_dose_fn != "" && parms.output_dose_fn != "") {
        printf ("Trying to wed dose...\n");
        wed_ct_compute (parms.output_dose_fn.c_str(), 
            &parms, dose_vol, &scene);
        printf ("Did wed dose...\n");
    }

    return 0;
}
