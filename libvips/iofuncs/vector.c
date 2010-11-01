/* helper functions for Orc
 *
 * 29/10/10
 * 	- from morph hacking
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

 	TODO

	- would setting params by index rather than name be any quicker?

 */

/* Verbose messages from Orc (or use ORC_DEBUG=99 on the command-line).
#define DEBUG_ORC
 */

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <vips/vips.h>
#include <vips/vector.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* Cleared by the command-line --vips-novector switch and the IM_NOVECTOR env
 * var.
 */
gboolean im__vector_enabled = TRUE;

void 
vips_vector_init( void )
{
#ifdef HAVE_ORC
	orc_init();

#ifdef DEBUG_ORC
	/* You can also do ORC_DEBUG=99 at the command-line.
	 */
	orc_debug_set_level( 99 );
#endif /*DEBUG_ORC*/

	/* Look for the environment variable IM_NOVECTOR and use that to turn
	 * off as well.
	 */
	if( g_getenv( "IM_NOVECTOR" ) ) 
		im__vector_enabled = FALSE;
#endif /*HAVE_ORC*/
}

gboolean 
vips_vector_get_enabled( void )
{
	return( im__vector_enabled );
}

void 
vips_vector_set_enabled( gboolean enabled )
{
	im__vector_enabled = enabled;
}

void
vips_vector_free( VipsVector *vector )
{
#ifdef HAVE_ORC
	IM_FREEF( orc_program_free, vector->program );
#endif /*HAVE_ORC*/
	IM_FREE( vector );
}

VipsVector *
vips_vector_new_ds( const char *name, int size1, int size2 )
{
	VipsVector *vector;

	if( !(vector = IM_NEW( NULL, VipsVector )) )
		return( NULL );
	vector->name = name;
	vector->n_temp = 0;
	vector->n_source = 0;
	vector->n_destination = 0;
	vector->n_constant = 0;
	vector->n_parameter = 0;
	vector->n_instruction = 0;
	vector->compiled = FALSE;

#ifdef HAVE_ORC
	vector->program = orc_program_new_ds( size1, size2 );
#endif /*HAVE_ORC*/
	vector->n_source += 1;
	vector->n_destination += 1;

	return( vector );
}

void 
vips_vector_asm2( VipsVector *vector, 
	const char *op, const char *a, const char *b )
{
	vector->n_instruction += 1;

#ifdef DEBUG
	 printf( "  %s %s %s\n", op, a, b );
#endif /*DEBUG*/

#ifdef HAVE_ORC
	 orc_program_append_ds_str( vector->program, op, a, b );
#endif /*HAVE_ORC*/
}

void 
vips_vector_asm3( VipsVector *vector, 
	const char *op, const char *a, const char *b, const char *c )
{
	vector->n_instruction += 1;

#ifdef DEBUG
	 printf( "  %s %s %s %s\n", op, a, b, c );
#endif /*DEBUG*/

#ifdef HAVE_ORC
	 orc_program_append_str( vector->program, op, a, b, c );
#endif /*HAVE_ORC*/
}

void
vips_vector_constant( VipsVector *vector, char *name, int value, int size )
{
#ifdef HAVE_ORC
	char *sname;

	if( size == 1 )
		sname = "b";
	else if( size == 2 )
		sname = "w";
	else if( size == 4 )
		sname = "l";
	else {
		printf( "vips_vector_constant: bad constant size\n" );

		/* Not really correct, heh.
		 */
		sname = "x";
	}

	if( value > 0 )
		im_snprintf( name, 256, "c%d%s", value, sname );
	else
		im_snprintf( name, 256, "cm%d%s", -value, sname );

	if( orc_program_find_var_by_name( vector->program, name ) == -1 ) {
		orc_program_add_constant( vector->program, size, value, name );
		vector->n_constant += 1;
	}
#endif /*HAVE_ORC*/
}

void
vips_vector_source_name( VipsVector *vector, char *name, int size )
{
#ifdef HAVE_ORC
#ifdef DEBUG
	if( orc_program_find_var_by_name( vector->program, name ) != -1 ) 
		printf( "argh! source %s defined twice\n", name );
#endif /*DEBUG*/

	orc_program_add_source( vector->program, size, name );
	vector->n_source += 1;
#endif /*HAVE_ORC*/
}

void
vips_vector_source( VipsVector *vector, char *name, int number, int size )
{
#ifdef HAVE_ORC
	im_snprintf( name, 256, "s%d", number );

	if( orc_program_find_var_by_name( vector->program, name ) == -1 ) 
		vips_vector_source_name( vector, name, size ); 
#endif /*HAVE_ORC*/
}

void
vips_vector_temporary( VipsVector *vector, char *name, int size )
{
#ifdef HAVE_ORC
	orc_program_add_temporary( vector->program, size, name );
	vector->n_temp += 1;
#endif /*HAVE_ORC*/
}

gboolean
vips_vector_full( VipsVector *vector )
{
	/* We can need a max of 2 constants plus one source per
	 * coefficient, so stop if we're sure we don't have enough.
	 * We need to stay under the 100 instruction limit too.
	 */
	if( vector->n_constant > 16 - 2 )
		return( TRUE );
	if( vector->n_source > 8 - 1 )
		return( TRUE );
	if( vector->n_instruction > 50 )
		return( TRUE );

	return( FALSE );
}

gboolean
vips_vector_compile( VipsVector *vector )
{
#ifdef HAVE_ORC
	OrcCompileResult result;

	result = orc_program_compile( vector->program );
	if( !ORC_COMPILE_RESULT_IS_SUCCESSFUL( result ) ) {
#ifdef DEBUG
		printf( "*** error compiling %s\n", vector->name );
#endif /*DEBUG*/

		return( FALSE );
	}

	vector->compiled = TRUE;
#endif /*HAVE_ORC*/

	return( TRUE );
}

void
vips_vector_print( VipsVector *vector )
{
	printf( "%s: ", vector->name );
	if( vector->compiled )
		printf( "successfully compiled\n" );
	else
		printf( "not compiled successfully\n" );
	printf( "  n_source = %d\n", vector->n_source );
	printf( "  n_parameter = %d\n", vector->n_parameter );
	printf( "  n_destination = %d\n", vector->n_destination );
	printf( "  n_constant = %d\n", vector->n_constant );
	printf( "  n_temp = %d\n", vector->n_temp );
	printf( "  n_instruction = %d\n", vector->n_instruction );
}

void
vips_executor_set_program( VipsExecutor *executor, VipsVector *vector, int n )
{
#ifdef HAVE_ORC
	orc_executor_set_program( executor, vector->program );
	orc_executor_set_n( executor, n );
#endif /*HAVE_ORC*/
}

void
vips_executor_set_source( VipsExecutor *executor, int n, void *value )
{
#ifdef HAVE_ORC
	char name[256];
	OrcProgram *program = executor->program;

	im_snprintf( name, 256, "s%d", n );
	if( orc_program_find_var_by_name( program, name ) != -1 ) 
		orc_executor_set_array_str( executor, name, value );
#endif /*HAVE_ORC*/
}

void
vips_executor_set_destination( VipsExecutor *executor, void *value )
{
#ifdef HAVE_ORC
	orc_executor_set_array_str( executor, "d1", value );
#endif /*HAVE_ORC*/
}

void
vips_executor_set_array( VipsExecutor *executor, char *name, void *value )
{
#ifdef HAVE_ORC
	OrcProgram *program = executor->program;

	if( orc_program_find_var_by_name( program, name ) != -1 ) 
		orc_executor_set_array_str( executor, name, value );
#endif /*HAVE_ORC*/
}

void
vips_executor_run( VipsExecutor *executor )
{
#ifdef HAVE_ORC
	orc_executor_run( executor );
#endif /*HAVE_ORC*/
}

