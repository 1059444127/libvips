/* memory utilities
 *
 * J.Cupitt, 8/4/93
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

#ifndef VIPS_MEMORY_H
#define VIPS_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define VIPS_FREEF( F, S ) \
G_STMT_START { \
        if( S ) { \
                (void) F( (S) ); \
                (S) = 0; \
        } \
} G_STMT_END

/* Can't just use VIPS_FREEF(), we want the extra cast to void on the argument
 * to im_free() to make sure we can work for "const char *" variables.
 */
#define VIPS_FREE( S ) \
G_STMT_START { \
        if( S ) { \
                (void) im_free( (void *) (S) ); \
                (S) = 0; \
        } \
} G_STMT_END

#define VIPS_SETSTR( S, V ) \
G_STMT_START { \
        const char *sst = (V); \
	\
        if( (S) != sst ) { \
                if( !(S) || !sst || strcmp( (S), sst ) != 0 ) { \
                        VIPS_FREE( S ); \
                        if( sst ) \
                                (S) = im_strdup( NULL, sst ); \
                } \
        } \
} G_STMT_END

#define VIPS_NEW( IM, T ) ((T *) im_malloc( (IM), sizeof( T )))
#define VIPS_ARRAY( IM, N, T ) ((T *) im_malloc( (IM), (N) * sizeof( T )))

void *im_malloc( VipsImage *im, size_t size );
char *im_strdup( VipsImage *im, const char *str );
int im_free( void *s );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*IM_MEMORY_H*/
