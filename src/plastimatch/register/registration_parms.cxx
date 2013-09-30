/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plmregister_config.h"
#include <fstream>
#include <list>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined (_WIN32)
// win32 directory stuff
#else
#include <sys/types.h>
#include <dirent.h>
#endif

#include "plm_path.h"
#include "print_and_exit.h"
#include "registration_parms.h"
#include "shared_parms.h"
#include "stage_parms.h"
#include "string_util.h"

#define BUFLEN 2048

class Registration_parms_private
{
public:
    std::string moving_fn;
    std::string fixed_fn;
    std::list<Stage_parms*> stages;
    Shared_parms *shared;
public:
    Registration_parms_private () {
        shared = new Shared_parms;
    }
    ~Registration_parms_private () {
        std::list<Stage_parms*>::iterator it;
        for (it = stages.begin(); it != stages.end(); it++) {
            delete *it;
        }
        delete shared;
    }
};

Registration_parms::Registration_parms()
{
    d_ptr = new Registration_parms_private;

    img_out_fmt = IMG_OUT_FMT_AUTO;
    img_out_type = PLM_IMG_TYPE_UNDEFINED;
    *img_out_fn = 0;
    *xf_in_fn = 0;
    xf_out_itk = false;
    *vf_out_fn = 0;
    *log_fn = 0;
    init_type = STAGE_TRANSFORM_NONE;
    default_value = 0.0;
    num_stages = 0;
    *moving_dir = 0;
    *fixed_dir = 0;
    *img_out_dir = 0;
    *vf_out_dir = 0;
    for (int i=0; i<256; i++) {
        moving_jobs[i][0] = '\0';
        fixed_jobs[i][0] = '\0';
    }
    job_idx = 0;
    num_jobs = 1;
}

Registration_parms::~Registration_parms()
{
    delete d_ptr;
}

// JAS 2012.02.13 -- TODO: Move somewhere more appropriate
static void
check_trailing_slash (char* s)
{
    int i=0;
    while (s[i++] != '\0');

    if (s[i-2] != '/') {
        strcat (s, "/");
    }
}

int
populate_jobs (char jobs[255][_MAX_PATH], char* dir)
{
#if defined (_WIN32)
    // Win32 Version goes here
    return 0;
#else
    DIR *dp;
    struct dirent *ep;
    int z=0;
    char buffer[_MAX_PATH];

    dp = opendir (dir);

    if (dp != NULL) {
        while ((ep=readdir(dp))) {
            memset (buffer, 0, _MAX_PATH);
            if (!strcmp(ep->d_name, ".")) {
                continue;
            } else if (!strcmp(ep->d_name, "..")) {
                continue;
            }
            strncpy (jobs[z++], ep->d_name, _MAX_PATH);
        }
        (void) closedir (dp);
    } else {
        printf ("Error: Could not open %s\n", dir);
    }

    return z;
#endif
}

int
Registration_parms::set_key_val (
    const char* key, 
    const char* val, 
    int section
)
{
    int rc;
    Stage_parms* stage = 0;
    Shared_parms* shared = 0;

    if (section != 0) {
        stage = d_ptr->stages.back();
        shared = stage->get_shared_parms();
    }
    else {
        shared = d_ptr->shared;
    }

    /* The following keywords are only allowed globally */
    if (!strcmp (key, "fixed")) {
        if (section != 0) goto error_not_stages;
        d_ptr->fixed_fn = val;
    }
    else if (!strcmp (key, "moving")) {
        if (section != 0) goto error_not_stages;
        d_ptr->moving_fn = val;
    }
    else if (!strcmp (key, "fixed_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (this->fixed_dir, val, _MAX_PATH);
        check_trailing_slash (this->fixed_dir);
        this->num_jobs = populate_jobs (this->fixed_jobs, this->fixed_dir);
    }
    else if (!strcmp (key, "moving_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (this->moving_dir, val, _MAX_PATH);
        check_trailing_slash (this->moving_dir);
        this->num_jobs = populate_jobs (this->moving_jobs, this->moving_dir);
    }
    else if (!strcmp (key, "img_out_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (this->img_out_dir, val, _MAX_PATH);
        check_trailing_slash (this->img_out_dir);
    }
    else if (!strcmp (key, "vf_out_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (this->vf_out_dir, val, _MAX_PATH);
        check_trailing_slash (this->vf_out_dir);
    }
    else if (!strcmp (key, "xf_in") 
        || !strcmp (key, "xform_in") 
        || !strcmp (key, "vf_in"))
    {
        if (section != 0) goto error_not_stages;
        strncpy (this->xf_in_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "log") || !strcmp (key, "logfile")) {
        if (section != 0) goto error_not_stages;
        strncpy (this->log_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "fixed_landmarks")) {
        if (section != 0) goto error_not_global;
        this->fixed_landmarks_fn = val;
    }
    else if (!strcmp (key, "moving_landmarks")) {
        if (section != 0) goto error_not_global;
        this->moving_landmarks_fn = val;
    }
    else if (!strcmp (key, "fixed_landmark_list")) {
        if (section != 0) goto error_not_global;
        this->fixed_landmarks_list = val;
    }
    else if (!strcmp (key, "moving_landmark_list")) {
        if (section != 0) goto error_not_global;
        this->moving_landmarks_list = val;
    }

    /* The following keywords are allowed either globally or in stages */
    else if (!strcmp (key, "background_val")
        || !strcmp (key, "background-val")
        || !strcmp (key, "default_value")
        || !strcmp (key, "default-value"))
    {
        float f;
        if (sscanf (val, "%g", &f) != 1) {
            goto error_exit;
        }
        if (section == 0) {
            this->default_value = f;
        } else {
            stage->default_value = f;
        }
    }
    else if (!strcmp (key, "fixed_mask") || !strcmp (key, "fixed_roi")) {
        shared->fixed_roi_fn = val;
    }
    else if (!strcmp (key, "moving_mask") || !strcmp (key, "moving_roi")) {
        shared->moving_roi_fn = val;
    }
    else if (!strcmp (key, "fixed_roi_enable")) {
        shared->fixed_roi_enable = string_value_true (val);
    }
    else if (!strcmp (key, "moving_roi_enable"))
    {
        shared->moving_roi_enable = string_value_true (val);
    }
    else if (!strcmp (key, "img_out") || !strcmp (key, "image_out")) {
        if (section == 0) {
            strncpy (this->img_out_fn, val, _MAX_PATH);
        } else {
            strncpy (stage->img_out_fn, val, _MAX_PATH);
        }
    }
    else if (!strcmp (key, "img_out_fmt")) {
        int fmt = IMG_OUT_FMT_AUTO;
        if (!strcmp (val, "dicom")) {
            fmt = IMG_OUT_FMT_DICOM;
        } else {
            goto error_exit;
        }
        if (section == 0) {
            this->img_out_fmt = fmt;
        } else {
            stage->img_out_fmt = fmt;
        }
    }
    else if (!strcmp (key, "img_out_type")) {
        Plm_image_type type = plm_image_type_parse (val);
        if (type == PLM_IMG_TYPE_UNDEFINED) {
            goto error_exit;
        }
        if (section == 0) {
            this->img_out_type = type;
        } else {
            stage->img_out_type = type;
        }
    }
    else if (!strcmp (key, "vf_out")) {
        if (section == 0) {
            strncpy (this->vf_out_fn, val, _MAX_PATH);
        } else {
            strncpy (stage->vf_out_fn, val, _MAX_PATH);
        }
    }
    else if (!strcmp (key, "xf_out_itk")) {
        bool value = true;
        if (!strcmp (val, "false")) {
            value = false;
        }
        if (section == 0) {
            this->xf_out_itk = value;
        } else {
            stage->xf_out_itk = value;
        }
    }
    else if (!strcmp (key, "xf_out") || !strcmp (key, "xform_out")) {
        /* xf_out is special.  You can have more than one of these.  
           This capability is used by the slicer plugin. */
        if (section == 0) {
            this->xf_out_fn.push_back (val);
        } else {
            stage->xf_out_fn.push_back (val);
        }
    }
    else if (!strcmp (key, "warped_landmarks")) {
        if (section == 0) {
            this->warped_landmarks_fn = val;
        } else {
            stage->warped_landmarks_fn = val;
        }
    }

    /* The following keywords are only allowed in stages */
    else if (!strcmp (key, "resume")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val, "1")) {
            stage->resume_stage = true;
        }
    }
    else if (!strcmp (key, "xform")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val,"translation")) {
            stage->xform_type = STAGE_TRANSFORM_TRANSLATION;
        }
        else if (!strcmp(val,"rigid") || !strcmp(val,"versor")) {
            stage->xform_type = STAGE_TRANSFORM_VERSOR;
        }
        else if (!strcmp (val,"quaternion")) {
            stage->xform_type = STAGE_TRANSFORM_QUATERNION;
        }
        else if (!strcmp (val,"affine")) {
            stage->xform_type = STAGE_TRANSFORM_AFFINE;
        }
        else if (!strcmp (val,"bspline")) {
            stage->xform_type = STAGE_TRANSFORM_BSPLINE;
        }
        else if (!strcmp (val,"vf")) {
            stage->xform_type = STAGE_TRANSFORM_VECTOR_FIELD;
        }
        else if (!strcmp (val,"align_center")) {
            stage->xform_type = STAGE_TRANSFORM_ALIGN_CENTER;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "optim")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"none")) {
            stage->optim_type = OPTIMIZATION_NO_REGISTRATION;
        }
        else if (!strcmp(val,"amoeba")) {
            stage->optim_type = OPTIMIZATION_AMOEBA;
        }
        else if (!strcmp(val,"oneplusone")) {
            stage->optim_type = OPTIMIZATION_ONEPLUSONE;
        }
        else if (!strcmp(val,"frpr")) {
            stage->optim_type = OPTIMIZATION_FRPR;
        }
        else if (!strcmp(val,"demons")) {
            stage->optim_type = OPTIMIZATION_DEMONS;
        }
        else if (!strcmp(val,"lbfgs")) {
            stage->optim_type = OPTIMIZATION_LBFGS;
        }
        else if (!strcmp(val,"lbfgsb")) {
            stage->optim_type = OPTIMIZATION_LBFGSB;
        }
        else if (!strcmp(val,"liblbfgs")) {
            stage->optim_type = OPTIMIZATION_LIBLBFGS;
        }
        else if (!strcmp(val,"nocedal")) {
            stage->optim_type = OPTIMIZATION_LBFGSB;
        }
        else if (!strcmp(val,"rsg")) {
            stage->optim_type = OPTIMIZATION_RSG;
        }
        else if (!strcmp(val,"steepest")) {
            stage->optim_type = OPTIMIZATION_STEEPEST;
        }
        else if (!strcmp(val,"versor")) {
            stage->optim_type = OPTIMIZATION_VERSOR;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "impl")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"none")) {
            stage->impl_type = IMPLEMENTATION_NONE;
        }
        else if (!strcmp(val,"itk")) {
            stage->impl_type = IMPLEMENTATION_ITK;
        }
        else if (!strcmp(val,"plastimatch")) {
            stage->impl_type = IMPLEMENTATION_PLASTIMATCH;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "optim_subtype")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"fsf")) {
            stage->optim_subtype = OPTIMIZATION_SUB_FSF;
        }
        else if (!strcmp(val,"diffeomorphic")) {
            stage->optim_subtype = OPTIMIZATION_SUB_DIFF_ITK;
        }
        else if (!strcmp(val,"log_domain")) {
            stage->optim_subtype = OPTIMIZATION_SUB_LOGDOM_ITK;
        }
        else if (!strcmp(val,"sym_log_domain")) {
            stage->optim_subtype = OPTIMIZATION_SUB_SYM_LOGDOM_ITK;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "threading")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"single")) {
            stage->threading_type = THREADING_CPU_SINGLE;
        }
        else if (!strcmp(val,"openmp")) {
#if (OPENMP_FOUND)
            stage->threading_type = THREADING_CPU_OPENMP;
#else
            stage->threading_type = THREADING_CPU_SINGLE;
#endif
        }
        else if (!strcmp(val,"cuda")) {
#if (CUDA_FOUND)
            stage->threading_type = THREADING_CUDA;
#elif (OPENMP_FOUND)
            stage->threading_type = THREADING_CPU_OPENMP;
#else
            stage->threading_type = THREADING_CPU_SINGLE;
#endif
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "alg_flavor")
        || !strcmp (key, "flavor"))
    {
        if (section == 0) goto error_not_global;
        if (strlen (val) >= 1) {
            stage->alg_flavor = val[0];
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "metric")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"mse") || !strcmp(val,"MSE")) {
            stage->metric_type = METRIC_MSE;
        }
        else if (!strcmp(val,"mi") || !strcmp(val,"MI")) {
            stage->metric_type = METRIC_MI;
        }
        else if (!strcmp(val,"mattes")) {
            stage->metric_type = METRIC_MI_MATTES;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "histogram_type")) {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"eqsp") || !strcmp(val,"EQSP")) {
            stage->mi_histogram_type = HIST_EQSP;
        }
        else if (!strcmp(val,"vopt") || !strcmp(val,"VOPT")) {
            stage->mi_histogram_type = HIST_VOPT;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "regularization"))
    {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"none")) {
            stage->regularization_type = REGULARIZATION_NONE;
        }
        else if (!strcmp(val,"analytic")) {
            stage->regularization_type = REGULARIZATION_BSPLINE_ANALYTIC;
        }
        else if (!strcmp(val,"semi-analytic")
            || !strcmp(val,"semi_analytic")) {
            stage->regularization_type = REGULARIZATION_BSPLINE_SEMI_ANALYTIC;
        }
        else if (!strcmp(val,"numeric")) {
            stage->regularization_type = REGULARIZATION_BSPLINE_NUMERIC;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "regularization_lambda")
        || !strcmp (key, "young_modulus")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%f", &stage->regularization_lambda) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "background_max")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->background_max) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "min_its")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->min_its) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "iterations") 
        || !strcmp (key, "max_iterations")
        || !strcmp (key, "max_its")
        || !strcmp (key, "its"))
    {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->max_its) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "learn_rate")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->learn_rate) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "grad_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->grad_tol) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "pgtol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%f", &stage->pgtol) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "max_step")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->max_step) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "min_step")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->min_step) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "rsg_grad_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->rsg_grad_tol) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "translation_scale_factor")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->translation_scale_factor) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "convergence_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->convergence_tol) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "opo_epsilon")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->opo_epsilon) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "opo_initial_search_rad")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->opo_initial_search_rad) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "frpr_step_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->frpr_step_tol) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "frpr_step_length")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->frpr_step_length) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "frpr_max_line_its")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->frpr_max_line_its) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "mattes_histogram_bins") 
        || !strcmp (key, "mi_histogram_bins")) {
        if (section == 0) goto error_not_global;
    rc = sscanf (val, "%d %d", &stage->mi_histogram_bins_fixed,
                               &stage->mi_histogram_bins_moving);
    if (rc == 1) {
        stage->mi_histogram_bins_moving = stage->mi_histogram_bins_fixed;
    } else if (rc != 2) {
        goto error_exit;
    }
    }
    else if (!strcmp (key, "mattes_fixed_minVal")
             ||!strcmp (key, "mi_fixed_minVal")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->mi_fixed_image_minVal) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "mattes_fixed_maxVal")
             ||!strcmp (key, "mi_fixed_maxVal")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->mi_fixed_image_maxVal) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "mattes_moving_minVal")
             ||!strcmp (key, "mi_moving_minVal")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->mi_moving_image_minVal) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "mattes_moving_maxVal")
             ||!strcmp (key, "mi_moving_maxVal")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->mi_moving_image_maxVal) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "num_samples")
        || !strcmp (key, "mattes_num_spatial_samples")
        || !strcmp (key, "mi_num_spatial_samples")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->mi_num_spatial_samples) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "num_samples_pct")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%f", &stage->mi_num_spatial_samples_pct) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_std_deformation_field")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_std) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_std_update_field")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_std_update_field) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_step_length")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_step_length) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_smooth_deformation_field")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val, "1")) {
            stage->demons_smooth_deformation_field = true;
        }
        else
           stage->demons_smooth_deformation_field = false;
    }
    else if (!strcmp (key, "demons_smooth_update_field")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val, "1")) {
            stage->demons_smooth_update_field = true;
        }
        else
           stage->demons_smooth_update_field = false;
    }
    else if (!strcmp (key, "demons_gradient_type"))
    {
        if (section == 0) goto error_not_global;
        if (!strcmp(val,"symmetric")) {
            stage->demons_gradient_type = SYMMETRIC;
        }
        else if (!strcmp(val,"fixed")) {
            stage->demons_gradient_type = FIXED_IMAGE;
        }
        else if (!strcmp(val,"warped_moving")) {
            stage->demons_gradient_type = WARPED_MOVING;
        }
        else if (!strcmp(val,"mapped_moving")) {
            stage->demons_gradient_type = MAPPED_MOVING;
        }
        else {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "num_approx_terms_log_demons")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->num_approx_terms_log_demons) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_homogenization")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_homogenization) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_filter_width")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d %d %d", 
                &(stage->demons_filter_width[0]), 
                &(stage->demons_filter_width[1]), 
                &(stage->demons_filter_width[2])) != 3) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "amoeba_parameter_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &(stage->amoeba_parameter_tol)) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "landmark_stiffness")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->landmark_stiffness) != 1) {
            goto error_exit;
        }
    }   
    else if (!strcmp (key, "landmark_flavor")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%c", &stage->landmark_flavor) != 1) {
            goto error_exit;
        }
    }   
    else if (!strcmp (key, "res") || !strcmp (key, "ss")) {
        if (section == 0) goto error_not_global;
        stage->subsampling_type = SUBSAMPLING_VOXEL_RATE;
        if (sscanf (val, "%d %d %d", 
                &(stage->fixed_subsample_rate[0]), 
                &(stage->fixed_subsample_rate[1]), 
                &(stage->fixed_subsample_rate[2])) != 3) {
            goto error_exit;
        }
        stage->moving_subsample_rate[0] = stage->fixed_subsample_rate[0];
        stage->moving_subsample_rate[1] = stage->fixed_subsample_rate[1];
        stage->moving_subsample_rate[2] = stage->fixed_subsample_rate[2];
    }
    else if (!strcmp (key, "ss_fixed") || !strcmp (key, "fixed_ss")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d %d %d", 
                &(stage->fixed_subsample_rate[0]), 
                &(stage->fixed_subsample_rate[1]), 
                &(stage->fixed_subsample_rate[2])) != 3) {
            goto error_exit;
        }
        if (stage->subsampling_type == SUBSAMPLING_AUTO) {
            stage->moving_subsample_rate[0] = stage->fixed_subsample_rate[0];
            stage->moving_subsample_rate[1] = stage->fixed_subsample_rate[1];
            stage->moving_subsample_rate[2] = stage->fixed_subsample_rate[2];
        }
        stage->subsampling_type = SUBSAMPLING_VOXEL_RATE;
    }
    else if (!strcmp (key, "ss_moving") || !strcmp (key, "moving_ss")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d %d %d", 
                &(stage->moving_subsample_rate[0]), 
                &(stage->moving_subsample_rate[1]), 
                &(stage->moving_subsample_rate[2])) != 3) {
            goto error_exit;
        }
        if (stage->subsampling_type == SUBSAMPLING_AUTO) {
            stage->fixed_subsample_rate[0] = stage->moving_subsample_rate[0];
            stage->fixed_subsample_rate[1] = stage->moving_subsample_rate[1];
            stage->fixed_subsample_rate[2] = stage->moving_subsample_rate[2];
        }
        stage->subsampling_type = SUBSAMPLING_VOXEL_RATE;
    }
    else if (!strcmp (key, "num_grid")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d %d %d", &(stage->num_grid[0]), &(stage->num_grid[1]), &(stage->num_grid[2])) != 3) {
            goto error_exit;
        }
        stage->grid_method = 0;
    }
    else if (!strcmp (key, "grid_spac")
        || !strcmp (key, "grid_spacing"))
    {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g %g %g", &(stage->grid_spac[0]), &(stage->grid_spac[1]), &(stage->grid_spac[2])) != 3) {
            goto error_exit;
        }
        stage->grid_method = 1;
    }
    else if (!strcmp (key, "histo_equ")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val, "1")) {
            stage->histoeq = true;
        }
        else
           stage->histoeq= false;
    }
    else if (!strcmp (key, "thresh_mean_intensity")) {
        if (section == 0) goto error_not_global;
        if (!strcmp (val, "1")) {
            stage->thresh_mean_intensity = true;
        }
        else
           stage->thresh_mean_intensity= false;
    }
    else if (!strcmp (key, "num_hist_levels")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->num_hist_levels) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "num_matching_points")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->num_matching_points) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "debug_dir")) {
        if (section == 0) goto error_not_global;
        stage->debug_dir = val;
    }
    else {
        goto error_exit;
    }
    return 0;

  error_not_stages:
    print_and_exit ("This key (%s) not allowed in a stages section\n", key);
    return -1;

  error_not_global:
    print_and_exit ("This key (%s) not is allowed in a global section\n", key);
    return -1;

  error_exit:
    print_and_exit ("Unknown (key,val) combination: (%s,%s)\n", key, val);
    return -1;
}

int
Registration_parms::set_command_string (
    const std::string& command_string
)
{
    std::string buf;
    std::string buf_ori;    /* An extra copy for diagnostics */
    int section = 0;

    std::stringstream ss (command_string);

    while (getline (ss, buf)) {
        buf_ori = buf;
        buf = trim (buf);
        buf_ori = trim (buf_ori, "\r\n");
        if (buf == "") continue;
        if (buf[0] == '#') continue;
        if (buf[0] == '[') {
            if (buf.find ("[GLOBAL]") != std::string::npos
                || buf.find ("[global]") != std::string::npos)
            {
                section = 0;
                continue;
            }
            else if (buf.find ("[STAGE]") != std::string::npos
                || buf.find ("[stage]") != std::string::npos)
            {
                section = 1;
                this->append_stage ();
                continue;
            }
            else if (buf.find ("[COMMENT]") != std::string::npos
                || buf.find ("[comment]") != std::string::npos)
            {
                section = 2;
                continue;
            }
            else {
                printf ("Parse error: %s\n", buf_ori.c_str());
                return -1;
            }
        }
        if (section == 2) continue;
        size_t key_loc = buf.find ("=");
        if (key_loc == std::string::npos) {
            continue;
        }
        std::string key = buf.substr (0, key_loc);
        std::string val = buf.substr (key_loc+1);
        key = trim (key);
        val = trim (val);

        if (key != "" && val != "") {
            if (this->set_key_val (key.c_str(), val.c_str(), section) < 0) {
                printf ("Parse error: %s\n", buf_ori.c_str());
                return -1;
            }
        }
    }
    return 0;
}

int
Registration_parms::parse_command_file (const char* options_fn)
{
    /* Read file into string */
    std::ifstream t (options_fn);
    std::stringstream buffer;
    buffer << t.rdbuf();

    /* Parse the string */
    return this->set_command_string (buffer.str());
}

/* JAS 2012.03.13
 *  This is a temp solution */
/* GCS 2012-12-28: Nb. regp->job_idx must be set prior to calling 
   this function */
void
Registration_parms::set_job_paths (void)
{
    /* Setup input paths */
    if (*(this->fixed_dir)) {
        d_ptr->fixed_fn = string_format (
            "%s%s", this->fixed_dir, this->fixed_jobs[this->job_idx]);
    }
    if (*(this->moving_dir)) {
        d_ptr->moving_fn = string_format (
            "%s%s", this->moving_dir, this->moving_jobs[this->job_idx]);
    }

    /* Setup output paths */
    /*   NOTE: For now, output files inherit moving image names */
    if (*(this->img_out_dir)) {
        if (!strcmp (this->img_out_dir, this->moving_dir)) {
            strcpy (this->img_out_fn, this->img_out_dir);
            strcat (this->img_out_fn, "warp/");
            strcat (this->img_out_fn, this->moving_jobs[this->job_idx]);
        } else {
            strcpy (this->img_out_fn, this->img_out_dir);
            strcat (this->img_out_fn, this->moving_jobs[this->job_idx]);
        }
        /* If not dicom, we give a default name */
        if (this->img_out_fmt != IMG_OUT_FMT_DICOM) {
            std::string fn = string_format ("%s.mha", this->img_out_fn);
            strcpy (this->img_out_fn, fn.c_str());
        }
    } else {
        /* Output directory not specifed but img_out was... smart fallback*/
        if (*(this->img_out_fn)) {
            strcpy (this->img_out_fn, this->moving_dir);
            strcat (this->img_out_fn, "warp/");
            strcat (this->img_out_fn, this->moving_jobs[this->job_idx]);
        }
    }
    if (*(this->vf_out_dir)) {
        if (!strcmp (this->vf_out_dir, this->moving_dir)) {
            strcpy (this->vf_out_fn, this->img_out_dir);
            strcat (this->vf_out_fn, "vf/");
            strcat (this->vf_out_fn, this->moving_jobs[this->job_idx]);
        } else {
            strcpy (this->vf_out_fn, this->vf_out_dir);
            strcat (this->vf_out_fn, this->moving_jobs[this->job_idx]);
        }
        /* Give a default name */
        std::string fn = string_format ("%s_vf.mha", this->vf_out_fn);
        strcpy (this->vf_out_fn, fn.c_str());
    } else {
        /* Output directory not specifed but vf_out was... smart fallback*/
        if (*(this->vf_out_fn)) {
            strcpy (this->vf_out_fn, this->moving_dir);
            strcat (this->vf_out_fn, "vf/");
            strcat (this->vf_out_fn, this->moving_jobs[this->job_idx]);
        }
    }
}

const std::string& 
Registration_parms::get_fixed_fn ()
{
    return d_ptr->fixed_fn;
}

const std::string& 
Registration_parms::get_moving_fn ()
{
    return d_ptr->moving_fn;
}

Shared_parms*
Registration_parms::get_shared_parms ()
{
    return d_ptr->shared;
}

std::list<Stage_parms*>& 
Registration_parms::get_stages ()
{
    return d_ptr->stages;
}

Stage_parms* 
Registration_parms::append_stage ()
{
    Stage_parms *sp;

    this->num_stages ++;
    if (this->num_stages == 1) {
        sp = new Stage_parms();
    } else {
        sp = new Stage_parms(*d_ptr->stages.back());
    }
    d_ptr->stages.push_back (sp);

    /* Some parameters that should be copied from global 
       to the first stage. */
    if (this->num_stages == 1) {
        sp->default_value = this->default_value;
    }

    sp->stage_no = this->num_stages;

    return sp;
}
