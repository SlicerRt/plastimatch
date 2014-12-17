/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _rtss_roi_h_
#define _rtss_roi_h_

#include "plmbase_config.h"
#include "pstring.h"

class Rtss_contour;

class PLMBASE_API Rtss_roi {
public:
    Pstring name;
    Pstring color;
    int id;                    /* Used for import/export (must be >= 1) */
    int bit;                   /* Used for ss-img (-1 for no bit) */
    size_t num_contours;
    Rtss_contour** pslist;
public:
    Rtss_roi ();
    ~Rtss_roi ();

    void clear ();
    Rtss_contour* add_polyline ();
    void set_color (const char* color_string);
    void get_dcm_color_string (Pstring *dcm_color) const;
    void structure_rgb (int *r, int *g, int *b) const;

    static void adjust_name (Pstring *name_out, const Pstring *name_in);
};


#endif
