/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plm_path.h"
#include "fdk.h"
#include "fdk_opts.h"
#include "fdk_utils.h"
#include "file_util.h"
#include "proj_image.h"
#include "readmha.h"
#include "ramp_filter.h"
#include "volume.h"

Volume*
my_create_volume (Fdk_options* options)
{
    float offset[3];
    float spacing[3];
    float* vol_size = options->vol_size;
    int* resolution = options->resolution;

    spacing[0] = vol_size[0] / resolution[0];
    spacing[1] = vol_size[1] / resolution[1];
    spacing[2] = vol_size[2] / resolution[2];

    offset[0] = -vol_size[0] / 2.0f + spacing[0] / 2.0f;
    offset[1] = -vol_size[1] / 2.0f + spacing[1] / 2.0f;
    offset[2] = -vol_size[2] / 2.0f + spacing[2] / 2.0f;

    return volume_create (resolution, offset, spacing, PT_FLOAT, 0, 0);
}

float
convert_to_hu_pixel (float in_value)
{
    float hu;
    float diameter = 40.0;  /* reconstruction diameter in cm */
    hu = 1000 * ((in_value / diameter) - .167) / .167;
    return hu;
}

void
convert_to_hu (Volume* vol, Fdk_options* options)
{
    int i, j, k, p;
    float* img = (float*) vol->img;
    
    p = 0;
    for (k = 0; k < vol->dim[2]; k++) {
	for (j = 0; j < vol->dim[1]; j++) {
	    for (i = 0; i < vol->dim[0]; i++) {
		img[p] = convert_to_hu_pixel (img[p]);
		p++;
	    }
	}
    }
}

CB_Image*
get_image_pfm (Fdk_options* options, int image_num)
{
    char* img_file_pat = "out_%04d.pfm";
    char* mat_file_pat = "out_%04d.txt";

    char img_file[1024], mat_file[1024], fmt[1024];
    sprintf (fmt, "%s/%s", options->input_dir, img_file_pat);
    sprintf (img_file, fmt, image_num);
    sprintf (fmt, "%s/%s", options->input_dir, mat_file_pat);
    sprintf (mat_file, fmt, image_num);
    return proj_image_load_pfm (img_file, mat_file);
}

CB_Image*
get_image_raw (Fdk_options* options, int image_num)
{
    char* img_file_pat = "Proj_%03d.raw";
    char* mat_file_pat = "";

    char img_file[1024], mat_file[1024];
    sprintf (img_file, "%s/Proj_%03d.raw", options->input_dir,image_num);
    sprintf (mat_file, "%s/tmp/out_%04d.txt",options->input_dir, image_num);
    return proj_image_load_and_filter (options, img_file, mat_file);
}

void
write_coronal_sagittal (Fdk_options* options, Volume* vol)
{
    if (options->coronal) {
	Volume *cor;
	char fn[_MAX_PATH];

	strcpy (fn, options->output_file);
	strip_extension (fn);
	strcat (fn, "-cor.mh5");

	cor = volume_axial2coronal (vol);
	write_mha (fn, cor);
	volume_free (cor);
    }

    if (options->sagittal) {
	Volume *sag;
	char fn[_MAX_PATH];

	strcpy (fn, options->output_file);
	strip_extension (fn);
	strcat (fn, "-sag.mh5");

	sag = volume_axial2sagittal (vol);
	write_mha (fn, sag);
	volume_free (sag);
    }
}
