/* im_aconv ... approximate convolution
 *
 * This operation does an approximate convolution. 
 *
 * Author: John Cupitt & Nicolas Robidoux
 * Written on: 31/5/11
 * Modified on: 
 * 31/5/11
 *      - from im_aconvsep()
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

/*

  See:

	http://incubator.quasimondo.com/processing/stackblur.pde

  This thing is a little like stackblur, but generalised to any 2D 
  convolution.

 */

/*

  TODO

  	- are we handling mask offset correctly?

	- just done boxes_new()

 */

/* Show sample pixels as they are transformed.
#define DEBUG_PIXELS
 */

/*
#define DEBUG
#define VIPS_DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <vips/vips.h>
#include <vips/vector.h>
#include <vips/debug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* Maximum number of boxes we can break the mask into.
 */
#define MAX_LINES (1000)

/* Get an (x,y) value from a mask.
 */
#define MASK( M, X, Y ) ((M)->coeff[(X) + (Y) * (M)->xsize])

/* Euclid's algorithm. Use this to common up mults.
 */
static int
gcd( int a, int b )
{
	if( b == 0 )
		return( abs( a ) );
	else
		return( gcd( b, a % b ) );
}

/* A set of boxes. 
 */
typedef struct _Boxes {
	/* Copy of our arguments.
	 */
	IMAGE *in;
	IMAGE *out;
	DOUBLEMASK *mask;
	int n_layers;

	int area;
	int rounding;

	/* The horizontal lines we gather.
	 */
	int n_hlines;

	/* Start is the left-most pixel in the line, end is one beyond the
	 * right-most pixel. start[3]/end[3] writes to band 3 in the
	 * intermediate image.
	 */
	int start[MAX_LINES];
	int end[MAX_LINES];

	/* The hlines have weights. weight 0 means this line is unused.
	 */
	int weight[MAX_LINES];

	/* Scale and sum a set of hlines to make the final value. band[] is
	 * the index into start/end we add, row[] is the row we take it from.
	 */
	int n_vlines;
	int row[MAX_LINES];
	int band[MAX_LINES];

	/* Each hline has a factor during gather, eg. -1 for -ve lobes.
	 */
	int factor[MAX_LINES];
} Boxes;

static void
boxes_start( Boxes *boxes, int x )
{
	boxes->start[boxes->n_hlines] = x;
	boxes->weight[boxes->n_hlines] = 1;
}

static int
boxes_end( Boxes *lines, int x, int y, int factor )
{
	boxes->end[boxes->n_hlines] = x;

	boxes->row[boxes->n_vlines] = y;
	boxes->band[boxes->n_vlines] = boxes->n_hlines;
	boxes->factor[boxes->n_vlines] = factor;

	if( boxes->n_hlines >= MAX_LINES - 1 ) {
		vips_error( "im_aconv", "%s", _( "mask too complex" ) );
		return( -1 );
	}
	boxes->n_hlines += 1;

	if( boxes->n_vlines >= MAX_LINES - 1 ) {
		vips_error( "im_aconv", "%s", _( "mask too complex" ) );
		return( -1 );
	}
	boxes->n_vlines += 1;

	return( 0 );
}

/* The 'distance' between a pair of hlines.
 */
static int
boxes_distance( Boxes *boxes, int a, int b )
{
	return( abs( boxes->start[a] - boxes->start[b] ) + 
		abs( boxes->end[a] - boxes->end[b] ) ); 
}

/* Merge two hlines. Line b is deleted, and any refs to b in vlines updated to
 * point at a.
 */
static void
boxes_merge( Boxes *boxes, int a, int b )
{
	int i;

	/* Scale weights. 
	 */
	int fa = boxes->weight[a];
	int fb = boxes->weight[b];
	double w = (double) fb / (fa + fb);

	/* New endpoints.
	 */
	boxes->start[a] += w * (boxes->start[b] - boxes->start[a]);
	boxes->end[a] += w * (boxes->end[b] - boxes->end[a]);
	boxes->weight[a] += boxes->weight[b];

	/* Update refs to b in vlines to refer to a instead.
	 */
	for( i = 0; i < boxes->n_vlines; i++ )
		if( boxes->band[i] == b )
			boxes->band[i] = a;

	/* Delete b.
	 */
	boxes->weight[b] = 0;
}

/* Find the closest pair of hlines, join them up if the distance is less than 
 * a threshold. Return non-zero if we made a change.
 */
static int
boxes_cluster( Boxes *boxes, int threshold )
{
	int i, j;
	int best, a, b;
	int acted;

	best = 9999999;

	for( i = 0; i < boxes->n_hlines; i++ ) {
		if( boxes->weight[i] == 0 )
			continue;

		for( j = i + 1; j < lines->n_lines; j++ ) {
			int d;

			if( boxes->weight[j] == 0 )
				continue;

			d = lines_distance( lines, i, j ); 
			if( d < best ) {
				best = d;
				a = i;
				b = j;
			}
		}
	}

	acted = 0;
	if( best < threshold ) {
		boxes_merge( boxes, a, b );
		acted = 1;
	}

	return( acted );
}

/* Renumber after clustering. We will have removed a lot of hlines ... shuffle
 * the rest down, adjust all the vline references.
 */
static void
boxes_renumber( Boxes *boxes )
{
	int i, j;

	for( i = 0; i < boxes->n_hlines; i++ ) {
		if( boxes->weight[i] == 0 ) {
			/* We move hlines i + 1 down, so we need to adjust all
			 * band[] refs to match.
			 */
			for( j = 0; j < boxes->n_vlines; j++ )
				if( boxes->band[j] <= i )
					boxes->band[j] -= 1;

			for( j = i; j < boxes->n_hlines; j++ ) {
				boxes->start[j] = boxes->start[j + 1];
				boxes->end[j] = boxes->end[j + 1];
				boxes->weight[j] = boxes->weight[j + 1];
			}

			boxes->n_hlines -= 1;
		}
	}
}

/* Break a mask into boxes.
 */
static Boxes *
boxes_new( IMAGE *in, IMAGE *out, DOUBLEMASK *mask, int n_layers )
{
	const int size = mask->xsize * mask->ysize;

	Boxes *boxes;
	double max;
	double min;
	double depth;
	double sum;
	int layers_above;
	int layers_below;
	int z, n, x, y;

	/* Check parameters.
	 */
	if( im_piocheck( in, out ) ||
		im_check_uncoded( "im_aconv", in ) ||
		vips_check_dmask_1d( "im_aconv", mask ) ) 
		return( NULL );

	if( !(boxes = VIPS_NEW( out, Boxes )) )
		return( NULL );
	boxes->in = in;
	boxes->out = out;
	if( !(boxes->mask = (DOUBLEMASK *) im_local( out, 
		(im_construct_fn) im_dup_dmask,
		(im_callback_fn) im_free_dmask, mask, mask->filename, NULL )) )
		return( NULL );
	boxes->n_layers = n_layers;

	boxes->n_hlines = 0;
	boxes->n_vlines = 0;

	/* Find mask range. We must always include the zero axis in the mask.
	 */
	max = 0;
	min = 0;
	for( n = 0; n < size; n++ ) {
		max = IM_MAX( max, mask->coeff[n] );
		min = IM_MIN( min, mask->coeff[n] );
	}

	/* The zero axis must fall on a layer boundary. Estimate the
	 * depth, find n-lines-above-zero, get exact depth, then calculate a
	 * fixed n-lines which includes any negative parts.
	 */
	depth = (max - min) / n_layers;
	layers_above = ceil( max / depth );
	depth = max / layers_above;
	layers_below = floor( min / depth );
	n_layers = layers_above - layers_below;

	VIPS_DEBUG_MSG( "boxes_new: depth = %g, n_layers = %d\n", 
		depth, n_layers );

	/* For each layer, generate a set of lines which are inside the
	 * perimeter. Work down from the top.
	 */
	band_offset = 0;
	for( z = 0; z < n_layers; z++ ) {
		/* How deep we are into the mask, as a double we can test
		 * against. Add half the layer depth so we can easily find >50%
		 * mask elements.
		 */
		double z_ph = max - (1 + z) * depth + depth / 2;

		/* Odd, but we must avoid rounding errors that make us miss 0
		 * in the line above.
		 */
		int z_positive = z < layers_above;

		for( y = 0; y < mask->ysize; y++ ) {
			int inside;

			/* Start outside the perimeter.
			 */
			inside = 0;

			for( x = 0; x < mask->xsize; x++ ) {
				double coeff = MASK( mask, x, y );

				/* The vertical line from mask[x, y] to 0 is 
				 * inside. Is our current square (x, y) part 
				 * of that line?
				 */
				if( (y_positive && coeff >= y_ph) ||
					(!y_positive && coeff <= y_ph) ) {
					if( !inside ) {
						boxes_start( lines, x );
						inside = 1;
					}
				}
				else {
					if( inside ) {
						boxes_end( lines, x, y,
							y_positive ? 1 : -1 );
						inside = 0;
					}
				}
			}

			if( inside && 
				boxes_end( lines, mask->xsize, y, 
					y_positive ? 1 : -1 ) )
				return( NULL );
		}
	}

	VIPS_DEBUG_MSG( "boxes_new: generated %d boxes\n", 
		boxes->n_hlines );

	VIPS_DEBUG_MSG( "boxes_new: clustering with thresh %d ...\n", 5 ); 
	while( boxes_cluster( boxes, 5 ) )
		;
	boxes_renumber( boxes );
	VIPS_DEBUG_MSG( "boxes_new: after clustering, %d boxes remain\n", 
		boxes->n_hlines );

	/* Find the area of the lines.
	 */
	boxes->area = 0;
	for( y = 0; y < boxes->n_vlines; y++ ) {
		int x = boxes->band[y];

		boxes->area += boxes->factor[y] * 
			(boxes->end[x] - boxes->start[x]);
	}

	/* Strength reduction: if all lines are divisible by n, we can move
	 * that n out into the ->area factor. The aim is to produce as many
	 * factor 1 lines as we can and to reduce the chance of overflow.
	 */
	x = boxes->factor[0];
	for( y = 1; y < boxes->n_vlines; y++ ) 
		x = gcd( x, boxes->factor[y] );
	for( y = 0; y < boxes->n_lines; y++ ) 
		boxes->factor[y] /= x;
	boxes->area *= x;

	/* Find the area of the original mask.
	 */
	sum = 0;
	for( z = 0; z < size; z++ ) 
		sum += mask->coeff[z];

	boxes->area = rint( sum * boxes->area / mask->scale );
	boxes->rounding = (boxes->area + 1) / 2 + mask->offset * boxes->area;

	/* ASCII-art layer drawing.
	 */
	printf( "lines:\n" );
	for( y = 0; y < boxes->n_vlines; y++ ) {
		printf( "%3d - %2d x ", y, boxes->factor[z] );
		for( x = 0; x < 55; x++ ) {
			int rx = x * (width + 1) / 55;
			int b = boxes->band[y];

			if( rx >= boxes->start[b] && rx < boxes->end[b] )
				printf( "#" );
			else
				printf( " " );
		}
		printf( " %3d .. %3d\n", boxes->start[z], boxes->end[z] );
	}
	printf( "area = %d\n", boxes->area );
	printf( "rounding = %d\n", boxes->rounding );

	return( boxes );
}

/* Our sequence value.
 */
typedef struct {
	Boxes *boxes;
	REGION *ir;		/* Input region */

	int *start;		/* Offsets for start and stop */
	int *end;

	/* The sums for each line. int for integer types, double for floating
	 * point types.
	 */
	void *sum;		

	int last_stride;	/* Avoid recalcing offsets, if we can */
} AConvSequence;

/* Free a sequence value.
 */
static int
aconv_stop( void *vseq, void *a, void *b )
{
	AConvSequence *seq = (AConvSequence *) vseq;

	IM_FREEF( im_region_free, seq->ir );

	return( 0 );
}

/* Convolution start function.
 */
static void *
aconv_start( IMAGE *out, void *a, void *b )
{
	IMAGE *in = (IMAGE *) a;
	Lines *lines = (Lines *) b;

	AConvSequence *seq;

	if( !(seq = IM_NEW( out, AConvSequence )) )
		return( NULL );

	/* Init!
	 */
	seq->lines = lines;
	seq->ir = im_region_create( in );
	seq->start = IM_ARRAY( out, lines->n_lines, int );
	seq->end = IM_ARRAY( out, lines->n_lines, int );
	if( vips_band_format_isint( out->BandFmt ) )
		seq->sum = IM_ARRAY( out, lines->n_lines, int );
	else
		seq->sum = IM_ARRAY( out, lines->n_lines, double );
	seq->last_stride = -1;

	if( !seq->ir || !seq->start || !seq->end || !seq->sum ) {
		aconv_stop( seq, in, lines );
		return( NULL );
	}

	return( seq );
}

#define CLIP_UCHAR( V ) \
G_STMT_START { \
	if( (V) < 0 ) \
		(V) = 0; \
	else if( (V) > UCHAR_MAX ) \
		(V) = UCHAR_MAX; \
} G_STMT_END

#define CLIP_CHAR( V ) \
G_STMT_START { \
	if( (V) < SCHAR_MIN ) \
		(V) = SCHAR_MIN; \
	else if( (V) > SCHAR_MAX ) \
		(V) = SCHAR_MAX; \
} G_STMT_END

#define CLIP_USHORT( V ) \
G_STMT_START { \
	if( (V) < 0 ) \
		(V) = 0; \
	else if( (V) > USHRT_MAX ) \
		(V) = USHRT_MAX; \
} G_STMT_END

#define CLIP_SHORT( V ) \
G_STMT_START { \
	if( (V) < SHRT_MIN ) \
		(V) = SHRT_MIN; \
	else if( (V) > SHRT_MAX ) \
		(V) = SHRT_MAX; \
} G_STMT_END

#define CLIP_NONE( V ) {}

/* The h and v loops are very similar, but also annoyingly different. Keep
 * them separate for easy debugging.
 */

#define HCONV_INT( TYPE, CLIP ) { \
	for( i = 0; i < bands; i++ ) { \
		int *seq_sum = (int *) seq->sum; \
		\
		TYPE *q; \
		TYPE *p; \
		int sum; \
		\
		p = i + (TYPE *) IM_REGION_ADDR( ir, r->left, r->top + y ); \
		q = i + (TYPE *) IM_REGION_ADDR( or, r->left, r->top + y ); \
		\
		sum = 0; \
		for( z = 0; z < lines->n_lines; z++ ) { \
			seq_sum[z] = 0; \
			for( x = lines->start[z]; x < lines->end[z]; x++ ) \
				seq_sum[z] += p[x * istride]; \
			sum += lines->factor[z] * seq_sum[z]; \
		} \
		sum = (sum + lines->rounding) / lines->area; \
		CLIP( sum ); \
		*q = sum; \
		q += ostride; \
		\
		for( x = 1; x < r->width; x++ ) {  \
			sum = 0; \
			for( z = 0; z < lines->n_lines; z++ ) { \
				seq_sum[z] += p[seq->end[z]]; \
				seq_sum[z] -= p[seq->start[z]]; \
				sum += lines->factor[z] * seq_sum[z]; \
			} \
			p += istride; \
			sum = (sum + lines->rounding) / lines->area; \
			CLIP( sum ); \
			*q = sum; \
			q += ostride; \
		} \
	} \
}

#define HCONV_FLOAT( TYPE ) { \
	for( i = 0; i < bands; i++ ) { \
		double *seq_sum = (double *) seq->sum; \
		\
		TYPE *q; \
		TYPE *p; \
		double sum; \
		\
		p = i + (TYPE *) IM_REGION_ADDR( ir, r->left, r->top + y ); \
		q = i + (TYPE *) IM_REGION_ADDR( or, r->left, r->top + y ); \
		\
		sum = 0; \
		for( z = 0; z < lines->n_lines; z++ ) { \
			seq_sum[z] = 0; \
			for( x = lines->start[z]; x < lines->end[z]; x++ ) \
				seq_sum[z] += p[x * istride]; \
			sum += lines->factor[z] * seq_sum[z]; \
		} \
		sum = sum / lines->area + mask->offset; \
		*q = sum; \
		q += ostride; \
		\
		for( x = 1; x < r->width; x++ ) {  \
			sum = 0; \
			for( z = 0; z < lines->n_lines; z++ ) { \
				seq_sum[z] += p[seq->end[z]]; \
				seq_sum[z] -= p[seq->start[z]]; \
				sum += lines->factor[z] * seq_sum[z]; \
			} \
			p += istride; \
			sum = sum / lines->area + mask->offset; \
			*q = sum; \
			q += ostride; \
		} \
	} \
}

/* Do horizontal masks ... we scan the mask along scanlines.
 */
static int
aconv_generate_horizontal( REGION *or, void *vseq, void *a, void *b )
{
	AConvSequence *seq = (AConvSequence *) vseq;
	IMAGE *in = (IMAGE *) a;
	Lines *lines = (Lines *) b;

	REGION *ir = seq->ir;
	const int n_lines = lines->n_lines;
	DOUBLEMASK *mask = lines->mask;
	Rect *r = &or->valid;

	/* Double the bands (notionally) for complex.
	 */
	int bands = vips_band_format_iscomplex( in->BandFmt ) ? 
		2 * in->Bands : in->Bands;

	Rect s;
	int x, y, z, i;
	int istride;
	int ostride;

	/* Prepare the section of the input image we need. A little larger
	 * than the section of the output image we are producing.
	 */
	s = *r;
	s.width += mask->xsize - 1;
	s.height += mask->ysize - 1;
	if( im_prepare( ir, &s ) )
		return( -1 );

	/* Stride can be different for the vertical case, keep this here for
	 * ease of direction change.
	 */
	istride = IM_IMAGE_SIZEOF_PEL( in ) / 
		IM_IMAGE_SIZEOF_ELEMENT( in );
	ostride = IM_IMAGE_SIZEOF_PEL( lines->out ) / 
		IM_IMAGE_SIZEOF_ELEMENT( lines->out );

        /* Init offset array. 
	 */
	if( seq->last_stride != istride ) {
		seq->last_stride = istride;

		for( z = 0; z < n_lines; z++ ) {
			seq->start[z] = lines->start[z] * istride;
			seq->end[z] = lines->end[z] * istride;
		}
	}

	for( y = 0; y < r->height; y++ ) { 
		switch( in->BandFmt ) {
		case IM_BANDFMT_UCHAR: 	
			HCONV_INT( unsigned char, CLIP_UCHAR );
			break;

		case IM_BANDFMT_CHAR: 	
			HCONV_INT( signed char, CLIP_UCHAR );
			break;

		case IM_BANDFMT_USHORT: 	
			HCONV_INT( unsigned short, CLIP_USHORT );
			break;

		case IM_BANDFMT_SHORT: 	
			HCONV_INT( signed short, CLIP_SHORT );
			break;

		case IM_BANDFMT_UINT: 	
			HCONV_INT( unsigned int, CLIP_NONE );
			break;

		case IM_BANDFMT_INT: 	
			HCONV_INT( signed int, CLIP_NONE );
			break;

		case IM_BANDFMT_FLOAT: 	
			HCONV_FLOAT( float );
			break;

		case IM_BANDFMT_DOUBLE: 	
			HCONV_FLOAT( double );
			break;

		case IM_BANDFMT_COMPLEX: 	
			HCONV_FLOAT( float );
			break;

		case IM_BANDFMT_DPCOMPLEX: 	
			HCONV_FLOAT( double );
			break;

		default:
			g_assert( 0 );
		}
	}

	return( 0 );
}

#define VCONV_INT( TYPE, CLIP ) { \
	for( x = 0; x < sz; x++ ) { \
		int *seq_sum = (int *) seq->sum; \
		\
		TYPE *q; \
		TYPE *p; \
		int sum; \
		\
		p = x + (TYPE *) IM_REGION_ADDR( ir, r->left, r->top ); \
		q = x + (TYPE *) IM_REGION_ADDR( or, r->left, r->top ); \
		\
		sum = 0; \
		for( z = 0; z < lines->n_lines; z++ ) { \
			seq_sum[z] = 0; \
			for( y = lines->start[z]; y < lines->end[z]; y++ ) \
				seq_sum[z] += p[y * istride]; \
			sum += lines->factor[z] * seq_sum[z]; \
		} \
		sum = (sum + lines->rounding) / lines->area; \
		CLIP( sum ); \
		*q = sum; \
		q += ostride; \
		\
		for( y = 1; y < r->height; y++ ) { \
			sum = 0; \
			for( z = 0; z < lines->n_lines; z++ ) { \
				seq_sum[z] += p[seq->end[z]]; \
				seq_sum[z] -= p[seq->start[z]]; \
				sum += lines->factor[z] * seq_sum[z]; \
			} \
			p += istride; \
			sum = (sum + lines->rounding) / lines->area; \
			CLIP( sum ); \
			*q = sum; \
			q += ostride; \
		}   \
	} \
}

#define VCONV_FLOAT( TYPE ) { \
	for( x = 0; x < sz; x++ ) { \
		double *seq_sum = (double *) seq->sum; \
		\
		TYPE *q; \
		TYPE *p; \
		double sum; \
		\
		p = x + (TYPE *) IM_REGION_ADDR( ir, r->left, r->top ); \
		q = x + (TYPE *) IM_REGION_ADDR( or, r->left, r->top ); \
		\
		sum = 0; \
		for( z = 0; z < lines->n_lines; z++ ) { \
			seq_sum[z] = 0; \
			for( y = lines->start[z]; y < lines->end[z]; y++ ) \
				seq_sum[z] += p[y * istride]; \
			sum += lines->factor[z] * seq_sum[z]; \
		} \
		sum = sum / lines->area + mask->offset; \
		*q = sum; \
		q += ostride; \
		\
		for( y = 1; y < r->height; y++ ) { \
			sum = 0; \
			for( z = 0; z < lines->n_lines; z++ ) { \
				seq_sum[z] += p[seq->end[z]]; \
				seq_sum[z] -= p[seq->start[z]]; \
				sum += lines->factor[z] * seq_sum[z]; \
			} \
			p += istride; \
			sum = sum / lines->area + mask->offset; \
			*q = sum; \
			q += ostride; \
		}   \
	} \
}

/* Do vertical masks ... we scan the mask down columns of pixels. Copy-paste
 * from above with small changes.
 */
static int
aconv_generate_vertical( REGION *or, void *vseq, void *a, void *b )
{
	AConvSequence *seq = (AConvSequence *) vseq;
	IMAGE *in = (IMAGE *) a;
	Lines *lines = (Lines *) b;

	REGION *ir = seq->ir;
	const int n_lines = lines->n_lines;
	DOUBLEMASK *mask = lines->mask;
	Rect *r = &or->valid;

	/* Double the width (notionally) for complex.
	 */
	int sz = vips_band_format_iscomplex( in->BandFmt ) ? 
		2 * IM_REGION_N_ELEMENTS( or ) : IM_REGION_N_ELEMENTS( or );

	Rect s;
	int x, y, z;
	int istride;
	int ostride;

	/* Prepare the section of the input image we need. A little larger
	 * than the section of the output image we are producing.
	 */
	s = *r;
	s.width += mask->xsize - 1;
	s.height += mask->ysize - 1;
	if( im_prepare( ir, &s ) )
		return( -1 );

	/* Stride can be different for the vertical case, keep this here for
	 * ease of direction change.
	 */
	istride = IM_REGION_LSKIP( ir ) / 
		IM_IMAGE_SIZEOF_ELEMENT( lines->in );
	ostride = IM_REGION_LSKIP( or ) / 
		IM_IMAGE_SIZEOF_ELEMENT( lines->out );

        /* Init offset array. 
	 */
	if( seq->last_stride != istride ) {
		seq->last_stride = istride;

		for( z = 0; z < n_lines; z++ ) {
			seq->start[z] = lines->start[z] * istride;
			seq->end[z] = lines->end[z] * istride;
		}
	}

	switch( in->BandFmt ) {
	case IM_BANDFMT_UCHAR: 	
		VCONV_INT( unsigned char, CLIP_UCHAR );
		break;

	case IM_BANDFMT_CHAR: 	
		VCONV_INT( signed char, CLIP_UCHAR );
		break;

	case IM_BANDFMT_USHORT: 	
		VCONV_INT( unsigned short, CLIP_USHORT );
		break;

	case IM_BANDFMT_SHORT: 	
		VCONV_INT( signed short, CLIP_SHORT );
		break;

	case IM_BANDFMT_UINT: 	
		VCONV_INT( unsigned int, CLIP_NONE );
		break;

	case IM_BANDFMT_INT: 	
		VCONV_INT( signed int, CLIP_NONE );
		break;

	case IM_BANDFMT_FLOAT: 	
		VCONV_FLOAT( float );
		break;

	case IM_BANDFMT_DOUBLE: 	
		VCONV_FLOAT( double );
		break;

	case IM_BANDFMT_COMPLEX: 	
		VCONV_FLOAT( float );
		break;

	case IM_BANDFMT_DPCOMPLEX: 	
		VCONV_FLOAT( double );
		break;

	default:
		g_assert( 0 );
	}

	return( 0 );
}

static int
aconv_raw( IMAGE *in, IMAGE *out, DOUBLEMASK *mask, int n_layers )
{
	Lines *lines;
	im_generate_fn generate;

#ifdef DEBUG
	printf( "aconv_raw: starting with matrix:\n" );
	im_print_dmask( mask );
#endif /*DEBUG*/

	if( !(lines = boxes_new( in, out, mask, n_layers )) )
		return( -1 );

	/* Prepare output. Consider a 7x7 mask and a 7x7 image --- the output
	 * would be 1x1.
	 */
	if( im_cp_desc( out, in ) )
		return( -1 );
	out->Xsize -= mask->xsize - 1;
	out->Ysize -= mask->ysize - 1;
	if( out->Xsize <= 0 || out->Ysize <= 0 ) {
		im_error( "im_aconv", "%s", _( "image too small for mask" ) );
		return( -1 );
	}

	if( mask->xsize == 1 )
		generate = aconv_generate_vertical;
	else 
		generate = aconv_generate_horizontal;

	if( im_demand_hint( out, IM_SMALLTILE, in, NULL ) ||
		im_generate( out, 
			aconv_start, generate, aconv_stop, in, lines ) )
		return( -1 );

	out->Xoffset = -mask->xsize / 2;
	out->Yoffset = -mask->ysize / 2;

	return( 0 );
}

/**
 * im_aconv:
 * @in: input image
 * @out: output image
 * @mask: convolution mask
 * @n_layers: number of layers for approximation
 * @cluster: cluster lines closer than this distance
 *
 * Perform an approximate convolution of @in with @mask.
 *
 * The output image 
 * always has the same #VipsBandFmt as the input image. 
 *
 * Larger values for @n_layers give more accurate
 * results, but are slower. As @n_layers approaches the mask radius, the
 * accuracy will become close to exact convolution and the speed will drop to 
 * match. For many large masks, such as Gaussian, @n_layers need be only 10% of
 * this value and accuracy will still be good.
 *
 * Smaller values of @cluster will give more accurate results, but be slower
 * and use more memory. 10% of the mask radius is a good rule of thumb.
 *
 * See also: im_convsep_f(), im_create_dmaskv().
 *
 * Returns: 0 on success, -1 on error
 */
int 
im_aconv( IMAGE *in, IMAGE *out, DOUBLEMASK *mask, int n_layers, int cluster )
{
	IMAGE *t[2];
	const int n_mask = mask->xsize * mask->ysize;
	DOUBLEMASK *rmask;

	if( im_open_local_array( out, t, 2, "im_aconv", "p" ) ||
		!(rmask = (DOUBLEMASK *) im_local( out, 
		(im_construct_fn) im_dup_dmask,
		(im_callback_fn) im_free_dmask, mask, mask->filename, NULL )) )
		return( -1 );

	rmask->xsize = mask->ysize;
	rmask->ysize = mask->xsize;

	/*
	 */
	if( im_embed( in, t[0], 1, n_mask / 2, n_mask / 2, 
		in->Xsize + n_mask - 1, in->Ysize + n_mask - 1 ) ||
		aconv_raw( t[0], t[1], mask, n_layers ) ||
		aconv_raw( t[1], out, rmask, n_layers ) )
		return( -1 );

	/* For testing .. just try one direction.
	if( aconv_raw( in, out, mask, n_layers ) )
		return( -1 );
	 */

	out->Xoffset = 0;
	out->Yoffset = 0;

	return( 0 );
}

