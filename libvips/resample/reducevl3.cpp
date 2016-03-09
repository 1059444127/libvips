/* horizontal reduce by a float factor with lanczos3
 *
 * 29/1/16
 * 	- from shrinkv.c
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
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vips/vips.h>
#include <vips/debug.h>
#include <vips/internal.h>

#include "presample.h"
#include "templates.h"

typedef struct _VipsReducevl3 {
	VipsResample parent_instance;

	double yshrink;		/* Shrink factor */

} VipsReducevl3;

typedef VipsResampleClass VipsReducevl3Class;

/* Precalculated interpolation matrices. int (used for pel
 * sizes up to short), and double (for all others). We go to
 * scale + 1 so we can round-to-nearest safely.
 */

const int n_points = 6;

static int vips_reducevl3_matrixi[VIPS_TRANSFORM_SCALE + 1][n_points];
static double vips_reducevl3_matrixf[VIPS_TRANSFORM_SCALE + 1][n_points];

/* We need C linkage for this.
 */
extern "C" {
G_DEFINE_TYPE( VipsReducevl3, vips_reducevl3, VIPS_TYPE_RESAMPLE );
}

template <typename T, int max_value>
static void inline
reducevl3_unsigned_int_tab( VipsPel *pout, const VipsPel *pin,
	const int ne, const int lskip,
	const int * restrict cy )
{
	T* restrict out = (T *) pout;
	const T* restrict in = (T *) pin;

	const int l1 = lskip / sizeof( T );

	for( int z = 0; z < ne; z++ ) {
		int sum;

		sum = 0;
		for( int i = 0; i < n_points; i++ )
			sum += cy[i] * in[i * l1];

		sum = unsigned_fixed_round( sum ); 

		sum = VIPS_CLIP( 0, sum, max_value ); 

		out[z] = sum;

		in += 1;
	}
}

static int
vips_reducevl3_gen( VipsRegion *out_region, void *seq, 
	void *a, void *b, gboolean *stop )
{
	VipsImage *in = (VipsImage *) a;
	VipsReducevl3 *reducevl3 = (VipsReducevl3 *) b;
	VipsRegion *ir = (VipsRegion *) seq;
	VipsRect *r = &out_region->valid;

	/* Double bands for complex.
	 */
	const int bands = in->Bands * 
		(vips_band_format_iscomplex( in->BandFmt ) ?  2 : 1);
	int ne = r->width * bands;

	VipsRect s;

#ifdef DEBUG
	printf( "vips_reducevl3_gen: generating %d x %d at %d x %d\n",
		r->width, r->height, r->left, r->top ); 
#endif /*DEBUG*/

	s.left = r->left;
	s.top = r->top * reducevl3->yshrink;
	s.width = r->width;
	s.height = r->height * reducevl3->yshrink + n_points;
	if( vips_region_prepare( ir, &s ) )
		return( -1 );

	VIPS_GATE_START( "vips_reducevl3_gen: work" ); 

	for( int y = 0; y < r->height; y ++ ) { 
		VipsPel *q = VIPS_REGION_ADDR( out_region, r->left, r->top + y );
		const double Y = (r->top + y) * reducevl3->yshrink; 
		VipsPel *p = VIPS_REGION_ADDR( ir, r->left, (int) Y ); 
		const int sy = Y * VIPS_TRANSFORM_SCALE * 2;
		const int siy = sy & (VIPS_TRANSFORM_SCALE * 2 - 1);
		const int ty = (siy + 1) >> 1;
		const int *cyi = vips_reducevl3_matrixi[ty];
		const double *cyf = vips_reducevl3_matrixf[ty];
		const int lskip = VIPS_REGION_LSKIP( ir );

		switch( in->BandFmt ) {
		case VIPS_FORMAT_UCHAR:
			reducevl3_unsigned_int_tab
				<unsigned char, UCHAR_MAX>(
				q, p, ne, lskip, cyi );
			break;

		default:
			g_assert_not_reached();
			break;
		}
	}

	VIPS_GATE_STOP( "vips_reducevl3_gen: work" ); 

	return( 0 );
}

static int
vips_reducevl3_build( VipsObject *object )
{
	VipsObjectClass *object_class = VIPS_OBJECT_GET_CLASS( object );
	VipsResample *resample = VIPS_RESAMPLE( object );
	VipsReducevl3 *reducevl3 = (VipsReducevl3 *) object;
	VipsImage **t = (VipsImage **) vips_object_local_array( object, 2 );

	VipsImage *in;

	if( VIPS_OBJECT_CLASS( vips_reducevl3_parent_class )->build( object ) )
		return( -1 );

	in = resample->in; 

	if( reducevl3->yshrink < 1 ) { 
		vips_error( object_class->nickname, 
			"%s", _( "reduce factors should be >= 1" ) );
		return( -1 );
	}
	if( reducevl3->yshrink > 3 )  
		vips_warn( object_class->nickname, 
			"%s", _( "reduce factor greater than 3" ) );

	if( reducevl3->yshrink == 1 ) 
		return( vips_image_write( in, resample->out ) );

	/* Unpack for processing.
	 */
	if( vips_image_decode( in, &t[0] ) )
		return( -1 );
	in = t[0];

	/* Add new pixels around the input so we can interpolate at the edges.
	 */
	if( vips_embed( in, &t[1], 
		0, n_points / 2, 
		in->Xsize, in->Ysize + n_points - 1, 
		"extend", VIPS_EXTEND_COPY,
		NULL ) )
		return( -1 );
	in = t[1];

	if( vips_image_pipelinev( resample->out, 
		VIPS_DEMAND_STYLE_SMALLTILE, in, NULL ) )
		return( -1 );

	/* Size output. Note: we round the output width down!
	 *
	 * Don't change xres/yres, leave that to the application layer. For
	 * example, vipsthumbnail knows the true reduce factor (including the
	 * fractional part), we just see the integer part here.
	 */
	resample->out->Ysize = (in->Ysize - n_points + 1) / reducevl3->yshrink;
	if( resample->out->Ysize <= 0 ) { 
		vips_error( object_class->nickname, 
			"%s", _( "image has shrunk to nothing" ) );
		return( -1 );
	}

#ifdef DEBUG
	printf( "vips_reducevl3_build: reducing %d x %d image to %d x %d\n", 
		in->Xsize, in->Ysize, 
		resample->out->Xsize, resample->out->Ysize );  
#endif /*DEBUG*/

	if( vips_image_generate( resample->out,
		vips_start_one, vips_reducevl3_gen, vips_stop_one, 
		in, reducevl3 ) )
		return( -1 );

	return( 0 );
}

static void
vips_reducevl3_class_init( VipsReducevl3Class *reducevl3_class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( reducevl3_class );
	VipsObjectClass *vobject_class = VIPS_OBJECT_CLASS( reducevl3_class );
	VipsOperationClass *operation_class = 
		VIPS_OPERATION_CLASS( reducevl3_class );

	VIPS_DEBUG_MSG( "vips_reducevl3_class_init\n" );

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	vobject_class->nickname = "reducevl3";
	vobject_class->description = _( "shrink an image vertically" );
	vobject_class->build = vips_reducevl3_build;

	operation_class->flags = VIPS_OPERATION_SEQUENTIAL_UNBUFFERED;

	VIPS_ARG_DOUBLE( reducevl3_class, "yshrink", 3, 
		_( "Xshrink" ), 
		_( "Vertical shrink factor" ),
		VIPS_ARGUMENT_REQUIRED_INPUT,
		G_STRUCT_OFFSET( VipsReducevl3, yshrink ),
		1, 1000000, 1 );

	/* Build the tables of pre-computed coefficients.
	 */
	for( int y = 0; y < VIPS_TRANSFORM_SCALE + 1; y++ ) {
		calculate_coefficients_lanczos( 3, 
			(float) y / VIPS_TRANSFORM_SCALE,
			vips_reducevl3_matrixf[y] );

		for( int i = 0; i < n_points; i++ )
			vips_reducevl3_matrixi[y][i] =
				vips_reducevl3_matrixf[y][i] * 
				VIPS_INTERPOLATE_SCALE;
	}

}

static void
vips_reducevl3_init( VipsReducevl3 *reducevl3 )
{
}

/**
 * vips_reducevl3:
 * @in: input image
 * @out: output image
 * @yshrink: horizontal reduce
 * @...: %NULL-terminated list of optional named arguments
 *
 * Reduce @in vertically by a float factor. The pixels in @out are
 * interpolated with a 1D cubic mask. This operation will not work well for
 * a reduction of more than a factor of two.
 *
 * This is a very low-level operation: see vips_resize() for a more
 * convenient way to resize images. 
 *
 * This operation does not change xres or yres. The image resolution needs to
 * be updated by the application. 
 *
 * See also: vips_shrink(), vips_resize(), vips_affine().
 *
 * Returns: 0 on success, -1 on error
 */
int
vips_reducevl3( VipsImage *in, VipsImage **out, double yshrink, ... )
{
	va_list ap;
	int result;

	va_start( ap, yshrink );
	result = vips_call_split( "reducevl3", ap, in, out, yshrink );
	va_end( ap );

	return( result );
}
