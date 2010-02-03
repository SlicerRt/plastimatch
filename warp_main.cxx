/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <time.h>

#include "cxt_io.h"
#include "cxt_warp.h"
#include "gdcm_rtss.h"
#include "getopt.h"
#include "plm_file_format.h"
#include "plm_image_header.h"
#include "plm_warp.h"
#include "print_and_exit.h"
#include "rtds.h"
#include "rtss_warp.h"
#include "warp_main.h"
#include "xform.h"
#include "xio_warp.h"

static void
print_usage (char* command)
{
    printf (
	"Usage: plastimatch %s [options]\n"
	"Options:\n"
	"    --input=filename\n"
	"    --output=filename\n"
	"    --xf=filename\n"
	"    --interpolation=nn\n"
	"    --fixed=filename\n"
	"    --offset=\"x y z\"\n"
	"    --spacing=\"x y z\"\n"
	"    --dims=\"x y z\"\n"
	"    --output-vf=filename\n"
	"    --default-val=number\n"
	"    --output-format=dicom\n"
	"    --output-type={uchar,short,float,...}\n"
	"    --algorithm=itk\n"
	"    --ctatts=filename          (for dij)\n"
	"    --dif=filename             (for dij)\n"
	"    --input-ss-img=filename    (for structures)\n"
	"    --dicom-dir=directory      (for structure association)\n"
	"    --output-prefix=string     (for structures)\n"
	"    --output-labelmap=filename (for structures)\n"
	"    --output-ss-img=filename   (for structures)\n"
	"    --output-ss-list=filename  (for structures)\n"
	"    --output-cxt=filename      (for structures)\n"
	"    --output-xio=directory     (for structures)\n"
	"    --prune-empty              (for structures)\n"
	,
	command);
    exit (-1);
}

void
warp_parse_args (Warp_parms* parms, int argc, char* argv[])
{
    int ch;
    int rc;
    int have_offset = 0;
    int have_spacing = 0;
    int have_dims = 0;
    static struct option longopts[] = {
	{ "input",          required_argument,      NULL,           2 },
	{ "output",         required_argument,      NULL,           3 },
	{ "img-output",     required_argument,      NULL,           3 },
	{ "vf",             required_argument,      NULL,           4 },
	{ "default_val",    required_argument,      NULL,           5 },
	{ "default-val",    required_argument,      NULL,           5 },
	{ "xf",             required_argument,      NULL,           6 },
	{ "fixed",	    required_argument,      NULL,           7 },
	{ "output_vf",      required_argument,      NULL,           8 },
	{ "output-vf",      required_argument,      NULL,           8 },
	{ "interpolation",  required_argument,      NULL,           9 },
	{ "offset",         required_argument,      NULL,           10 },
	{ "spacing",        required_argument,      NULL,           11 },
	{ "dims",           required_argument,      NULL,           12 },
	{ "output_format",  required_argument,      NULL,           13 },
	{ "output-format",  required_argument,      NULL,           13 },
	{ "ctatts",         required_argument,      NULL,           14 },
	{ "dif",            required_argument,      NULL,           15 },
	{ "algorithm",      required_argument,      NULL,           16 },
	{ "output-type",    required_argument,      NULL,           17 },
	{ "output_type",    required_argument,      NULL,           17 },
	{ "dicom-dir",      required_argument,      NULL,           18 },
	{ "dicom_dir",      required_argument,      NULL,           18 },
	{ "output-prefix",  required_argument,      NULL,           19 },
	{ "output_prefix",  required_argument,      NULL,           19 },
	{ "output-labelmap",required_argument,      NULL,           20 },
	{ "output_labelmap",required_argument,      NULL,           20 },
	{ "output-ss-img",  required_argument,      NULL,           21 },
	{ "output_ss_img",  required_argument,      NULL,           21 },
	{ "output_ss_list", required_argument,      NULL,           22 },
	{ "output-ss-list", required_argument,      NULL,           22 },
	{ "output_cxt",     required_argument,      NULL,           23 },
	{ "output-cxt",     required_argument,      NULL,           23 },
	{ "prune_empty",    required_argument,      NULL,           24 },
	{ "prune-empty",    no_argument,            NULL,           24 },
	{ "output_xio",     required_argument,      NULL,           25 },
	{ "output-xio",     required_argument,      NULL,           25 },
	{ "input_ss_list",  required_argument,      NULL,           26 },
	{ "input-ss-list",  required_argument,      NULL,           26 },
	{ NULL,             0,                      NULL,           0 }
    };

    /* Skip command "warp" */
    optind ++;

    while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
	switch (ch) {
	case 2:
	    strncpy (parms->input_fn, optarg, _MAX_PATH);
	    break;
	case 3:
	    strncpy (parms->output_fn, optarg, _MAX_PATH);
	    break;
	case 4:
	    strncpy (parms->vf_in_fn, optarg, _MAX_PATH);
	    break;
	case 5:
	    if (sscanf (optarg, "%f", &parms->default_val) != 1) {
		printf ("Error: default_val takes an argument\n");
		print_usage (argv[1]);
	    }
	    break;
	case 6:
	    strncpy (parms->xf_in_fn, optarg, _MAX_PATH);
	    break;
	case 7:
	    strncpy (parms->fixed_im_fn, optarg, _MAX_PATH);
	    break;
	case 8:
	    strncpy (parms->vf_out_fn, optarg, _MAX_PATH);
	    break;
	case 9:
	    if (!strcmp (optarg, "nn")) {
		parms->interp_lin = 0;
	    } else if (!strcmp (optarg, "linear")) {
		parms->interp_lin = 1;
	    } else {
		fprintf (stderr, "Error.  --interpolation must be either nn or linear.\n");
		print_usage (argv[1]);
	    }
	    break;
	case 10:
	    rc = sscanf (optarg, "%f %f %f", &parms->offset[0], &parms->offset[1], &parms->offset[2]);
	    if (rc != 3) {
		fprintf (stderr, "Error.  --offset requires 3 values.");
		print_usage (argv[1]);
	    }
	    have_offset = 1;
	    break;
	case 11:
	    rc = sscanf (optarg, "%f %f %f", &parms->spacing[0], &parms->spacing[1], &parms->spacing[2]);
	    if (rc != 3) {
		fprintf (stderr, "Error.  --spacing requires 3 values.");
		print_usage (argv[1]);
	    }
	    have_spacing = 1;
	    break;
	case 12:
	    rc = sscanf (optarg, "%d %d %d", &parms->dims[0], &parms->dims[1], &parms->dims[2]);
	    if (rc != 3) {
		fprintf (stderr, "Error.  --dims requires 3 values.");
		print_usage (argv[1]);
	    }
	    have_dims = 1;
	    break;
	case 13:
	    parms->output_format = plm_file_format_parse (optarg);
	    break;
	case 14:
	    strncpy (parms->ctatts_in_fn, optarg, _MAX_PATH);
	    break;
	case 15:
	    strncpy (parms->dif_in_fn, optarg, _MAX_PATH);
	    break;
	case 16:
	    if (!strcmp (optarg, "itk")) {
		parms->use_itk = 1;
	    } else {
		fprintf (stderr, "Error.  --algorithm option only supports itk.\n");
		print_usage (argv[1]);
	    }
	    break;
	case 17:
	    parms->output_type = plm_image_type_parse (optarg);
	    if (parms->output_type == PLM_IMG_TYPE_UNDEFINED) {
		fprintf (stderr, "Error, unknown output type %s\n", optarg);
		print_usage (argv[1]);
	    }
	    break;
	case 18:
	    strncpy (parms->dicom_dir, optarg, _MAX_PATH);
	    break;
	case 19:
	    strncpy (parms->output_prefix, optarg, _MAX_PATH);
	    break;
	case 20:
	    strncpy (parms->output_labelmap_fn, optarg, _MAX_PATH);
	    break;
	case 21:
	    strncpy (parms->output_ss_img_fn, optarg, _MAX_PATH);
	    break;
	case 22:
	    strncpy (parms->output_ss_list_fn, optarg, _MAX_PATH);
	    break;
	case 23:
	    strncpy (parms->output_cxt_fn, optarg, _MAX_PATH);
	    break;
	case 24:
	    parms->prune_empty = 1;
	    break;
	case 25:
	    strncpy (parms->output_xio_dirname, optarg, _MAX_PATH);
	    break;
	case 26:
	    strncpy (parms->input_ss_list_fn, optarg, _MAX_PATH);
	    break;
	default:
	    fprintf (stderr, "Error.  Unknown option.");
	    print_usage (argv[1]);
	    break;
	}
    }
    if (!parms->input_fn[0]) {
	print_usage (argv[1]);
    }
#if defined (commentout)
    if (!parms->input_fn[0] || !parms->output_fn[0]) {
	print_usage (argv[1]);
    }
#endif
}

void
load_input_files (Rtds *rtds, Plm_file_format file_type, Warp_parms *parms)
{
    switch (file_type) {
    case PLM_FILE_FMT_NO_FILE:
	print_and_exit ("Could not open input file %s for read\n",
	    parms->input_fn);
	break;
    case PLM_FILE_FMT_UNKNOWN:
    case PLM_FILE_FMT_IMG:
	rtds->m_img = plm_image_load_native (parms->input_fn);
	//warp_image_main (&parms);
	break;
    case PLM_FILE_FMT_DICOM_DIR:
	/* GCS FIX: Need to load rtss too */
	rtds->m_img = plm_image_load_native (parms->input_fn);
	//warp_image_main (&parms);
	break;
    case PLM_FILE_FMT_XIO_DIR:
	rtds->load_xio (parms->input_fn);
	//xio_warp_main (&parms);
	break;
    case PLM_FILE_FMT_DIJ:
	print_and_exit ("Warping dij files requires ctatts_in, dif_in files\n");
	break;
    case PLM_FILE_FMT_DICOM_RTSS:
	rtds->m_cxt = cxt_create ();
	gdcm_rtss_load (rtds->m_cxt, parms->input_fn, parms->dicom_dir);
	printf ("gdcm_rtss_load complete.\n");
	//rtss_warp (&parms);
	break;
    case PLM_FILE_FMT_CXT:
	//ctx_warp (&parms);
	break;
    default:
	print_and_exit (
	    "Sorry, don't know how to convert/warp input type %s (%s)\n",
	    plm_file_format_string (file_type),
	    parms->input_fn);
	break;
    }
}

void
save_ss_output (Rtds *rtds,  Warp_parms *parms)
{
    if (!rtds->m_cxt) {
	return;
    }

    if (parms->output_fn[0]) {
	/* If user didn't specify output format, see if we can guess from 
	   filename extension */
	if (parms->output_format == PLM_FILE_FMT_UNKNOWN) {
	    parms->output_format = plm_file_format_from_extension (
		parms->output_fn);
	}

	/* Save output */
	switch (parms->output_format) {
	case PLM_FILE_FMT_CXT:
	    cxt_write (rtds->m_cxt, parms->output_fn, true);
	    break;
	case PLM_FILE_FMT_DICOM_RTSS:
	case PLM_FILE_FMT_DICOM_DIR:
	    cxt_adjust_structure_names (rtds->m_cxt);
	    gdcm_rtss_save (rtds->m_cxt, parms->output_fn, parms->dicom_dir);
	    break;
	case PLM_FILE_FMT_IMG:
	default:
	    cxt_to_mha_write (rtds->m_cxt, parms);
	    break;
	}
    }
}

void
warp_rtds (Rtds *rtds, Plm_file_format file_type, Warp_parms *parms)
{
    DeformationFieldType::Pointer vf = DeformationFieldType::New();
    Xform xform;
    PlmImageHeader pih;

    /* Load input file(s) */
    load_input_files (rtds, file_type, parms);

    printf ("checkpoint 1.\n");

    /* Load transform */
    if (parms->xf_in_fn[0]) {
	printf ("Loading xform (%s)\n", parms->xf_in_fn);
	load_xform (&xform, parms->xf_in_fn);
    }

    /* Try to guess the proper dimensions and spacing for output image */
    if (parms->fixed_im_fn[0]) {
	/* use the spacing of user-supplied fixed image */
	FloatImageType::Pointer fixed = load_float (parms->fixed_im_fn, 0);
	pih.set_from_itk_image (fixed);
    } else if (xform.m_type == XFORM_ITK_VECTOR_FIELD) {
	/* use the spacing from input vector field */
	pih.set_from_itk_image (xform.get_itk_vf());
    } else if (xform.m_type == XFORM_GPUIT_BSPLINE) {
	/* use the spacing from input bxf file */
	pih.set_from_gpuit_bspline (xform.get_gpuit_bsp());
    } else if (rtds->m_img) {
	/* use the spacing of the input image */
	pih.set_from_plm_image (rtds->m_img);
    } else if (rtds->m_cxt) {
	/* use the spacing of the structure set */
	pih.set_from_gpuit (rtds->m_cxt->offset, rtds->m_cxt->spacing, 
	    rtds->m_cxt->dim, 0);
    } else {
	/* out of options?  :( */
	print_and_exit ("Sorry, I couldn't determine the output geometry\n");
    }

    printf ("PIH is:\n");
    pih.print ();

    /* Set the geometry */
    if (rtds->m_cxt) {
	cxt_set_geometry_from_plm_image_header (rtds->m_cxt, &pih);
    }

    /* Do the warp */
    if (parms->xf_in_fn[0] && rtds->m_img) {
	PlmImage *im_out;
	im_out = new PlmImage;
	plm_warp (im_out, &vf, &xform, &pih, rtds->m_img, parms->default_val, 
	    parms->use_itk, parms->interp_lin);
	delete rtds->m_img;
	rtds->m_img = im_out;
    }

    /* Save output image */
    if (parms->output_fn[0] && rtds->m_img) {
	printf ("Saving image...\n");
	switch (parms->output_format) {
	case PLM_FILE_FMT_DICOM_DIR:
	    rtds->m_img->save_short_dicom (parms->output_fn);
	    break;
	default:
	    rtds->m_img->save_image (parms->output_fn);
	    break;
	}
    }

    /* Save output vector field */
    if (parms->xf_in_fn[0] && parms->vf_out_fn[0]) {
	printf ("Saving vf...\n");
	itk_image_save (vf, parms->vf_out_fn);
    }

    /* Save output structure set */
    save_ss_output (rtds, parms);

}

void
do_command_warp (int argc, char* argv[])
{
    Warp_parms parms;
    Plm_file_format file_type;
    Rtds rtds;

    /* Parse command line parameters */
    warp_parse_args (&parms, argc, argv);

    /* Dij matrices are a special case */
    if (parms.ctatts_in_fn[0] && parms.dif_in_fn[0]) {
	warp_dij_main (&parms);
	return;
    }

    /* What is the input file type? */
    file_type = plm_file_format_deduce (parms.input_fn);

    /* Pointsets are a special case */
    if (file_type == PLM_FILE_FMT_POINTSET) {
	warp_pointset_main (&parms);
	return;
    }

    /* Process warp */
    warp_rtds (&rtds, file_type, &parms);

    printf ("Finished!\n");
}
