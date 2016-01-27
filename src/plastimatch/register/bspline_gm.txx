/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _bspline_gm_txx_
#define _bspline_gm_txx_

class Bspline_gm_k
{
public:
    float *m_grad;
    /* GCS: Oct 5, 2009.  We have determined that sequential accumulation
       of the score requires double precision.  However, reduction 
       accumulation does not. */
    double score_acc;
public:
    Bspline_gm_k (Bspline_optimize *bod)
    {
        Bspline_parms *parms = bod->get_bspline_parms ();
        Volume *moving_grad = parms->moving_grad;
        m_grad = (float*) moving_grad->img;
        score_acc = 0.;
    }
public:
    void
    loop_function (
        Bspline_optimize *bod,    /* In/out: generic optimization data */
        Bspline_xform *bxf,       /* Input:  coefficient values */
        Bspline_state *bst,       /* Input:  state of bspline */
        Bspline_score *ssd,       /* In/out: score and gradient */
        const Volume *fixed,      /* Input:  fixed image */
        const Volume *moving,     /* Input:  moving image */
        const float *f_img,       /* Input:  raw intensity array for fixed */
        const float *m_img,       /* Input:  raw intensity array for moving */
        plm_long fidx,            /* Input:  index of voxel in fixed image */
        plm_long midx_f,          /* Input:  index (floor) in moving image*/
        plm_long mijk_r[3],       /* Input:  coords (rounded) in moving image*/
        plm_long pidx,            /* Input:  region index of fixed voxel */
        plm_long qidx,            /* Input:  offset index of fixed voxel */
        float li_1[3],            /* Input:  linear interpolation fraction */
        float li_2[3]             /* Input:  linear interpolation fraction */
    )
    {
        float m_val;
        float dc_dv[3];

        /* Get value in fixed image */
        float f_val = f_img[fidx];

        /* Compute moving image intensity using linear interpolation */
        /* Macro is slightly faster than function */
        LI_VALUE (m_val, 
            li_1[0], li_2[0],
            li_1[1], li_2[1],
            li_1[2], li_2[2],
            midx_f, m_img, moving);

        /* This replaces the commented out code */
        plm_long mvr = volume_index (moving->dim, mijk_r);

        /* Compute intensity difference */
        float diff = m_val - f_val;

        /* Update score */
        this->score_acc += diff * diff;

        /* Compute spatial gradient using nearest neighbors */
        dc_dv[0] = diff * m_grad[3*mvr+0];  /* x component */
        dc_dv[1] = diff * m_grad[3*mvr+1];  /* y component */
        dc_dv[2] = diff * m_grad[3*mvr+2];  /* z component */

        /* Update cost function gradient */
        ssd->update_smetric_grad_b (bxf, pidx, qidx, dc_dv);
        ssd->num_vox++;
    }
};

#endif
