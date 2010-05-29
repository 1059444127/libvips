/* vertex split subdivision followed by quadratic b-spline smoothing
 *
 * C. Racette 23-28/05/2010
 *
 */

/*

    This file is part of VIPS.

    VIPS is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
 * 2010 (c) Chantal Racette, Nicolas Robidoux, John Cupitt.
 *
 * Nicolas Robidoux thanks Adam Turcotte, Geert Jordaens, Ralf Meyer,
 * Øyvind Kolås, Minglun Gong, Eric Daoust and Sven Neumann for useful
 * comments and code.
 *
 * Chantal Racette's image resampling research and programming funded
 * in part by a NSERC Discovery Grant awarded to Julien Dompierre
 * (20-61098).
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>

#include <vips/vips.h>
#include <vips/internal.h>

#include "templates.h"

#define VIPS_TYPE_INTERPOLATE_VSQBS \
	(vips_interpolate_vsqbs_get_type())
#define VIPS_INTERPOLATE_VSQBS( obj ) \
	(G_TYPE_CHECK_INSTANCE_CAST( (obj), \
	VIPS_TYPE_INTERPOLATE_VSQBS, VipsInterpolateVsqbs ))
#define VIPS_INTERPOLATE_VSQBS_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_CAST( (klass), \
	VIPS_TYPE_INTERPOLATE_VSQBS, VipsInterpolateVsqbsClass))
#define VIPS_IS_INTERPOLATE_VSQBS( obj ) \
	(G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_INTERPOLATE_VSQBS ))
#define VIPS_IS_INTERPOLATE_VSQBS_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_INTERPOLATE_VSQBS ))
#define VIPS_INTERPOLATE_VSQBS_GET_CLASS( obj ) \
	(G_TYPE_INSTANCE_GET_CLASS( (obj), \
	VIPS_TYPE_INTERPOLATE_VSQBS, VipsInterpolateVsqbsClass ))

typedef struct _VipsInterpolateVsqbs {
	VipsInterpolate parent_object;

} VipsInterpolateVsqbs;

typedef struct _VipsInterpolateVsqbsClass {
	VipsInterpolateClass parent_class;

} VipsInterpolateVsqbsClass;

/*
 * THE STENCIL OF INPUT VALUES:
 *
 * Pointer arithmetic is used to implicitly reflect the input stencil
 * about dos_two---assumed closer to the sampling location than other
 * pixels (ties are OK)---in such a way that after reflection the
 * sampling point is to the bottom right of dos_two.
 *
 * The following code and picture assumes that the stencil reflexion
 * has already been performed.
 *
 *
 *               (ix,iy-1)    (ix+1,iy-1)
 *               = uno_two    = uno_thr
 *
 *
 *
 *  (ix-1,iy)    (ix,iy)      (ix+1,iy)
 *  = dos_one    = dos_two    = dos_thr
 *                       X
 *
 *
 *  (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)
 *  = tre_one    = tre_two    = tre_thr
 *
 *
 * (X is the sampling location.)
 *
 * The above input pixel values are the ones needed in order to make
 * available the following values, needed by quadratic B-Splines:
 *
 *
 *  uno_one_1 =      uno_two_1 =  uno_thr_1 =
 *  (ix-1/2,iy-1/2)  (ix,iy-1/2)  (ix+1/2,iy-1/2)
 *
 *
 *                 X
 *
 *  dos_one_1 =      dos_two_1 =  dos_thr_1 =
 *  (ix-1/2,iy)      (ix,iy)      (ix+1/2,iy)
 *
 *
 *
 *
 *  tre_one_1 =      tre_two_1 =  tre_thr_1 =
 *  (ix-1/2,iy+1/2)  (ix,iy+1/2)  (ix+1/2,iy+1/2)
 *
 */

static inline double
vsqbs( const double fou_coef_uno_two,
       const double fou_coef_uno_thr,
       const double fou_coef_dos_one,
       const double fou_coef_dos_two,
       const double fou_coef_dos_thr,
       const double fou_coef_tre_one,
       const double fou_coef_tre_two,
       const double fou_coef_tre_thr,
       const double uno_two,
       const double uno_thr,
       const double dos_one,
       const double dos_two,
       const double dos_thr,
       const double tre_one,
       const double tre_two,
       const double tre_thr )
{
  const double newval = (
                          fou_coef_uno_two * uno_two +
                          fou_coef_uno_thr * uno_thr +
                          fou_coef_dos_one * dos_one +
                          fou_coef_dos_two * dos_two +
                          fou_coef_dos_thr * dos_thr +
                          fou_coef_tre_one * tre_one +
                          fou_coef_tre_two * tre_two +
                          fou_coef_tre_thr * tre_thr
                         ) * .25;

  return newval;
}

/*
 * Call vertex-split + quadratic B-splines with a careful type
 * conversion as a parameter.
 *
 * It would be nice to do this with templates somehow---for one thing
 * this would allow code comments!---but we can't figure a clean way
 * to do it.
 */
#define VSQBS_CONVERSION( conversion )               \
  template <typename T> static void inline           \
  vsqbs_ ## conversion(       PEL*   restrict pout,  \
                        const PEL*   restrict pin,   \
                        const int             bands, \
                        const int             lskip, \
                        const double          x_0,   \
                        const double          y_0 )  \
  { \
    T* restrict out = (T *) pout; \
    \
    const T* restrict in = (T *) pin; \
    \
    \
    const int sign_of_x_0 = 2 * ( x_0 >= 0. ) - 1; \
    const int sign_of_y_0 = 2 * ( y_0 >= 0. ) - 1; \
    \
    \
    const int shift_forw_1_pix = sign_of_x_0 * bands; \
    const int shift_forw_1_row = sign_of_y_0 * lskip; \
    \
    const int shift_back_1_pix = -shift_forw_1_pix; \
    const int shift_back_1_row = -shift_forw_1_row; \
    \
    \
    const int uno_two_shift =                    shift_back_1_row; \
    const int uno_thr_shift = shift_forw_1_pix + shift_back_1_row; \
    \
    const int dos_one_shift = shift_back_1_pix; \
    const int dos_two_shift = 0;                \
    const int dos_thr_shift = shift_forw_1_pix; \
    \
    const int tre_one_shift = shift_back_1_pix + shift_forw_1_row; \
    const int tre_two_shift =                    shift_forw_1_row; \
    const int tre_thr_shift = shift_forw_1_pix + shift_forw_1_row; \
    \
    \
    const double x        = sign_of_x_0 * x_0; \
    const double twicex   = x + x; \
    const double twicexsq = twicex * x; \
    const double fourxsq  = twicexsq + twicexsq; \
    \
    const double y        = sign_of_y_0 * y_0; \
    const double twicey   = y + y; \
    const double twiceysq = twicey * y; \
    const double fourysq  = twiceysq + twiceysq; \
    \
    const double spline_end_x = twicexsq - twicex + .5; \
    const double spline_mid_x = -fourxsq + twicex + .5; \
    const double spline_beg_x = twicexsq; \
    \
    const double spline_end_y = twiceysq - twicey + .5; \
    const double spline_mid_y = -fourysq + twicey + .5; \
    const double spline_beg_y = twiceysq; \
    \
    const double spline_end_x_end_y = spline_end_x * spline_end_y; \
    const double spline_end_x_mid_y = spline_end_x * spline_mid_y; \
    const double spline_end_x_beg_y = spline_end_x * spline_beg_y; \
    \
    const double spline_mid_x_end_y = spline_mid_x * spline_end_y; \
    const double spline_mid_x_mid_y = spline_mid_x * spline_mid_y; \
    const double spline_mid_x_beg_y = spline_mid_x * spline_beg_y; \
    \
    const double spline_beg_x_end_y = spline_beg_x * spline_end_y; \
    const double spline_beg_x_mid_y = spline_beg_x * spline_mid_y; \
    const double spline_beg_x_beg_y = spline_beg_x * spline_beg_y; \
    \
    \
    const double fou_coef_uno_two = spline_end_x_end_y + spline_mid_x_end_y; \
    const double fou_coef_uno_thr = spline_beg_x_end_y; \
    const double fou_coef_dos_one = spline_end_x_end_y + spline_end_x_mid_y; \
    const double fou_coef_tre_one = spline_end_x_beg_y; \
    \
    const double spline_beg_x_mid_y_p_mid_x_beg_y = spline_beg_x_mid_y + spline_mid_x_beg_y; \
    const double spline_end_x_mid_y_p_mid_x_mid_y = spline_end_x_mid_y + spline_mid_x_mid_y; \
    const double spline_end_x_beg_y_p_mid_x_beg_y = spline_end_x_beg_y + spline_mid_x_beg_y; \
    const double spline_beg_x_end_y_p_beg_x_mid_y = spline_beg_x_end_y + spline_beg_x_mid_y; \
    \
    const double fou_coef_tre_thr = spline_beg_x_mid_y_p_mid_x_beg_y + \
      spline_beg_x_beg_y + spline_beg_x_beg_y; \
    const double fou_coef_tre_two = spline_end_x_mid_y_p_mid_x_mid_y + \
      spline_end_x_beg_y_p_mid_x_beg_y + spline_end_x_beg_y_p_mid_x_beg_y + spline_beg_x_beg_y; \
    const double fou_coef_dos_thr = spline_beg_x_end_y_p_beg_x_mid_y + \
      spline_beg_x_end_y_p_beg_x_mid_y + spline_mid_x_end_y + spline_mid_x_mid_y + spline_beg_x_beg_y; \
    const double fou_coef_dos_two = fou_coef_uno_two + fou_coef_uno_two + \
      spline_beg_x_end_y_p_beg_x_mid_y + spline_end_x_mid_y_p_mid_x_mid_y + \
      spline_end_x_mid_y_p_mid_x_mid_y + spline_end_x_beg_y_p_mid_x_beg_y; \
    \
    int band = bands; \
    \
    \
    do \
      { \
        const double double_result =  \
          vsqbs( fou_coef_uno_two,    \
                 fou_coef_uno_thr,    \
                 fou_coef_dos_one,    \
                 fou_coef_dos_two,    \
                 fou_coef_dos_thr,    \
                 fou_coef_tre_one,    \
                 fou_coef_tre_two,    \
                 fou_coef_tre_thr,    \
                 in[uno_two_shift],   \
                 in[uno_thr_shift],   \
                 in[dos_one_shift],   \
                 in[dos_two_shift],   \
                 in[dos_thr_shift],   \
                 in[tre_one_shift],   \
                 in[tre_two_shift],   \
                 in[tre_thr_shift] ); \
        \
        {                                                         \
          const T result = to_ ## conversion<T>( double_result ); \
          in++;                                                   \
          *out++ = result;                                        \
        }                                                         \
        \
      } while (--band); \
  }


VSQBS_CONVERSION( fptypes )
VSQBS_CONVERSION( withsign )
VSQBS_CONVERSION( nosign )


#define CALL( T, conversion )               \
  vsqbs_ ## conversion<T>( out,          \
                              p,            \
                              bands,        \
                              lskip,        \
                              relative_x,   \
                              relative_y );


/*
 * We need C linkage:
 */
extern "C" {
  G_DEFINE_TYPE( VipsInterpolateVsqbs, vips_interpolate_vsqbs,
                 VIPS_TYPE_INTERPOLATE );
}


static void
vips_interpolate_vsqbs_interpolate( VipsInterpolate* restrict interpolate,
                                       PEL*             restrict out,
                                       REGION*          restrict in,
                                       double                    absolute_x,
                                       double                    absolute_y )
{
  /*
   * Floor's surrogate FAST_PSEUDO_FLOOR is used to make sure that the
   * transition through 0 is smooth. If it is known that absolute_x
   * and absolute_y will never be less than 0, plain cast---that is,
   * const int ix = absolute_x---should be used instead.  Actually,
   * any function which agrees with floor for non-integer values, and
   * picks one of the two possibilities for integer values, can be
   * used. FAST_PSEUDO_FLOOR fits the bill.
   *
   * Then, x is the x-coordinate of the sampling point relative to the
   * position of the center of the convex hull of the 2x2 block of
   * closest pixels. Similarly for y. Range of values: [-.5,.5).
   */
  const int ix = FAST_PSEUDO_FLOOR( absolute_x + .5 );
  const int iy = FAST_PSEUDO_FLOOR( absolute_y + .5 );

  /*
   * Move the pointer to (the first band of) the top/left pixel of the
   * 2x2 group of pixel centers which contains the sampling location
   * in its convex hull:
   */
  const PEL* restrict p = (PEL *) IM_REGION_ADDR( in, ix, iy );

  const double relative_x = absolute_x - ix;
  const double relative_y = absolute_y - iy;

  /*
   * VIPS versions of Nicolas's pixel addressing values.
   */
  const int actual_bands = in->im->Bands;
  const int lskip = IM_REGION_LSKIP( in ) / IM_IMAGE_SIZEOF_ELEMENT( in->im );
  /*
   * Double the bands for complex images to account for the real and
   * imaginary parts being computed independently:
   */
  const int bands =
    vips_bandfmt_iscomplex( in->im->BandFmt ) ? 2 * actual_bands : actual_bands;

  switch( in->im->BandFmt ) {
  case IM_BANDFMT_UCHAR:
    CALL( unsigned char, nosign );
    break;

  case IM_BANDFMT_CHAR:
    CALL( signed char, withsign );
    break;

  case IM_BANDFMT_USHORT:
    CALL( unsigned short, nosign );
    break;

  case IM_BANDFMT_SHORT:
    CALL( signed short, withsign );
    break;

  case IM_BANDFMT_UINT:
    CALL( unsigned int, nosign );
    break;

  case IM_BANDFMT_INT:
    CALL( signed int, withsign );
    break;

  /*
   * Complex images are handled by doubling of bands.
   */
  case IM_BANDFMT_FLOAT:
  case IM_BANDFMT_COMPLEX:
    CALL( float, fptypes );
    break;

  case IM_BANDFMT_DOUBLE:
  case IM_BANDFMT_DPCOMPLEX:
    CALL( double, fptypes );
    break;

  default:
    g_assert( 0 );
    break;
  }
}

static void
vips_interpolate_vsqbs_class_init( VipsInterpolateVsqbsClass *klass )
{
  GObjectClass *gobject_class = G_OBJECT_CLASS( klass );
  VipsObjectClass *object_class = VIPS_OBJECT_CLASS( klass );
  VipsInterpolateClass *interpolate_class = VIPS_INTERPOLATE_CLASS( klass );

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "vsqbs";
  object_class->description = _( "B-Splines with antialiasing smoothing" );

  interpolate_class->interpolate   = vips_interpolate_vsqbs_interpolate;
  interpolate_class->window_size   = 3;
  interpolate_class->window_offset = 1;
}

static void
vips_interpolate_vsqbs_init( VipsInterpolateVsqbs *vsqbs )
{
}
