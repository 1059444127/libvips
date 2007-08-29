/* Close and generate callbacks.
 * 
 * 1/7/93 JC
 * 20/7/93 JC
 *	- eval callbacks added
 * 16/8/94 JC
 *	- evalend callbacks added
 * 16/1/04 JC
 *	- now always calls all callbacks, even if some fail
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
#include <stdarg.h>

#include <vips/vips.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* Callback struct. We attach a list of callbacks to images to be invoked when
 * the image is closed. These do things like closing previous elements in a
 * chain of operations, freeing client data, etc.
 */
typedef struct {
	IMAGE *im;		/* IMAGE we are attached to */
	int (*fn)();		/* callback function */
	void *a, *b;		/* arguments to callback */
} VCallback;

/* Add a callback to an IMAGE. We can't use IM_NEW(), note! Freed eventually by
 * im__close(), or by im_generate(), etc. for evalend callbacks.
 */
static int
add_callback( IMAGE *im, GSList **cblist, int (*fn)(), void *a, void *b )
{	
	VCallback *cbs;

	if( !(cbs = IM_NEW( NULL, VCallback )) )
		return( -1 );
	
	cbs->fn = fn;
	cbs->a = a;
	cbs->b = b;
	cbs->im = im;
	*cblist = g_slist_prepend( *cblist, cbs );

	return( 0 );
}

/* Add a close callback to an IMAGE.
 */
int
im_add_close_callback( IMAGE *im, int (*fn)(), void *a, void *b )
{	
	return( add_callback( im, &im->closefns, fn, a, b ) );
}

/* Add an eval callback to an IMAGE.
 */
int
im_add_eval_callback( IMAGE *im, int (*fn)(), void *a, void *b )
{	
	return( add_callback( im, &im->evalfns, fn, a, b ) );
}

/* Add an eval end callback to an IMAGE.
 */
int
im_add_evalend_callback( IMAGE *im, int (*fn)(), void *a, void *b )
{	
	return( add_callback( im, &im->evalendfns, fn, a, b ) );
}

/* Perform a user callback. 
 */
static void *
call_callback( VCallback *cbs, int *result )
{
	int res;

	if( (res = cbs->fn( cbs->a, cbs->b )) ) {
		im_error( "im__trigger_callbacks", _( "user callback "
			"failed for %s" ), cbs->im->filename );
		*result = res;

#ifdef DEBUG_IO
		printf( "im__trigger_callbacks: user callback "
			"failed for %s\n", cbs->im->filename );
#endif /*DEBUG_IO*/
	}

	return( NULL );
}

/* Perform a list of user callbacks.
 */
int
im__trigger_callbacks( GSList *cblist )
{
	int result;

#ifdef DEBUG_IO
	printf( "im__trigger_callbacks: calling %d user callbacks ..\n",
		g_slist_length( cblist ) );
#endif /*DEBUG_IO*/

	result = 0;
	(void) im_slist_map2( cblist, 
		(VSListMap2Fn) call_callback, &result, NULL );

	return( result );
}
