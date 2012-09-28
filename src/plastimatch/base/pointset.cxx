/* -----------------------------------------------------------------------
   See COPYRIGHT.TXT and LICENSE.TXT for copyright and license information
   ----------------------------------------------------------------------- */
#include "plmbase_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "plmbase.h"

#include "file_util.h"
#include "plm_math.h"
#include "print_and_exit.h"
#include "string_util.h"

template<class T>
void
Pointset<T>::load (const char *fn)
{
    if (extension_is (fn, ".fcsv")) {
	this->load_fcsv (fn);
    } else {
	this->load_txt (fn);
    }
}

template<class T>
void
Pointset<T>::load_fcsv (const char *fn)
{
    FILE *fp;
    char s[1024];

    fp = fopen (fn, "r");
    if (!fp) {
	return;
    }

    /* Got an fcsv file.  Parse it. */
    while (!feof(fp)) {
	float lm[3];
	int land_sel, land_vis;
	int rc;

        fgets (s, 1024, fp);
	if (feof(fp)) break;
        if (s[0]=='#') continue;

	char buf[1024];
        rc = sscanf (s, "%1023[^,],%f,%f,%f,%d,%d\n", buf, 
	    &lm[0], &lm[1], &lm[2], &land_sel, &land_vis);
	if (rc < 4) {
	    /* Error parsing file */
	    point_list.clear();
	    return;
	}

	/* Note: Plastimatch landmarks are in LPS coordinates. 
	   Slicer landmarks are in RAS coordinates. 
	   Change RAS to LPS (note that LPS == ITK RAI). */
	T lp;
	lp.set_label (buf);
	lp.p[0] = - lm[0];
	lp.p[1] = - lm[1];
	lp.p[2] = lm[2];
	point_list.push_back (lp);
    }
    fclose (fp);
}

template<class T>
void
Pointset<T>::load_txt (const char *fn)
{
    FILE *fp;
    char s[1024];

    fp = fopen (fn, "r");
    if (!fp) {
	return;
    }

    /* Parse as txt file */
    while (!feof(fp)) {
	float lm[3];
	int rc;

        fgets (s, 1024, fp);
	if (feof(fp)) break;
        if (s[0]=='#') continue;

        rc = sscanf (s, "%f , %f , %f\n", &lm[0], &lm[1], &lm[2]);
	if (rc != 3) {
	    rc = sscanf (s, "%f %f %f\n", &lm[0], &lm[1], &lm[2]);
	}
	if (rc != 3) {
	    print_and_exit ("Error parsing landmark file: %s\n", fn);
	}

	/* Assume LPS */
	T lp;
	lp.set_label ("");
	lp.p[0] = lm[0];
	lp.p[1] = lm[1];
	lp.p[2] = lm[2];
	point_list.push_back (lp);
    }
    fclose (fp);
}

template<class T>
void
Pointset<T>::set_ras (const Pstring& p)
{
    int loc = 0;
    while (1) {
        int rc;
        float f1, f2, f3;
        rc = sscanf (p.c_str() + loc, "%f,%f,%f", &f1, &f2, &f3);
        if (rc != 3) {
            break;
        }
        this->insert_ras ("", f1, f2, f3);
        rc = p.findchr (';', loc);
        if (rc == BSTR_ERR) {
            break;
        }
        loc += rc + 1;
    }
}

template<class T>
void
Pointset<T>::insert_ras (
    const std::string& label,
    float x,
    float y,
    float z
)
{
    /* RAS to LPS adjustment */
    this->point_list.push_back (T (label, -x, -y, z));
}

template<class T>
void
Pointset<T>::insert_lps (
    const std::string& label,
    float x,
    float y,
    float z
)
{
    /* No RAS to LPS adjustment */
    this->point_list.push_back (T (label, x, y, z));
}

template<class T>
void
Pointset<T>::save (const char *fn)
{
    if (extension_is (fn, ".fcsv")) {
	this->save_fcsv (fn);
    } else {
	this->save_txt (fn);
    }
}

template<class T>
void
Pointset<T>::save_fcsv (const char *fn)
{
    FILE *fp;

    printf ("Trying to save: %s\n", (const char*) fn);
    make_directory_recursive (fn);
    fp = fopen (fn, "w");
    if (!fp) return;

    fprintf (fp, 
	"# Fiducial List file %s\n"
	"# version = 2\n"
	"# name = plastimatch-fiducials\n"
	"# numPoints = %d\n"
	"# symbolScale = 5\n"
	"# symbolType = 12\n"
	"# visibility = 1\n"
	"# textScale = 4.5\n"
	"# color = 0.4,1,1\n"
	"# selectedColor = 1,0.5,0.5\n"
	"# opacity = 1\n"
	"# ambient = 0\n"
	"# diffuse = 1\n"
	"# specular = 0\n"
	"# power = 1\n"
	"# locked = 0\n"
	"# numberingScheme = 0\n"
	"# columns = label,x,y,z,sel,vis\n",
	fn, 
	(int) this->point_list.size());

    for (unsigned int i = 0; i < this->point_list.size(); i++) {
	const T& lp = this->point_list[i];
	if (lp.get_label() == "") {
	    fprintf (fp, "p-%03d", i);
	} else {
	    fprintf (fp, "%s", lp.get_label().c_str());
	}
	/* Note: Plastimatch landmarks are in LPS coordinates. 
	   Slicer landmarks are in RAS coordinates. 
	   Change LPS to RAS (note that LPS == ITK RAI). */
	fprintf (fp, ",%f,%f,%f,1,1\n", 
	    - lp.p[0], 
	    - lp.p[1], 
	    lp.p[2]);
    }
    fclose (fp);
}

template<class T>
void
Pointset<T>::save_txt (const char *fn)
{
    FILE *fp;

    printf ("Trying to save: %s\n", (const char*) fn);
    make_directory_recursive (fn);
    fp = fopen (fn, "w");
    if (!fp) return;

    for (unsigned int i = 0; i < this->point_list.size(); i++) {
	const T& lp = this->point_list[i];
	fprintf (fp, "%f %f %f\n", lp.p[0], lp.p[1], lp.p[2]);
    }
    fclose (fp);
}

template<class T>
size_t
Pointset<T>::count (void) const
{
    return (size_t) this->point_list.size();
}

template<class T>
void
Pointset<T>::truncate (size_t new_length)
{
    this->point_list.resize (new_length);
}

template class PLMBASE_API Pointset<Labeled_point>;
template class PLMBASE_API Pointset<Point>;
