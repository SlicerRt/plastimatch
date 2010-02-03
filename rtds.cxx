/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rtds.h"
#include "xio_ct.h"
#include "xio_dir.h"
#include "xio_io.h"
#include "xio_structures.h"

void Rtds::load_xio (char *xio_dir)
{
    Xio_dir *xd;
    Xio_patient_dir *xpd;
    Xio_studyset_dir *xsd;

    xd = xio_dir_create (xio_dir);

    if (xd->num_patient_dir <= 0) {
	print_and_exit ("Error, xio num_patient_dir = %d\n", 
	    xd->num_patient_dir);
    }
    xpd = &xd->patient_dir[0];
    if (xd->num_patient_dir > 1) {
	printf ("Warning: multiple patients found in xio directory.\n"
	    "Defaulting to first directory: %s\n", xpd->path);
    }
    if (xpd->num_studyset_dir <= 0) {
	print_and_exit ("Error, xio patient has no studyset.");
    }
    xsd = &xpd->studyset_dir[0];
    if (xpd->num_studyset_dir > 1) {
	printf ("Warning: multiple studyset found in xio patient directory.\n"
	    "Defaulting to first directory: %s\n", xsd->path);
    }

    /* Load the input image */
    this->m_img = new PlmImage;
    xio_ct_load (this->m_img, xsd->path);

    /* Load the structure set */
    this->m_cxt = cxt_create ();
    printf ("calling xio_structures_load\n");
    xio_structures_load (this->m_cxt, xsd->path, 0, 0);

    /* Copy geometry from Xio CT to structures */
    printf ("calling cxt_set_geometry_from_plm_image\n");
    cxt_set_geometry_from_plm_image (this->m_cxt, this->m_img);
}
