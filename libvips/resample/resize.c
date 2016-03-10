/* resize an image ... up and down resampling.
 *
 * 13/8/14
 * 	- from affine.c
 * 18/11/14
 * 	- add the fancier algorithm from vipsthumbnail
 * 11/11/15
 * 	- smarter cache sizing
 * 29/2/16
 * 	- shrink more affine less, now we have better anti-alias settings
 * 10/3/16
 * 	- revise again, using new vips_reduce() code
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG_VERBOSE
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <vips/vips.h>
#include <vips/debug.h>
#include <vips/internal.h>
#include <vips/transform.h>

#include "presample.h"

typedef struct _VipsResize {
	VipsResample parent_instance;

	double scale;
	double vscale;

	/* Deprecated.
	 */
	VipsInterpolate *interpolate;
	double idx;
	double idy;

} VipsResize;

typedef VipsResampleClass VipsResizeClass;

G_DEFINE_TYPE( VipsResize, vips_resize, VIPS_TYPE_RESAMPLE ); 

static int
vips_resize_build( VipsObject *object )
{
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( object );
	VipsResample *resample = VIPS_RESAMPLE( object );
	VipsResize *resize = (VipsResize *) object;

	VipsImage **t = (VipsImage **) vips_object_local_array( object, 7 );

	VipsImage *in;
	int target_width;
	int target_height;
	int int_hshrink;
	int int_vshrink;
	double hresidual;
	double vresidual;
	double sigma;
	gboolean anti_alias;

	if( VIPS_OBJECT_CLASS( vips_resize_parent_class )->build( object ) )
		return( -1 );

	in = resample->in;

	/* The image size we are aiming for.
	 */
	target_width = in->Xsize * resize->scale;
	if( vips_object_argument_isset( object, "vscale" ) ) 
		target_height = in->Ysize * resize->vscale;
	else
		target_height = in->Ysize * resize->scale;

	/* If the factor is > 1.0, we need to zoom rather than shrink.
	 * Just set the int part to 1 in this case.
	 */
	int_hshrink = resize->scale > 1.0 ? 
		1 : VIPS_FLOOR( 1.0 / resize->scale );
	if( vips_object_argument_isset( object, "vscale" ) ) 
		int_vshrink = resize->vscale > 1.0 ? 
			1 : VIPS_FLOOR( 1.0 / resize->vscale );
	else
		int_vshrink = int_hshrink;

	if( int_hshrink > 1 ||
		int_vshrink > 1 ) { 
		vips_info( class->nickname, "box shrink by %d x %d", 
			int_hshrink, int_vshrink );
		if( vips_shrink( in, &t[0], int_hshrink, int_vshrink, NULL ) )
			return( -1 );
		in = t[0];
	}

	/* Do we need a further size adjustment? It's the difference
	 * between our target size and the size we have after vips_shrink().
	 */
	hresidual = (double) target_width / in->Xsize;
	if( vips_object_argument_isset( object, "vscale" ) ) 
		vresidual = (double) target_height / in->Ysize;
	else
		vresidual = hresidual;

	/* If the final affine will be doing a large downsample, we can get 
	 * nasty aliasing on hard edges. Blur before affine to smooth this out.
	 *
	 * Don't blur for very small shrinks, blur with radius 1 for x1.5
	 * shrinks, blur radius 2 for x2.5 shrinks and above, etc.
	 *
	 * Don't try to be clever for non-rectangular shrinks. We just
	 * consider the horizontal factor.
	 */
	sigma = ((1.0 / hresidual) - 0.5) / 2.5;
	anti_alias = hresidual < 0.9 && sigma > 0.1;
	if( anti_alias ) { 
		vips_info( class->nickname, "anti-alias sigma %g", sigma );
		if( vips_gaussblur( in, &t[1], sigma, NULL ) )
			return( -1 );
		in = t[1];
	}

	if( hresidual < 1.0 || 
		vresidual < 1.0 ) { 
		vips_info( class->nickname, "residual reduce by %g x %g", 
			hresidual, vresidual );

		if( vips_reducel3( in, &t[2], 
			1.0 / hresidual, 1.0 / vresidual, NULL ) )  
			return( -1 );
		in = t[2];
	}
	if( hresidual > 1.0 || 
		vresidual > 1.0 ) { 
		vips_info( class->nickname, "residual scale %g x %g", 
			hresidual, vresidual );
		if( vips_affine( in, &t[3], hresidual, 0, 0, vresidual, 
			"interpolate", vips_interpolate_nearest_static(), 
			NULL ) )  
			return( -1 );
		in = t[3];
	}

	if( vips_image_write( in, resample->out ) )
		return( -1 ); 

	return( 0 );
}

static void
vips_resize_class_init( VipsResizeClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS( class );
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( class );

	VIPS_DEBUG_MSG( "vips_resize_class_init\n" );

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	vobject_class->nickname = "resize";
	vobject_class->description = _( "resize an image" );
	vobject_class->build = vips_resize_build;

	operation_class->flags = VIPS_OPERATION_SEQUENTIAL;

	VIPS_ARG_DOUBLE( class, "scale", 113, 
		_( "Scale factor" ), 
		_( "Scale image by this factor" ),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET( VipsResize, scale ),
		0, 10000000, 0 );

	VIPS_ARG_DOUBLE( class, "vscale", 113, 
		_( "Vertical scale factor" ), 
		_( "Vertical scale image by this factor" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT,
		G_STRUCT_OFFSET( VipsResize, vscale ),
		0, 10000000, 0 );

	/* We used to let people set the input offset so you could pick centre
	 * or corner interpolation, but it's not clear this was useful. 
	 */
	VIPS_ARG_DOUBLE( class, "idx", 115, 
		_( "Input offset" ), 
		_( "Horizontal input displacement" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT | VIPS_ARGUMENT_DEPRECATED,
		G_STRUCT_OFFSET( VipsResize, idx ),
		-10000000, 10000000, 0 );

	VIPS_ARG_DOUBLE( class, "idy", 116, 
		_( "Input offset" ), 
		_( "Vertical input displacement" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT | VIPS_ARGUMENT_DEPRECATED,
		G_STRUCT_OFFSET( VipsResize, idy ),
		-10000000, 10000000, 0 );

	/* We used to let people set the interpolator, but it's not clear this
	 * was useful. Anyway, vips_reduce() no longer has an interpolator
	 * param.
	 */
	VIPS_ARG_INTERPOLATE( class, "interpolate", 2, 
		_( "Interpolate" ), 
		_( "Interpolate pixels with this" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT | VIPS_ARGUMENT_DEPRECATED, 
		G_STRUCT_OFFSET( VipsResize, interpolate ) );

}

static void
vips_resize_init( VipsResize *resize )
{
}

/**
 * vips_resize:
 * @in: input image
 * @out: output image
 * @scale: scale factor
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * @vscale: vertical scale factor
 *
 * Resize an image. When upsizing (@scale > 1), the image is simply block
 * upsized. When downsizing, the
 * image is block-shrunk with vips_shrink(), then an anti-alias blur is
 * applied with vips_gaussblur(), then the image is shrunk again to the 
 * target size with vips_reduce(). 
 *
 * vips_resize() normally maintains the image apect ratio. If you set
 * @vscale, that factor is used for the vertical scale and @scale for the
 * horizontal.
 *
 * This operation does not change xres or yres. The image resolution needs to
 * be updated by the application. 
 *
 * See also: vips_shrink(), vips_reduce(), vips_gaussblur().
 *
 * Returns: 0 on success, -1 on error
 */
int
vips_resize( VipsImage *in, VipsImage **out, 
	double scale, ... )
{
	va_list ap;
	int result;

	va_start( ap, scale );
	result = vips_call_split( "resize", ap, in, out, scale );
	va_end( ap );

	return( result );
}
