/* Some basic util functions.
 */

/*

    Copyright (C) 1991-2003 The National Gallery

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /*HAVE_UNISTD_H*/
#include <fcntl.h>

#ifdef OS_WIN32
#include <windows.h>
#endif /*OS_WIN32*/

#include <vips/vips.h>
#include <vips/internal.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* Try to make an O_BINARY ... sometimes need the leading '_'.
 */
#ifdef BINARY_OPEN
#ifndef O_BINARY
#ifdef _O_BINARY
#define O_BINARY _O_BINARY
#endif /*_O_BINARY*/
#endif /*!O_BINARY*/
#endif /*BINARY_OPEN*/

/* If we have O_BINARY, add it to a mode flags set.
 */
#ifdef O_BINARY
#define BINARYIZE(M) ((M) | O_BINARY)
#else /*!O_BINARY*/
#define BINARYIZE(M) (M)
#endif /*O_BINARY*/

/* Open mode for image write ... on some systems, have to set BINARY too.
 */
#define MODE_WRITE BINARYIZE (O_WRONLY | O_CREAT | O_TRUNC)

/* Mode for read/write. This is if we might later want to mmaprw () the file.
 */
#define MODE_READWRITE BINARYIZE (O_RDWR)

/* Mode for read only. This is the fallback if READWRITE fails.
 */
#define MODE_READONLY BINARYIZE (O_RDONLY)

/* Temp buffer for snprintf() layer on old systems.
 */
#define MAX_BUF (32768)

/* Test two lists for eqality.
 */
gboolean
vips_slist_equal( GSList *l1, GSList *l2 )
{
	while( l1 && l2 ) {
		if( l1->data != l2->data )
			return( FALSE );

		l1 = l1->next;
		l2 = l2->next;
	}

	if( l1 || l2 )
		return( FALSE );
	
	return( TRUE );
}

/* Map over an slist. _copy() the list in case the callback changes it.
 */
void *
vips_slist_map2( GSList *list, VipsSListMap2Fn fn, void *a, void *b )
{
	GSList *copy;
	GSList *i;
	void *result;

	copy = g_slist_copy( list );
	result = NULL;
	for( i = copy; i && !(result = fn( i->data, a, b )); i = i->next ) 
		;
	g_slist_free( copy );

	return( result );
}

/* Map backwards. We _reverse() rather than recurse and unwind to save stack.
 */
void *
vips_slist_map2_rev( GSList *list, VipsSListMap2Fn fn, void *a, void *b )
{
	GSList *copy;
	GSList *i;
	void *result;

	copy = g_slist_copy( list );
	copy = g_slist_reverse( copy );
	result = NULL;
	for( i = copy; i && !(result = fn( i->data, a, b )); i = i->next ) 
		;
	g_slist_free( copy );

	return( result );
}

void *
vips_slist_map4( GSList *list, 
	VipsSListMap4Fn fn, void *a, void *b, void *c, void *d )
{
	GSList *copy;
	GSList *i;
	void *result;

	copy = g_slist_copy( list );
	result = NULL;
	for( i = copy; 
		i && !(result = fn( i->data, a, b, c, d )); i = i->next ) 
		;
	g_slist_free( copy );

	return( result );
}

void *
vips_slist_fold2( GSList *list, void *start, 
	VipsSListFold2Fn fn, void *a, void *b )
{
        void *c;
        GSList *this, *next;

        for( c = start, this = list; this; this = next ) {
                next = this->next;

                if( !(c = fn( this->data, c, a, b )) )
			return( NULL );
        }

        return( c );
}

/* Remove all occurences of an item from a list.
 */
GSList *
vips_slist_filter( GSList *list, VipsSListMap2Fn fn, void *a, void *b )
{
	GSList *tmp;
	GSList *prev;

	prev = NULL;
	tmp = list;

	while( tmp ) {
		if( fn( tmp->data, a, b ) ) {
			GSList *next = tmp->next;

			if( prev )
				prev->next = next;
			if( list == tmp )
				list = next;

			tmp->next = NULL;
			g_slist_free( tmp );
			tmp = next;
		}
		else {
			prev = tmp;
			tmp = tmp->next;
		}
	}

	return( list );
}

static void
vips_slist_free_all_cb( void * thing, void * dummy )
{
	vips_free( thing );
}

/* Free a g_slist of things which need vips_free()ing.
 */
void
vips_slist_free_all( GSList *list )
{
	g_slist_foreach( list, vips_slist_free_all_cb, NULL );
	g_slist_free( list );
}

void *
vips_map_equal( void *a, void *b )
{
	if( a == b )
		return( a );

	return( NULL );
}

typedef struct {
	void *a;
	void *b;
	VipsSListMap2Fn fn;
	void *result;
} Pair;

static gboolean
vips_hash_table_predicate( const char *key, void *value, Pair *pair )
{
	return( (pair->result == pair->fn( value, pair->a, pair->b )) );
}

/* Like slist map, but for a hash table.
 */
void *
vips_hash_table_map( GHashTable *hash, VipsSListMap2Fn fn, void *a, void *b )
{
	Pair pair;

	pair.a = a;
	pair.b = b;
	pair.fn = fn;
	pair.result = NULL;

	g_hash_table_find( hash, (GHRFunc) vips_hash_table_predicate, &pair ); 

	return( pair.result );
}

/* Like strncpy(), but always NULL-terminate, and don't pad with NULLs.
 */
char *
vips_strncpy( char *dest, const char *src, int n )
{
        int i;

        g_assert( n > 0 );

        for( i = 0; i < n - 1; i++ )
                if( !(dest[i] = src[i]) )
                        break;
        dest[i] = '\0';

        return( dest );
}

/* Find the rightmost occurrence of needle in haystack.
 */
char *
vips_strrstr( const char *haystack, const char *needle )
{
	int haystack_len = strlen( haystack );
	int needle_len = strlen( needle );
	int i;

	for( i = haystack_len - needle_len; i >= 0; i-- )
		if( strncmp( needle, haystack + i, needle_len ) == 0 )
			return( (char *) haystack + i );
	
	return( NULL );
}

/* Test for string b ends string a. 
 */
gboolean
vips_ispostfix( const char *a, const char *b )
{	
	int m = strlen( a );
	int n = strlen( b );

	if( n > m )
		return( FALSE );

	return( strcmp( a + m - n, b ) == 0 );
}

/* Test for string a starts string b. 
 */
gboolean
vips_isprefix( const char *a, const char *b )
{
	int n = strlen( a );
	int m = strlen( b );
	int i;

	if( m < n )
		return( FALSE );
	for( i = 0; i < n; i++ )
		if( a[i] != b[i] )
			return( FALSE );
	
	return( TRUE );
}

/* Like strtok(). Give a string and a list of break characters. Then:
 * - skip initial break characters
 * - EOS? return NULL
 * - skip a series of non-break characters
 * - write a '\0' over the next break character and return a pointer to the
 *   char after that
 *
 * The idea is that this can be used in loops as the iterator. Example:
 *
 * char *p = " 1 2 3   "; // mutable 
 * char *q;
 * int i;
 * int v[...];
 *
 * for( i = 0; (q = vips_break_token( p, " " )); i++, p = q )
 * 	v[i] = atoi( p );
 *
 * will set
 * 	v[0] = 1;
 * 	v[1] = 2;
 * 	v[2] = 3;
 *
 * or with just one pointer, provided your atoi() is OK with trailing chars
 * and you know there is at least one item there
 *
 * char *p = " 1 2 3   "; // mutable
 * int i;
 * int v[...];
 *
 * for( i = 0; p; p = vips_break_token( p, " " ) )
 *   v[i] = atoi( p );
 */
char *
vips_break_token( char *str, const char *brk )
{
        char *p;

        /* Is the string empty? If yes, return NULL immediately.
         */
        if( !str || !*str )
                return( NULL );

        /* Skip initial break characters.
         */
        p = str + strspn( str, brk );

	/* No item?
	 */
        if( !*p ) 
		return( NULL );

        /* We have a token ... search for the first break character after the 
	 * token.
         */
        p += strcspn( p, brk );

        /* Is there string left?
         */
        if( *p ) {
                /* Write in an end-of-string mark and return the start of the
                 * next token.
                 */
                *p++ = '\0';
                p += strspn( p, brk );
        }

        return( p );
}

/* Wrapper over (v)snprintf() ... missing on old systems.
 */
int
vips_vsnprintf( char *str, size_t size, const char *format, va_list ap )
{
#ifdef HAVE_VSNPRINTF
	return( vsnprintf( str, size, format, ap ) );
#else /*HAVE_VSNPRINTF*/
	/* Bleurg!
	 */
	int n;
	static char buf[MAX_BUF];

	if( size > MAX_BUF )
		vips_error_exit( "panic: buffer overflow "
			"(request to write %d bytes to buffer of %d bytes)",
			size, MAX_BUF );
	n = vsprintf( buf, format, ap );
	if( n > MAX_BUF )
		vips_error_exit( "panic: buffer overflow "
			"(%d bytes written to buffer of %d bytes)",
			n, MAX_BUF );

	vips_strncpy( str, buf, size );

	return( n );
#endif /*HAVE_VSNPRINTF*/
}

int
vips_snprintf( char *str, size_t size, const char *format, ... )
{
	va_list ap;
	int n;

	va_start( ap, format );
	n = vips_vsnprintf( str, size, format, ap );
	va_end( ap );

	return( n );
}

/* Split filename into name / mode components. name and mode should both be
 * FILENAME_MAX chars.
 *
 * We look for the ':' splitting the name and mode by searching for the
 * rightmost occurence of the regexp ".[A-Za-z0-9]+:". Example: consider the
 * horror that is
 *
 * 	c:\silly:dir:name\fr:ed.tif:jpeg:95,,,,c:\icc\srgb.icc
 *
 */
void
vips_filename_split( const char *path, char *name, char *mode )
{
        char *p;

        vips_strncpy( name, path, FILENAME_MAX );

	/* Search back towards start stopping at each ':' char.
	 */
	for( p = name + strlen( name ) - 1; p > name; p -= 1 )
		if( *p == ':' ) {
			char *q;

			for( q = p - 1; isalnum( *q ) && q > name; q -= 1 )
				;

			if( *q == '.' )
				break;
		}

	if( *p == ':' ) {
                vips_strncpy( mode, p + 1, FILENAME_MAX );
                *p = '\0';
        }
        else
                strcpy( mode, "" );
}

/* Skip any leading path stuff. Horrible: if this is a filename which came
 * from win32 and we're a *nix machine, it'll have '\\' not '/' as the
 * separator :-(
 *
 * Try to fudge this ... if the file doesn't contain any of our native
 * separators, look for the opposite one as well. If there are none of those
 * either, just return the filename.
 */
const char *
vips_skip_dir( const char *path )
{
	char name[FILENAME_MAX];
	char mode[FILENAME_MAX];
        const char *p;
        const char *q;

	const char native_dir_sep = G_DIR_SEPARATOR;
	const char non_native_dir_sep = native_dir_sep == '/' ? '\\' : '/';

	/* Remove any trailing save modifiers: we don't want '/' or '\' in the
	 * modifier confusing us.
	 */
	vips_filename_split( path, name, mode );

	/* The '\0' char at the end of the string.
	 */
	p = name + strlen( name );

	/* Search back for the first native dir sep, or failing that, the first
	 * non-native dir sep.
	 */
	for( q = p; q > name && q[-1] != native_dir_sep; q-- )
		;
	if( q == name )
		for( q = p; q > name && q[-1] != non_native_dir_sep; q-- )
			;

        return( path + (q - name) );
}

/* Extract suffix from filename, ignoring any mode string. Suffix should be
 * FILENAME_MAX chars. Include the "." character, if any.
 */
void
vips_filename_suffix( const char *path, char *suffix )
{
	char name[FILENAME_MAX];
	char mode[FILENAME_MAX];
        char *p;

	vips_filename_split( path, name, mode );
        if( (p = strrchr( name, '.' )) ) 
                strcpy( suffix, p );
        else
                strcpy( suffix, "" );
}

/* Does a filename have one of a set of suffixes. Ignore case.
 */
int
vips_filename_suffix_match( const char *path, const char *suffixes[] )
{
	char suffix[FILENAME_MAX];
	const char **p;

	vips_filename_suffix( path, suffix );
	for( p = suffixes; *p; p++ )
		if( g_ascii_strcasecmp( suffix, *p ) == 0 )
			return( 1 );

	return( 0 );
}

/* p points to the start of a buffer ... move it on through the buffer (ready
 * for the next call), and return the current option (or NULL for option
 * missing). ',' characters inside options can be escaped with a '\'.
 */
char *
vips_getnextoption( char **in )
{
        char *p = *in;
        char *q = p;

        if( !p || !*p )
                return( NULL );

	/* Find the next ',' not prefixed with a '\'.
	 */
	while( (p = strchr( p, ',' )) && p[-1] == '\\' )
		p += 1;

        if( p ) {
                /* Another option follows this one .. set up to pick that out
                 * next time.
                 */
                *p = '\0';
                *in = p + 1;
        }
        else {
                /* This is the last one.
                 */
                *in = NULL;
        }

        if( strlen( q ) > 0 )
                return( q );
        else
                return( NULL );
}

/* Get a suboption string, or NULL.
 */
char *
vips_getsuboption( const char *buf )
{
        char *p, *q, *r;

        if( !(p = strchr( buf, ':' )) ) 
		/* No suboption.
		 */
		return( NULL );

	/* Step over the ':'.
	 */
	p += 1;

	/* Need to unescape any \, pairs. Shift stuff down one if we find one.
	 */
	for( q = p; *q; q++ ) 
		if( q[0] == '\\' && q[1] == ',' )
			for( r = q; *r; r++ )
				r[0] = r[1];

        return( p );
}

/* Get file length ... 64-bitally. -1 for error.
 */
gint64
vips_file_length( int fd )
{
#ifdef OS_WIN32
	struct _stati64 st;

	if( _fstati64( fd, &st ) == -1 ) {
#else /*!OS_WIN32*/
	struct stat st;

	if( fstat( fd, &st ) == -1 ) {
#endif /*OS_WIN32*/
		vips_error_system( errno, "vips_file_length", 
			"%s", _( "unable to get file stats" ) );
		return( -1 );
	}

	return( st.st_size );
}

/* Wrap write() up
 */
int
vips__write( int fd, const void *buf, size_t count )
{
	do {
		size_t nwritten = write( fd, buf, count );

		if( nwritten == (size_t) -1 ) {
                        vips_error_system( errno, "vips__write", 
				"%s", _( "write failed" ) );
                        return( -1 ); 
		}

		buf = (void *) ((char *) buf + nwritten);
		count -= nwritten;
	} while( count > 0 );

	return( 0 );
}

/* Does a filename contain a directory separator?
 */
static gboolean 
filename_hasdir( const char *filename )
{
	char *dirname;
	gboolean hasdir;

	dirname = g_path_get_dirname( filename );
	hasdir = (strcmp( dirname, "." ) != 0);
	g_free( dirname );

	return( hasdir );
}

/* Open a file. We take an optional fallback dir as well and will try opening
 * there if opening directly fails.
 *
 * This is used for things like finding ICC profiles. We try to open the file
 * directly first, and if that fails and the filename does not contain a
 * directory separator, we try looking in the fallback dir.
 */
FILE *
vips__file_open_read( const char *filename, const char *fallback_dir, 
	gboolean text_mode )
{
	char *mode;
	FILE *fp;

#ifdef BINARY_OPEN
	if( text_mode )
		mode = "r";
	else
		mode = "rb";
#else /*BINARY_OPEN*/
	mode = "r";
#endif /*BINARY_OPEN*/

	if( (fp = fopen( filename, mode )) )
		return( fp );

	if( fallback_dir && !filename_hasdir( filename ) ) {
		char *path;

		path = g_build_filename( fallback_dir, filename, NULL );
	        fp = fopen( path, mode );
		g_free( path );

		if( fp )
			return( fp );
	}

	vips_error( "vips__file_open_read", 
		_( "unable to open file \"%s\" for reading" ), filename );

	return( NULL );
}

FILE *
vips__file_open_write( const char *filename, gboolean text_mode )
{
	char *mode;
	FILE *fp;

#ifdef BINARY_OPEN
	if( text_mode )
		mode = "w";
	else
		mode = "wb";
#else /*BINARY_OPEN*/
	mode = "w";
#endif /*BINARY_OPEN*/

        if( !(fp = fopen( filename, mode )) ) {
		vips_error( "vips__file_open_write", 
			_( "unable to open file \"%s\" for writing" ), 
			filename );
		return( NULL );
	}

	return( fp );
}

/* Load up a file as a string.
 */
char *
vips__file_read( FILE *fp, const char *filename, unsigned int *length_out )
{
        long len;
	size_t read;
        char *str;

        /* Find length.
         */
        fseek( fp, 0L, 2 );
        len = ftell( fp );
	if( len > 20 * 1024 * 1024 ) {
		/* Seems crazy!
		 */
                vips_error( "vips__file_read", 
			_( "\"%s\" too long" ), filename );
                return( NULL );
        }

	if( len == -1 ) {
		int size;

		/* Can't get length: read in chunks and realloc() to end of
		 * file.
		 */
		str = NULL;
		len = 0;
		size = 0;
		do {
			size += 1024;
			if( !(str = realloc( str, size )) ) {
				vips_error( "vips__file_read", 
					"%s", _( "out of memory" ) );
				return( NULL );
			}

			/* -1 to allow space for an extra NULL we add later.
			 */
			read = fread( str + len, sizeof( char ), 
				(size - len - 1) / sizeof( char ),
				fp );
			len += read;
		} while( !feof( fp ) );

#ifdef DEBUG
		printf( "read %ld bytes from unseekable stream\n", len );
#endif /*DEBUG*/
	}
	else {
		/* Allocate memory and fill.    
		 */
		if( !(str = vips_malloc( NULL, len + 1 )) )
			return( NULL );
		rewind( fp );
		read = fread( str, sizeof( char ), (size_t) len, fp );
		if( read != (size_t) len ) {
			vips_free( str );
			vips_error( "vips__file_read", 
				_( "error reading from file \"%s\"" ), 
				filename );
			return( NULL );
		}
	}

	str[len] = '\0';

	if( length_out )
		*length_out = len;

        return( str );
}

/* Load from a filename as a string. Used for things like reading in ICC
 * profiles, ie. binary objects.
 */
char *
vips__file_read_name( const char *filename, const char *fallback_dir, 
	unsigned int *length_out )
{
	FILE *fp;
	char *buffer;

        if( !(fp = vips__file_open_read( filename, fallback_dir, FALSE )) ) 
		return( NULL );
	if( !(buffer = vips__file_read( fp, filename, length_out )) ) {
		fclose( fp );
		return( NULL );
	}
	fclose( fp );

	return( buffer );
}

/* Like fwrite(), but returns non-zero on error and sets error message.
 */
int
vips__file_write( void *data, size_t size, size_t nmemb, FILE *stream )
{
	size_t n;

	if( !data ) 
		return( 0 );

	if( (n = fwrite( data, size, nmemb, stream )) != nmemb ) {
		vips_error( "vips__file_write", 
			_( "writing error (%zd out of %zd blocks written) "
			"... disc full?" ), n, nmemb );
		return( -1 );
	}

	return( 0 );
}

/* Read a few bytes from the start of a file. For sniffing file types.
 * Filename may contain a mode. 
 */
int
vips__get_bytes( const char *filename, unsigned char buf[], int len )
{
	char name[FILENAME_MAX];
	char mode[FILENAME_MAX];
	int fd;

	/* Split off the mode part.
	 */
	im_filename_split( filename, name, mode );

	/* File may not even exist (for tmp images for example!)
	 * so no hasty messages. And the file might be truncated, so no error
	 * on read either.
	 */
	if( (fd = open( name, MODE_READONLY )) == -1 )
		return( 0 );
	if( read( fd, buf, len ) != len ) {
		close( fd );
		return( 0 );
	}
	close( fd );

	return( 1 );
}

/* Alloc/free a GValue.
 */
static GValue *
vips__gvalue_new( GType type )
{
	GValue *value;

	value = g_new0( GValue, 1 );
	g_value_init( value, type );

	return( value );
}

static GValue *
vips__gvalue_copy( GValue *value )
{
	GValue *value_copy;

	value_copy = vips__gvalue_new( G_VALUE_TYPE( value ) );
	g_value_copy( value, value_copy );

	return( value_copy );
}

static void
vips__gvalue_free( GValue *value )
{
	g_value_unset( value );
	g_free( value );
}

GValue *
vips__gvalue_ref_string_new( const char *text )
{
	GValue *value;

	value = vips__gvalue_new( VIPS_TYPE_REF_STRING );
	vips_ref_string_set( value, text );

	return( value );
}

/* Free a GSList of GValue.
 */
void
vips__gslist_gvalue_free( GSList *list )
{
	g_slist_foreach( list, (GFunc) vips__gvalue_free, NULL );
	g_slist_free( list );
}

/* Copy a GSList of GValue.
 */
GSList *
vips__gslist_gvalue_copy( const GSList *list )
{
	GSList *copy;
	const GSList *p;

	copy = NULL;

	for( p = list; p; p = p->next ) 
		copy = g_slist_prepend( copy, 
			vips__gvalue_copy( (GValue *) p->data ) );

	copy = g_slist_reverse( copy );

	return( copy );
}

/* Merge two GSList of GValue ... append to a all elements in b which are not 
 * in a. Return the new value of a. Works for any vips refcounted type 
 * (string, blob, etc.).
 */
GSList *
vips__gslist_gvalue_merge( GSList *a, const GSList *b )
{
	const GSList *i, *j;
	GSList *tail;

	tail = NULL;

	for( i = b; i; i = i->next ) {
		GValue *value = (GValue *) i->data;

		g_assert( G_VALUE_TYPE( value ) == VIPS_TYPE_REF_STRING );

		for( j = a; j; j = j->next ) {
			GValue *value2 = (GValue *) j->data;

			g_assert( G_VALUE_TYPE( value2 ) == 
				VIPS_TYPE_REF_STRING );

			/* Just do a pointer compare ... good enough 99.9% of 
			 * the time.
			 */
			if( vips_ref_string_get( value ) ==
				vips_ref_string_get( value2 ) )
				break;
		}

		if( !j )
			tail = g_slist_prepend( tail, 
				vips__gvalue_copy( value ) );
	}

	a = g_slist_concat( a, g_slist_reverse( tail ) );

	return( a );
}

/* Make a char* from GSList of GValue. Each GValue should be a ref_string.
 * free the result. Empty list -> "", not NULL. Join strings with '\n'.
 */
char *
vips__gslist_gvalue_get( const GSList *list )
{
	const GSList *p;
	size_t length;
	char *all;
	char *q;

	/* Need to estimate length first.
	 */
	length = 0;
	for( p = list; p; p = p->next ) {
		GValue *value = (GValue *) p->data;

		g_assert( G_VALUE_TYPE( value ) == VIPS_TYPE_REF_STRING );

		/* +1 for the newline we will add for each item.
		 */
		length += vips_ref_string_get_length( value ) + 1;
	}

	if( length == 0 )
		return( NULL );

	/* More than 10MB of history? Madness!
	 */
	g_assert( length < 10 * 1024 * 1024 );

	/* +1 for '\0'.
	 */
	if( !(all = vips_malloc( NULL, length + 1 )) )
		return( NULL );

	q = all;
	for( p = list; p; p = p->next ) {
		GValue *value = (GValue *) p->data;

		strcpy( q, vips_ref_string_get( value ) );
		q += vips_ref_string_get_length( value );
		strcpy( q, "\n" );
		q += 1;
	}

	g_assert( (size_t) (q - all) == length );

	return( all );
}

/* Need our own seek(), since lseek() on win32 can't do long files.
 */
int
vips__seek( int fd, gint64 pos )
{
#ifdef OS_WIN32
{
	HANDLE hFile = (HANDLE) _get_osfhandle( fd );
	LARGE_INTEGER p;

	p.QuadPart = pos;
	if( !SetFilePointerEx( hFile, p, NULL, FILE_BEGIN ) ) {
                vips_error_system( GetLastError(), "vips__seek", 
			"%s", _( "unable to seek" ) );
		return( -1 );
	}
}
#else /*!OS_WIN32*/
	if( lseek( fd, pos, SEEK_SET ) == (off_t) -1 ) {
		vips_error( "vips__seek", "%s", _( "unable to seek" ) );
		return( -1 );
	}
#endif /*OS_WIN32*/

	return( 0 );
}

/* Need our own ftruncate(), since ftruncate() on win32 can't do long files.

	DANGER ... this moves the file pointer to the end of file on win32,
	but not on *nix; don't make any assumptions about the file pointer
	position after calling this

 */
int
vips__ftruncate( int fd, gint64 pos )
{
#ifdef OS_WIN32
{
	HANDLE hFile = (HANDLE) _get_osfhandle( fd );
	LARGE_INTEGER p;

	p.QuadPart = pos;
	if( vips__seek( fd, pos ) )
		return( -1 );
	if( !SetEndOfFile( hFile ) ) {
                vips_error_system( GetLastError(), "vips__ftruncate", 
			"%s", _( "unable to truncate" ) );
		return( -1 );
	}
}
#else /*!OS_WIN32*/
	if( ftruncate( fd, pos ) ) {
		vips_error_system( errno, "vips__ftruncate", 
			"%s", _( "unable to truncate" ) );
		return( -1 );
	}
#endif /*OS_WIN32*/

	return( 0 );
}

/* Test for file exists.
 */
int
vips_existsf( const char *name, ... )
{
        va_list ap;
        char buf1[PATH_MAX];

        va_start( ap, name );
        (void) vips_vsnprintf( buf1, PATH_MAX - 1, name, ap );
        va_end( ap );

        /* Try that.
         */
        if( !access( buf1, R_OK ) )
                return( 1 );

        return( 0 );
}

#ifdef OS_WIN32
#define popen(b,m) _popen(b,m)
#define pclose(f) _pclose(f)
#endif /*OS_WIN32*/

/* Do popen(), with printf-style args.
 */
FILE *
vips_popenf( const char *fmt, const char *mode, ... )
{
        va_list args;
	char buf[4096];
	FILE *fp;

        va_start( args, mode );
        (void) vips_vsnprintf( buf, 4096, fmt, args );
        va_end( args );

#ifdef DEBUG
	printf( "vips_popenf: running: %s\n", buf );
#endif /*DEBUG*/

        if( !(fp = popen( buf, mode )) ) {
		vips_error( "popenf", "%s", strerror( errno ) );
		return( NULL );
	}

	return( fp );
}

/* Break a command-line argument into tokens separated by whitespace. Strings
 * can't be adjacent, so "hello world" (without quotes) is a single string.
 * Strings are written (with \" escaped) into string, which must be size 
 * characters large.
 */
const char *
vips__token_get( const char *p, VipsToken *token, char *string, int size )
{
	const char *q;
	int ch;
	int n;

	/* Parse this token with p.
	 */
	if( !p )
		return( NULL );

	/* Skip initial whitespace.
	 */
        p += strspn( p, " \t\n\r" );
	if( !p[0] )
		return( NULL );

	switch( (ch = p[0]) ) {
	case '{':
	case '[':
	case '(':
		*token = VIPS_TOKEN_LEFT;
		p += 1;
		break;

	case ')':
	case ']':
	case '}':
		*token = VIPS_TOKEN_RIGHT;
		p += 1;
		break;

	case '=':
		*token = VIPS_TOKEN_EQUALS;
		p += 1;
		break;

	case ',':
		*token = VIPS_TOKEN_COMMA;
		p += 1;
		break;

	case '"':
	case '\'':
		/* Parse a quoted string. Copy up to ", interpret any \",
		 * error if no closing ".
		 */
		*token = VIPS_TOKEN_STRING;

		do {
			/* Number of characters until the next quote
			 * character or end of string.
			 */
			if( (q = strchr( p + 1, ch )) )
				n = q - p + 1;
			else
				n = strlen( p + 1 );

			g_assert( size > n + 1 );
			memcpy( string, p + 1, n );
			string[n] = '\0';

			/* p[n + 1] might not be " if there's no closing ".
			 */
			if( p[n + 1] == ch && p[n] == '\\' ) 
				/* An escaped ": overwrite the '\' with '"'
				 */
				string[n - 1] = ch;

			string += n;
			size -= n;
			p += n + 1;
		} while( p[0] && p[-1] == '\\' );

		p += 1;

		break;

	default:
		/* It's an unquoted string: read up to the next non-string
		 * character. We don't allow two strings next to each other,
		 * so the next break must be bracket, equals, comma.
		 */
		*token = VIPS_TOKEN_STRING;
		n = strcspn( p, "[{()}]=," );
		g_assert( size > n + 1 );
		memcpy( string, p, n );
		string[n] = '\0';
		p += n;

		/* We remove leading whitespace, so we trim trailing
		 * whitespace from unquoted strings too.
		 */
		while( isspace( string[n - 1] ) ) {
			string[n - 1] = '\0';
			n -= 1;
		}

		break;
	}

	return( p );
}

/* We expect a token.
 */
const char *
vips__token_must( const char *p, VipsToken *token, 
	char *string, int size )
{
	if( !(p = vips__token_get( p, token, string, size )) ) {
		vips_error( "get_token", 
			"%s", _( "unexpected end of string" ) );
		return( NULL );
	}

	return( p );
}

/* Turn a VipsToken into a string.
 */
static const char *
vips__token_string( VipsToken token )
{
	switch( token ) {
 	case VIPS_TOKEN_LEFT:	return( _( "opening brace" ) );
	case VIPS_TOKEN_RIGHT:	return( _( "closing brace" ) );
	case VIPS_TOKEN_STRING:	return( _( "string" ) );
	case VIPS_TOKEN_EQUALS:	return( "=" );
	case VIPS_TOKEN_COMMA:	return( "," );

	default:
		g_assert( 0 );
	}
}

/* We expect a certain token.
 */
const char *
vips__token_need( const char *p, VipsToken need_token, 
	char *string, int size )
{
	VipsToken token;

	if( !(p = vips__token_must( p, &token, string, size )) ) 
		return( NULL );
	if( token != need_token ) {
		vips_error( "get_token", _( "expected %s, saw %s" ), 
			vips__token_string( need_token ), 
			vips__token_string( token ) );
		return( NULL );
	}

	return( p );
}

/* True if an int is a power of two ... 1, 2, 4, 8, 16, 32, etc. Do with just
 * integer arithmetic for portability. A previous Nicos version using doubles
 * and log/log failed on x86 with rounding problems. Return 0 for not
 * power of two, otherwise return the position of the set bit (numbering with
 * bit 1 as the lsb).
 */
int
vips_ispoweroftwo( int p )
{
	int i, n;

	/* Count set bits. Could use a LUT, I guess.
	 */
	for( i = 0, n = 0; p; i++, p >>= 1 )
		if( p & 1 )
			n++;

	/* Should be just one set bit.
	 */
	if( n == 1 )
		/* Return position of bit.
		 */
		return( i );
	else
		return( 0 );
}

/* Test this processor for endianness. True for SPARC order.
 */
int
vips_amiMSBfirst( void )
{
        int test;
        unsigned char *p = (unsigned char *) &test;

        test = 0;
        p[0] = 255;

        if( test == 255 )
                return( 0 );
        else
                return( 1 );
}

/* Return the tmp dir. On Windows, GetTempPath() will also check the values of 
 * TMP, TEMP and USERPROFILE.
 */
static const char *
vips__temp_dir( void )
{
	const char *tmpd;

	if( !(tmpd = g_getenv( "TMPDIR" )) ) {
#ifdef OS_WIN32
		static gboolean done = FALSE;
		static char buf[256];

		if( !done ) {
			if( !GetTempPath( 256, buf ) )
				strcpy( buf, "C:\\temp" );
		}
		tmpd = buf;
#else /*!OS_WIN32*/
		tmpd = "/tmp";
#endif /*!OS_WIN32*/
	}

	return( tmpd );
}

/* Make a temporary file name. The format parameter is something like "%s.jpg" 
 * and will be expanded to something like "/tmp/vips-12-34587.jpg".
 *
 * You need to free the result. A real file will also be created, though we
 * delete it for you.
 */
char *
vips__temp_name( const char *format )
{
	static int serial = 1;

	char file[FILENAME_MAX];
	char file2[FILENAME_MAX];

	char *name;
	int fd;

	vips_snprintf( file, FILENAME_MAX, "vips-%d-XXXXXX", serial++ );
	vips_snprintf( file2, FILENAME_MAX, format, file );
	name = g_build_filename( vips__temp_dir(), file2, NULL );

	if( (fd = g_mkstemp( name )) == -1 ) {
		vips_error( "tempfile", 
			_( "unable to make temporary file %s" ), name );
		g_free( name );
		return( NULL );
	}
	close( fd );
	g_unlink( name );

	return( name );
}

/* Strip off any of a set of old suffixes (eg. [".v", ".jpg"]), add a single 
 * new suffix (eg. ".tif"). 
 */
void
vips__change_suffix( const char *name, char *out, int mx,
        const char *new, const char **olds, int nolds )
{
        char *p;
        int i;
	int len;

        /* Copy start string.
         */
        vips_strncpy( out, name, mx );

        /* Drop all matching suffixes.
         */
        while( (p = strrchr( out, '.' )) ) {
                /* Found suffix - test against list of alternatives. Ignore
                 * case.
                 */
                for( i = 0; i < nolds; i++ )
                        if( g_ascii_strcasecmp( p, olds[i] ) == 0 ) {
                                *p = '\0';
                                break;
                        }

                /* Found match? If not, break from loop.
                 */
                if( *p )
                        break;
        }

        /* Add new suffix.
         */
	len = strlen( out );
	vips_strncpy( out + len, new, mx - len );
}

