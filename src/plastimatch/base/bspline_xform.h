/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _bspline_xform_h_
#define _bspline_xform_h_

#include "plmbase_config.h"
#include "direction_cosines.h"
#include "smart_pointer.h"
#include "plm_int.h"

//TODO: Change type of dc to Direction_cosines*

//class Direction_cosines;
class Volume;
class Volume_header;

class PLMBASE_API Bspline_xform {
public:
    SMART_POINTER_SUPPORT (Bspline_xform);
public:
    Bspline_xform ();
    ~Bspline_xform ();
public:
    float img_origin[3];         /* Image origin (in mm) */
    float img_spacing[3];        /* Image spacing (in mm) */
    plm_long img_dim[3];         /* Image size (in vox) */
    Direction_cosines dc;        /* Image direction cosines */
    plm_long roi_offset[3];	 /* Position of first vox in ROI (in vox) */
    plm_long roi_dim[3];	 /* Dimension of ROI (in vox) */
    plm_long vox_per_rgn[3];	 /* Knot spacing (in vox) */
    float grid_spac[3];          /* Knot spacing (in mm) */
    plm_long rdims[3];           /* # of regions in (x,y,z) */
    plm_long cdims[3];           /* # of knots in (x,y,z) */
    int num_knots;               /* Total number of knots (= product(cdims)) */
    int num_coeff;               /* Total number of coefficents (= product(cdims) * 3) */
    float* coeff;                /* Coefficients.  Vector directions interleaved. */

    /* Aligned grid (3D) LUTs */
    plm_long* cidx_lut;          /* Lookup volume for region number */
    plm_long* c_lut;             /* Lookup table for control point indices */
    plm_long* qidx_lut;          /* Lookup volume for region offset */
    float* q_lut;                /* Lookup table for influence multipliers */

    /* Non-aligned grid (1D) LUTs */
    float *bx_lut;               /* LUT for influence multiplier in x dir */
    float *by_lut;               /* LUT for influence multiplier in y dir */
    float *bz_lut;               /* LUT for influence multiplier in z dir */

    /* Used by bspline_regularize.c */
    float* q_dxdyz_lut;          /* LUT for influence of dN1/dx*dN2/dy*N3 */
    float* q_xdydz_lut;          /* LUT for influence of N1*dN2/dy*dN3/dz */
    float* q_dxydz_lut;          /* LUT for influence of dN1/dx*N2*dN3/dz */
    float* q_d2xyz_lut;          /* LUT for influence of (d2N1/dx2)*N2*N3 */
    float* q_xd2yz_lut;          /* LUT for influence of N1*(d2N2/dy2)*N3 */
    float* q_xyd2z_lut;          /* LUT for influence of N1*N2*(d2N3/dz2) */

public:
    void fill_coefficients (float val);
    void get_volume_header (Volume_header *vh);
};

//PLMBASE_C_API void bspline_xform_set_default (Bspline_xform* bxf);
PLMBASE_C_API void bspline_xform_initialize (
        Bspline_xform* bxf,	          /* Output: bxf is initialized */
        float img_origin[3],          /* Image origin (in mm) */
        float img_spacing[3],         /* Image spacing (in mm) */
        plm_long img_dim[3],          /* Image size (in vox) */
        plm_long roi_offset[3],       /* Position of first vox in ROI (in vox) */
        plm_long roi_dim[3],	      /* Dimension of ROI (in vox) */
        plm_long vox_per_rgn[3],      /* Knot spacing (in vox) */
        float direction_cosines[9]    /* Direction cosines */
);
//PLMBASE_C_API void bspline_xform_free (Bspline_xform* bxf);
PLMBASE_C_API Bspline_xform* bspline_xform_load (const char* filename);
PLMBASE_C_API void bspline_xform_save (Bspline_xform* bxf, const char* filename);
PLMBASE_C_API void bspline_set_coefficients (Bspline_xform* bxf, float val);

/* Debugging routines */
PLMBASE_C_API void bspline_xform_dump_coeff (Bspline_xform* bxf, const char* fn);
PLMBASE_C_API void bspline_xform_dump_luts (Bspline_xform* bxf);


#endif
