/* im_invfftr
 *
 * Modified on :
 * 27/2/03 JC
 *	- from im_invfft.c
 * 22/1/04 JC
 *	- oops, fix for segv on wider than high fftw transforms
 * 3/11/04
 *	- added fftw3 support
 * 7/2/10
 * 	- gtkdoc
 * 27/1/12
 * 	- better setting of interpretation
 * 	- remove own fft fallback code
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
#include <math.h>

#ifdef HAVE_FFTW
#include <rfftw.h>
#endif /*HAVE_FFTW*/

#ifdef HAVE_FFTW3
#include <fftw3.h>
#endif /*HAVE_FFTW3*/

#include <vips/vips.h>
#include <vips/internal.h>

#ifdef HAVE_FFTW

/* Use fftw2.
 */
static int 
invfft1( IMAGE *dummy, IMAGE *in, IMAGE *out )
{
	IMAGE *cmplx = im_open_local( dummy, "invfft1-1", "t" );
	IMAGE *real = im_open_local( out, "invfft1-2", "t" );
	const int half_width = in->Xsize / 2 + 1;

	/* Transform to halfcomplex here.
	 */
	double *half_complex = IM_ARRAY( dummy, 
		in->Ysize * half_width * 2, double );

	rfftwnd_plan plan;
	int x, y;
	double *q, *p;

	if( !cmplx || !real || !half_complex || im_pincheck( in ) || 
		im_poutcheck( out ) )
		return( -1 );
	if( in->Coding != IM_CODING_NONE || in->Bands != 1 ) {
                im_error( "im_invfft", _( "one band uncoded only" ) );
                return( -1 );
	}

	/* Make dp complex image for input.
	 */
	if( im_clip2fmt( in, cmplx, IM_BANDFMT_DPCOMPLEX ) )
                return( -1 );

	/* Build half-complex image.
	 */
	if( im_incheck( cmplx ) )
		return( -1 );
	q = half_complex;
	for( y = 0; y < cmplx->Ysize; y++ ) {
		p = ((double *) cmplx->data) + (guint64) y * in->Xsize * 2; 

		for( x = 0; x < half_width; x++ ) {
			q[0] = p[0];
			q[1] = p[1];
			p += 2;
			q += 2;
		}
	}

	/* Make mem buffer real image for output.
	 */
        if( im_cp_desc( real, in ) )
                return( -1 );
	real->BandFmt = IM_BANDFMT_DOUBLE;
	real->Type = IM_TYPE_B_W;
        if( im_setupout( real ) )
                return( -1 );

	/* Make the plan for the transform. Yes, they really do use nx for
	 * height and ny for width.
	 */
	if( !(plan = rfftw2d_create_plan( in->Ysize, in->Xsize,
		FFTW_BACKWARD, FFTW_MEASURE | FFTW_USE_WISDOM )) ) {
                im_error( "im_invfft", _( "unable to create transform plan" ) );
		return( -1 );
	}

	rfftwnd_one_complex_to_real( plan, 
		(fftw_complex *) half_complex, (fftw_real *) real->data );

	rfftwnd_destroy_plan( plan );

	/* Copy to out.
	 */
        if( im_copy( real, out ) )
                return( -1 );

	return( 0 );
}

#elif defined HAVE_FFTW3

/* Complex to real inverse transform.
 */
static int 
invfft1( IMAGE *dummy, IMAGE *in, IMAGE *out )
{
	IMAGE *cmplx = im_open_local( dummy, "invfft1-1", "t" );
	IMAGE *real = im_open_local( out, "invfft1-2", "t" );
	const int half_width = in->Xsize / 2 + 1;

	/* Transform to halfcomplex here.
	 */
	double *half_complex = IM_ARRAY( dummy, 
		in->Ysize * half_width * 2, double );

	/* We have to have a separate real buffer for the planner to work on.
	 */
	double *planner_scratch = IM_ARRAY( dummy, 
		in->Ysize * half_width * 2, double );

	fftw_plan plan;
	int x, y;
	double *q, *p;

	if( !cmplx || !real || !half_complex || 
		im_pincheck( in ) || 
		im_poutcheck( out ) )
		return( -1 );
	if( in->Coding != IM_CODING_NONE || in->Bands != 1 ) {
                im_error( "im_invfft", 
			"%s", _( "one band uncoded only" ) );
                return( -1 );
	}

	/* Make dp complex image for input.
	 */
	if( im_clip2fmt( in, cmplx, IM_BANDFMT_DPCOMPLEX ) )
                return( -1 );

	/* Build half-complex image.
	 */
	if( im_incheck( cmplx ) )
		return( -1 );
	q = half_complex;
	for( y = 0; y < cmplx->Ysize; y++ ) {
		p = ((double *) cmplx->data) + (guint64) y * in->Xsize * 2; 

		for( x = 0; x < half_width; x++ ) {
			q[0] = p[0];
			q[1] = p[1];
			p += 2;
			q += 2;
		}
	}

	/* Make mem buffer real image for output.
	 */
        if( im_cp_desc( real, in ) )
                return( -1 );
	real->BandFmt = IM_BANDFMT_DOUBLE;
	real->Type = IM_TYPE_B_W;
        if( im_setupout( real ) ||
		im_outcheck( real ) )
                return( -1 );

	/* Make the plan for the transform. Yes, they really do use nx for
	 * height and ny for width.
	 */
	if( !(plan = fftw_plan_dft_c2r_2d( in->Ysize, in->Xsize,
		(fftw_complex *) planner_scratch, (double *) real->data,
		0 )) ) {
                im_error( "im_invfft", 
			"%s", _( "unable to create transform plan" ) );
		return( -1 );
	}

	fftw_execute_dft_c2r( plan,
		(fftw_complex *) half_complex, (double *) real->data );

	fftw_destroy_plan( plan );

	/* Copy to out.
	 */
        if( im_copy( real, out ) )
                return( -1 );

	return( 0 );
}

#else 

static int 
invfft1( IMAGE *dummy, IMAGE *in, IMAGE *out )
{
	im_error( "im_invfftr", 
		"%s", _( "vips configured without FFT support" ) );
	return( -1 );
}

#endif 

/**
 * im_invfftr:
 * @in: input image
 * @out: output image
 *
 * Transform an image from Fourier space to real space, giving a real result.
 * This is faster than im_invfft(), which gives a complex result. 
 *
 * VIPS uses the fftw3 or fftw2 Fourier transform libraries if possible. If 
 * they were not available when VIPS was built, it falls back to it's own 
 * FFT functions which are slow and only work for square images whose sides
 * are a power of two.
 *
 * See also: im_invfft(), im_fwfft(), im_disp_ps().
 *
 * Returns: 0 on success, -1 on error.
 */
int 
im_invfftr( IMAGE *in, IMAGE *out )
{
	IMAGE *dummy = im_open( "im_invfft:1", "p" );

	if( !dummy )
		return( -1 );
	if( im__fftproc( dummy, in, out, invfft1 ) ) {
		im_close( dummy );
		return( -1 );
	}
	im_close( dummy );

	return( 0 );
}
