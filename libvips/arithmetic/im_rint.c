/* im_rint.c
 *
 * 20/6/02 JC
 *	- adapted from im_floor()
 * 9/9/09
 * 	- updated from new im_floor()
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <vips/vips.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

#define RINT( TYPE ) { \
	TYPE *p = (TYPE *) in; \
	TYPE *q = (TYPE *) out; \
	\
	for( x = 0; x < sz; x++ ) \
		q[x] = IM_RINT( p[x] );\
}

/* RINT a buffer of PELs.
 */
static void
rint_gen( PEL *in, PEL *out, int width, IMAGE *im )
{	
	/* Complex just doubles the size.
	 */
	const int sz = width * im->Bands * (im_iscomplex( im ) ? 2 : 1);

	int x;

        switch( im->BandFmt ) {
        case IM_BANDFMT_COMPLEX:	
        case IM_BANDFMT_FLOAT: 		
		RINT( float ); 
		break; 

        case IM_BANDFMT_DOUBLE:
        case IM_BANDFMT_DPCOMPLEX:	
		RINT( double ); 
		break;

        default:
		g_assert( 0 );
        }
}

/**
 * im_rint:
 * @in: input #IMAGE
 * @out: output #IMAGE
 *
 * Finds the nearest integral value. Copy for integer types, 
 * call IM_RINT() for float and complex types. Output type == input type.
 *
 * IM_RINT() is a pseudo-round-to-nearest. It is much faster than
 * <function>rint</function>(3), but does not give the same result for
 * negative integral values.
 *
 * See also: im_ceil(), im_floor(), im_clip2fmt()
 *
 * Returns: 0 on success, -1 on error
 */
int 
im_rint( IMAGE *in, IMAGE *out )
{
	if( im_piocheck( in, out ) ||
		im_check_uncoded( "im_rint", in ) )
		return( -1 );

	/* Is this one of the int types? Degenerate to im_copy() if it
	 * is.
	 */
	if( im_isint( in ) )
		return( im_copy( in, out ) );

	/* Output type == input type.
	 */
	if( im_cp_desc( out, in ) )
		return( -1 );

	/* Generate!
	 */
	if( im_wrapone( in, out, 
		(im_wrapone_fn) rint_gen, in, NULL ) )
		return( -1 );

	return( 0 );
}
