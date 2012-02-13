/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>
#if defined (_WIN32)
// win32 directory stuff
#else
#include <sys/types.h>
#include <dirent.h>
#endif
#include "itk_image.h"
#include "itk_optim.h"
#include "plm_parms.h"
#include "string_util.h"
#include "xform.h"

#define BUFLEN 2048

// JAS 2012.02.13 -- TODO: Move somewhere more appropriate
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
        printf ("Listing for %s\n", dir);
        while ((ep=readdir(dp))) {
            memset (buffer, 0, _MAX_PATH);
            if (!strcmp(ep->d_name, ".")) {
                continue;
            } else if (!strcmp(ep->d_name, "..")) {
                continue;
            }
            strcpy (buffer, dir);
            strcat (buffer, ep->d_name);
            strncpy (jobs[z++], buffer, _MAX_PATH);
            printf ("  %s\n", jobs[z-1]);
        }
        (void) closedir (dp);
    } else {
        printf ("Error: Could not open %s\n", dir);
    }

    return z;
#endif
}

int
set_key_val (
    Registration_parms* regp, 
    const char* key, 
    const char* val, 
    int section
)
{
    int rc;
    Stage_parms* stage = 0;
    if (section != 0) {
        stage = regp->stages[regp->num_stages-1];
    }

    /* The following keywords are only allowed globally */
    if (!strcmp (key, "fixed")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->fixed_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "moving")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->moving_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "fixed_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->fixed_dir, val, _MAX_PATH);
        regp->num_jobs = populate_jobs (regp->fixed_jobs, regp->fixed_dir);
    }
    else if (!strcmp (key, "moving_dir")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->moving_dir, val, _MAX_PATH);
        regp->num_jobs = populate_jobs (regp->moving_jobs, regp->moving_dir);
    }
    else if (!strcmp (key, "fixed_mask")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->fixed_mask_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "moving_mask")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->moving_mask_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "xf_in") || !strcmp (key, "xform_in") || !strcmp (key, "vf_in")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->xf_in_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "log") || !strcmp (key, "logfile")) {
        if (section != 0) goto error_not_stages;
        strncpy (regp->log_fn, val, _MAX_PATH);
    }
    else if (!strcmp (key, "fixed_landmarks")) {
        if (section != 0) goto error_not_global;
        regp->fixed_landmarks_fn = val;
    }
    else if (!strcmp (key, "moving_landmarks")) {
        if (section != 0) goto error_not_global;
        regp->moving_landmarks_fn = val;
    }

    /* The following keywords are allowed either globally or in stages */
    else if (!strcmp (key, "img_out") || !strcmp (key, "image_out")) {
        if (section == 0) {
            strncpy (regp->img_out_fn, val, _MAX_PATH);
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
            regp->img_out_fmt = fmt;
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
            regp->img_out_type = type;
        } else {
            stage->img_out_type = type;
        }
    }
    else if (!strcmp (key, "vf_out")) {
        if (section == 0) {
            strncpy (regp->vf_out_fn, val, _MAX_PATH);
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
            regp->xf_out_itk = value;
        } else {
            stage->xf_out_itk = value;
        }
    }
    else if (!strcmp (key, "xf_out") || !strcmp (key, "xform_out")) {
        /* xf_out is special.  You can have more than one of these.  
           This capability is used by the slicer plugin. */
#if defined (commentout)
        if (section == 0) {
            strncpy (regp->xf_out_fn, val, _MAX_PATH);
        } else {
            strncpy (stage->xf_out_fn, val, _MAX_PATH);
        }
#endif
        if (section == 0) {
            regp->xf_out_fn.push_back (val);
        } else {
            stage->xf_out_fn.push_back (val);
        }
    }
    else if (!strcmp (key, "warped_landmarks")) {
        if (section == 0) {
            regp->warped_landmarks_fn = val;
        } else {
            stage->warped_landmarks_fn = val;
        }
    }

    /* The following keywords are only allowed in stages */
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
        else if (!strcmp(val,"rsg")) {
            stage->optim_type = OPTIMIZATION_RSG;
        }
        else if (!strcmp(val,"versor")) {
            stage->optim_type = OPTIMIZATION_VERSOR;
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
        else if (!strcmp(val,"demons")) {
            stage->optim_type = OPTIMIZATION_DEMONS;
        }
        else if (!strcmp(val,"steepest")) {
            stage->optim_type = OPTIMIZATION_STEEPEST;
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
    else if (!strcmp (key, "background_val")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->background_val) != 1) {
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
        || !strcmp (key, "max_its"))
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
    else if (!strcmp (key, "convergence_tol")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->convergence_tol) != 1) {
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
    else if (!strcmp (key, "num_samples")
        || !strcmp (key, "mattes_num_spatial_samples")
        || !strcmp (key, "mi_num_spatial_samples")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &stage->mi_num_spatial_samples) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_std")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_std) != 1) {
            goto error_exit;
        }
    }
    else if (!strcmp (key, "demons_acceleration")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g", &stage->demons_acceleration) != 1) {
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
    else if (!strcmp (key, "grid_spac")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%g %g %g", &(stage->grid_spac[0]), &(stage->grid_spac[1]), &(stage->grid_spac[2])) != 3) {
            goto error_exit;
        }
        stage->grid_method = 1;
    }
    else if (!strcmp (key, "histoeq")) {
        if (section == 0) goto error_not_global;
        if (sscanf (val, "%d", &(stage->histoeq)) != 1) {
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
                this->num_stages ++;
                this->stages = (Stage_parms**) realloc (
                    this->stages, this->num_stages * sizeof(Stage_parms*));
                if (this->num_stages == 1) {
                    this->stages[this->num_stages-1] = new Stage_parms();
                } else {
                    this->stages[this->num_stages-1] = new Stage_parms(
                        *(this->stages[this->num_stages-2]));
                }
                this->stages[this->num_stages-1]->stage_no = this->num_stages;
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
            if (set_key_val (this, key.c_str(), val.c_str(), section) < 0) {
                printf ("Parse error: %s\n", buf_ori.c_str());
                return -1;
            }
        }
    }
    return 0;
}

int
plm_parms_parse_command_file (Registration_parms* regp, const char* options_fn)
{
    /* Read file into string */
    std::ifstream t (options_fn);
    std::stringstream buffer;
    buffer << t.rdbuf();

    /* Parse the string */
    return regp->set_command_string (buffer.str());
}
