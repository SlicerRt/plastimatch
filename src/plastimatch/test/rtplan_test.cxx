/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plm_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler_warnings.h"
#include "dcmtk_rt_study.h"

int
main (int argc, char *argv[])
{
    char *dicom_dir;
    UNUSED_VARIABLE (dicom_dir);
    if (argc == 2) {
	dicom_dir = argv[1];
    } else {
	printf ("Usage: rtplan_test output_dir\n");
	exit (1);
    }

    Dcmtk_rt_study drs;
    Rt_study_metadata::Pointer rsm = Rt_study_metadata::New ();
    Rtplan::Pointer rtplan = Rtplan::New ();

    drs.set_rt_study_metadata (rsm);
    drs.set_rtplan (rtplan);

    /* Fill in metadata */
    rsm->set_patient_name ("Test^Rtplan");

    /* Fill in plan data */

    /* Save to file */
    drs.save (dicom_dir);
    
    return 0;
}
