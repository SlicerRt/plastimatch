/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _interpolate_h_
#define _interpolate_h_

#include "plm_config.h"
#include "volume.h"

/* -----------------------------------------------------------------------
   Macros
   ----------------------------------------------------------------------- */
#define LI_CLAMP_3D(							\
    mijk, mijk_f, mijk_r,						\
    li_frac_1, li_frac_2, moving)					\
    do {								\
	li_clamp (mijk[0], moving->dim[0]-1,				\
	    &mijk_f[0], &mijk_r[0],					\
	    &li_frac_1[0], &li_frac_2[0]);				\
	li_clamp (mijk[1], moving->dim[1]-1,				\
	    &mijk_f[1], &mijk_r[1],					\
	    &li_frac_1[1], &li_frac_2[1]);				\
	li_clamp (mijk[2], moving->dim[2]-1,				\
	    &mijk_f[2], &mijk_r[2],					\
	    &li_frac_1[2], &li_frac_2[2]);				\
    } while (0)

#define LI_VALUE(m_val, fx1, fx2, fy1, fy2, fz1, fz2, mvf, m_img, moving) \
    do {								\
	float m_x1y1z1, m_x2y1z1, m_x1y2z1, m_x2y2z1;			\
	float m_x1y1z2, m_x2y1z2, m_x1y2z2, m_x2y2z2;			\
									\
	m_x1y1z1 = fx1 * fy1 * fz1 * m_img[mvf];			\
	m_x2y1z1 = fx2 * fy1 * fz1 * m_img[mvf+1];			\
	m_x1y2z1 = fx1 * fy2 * fz1 * m_img[mvf+moving->dim[0]];		\
	m_x2y2z1 = fx2 * fy2 * fz1 * m_img[mvf+moving->dim[0]+1];	\
	m_x1y1z2 = fx1 * fy1 * fz2 * m_img[mvf+moving->dim[1]*moving->dim[0]]; \
	m_x2y1z2 = fx2 * fy1 * fz2 * m_img[mvf+moving->dim[1]*moving->dim[0]+1]; \
	m_x1y2z2 = fx1 * fy2 * fz2 * m_img[mvf+moving->dim[1]*moving->dim[0]+moving->dim[0]]; \
	m_x2y2z2 = fx2 * fy2 * fz2 * m_img[mvf+moving->dim[1]*moving->dim[0]+moving->dim[0]+1]; \
	m_val = m_x1y1z1 + m_x2y1z1 + m_x1y2z1 + m_x2y2z1		\
		+ m_x1y1z2 + m_x2y1z2 + m_x1y2z2 + m_x2y2z2;		\
    } while (0)

#define LI_VALUE_ALT(m_val, fx1, fx2, fy1, fy2, fz1, fz2,		\
    mvf, m_img, moving, plane)						\
    do {								\
	int idx_x1y1z1, idx_x2y1z1, idx_x1y2z1, idx_x2y2z1;		\
	int idx_x1y1z2, idx_x2y1z2, idx_x1y2z2, idx_x2y2z2;		\
	float m_x1y1z1, m_x2y1z1, m_x1y2z1, m_x2y2z1;			\
	float m_x1y1z2, m_x2y1z2, m_x1y2z2, m_x2y2z2;			\
									\
	idx_x1y1z1 = mvf;						\
	idx_x2y1z1 = mvf+1;						\
	idx_x1y2z1 = mvf+moving->dim[0];				\
	idx_x2y2z1 = mvf+moving->dim[0+1];				\
	idx_x1y1z2 = mvf+moving->dim[1]*moving->dim[0];			\
	idx_x2y1z2 = mvf+moving->dim[1]*moving->dim[0]+1;		\
	idx_x1y2z2 = mvf+moving->dim[1]*moving->dim[0]+moving->dim[0];	\
	idx_x2y2z2 = mvf+moving->dim[1]*moving->dim[0]+moving->dim[0]+1; \
									\
	m_x1y1z1 = fx1 * fy1 * fz1 * m_img[idx_x1y1z1];			\
	m_x2y1z1 = fx2 * fy1 * fz1 * m_img[idx_x2y1z1];			\
	m_x1y2z1 = fx1 * fy2 * fz1 * m_img[idx_x1y2z1];			\
	m_x2y2z1 = fx2 * fy2 * fz1 * m_img[idx_x2y2z1];			\
	m_x1y1z2 = fx1 * fy1 * fz2 * m_img[idx_x1y1z2];			\
	m_x2y1z2 = fx2 * fy1 * fz2 * m_img[idx_x2y1z2];			\
	m_x1y2z2 = fx1 * fy2 * fz2 * m_img[idx_x1y2z2];			\
	m_x2y2z2 = fx2 * fy2 * fz2 * m_img[idx_x2y2z2];			\
	m_val = m_x1y1z1 + m_x2y1z1 + m_x1y2z1 + m_x2y2z1		\
		+ m_x1y1z2 + m_x2y1z2 + m_x1y2z2 + m_x2y2z2;		\
    } while (0)

#define LI_VALUE_INTERLEAVED(m_val, fx1, fx2, fy1, fy2, fz1, fz2,	\
    mvf, m_img, moving, plane)						\
    do {								\
	size_t idx_x1y1z1, idx_x2y1z1, idx_x1y2z1, idx_x2y2z1;		\
	size_t idx_x1y1z2, idx_x2y1z2, idx_x1y2z2, idx_x2y2z2;		\
	float m_x1y1z1, m_x2y1z1, m_x1y2z1, m_x2y2z1;			\
	float m_x1y1z2, m_x2y1z2, m_x1y2z2, m_x2y2z2;			\
									\
	idx_x1y1z1 = mvf;						\
	idx_x2y1z1 = mvf+1;						\
	idx_x1y2z1 = mvf+moving->dim[0];				\
	idx_x2y2z1 = mvf+moving->dim[0+1];				\
	idx_x1y1z2 = mvf+moving->dim[1]*moving->dim[0];			\
	idx_x2y1z2 = mvf+moving->dim[1]*moving->dim[0]+1;		\
	idx_x1y2z2 = mvf+moving->dim[1]*moving->dim[0]+moving->dim[0];	\
	idx_x2y2z2 = mvf+moving->dim[1]*moving->dim[0]+moving->dim[0]+1; \
									\
	idx_x1y1z1 = idx_x1y1z1 * moving->vox_planes + plane;		\
	idx_x2y1z1 = idx_x2y1z1 * moving->vox_planes + plane;		\
	idx_x1y2z1 = idx_x1y2z1 * moving->vox_planes + plane;		\
	idx_x2y2z1 = idx_x2y2z1 * moving->vox_planes + plane;		\
	idx_x1y1z2 = idx_x1y1z2 * moving->vox_planes + plane;		\
	idx_x2y1z2 = idx_x2y1z2 * moving->vox_planes + plane;		\
	idx_x1y2z2 = idx_x1y2z2 * moving->vox_planes + plane;		\
	idx_x2y2z2 = idx_x2y2z2 * moving->vox_planes + plane;		\
									\
	m_x1y1z1 = fx1 * fy1 * fz1 * m_img[idx_x1y1z1];			\
	m_x2y1z1 = fx2 * fy1 * fz1 * m_img[idx_x2y1z1];			\
	m_x1y2z1 = fx1 * fy2 * fz1 * m_img[idx_x1y2z1];			\
	m_x2y2z1 = fx2 * fy2 * fz1 * m_img[idx_x2y2z1];			\
	m_x1y1z2 = fx1 * fy1 * fz2 * m_img[idx_x1y1z2];			\
	m_x2y1z2 = fx2 * fy1 * fz2 * m_img[idx_x2y1z2];			\
	m_x1y2z2 = fx1 * fy2 * fz2 * m_img[idx_x1y2z2];			\
	m_x2y2z2 = fx2 * fy2 * fz2 * m_img[idx_x2y2z2];			\
	m_val = m_x1y1z1 + m_x2y1z1 + m_x1y2z1 + m_x2y2z1		\
		+ m_x1y1z2 + m_x2y1z2 + m_x1y2z2 + m_x2y2z2;		\
    } while (0)


/* -----------------------------------------------------------------------
   Function prototypes
   ----------------------------------------------------------------------- */
#if defined __cplusplus
extern "C" {
#endif

plmsys_EXPORT
void
li_clamp (float ma, size_t dmax, size_t* maf, size_t* mar, 
    float* fa1, float* fa2);
plmsys_EXPORT
void
li_clamp_3d (float mijk[3], size_t mijk_f[3], size_t mijk_r[3],
    float li_frac_1[3], float li_frac_2[3],
    Volume *moving);
plmsys_EXPORT
float
li_value (float fx1, float fx2, float fy1, float fy2, 
    float fz1, float fz2, size_t mvf, 
    float *m_img, Volume *moving);

#if defined __cplusplus
}
#endif

#endif
