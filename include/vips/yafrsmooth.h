/* Yafrsmooth (catmull-rom) interpolator.
 */

/*

    This file is part of VIPS.

    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifndef VIPS_YAFRSMOOTH_H
#define VIPS_YAFRSMOOTH_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define VIPS_TYPE_INTERPOLATE_YAFRSMOOTH \
	(vips_interpolate_yafrsmooth_get_type())
#define VIPS_INTERPOLATE_YAFRSMOOTH( obj ) \
	(G_TYPE_CHECK_INSTANCE_CAST( (obj), \
	VIPS_TYPE_INTERPOLATE_YAFRSMOOTH, VipsInterpolateYafrsmooth ))
#define VIPS_INTERPOLATE_YAFRSMOOTH_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_CAST( (klass), \
	VIPS_TYPE_INTERPOLATE_YAFRSMOOTH, VipsInterpolateYafrsmoothClass))
#define VIPS_IS_INTERPOLATE_YAFRSMOOTH( obj ) \
	(G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_INTERPOLATE_YAFRSMOOTH ))
#define VIPS_IS_INTERPOLATE_YAFRSMOOTH_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_INTERPOLATE_YAFRSMOOTH ))
#define VIPS_INTERPOLATE_YAFRSMOOTH_GET_CLASS( obj ) \
	(G_TYPE_INSTANCE_GET_CLASS( (obj), \
	VIPS_TYPE_INTERPOLATE_YAFRSMOOTH, VipsInterpolateYafrsmoothClass ))

typedef struct _VipsInterpolateYafrsmooth {
	VipsInterpolate parent_object;

	/* "sharpening" is a continuous method parameter which is
	 * proportional to the amount of "diagonal straightening" which the
	 * nonlinear correction part of the method may add to the underlying
	 * linear scheme. You may also think of it as a sharpening
	 * parameter: higher values correspond to more sharpening, and
	 * negative values lead to strange looking effects.
	 *
	 * The default value is sharpening = 29/32 when the scheme being
	 * "straightened" is Catmull-Rom---as is the case here. This value
	 * fixes key pixel values near the diagonal boundary between two
	 * monochrome regions (the diagonal boundary pixel values being set
	 * to the halfway colour).
	 *
	 * If resampling seems to add unwanted texture artifacts, push
	 * sharpening toward 0. It is not generally not recommended to set
	 * sharpening to a value larger than 4.
	 *
	 * Sharpening is halved because the .5 which has to do with the
	 * relative coordinates of the evaluation points (which has to do
	 * with .5*rite_width etc) is folded into the constant to save
	 * flops. Consequently, the largest recommended value of
	 * sharpening_over_two is 2=4/2.
	 *
	 * In order to simplify interfacing with users, the parameter which
	 * should be set by the user is normalized so that user_sharpening =
	 * 1 when sharpening is equal to the recommended value. Consistently
	 * with the above discussion, values of user_sharpening between 0
	 * and about 3.625 give good results.
	 */
	double sharpening;
} VipsInterpolateYafrsmooth;

typedef struct _VipsInterpolateYafrsmoothClass {
	VipsInterpolateClass parent_class;

	/* Precalculated interpolation matricies. int (used for pel sizes up 
	 * to short), and double (for all others). We go to scale + 1, so
	 * we can round-to-nearest safely.
	 */

	/* We could keep a large set of 2d 4x4 matricies, but this actually
	 * works out slower, since for many resizes the thing will no longer
	 * fit in L1.
	 */
	int matrixi[VIPS_TRANSFORM_SCALE + 1][4];
	double matrixf[VIPS_TRANSFORM_SCALE + 1][4];
} VipsInterpolateYafrsmoothClass;

GType vips_interpolate_yafrsmooth_get_type( void );
VipsInterpolate *vips_interpolate_yafrsmooth_new( void );
void vips_interpolate_yafrsmooth_set_sharpening( VipsInterpolateYafrsmooth *, 
	double sharpening );

/* Convenience: return a static default yafrsmooth, so no need to free it.
 */
VipsInterpolate *vips_interpolate_yafrsmooth_static( void );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_YAFRSMOOTH_H*/

