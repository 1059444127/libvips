/* Write an image to a memory buffer.
 * 
 * 16/4/10
 * 	- from vips_sink()
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
 */
#define VIPS_DEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>

#include <vips/vips.h>
#include <vips/thread.h>
#include <vips/internal.h>
#include <vips/debug.h>

#include "sink.h"

/* Per-call state.
 */
typedef struct _SinkMemory {
	SinkBase sink_base;

	/* A big region for the image memory. All the threads write to this.
	 */
	VipsRegion *all;
} SinkMemory;

static void
sink_memory_free( SinkMemory *memory )
{
	VIPS_UNREF( memory->all );
}

static int
sink_memory_init( SinkMemory *memory, VipsImage *image ) 
{
	VipsRect all;

	vips_sink_base_init( &memory->sink_base, image );

	all.left = 0;
	all.top = 0;
	all.width = image->Xsize;
	all.height = image->Ysize;

	if( !(memory->all = vips_region_new( image )) ||
		vips_region_image( memory->all, &all ) ) {
		sink_memory_free( memory );
		return( -1 );
	}

	return( 0 );
}

static int 
sink_memory_work( VipsThreadState *state, void *a )
{
	SinkMemory *memory = (SinkMemory *) a;

	VIPS_DEBUG_MSG( "sink_memory_work: %p "
		"left = %d, top = %d, width = %d, height = %d\n", 
		memory,
		state->pos.left, 
		state->pos.top, 
		state->pos.width, 
		state->pos.height ); 

	if( vips_region_prepare_to( state->reg, memory->all, 
		&state->pos, state->pos.left, state->pos.top ) )
		return( -1 );

	return( 0 );
}

/**
 * vips_sink_memory:
 * @im: generate this image to memory
 *
 * Loops over an image, generating it to a memory buffer attached to the
 * image. 
 *
 * See also: vips_sink(), vips_get_tile_size().
 *
 * Returns: 0 on success, or -1 on error.
 */
int
vips_sink_memory( VipsImage *image ) 
{
	SinkMemory memory;
	int result;

	VIPS_DEBUG_MSG( "vips_sink_memory: %p\n", image ); 

	g_assert( vips_object_sanity( VIPS_OBJECT( image ) ) );

	/* We don't use this, but make sure it's set in case any old binaries
	 * are expecting it.
	 */
	image->Bbits = vips_format_sizeof( image->BandFmt ) << 3;
 
	if( sink_memory_init( &memory, image ) )
		return( -1 );

	vips_image_preeval( image );

	result = vips_threadpool_run( image, 
		vips_thread_state_new,
		vips_sink_base_allocate, 
		sink_memory_work, 
		vips_sink_base_progress, 
		&memory );

	vips_image_posteval( image );

	sink_memory_free( &memory );

	return( result );
}
