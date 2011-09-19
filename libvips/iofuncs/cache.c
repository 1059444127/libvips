/* cache vips operations
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

	should the cache be thread-private? or lock? or say operations can 
	only be made from the main thread?

	listen for invalidate

	can we estimate the resource needs of operations and drop very 
	expensive ones first?

		get vips_malloc()/_free() to track current usage, check that 
		as well as hash table size when looking for cache overflow

 */

/*
#define VIPS_DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /*HAVE_UNISTD_H*/
#include <ctype.h>

#include <vips/vips.h>
#include <vips/internal.h>
#include <vips/debug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* Max cache size.
 */
static int vips_cache_max = 10000;

/* Hold a ref to all "recent" operations.
 */
static GHashTable *vips_cache_table = NULL;

/* A 'time' counter: increment on all cache ops. Use this to detect LRU.
 */
static int vips_cache_time = 0;

/* generic is the general type of the value. For example, the value could be
 * held in a GParamSpec allowing OBJECT, but the value could be of type
 * VipsImage. generics are much faster to compare.
 */
static unsigned int
vips_value_hash( GType generic, GValue *value )
{
	switch( generic ) { 
	case G_TYPE_BOOLEAN:
		return( (unsigned int) g_value_get_boolean( value ) );
	case G_TYPE_CHAR:
		return( (unsigned int) g_value_get_char( value ) );
	case G_TYPE_UCHAR:
		return( (unsigned int) g_value_get_uchar( value ) );
	case G_TYPE_INT:
		return( (unsigned int) g_value_get_int( value ) );
	case G_TYPE_UINT:
		return( (unsigned int) g_value_get_uint( value ) );
	case G_TYPE_LONG:
		return( (unsigned int) g_value_get_long( value ) );
	case G_TYPE_ULONG:
		return( (unsigned int) g_value_get_ulong( value ) );
	case G_TYPE_ENUM:
		return( (unsigned int) g_value_get_enum( value ) );
	case G_TYPE_FLAGS:
		return( (unsigned int) g_value_get_flags( value ) );
	case G_TYPE_UINT64:
		return( (unsigned int) g_value_get_uint64( value ) );

	case G_TYPE_INT64:
	{
		gint64 i = g_value_get_int64( value );

		return( g_int64_hash( &i ) );
	}
	case G_TYPE_FLOAT:
	{
		float f = g_value_get_float( value );

		return( *((unsigned int *) &f) );
	}
	case G_TYPE_DOUBLE:
	{
		double d = g_value_get_double( value );

		return( g_double_hash( &d ) );
	}
	case G_TYPE_STRING:
	{
		const char *s = g_value_get_string( value );

		return( g_str_hash( s ) );
	}
	case G_TYPE_BOXED:
	{
		void *p = g_value_get_boxed( value );

		return( g_direct_hash( p ) );
	}
	case G_TYPE_POINTER:
	{
		void *p = g_value_get_pointer( value );

		return( g_direct_hash( p ) );
	}
	case G_TYPE_OBJECT:
	{
		void *p = g_value_get_object( value );

		return( g_direct_hash( p ) );
	}

	default:
	{
		/* These GTypes are not compile-time constants and need to be
		 * in ifs.
		 */
		if( generic == VIPS_TYPE_IMAGE )
			return( g_direct_hash( g_value_get_object( value ) ) );
		else {
			/* Fallback: convert to a string and hash that. 
			 * This is very slow, print a warning if we use it 
			 * so we can add another case.
			 */
			char *s;
			unsigned int hash;

			s = g_strdup_value_contents( value ); 
			hash = g_str_hash( s );

			printf( "vips_value_hash: no case for %s\n", s );
			printf( "\ttype %d, %s\n", 
				(int) G_VALUE_TYPE( value ),
				g_type_name( G_VALUE_TYPE( value ) ) );
			printf( "\tgeneric %d, %s\n", 
				(int) G_VALUE_TYPE( generic ),
				g_type_name( generic ) );

			g_free( s );

			return( hash );
		}
	}
	}
}

/* generic is the general type of the value. For example, the value could be
 * held in a GParamSpec allowing OBJECT, but the value could be of type
 * VipsImage. generics are much faster to compare.
 */
static gboolean 
vips_value_equal( GType generic, GValue *v1, GValue *v2 )
{
	GType t1 = G_VALUE_TYPE( v1 );
	GType t2 = G_VALUE_TYPE( v2 );

	if( t1 != t2 )
		return( FALSE );

	switch( t1 ) { 
	case G_TYPE_BOOLEAN:
		return( g_value_get_boolean( v1 ) == 
			g_value_get_boolean( v2 ) );
	case G_TYPE_CHAR:
		return( g_value_get_char( v1 ) ==
			g_value_get_char( v2 ) );
	case G_TYPE_UCHAR:
		return( g_value_get_uchar( v1 ) ==
			g_value_get_uchar( v2 ) );
	case G_TYPE_INT:
		return( g_value_get_int( v1 ) ==
			g_value_get_int( v2 ) );
	case G_TYPE_UINT:
		return( g_value_get_uint( v1 ) ==
			g_value_get_uint( v2 ) );
	case G_TYPE_LONG:
		return( g_value_get_long( v1 ) ==
			g_value_get_long( v2 ) );
	case G_TYPE_ULONG:
		return( g_value_get_ulong( v1 ) ==
			g_value_get_ulong( v2 ) );
	case G_TYPE_ENUM:
		return( g_value_get_enum( v1 ) ==
			g_value_get_enum( v2 ) );
	case G_TYPE_FLAGS:
		return( g_value_get_flags( v1 ) ==
			g_value_get_flags( v2 ) );
	case G_TYPE_UINT64:
		return( g_value_get_uint64( v1 ) ==
			g_value_get_uint64( v2 ) );
	case G_TYPE_INT64:
		return( g_value_get_int64( v1 ) ==
			g_value_get_int64( v2 ) );
	case G_TYPE_FLOAT:
		return( g_value_get_float( v1 ) ==
			g_value_get_float( v2 ) );
	case G_TYPE_DOUBLE:
		return( g_value_get_double( v1 ) ==
			g_value_get_double( v2 ) );
	case G_TYPE_STRING:
		return( strcmp( g_value_get_string( v1 ),
			g_value_get_string( v2 ) ) == 0 );
	case G_TYPE_BOXED:
		return( g_value_get_boxed( v1 ) ==
			g_value_get_boxed( v2 ) );
	case G_TYPE_POINTER:
		return( g_value_get_pointer( v1 ) ==
			g_value_get_pointer( v2 ) );
	case G_TYPE_OBJECT:
		return( g_value_get_object( v1 ) ==
			g_value_get_object( v2 ) );

	default:
	{
		/* These GTypes are not compile-time constants and need to be
		 * in ifs.
		 */
		if( generic == VIPS_TYPE_IMAGE )
			return( g_value_get_object( v1 ) ==
				g_value_get_object( v2 ) );
		else {
			/* Fallback: convert to a string and compare that. 
			 * This is very slow, print a warning if we use it 
			 * so we can add another case.
			 */
			char *s1;
			char *s2;
			gboolean equal;

			s1 = g_strdup_value_contents( v1 ); 
			s2 = g_strdup_value_contents( v2 ); 
			equal = strcmp( s1, s2 ) == 0;

			printf( "vips_value_equal: no case for %s, %s\n", 
				s1, s2 );
			printf( "\tt1 %d, %s\n", (int) t1, g_type_name( t1 ) );
			printf( "\tt2 %d, %s\n", (int) t2, g_type_name( t2 ) );
			printf( "\tgeneric %d, %s\n", 
				(int) G_VALUE_TYPE( generic ),
				g_type_name( generic ) );

			g_free( s1 );
			g_free( s2 );

			return( equal );
		}
	}
	}
}

static void *
vips_object_hash_arg( VipsObject *object,
	GParamSpec *pspec,
	VipsArgumentClass *argument_class,
	VipsArgumentInstance *argument_instance,
	void *a, void *b )
{
	unsigned int *hash = (unsigned int *) a;

	if( (argument_class->flags & VIPS_ARGUMENT_CONSTRUCT) &&
		(argument_class->flags & VIPS_ARGUMENT_INPUT) &&
		argument_instance->assigned ) {
		GType type = G_PARAM_SPEC_VALUE_TYPE( pspec );
		GValue value = { 0, };

		g_value_init( &value, type );
		g_object_get_property( G_OBJECT( object ), 
			g_param_spec_get_name( pspec ), &value ); 
		*hash = (*hash << 1) ^ vips_value_hash( type, &value );
		g_value_unset( &value );
	}

	return( NULL );
}

/* Find a hash from the input arguments to a VipsOperstion.
 */
static unsigned int
vips_operation_hash( VipsOperation *operation )
{
	if( !operation->found_hash ) {
		guint hash;

		/* Include the operation type in the hash.
		 */
		hash = (guint) G_OBJECT_TYPE( operation );
		(void) vips_argument_map( VIPS_OBJECT( operation ),
			vips_object_hash_arg, &hash, NULL );

		/* Make sure we can't have a zero hash value.
		 */
		hash |= 1;

		operation->hash = hash;
		operation->found_hash = TRUE;
	}

	return( operation->hash );
}

static void *
vips_object_equal_arg( VipsObject *object,
	GParamSpec *pspec,
	VipsArgumentClass *argument_class,
	VipsArgumentInstance *argument_instance,
	void *a, void *b )
{
	VipsObject *other = (VipsObject *) a;

	if( (argument_class->flags & VIPS_ARGUMENT_CONSTRUCT) &&
		(argument_class->flags & VIPS_ARGUMENT_INPUT) &&
		argument_instance->assigned ) {
		const char *name = g_param_spec_get_name( pspec );
		GType type = G_PARAM_SPEC_VALUE_TYPE( pspec );
		GValue v1 = { 0, };
		GValue v2 = { 0, };

		gboolean equal;

		g_value_init( &v1, type );
		g_value_init( &v2, type );
		g_object_get_property( G_OBJECT( object ), name, &v1 ); 
		g_object_get_property( G_OBJECT( other ), name, &v2 ); 
		equal = vips_value_equal( type, &v1, &v2 );
		g_value_unset( &v1 );
		g_value_unset( &v2 );

		if( !equal )
			return( object );
	}

	return( NULL );
}

/* Are two objects equal, ie. have the same inputs.
 */
static gboolean 
vips_operation_equal( VipsOperation *a, VipsOperation *b )
{
	if( G_OBJECT_TYPE( a ) == G_OBJECT_TYPE( b ) &&
		vips_operation_hash( a ) == vips_operation_hash( b ) &&
		!vips_argument_map( VIPS_OBJECT( a ), 
			vips_object_equal_arg, b, NULL ) )
		return( TRUE );

	return( FALSE );
}

static void *
vips_object_ref_arg( VipsObject *object,
	GParamSpec *pspec,
	VipsArgumentClass *argument_class,
	VipsArgumentInstance *argument_instance,
	void *a, void *b )
{
	if( (argument_class->flags & VIPS_ARGUMENT_CONSTRUCT) &&
		(argument_class->flags & VIPS_ARGUMENT_OUTPUT) &&
		argument_instance->assigned &&
		G_IS_PARAM_SPEC_OBJECT( pspec ) ) {
		GObject *value;

		/* This will up the ref count for us.
		 */
		g_object_get( G_OBJECT( object ), 
			g_param_spec_get_name( pspec ), &value, NULL );
	}

	return( NULL );
}

/* All the output objects need reffing for this new usage.
 */
static gboolean
vips_object_ref_outputs( VipsObject *object )
{
	return( !vips_argument_map( object,
		vips_object_ref_arg, NULL, NULL ) );
}

static void
vips_operation_touch( VipsOperation *operation )
{
	vips_cache_time += 1;
	operation->time = vips_cache_time;
}

/* Look up an object in the cache. If we get a hit, unref the new one, ref the
 * old one and return that. 
 *
 * If we miss, build and add this object.
 */
int
vips_operation_build_cache( VipsOperation **operation )
{
	VipsOperation *hit;

	VIPS_DEBUG_MSG( "vips_operation_build_cache: %p\n", *object );

	if( !vips_cache_table ) 
		vips_cache_table = g_hash_table_new( 
			(GHashFunc) vips_operation_hash, 
			(GEqualFunc) vips_operation_equal );

	if( (hit = g_hash_table_lookup( vips_cache_table, *operation )) ) {
		VIPS_DEBUG_MSG( "\thit %p\n", hit );

		g_object_unref( *operation );
		g_object_ref( hit );
		vips_object_ref_outputs( VIPS_OBJECT( hit ) );
		vips_operation_touch( hit );
		*operation = hit;
	}
	else {
		VIPS_DEBUG_MSG( "\tmiss, build and add\n" );

		if( vips_object_build( VIPS_OBJECT( *operation ) ) )
			return( -1 );
		g_object_ref( *operation );
		vips_object_ref_outputs( VIPS_OBJECT( *operation ) );
		vips_operation_touch( *operation );

		g_hash_table_insert( vips_cache_table, *operation, *operation );
	}

	return( 0 );
}
