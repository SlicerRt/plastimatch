/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef __mathutil_h__
#define __mathutil_h__

#include "plm_config.h"
#include <string.h>

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2         1.41421356237309504880
#endif
#ifndef M_SQRTPI
#define M_SQRTPI        1.77245385090551602792981
#endif
#ifndef M_TWOPI
#define M_TWOPI         (M_PI * 2.0)
#endif


/* Returns integer data type */
#define ROUND_INT(x) ((x > 0) ? (int)(x+0.5) : (int)(ceil(x-0.5)))


/* Primatives */
static inline void vec3_add2 (double* v1, const double* v2) {
    v1[0] += v2[0]; v1[1] += v2[1]; v1[2] += v2[2];
}

static inline void vec3_add3 (double* v1, const double* v2, const double* v3) {
    v1[0] = v2[0] + v3[0]; v1[1] = v2[1] + v3[1]; v1[2] = v2[2] + v3[2];
}

static inline void vec3_copy (double* v1, const double* v2) {
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
}

static inline void vec4_copy (double* v1, const double* v2) {
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2]; v1[3] = v2[3];
}

static inline double vec3_dot (const double* v1, const double* v2) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static inline double vec4_dot (const double* v1, const double* v2) {
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
}

static inline void vec3_scale2 (double* v1, double a) {
    v1[0] *= a; v1[1] *= a; v1[2] *= a;
}

static inline void vec3_scale3 (double* v1, const double* v2, double a) {
    v1[0] = a * v2[0]; v1[1] = a * v2[1]; v1[2] = a * v2[2];
}

static inline void vec3_sub2 (double* v1, const double* v2) {
    v1[0] -= v2[0]; v1[1] -= v2[1]; v1[2] -= v2[2];
}

static inline void vec3_sub3 (double* v1, const double* v2, const double* v3) {
    v1[0] = v2[0] - v3[0]; v1[1] = v2[1] - v3[1]; v1[2] = v2[2] - v3[2];
}

static inline void vec_zero (double* v1, int n) {
    memset (v1, 0, n*sizeof(double));
}

/* Length & distance */
static inline double vec3_len (const double* v1) {
    return sqrt(vec3_dot(v1,v1));
}

static inline void vec3_normalize1 (double* v1) {
    vec3_scale2 (v1, 1 / vec3_len(v1));
}

static inline double vec3_dist (const double* v1, const double* v2) {
    double tmp[3];
    vec3_sub3 (tmp, v1, v2);
    return vec3_len(tmp);
}

/* Cross product */
static inline void vec3_cross (double* v1, const double* v2, const double* v3)
{
    v1[0] = v2[1] * v3[2] - v2[2] * v3[1];
    v1[1] = v2[2] * v3[0] - v2[0] * v3[2];
    v1[2] = v2[0] * v3[1] - v2[1] * v3[0];
}


/* Matrix ops */

/* Matrix element m[i,j] for matrix with c columns */
#define m_idx(m1,c,i,j) m1[i*c+j]

/* v1 = m2 * v3 */
static inline void mat43_mult_vec3 (double* v1, const double* m2, const double* v3) {
    v1[0] = vec4_dot(&m2[0], v3);
    v1[1] = vec4_dot(&m2[4], v3);
    v1[2] = vec4_dot(&m2[8], v3);
}

/* m1 = m2 * m3 */
static inline void mat_mult_mat (double* m1, 
				 const double* m2, int m2_rows, int m2_cols, 
				 const double* m3, int m3_rows, int m3_cols)
{
    int i,j,k;
    for (i = 0; i < m2_rows; i++) {
	for (j = 0; j < m3_cols; j++) {
	    double acc = 0.0;
	    for (k = 0; k < m2_cols; k++) {
		acc += m_idx(m2,m2_cols,i,k) * m_idx(m3,m3_cols,k,j);
	    }
	    m_idx(m1,m3_cols,i,j) = acc;
	}
    }
}


#endif /* __mathutil_h__ */
