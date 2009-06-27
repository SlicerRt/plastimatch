/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#ifndef _readcxt_h_
#define _readcxt_h_

#define CXT_BUFLEN 2048

typedef struct polyline POLYLINE;
struct polyline {
    int slice_no;
    int num_vertices;
    float* x;
    float* y;
    float* z;
};

typedef struct structure STRUCTURE;
struct structure {
    char name[CXT_BUFLEN];
    int num_contours;
    POLYLINE* pslist;
};

typedef struct structure_list STRUCTURE_List;
struct structure_list {
    int dim[3];
    float spacing[3];
    float offset[3];
    int num_structures;
    STRUCTURE* slist;
};


#if defined __cplusplus
extern "C" {
#endif

plastimatch1_EXPORT
void
cxt_initialize (STRUCTURE_List* structures);
plastimatch1_EXPORT
void
cxt_read (STRUCTURE_List* structures, const char* cxt_fn);
plastimatch1_EXPORT
void
cxt_write (STRUCTURE_List* structures, const char* cxt_fn);

#if defined __cplusplus
}
#endif

#endif
