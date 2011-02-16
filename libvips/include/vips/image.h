/* VIPS image class.
 *
 * 7/7/09
 * 	- from vips.h
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

#ifndef IM_IMAGE_H
#define IM_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* If you read MSB first, you get these two values.  
 * intel order: byte 0 = b6 
 * SPARC order: byte 0 = 08
 */
#define VIPS_MAGIC_INTEL (0xb6a6f208U)
#define VIPS_MAGIC_SPARC (0x08f2a6b6U)

/* Preferred demand type.
 */
typedef enum {
	VIPS_DEMAND_STYLE_SMALLTILE,	
	VIPS_DEMAND_STYLE_FATSTRIP,
	VIPS_DEMAND_STYLE_THINSTRIP,
	VIPS_DEMAND_STYLE_ANY			
} VipsDemandStyle;

/* Types of image descriptor we may have. The type field is advisory only: it
 * does not imply that any fields in IMAGE have valid data.
 */
typedef enum {
	VIPS_IMAGE_NONE,		/* no type set */
	VIPS_IMAGE_SETBUF,		/* malloced memory array */
	VIPS_IMAGE_SETBUF_FOREIGN,	/* memory array, don't free on close */
	VIPS_IMAGE_OPENIN,		/* input from fd with a window */
	VIPS_IMAGE_MMAPIN,		/* memory mapped input file */
	VIPS_IMAGE_MMAPINRW,		/* memory mapped read/write file */
	VIPS_IMAGE_OPENOUT,		/* output to fd */
	VIPS_IMAGE_PARTIAL		/* partial image */
} VipsImageType;

/* The gaps in the numbering are historical and need maintaining. Allocate new
 * numbers from the end.
 */
typedef enum {
	VIPS_INTERPRETATION_MULTIBAND = 0,
	VIPS_INTERPRETATION_B_W = 1,
	VIPS_INTERPRETATION_HISTOGRAM = 10,
	VIPS_INTERPRETATION_FOURIER = 24,
	VIPS_INTERPRETATION_XYZ = 12,
	VIPS_INTERPRETATION_LAB = 13,
	VIPS_INTERPRETATION_CMYK = 15,
	VIPS_INTERPRETATION_LABQ = 16,
	VIPS_INTERPRETATION_RGB = 17,
	VIPS_INTERPRETATION_UCS = 18,
	VIPS_INTERPRETATION_LCH = 19,
	VIPS_INTERPRETATION_LABS = 21,
	VIPS_INTERPRETATION_sRGB = 22,
	VIPS_INTERPRETATION_YXY = 23,
	VIPS_INTERPRETATION_RGB16 = 25,
	VIPS_INTERPRETATION_GREY16 = 26
} VipsInterpretation;

typedef enum {
	VIPS_FORMAT_NOTSET = -1,
	VIPS_FORMAT_UCHAR = 0,
	VIPS_FORMAT_CHAR = 1,
	VIPS_FORMAT_USHORT = 2,
	VIPS_FORMAT_SHORT = 3,
	VIPS_FORMAT_UINT = 4,
	VIPS_FORMAT_INT = 5,
	VIPS_FORMAT_FLOAT = 6,
	VIPS_FORMAT_COMPLEX = 7,
	VIPS_FORMAT_DOUBLE = 8,
	VIPS_FORMAT_DPCOMPLEX = 9,
	VIPS_FORMAT_LAST = 10
} VipsBandFormat;

typedef enum {
	VIPS_CODING_NONE = 0,
	VIPS_CODING_LABQ = 2,
	VIPS_CODING_RAD = 6
} VipsCoding;

/* Struct we keep a record of execution time in. Passed to eval signal so
 * it can assess progress.
 */
typedef struct {
	/*< private >*/
	struct _VipsImage *im;	/* Image we are part of */

	/*< public >*/
	int run;		/* Time we have been running */
	int eta;		/* Estimated seconds of computation left */
	gint64 tpels;		/* Number of pels we expect to calculate */
	gint64 npels;		/* Number of pels calculated so far */
	int percent;		/* Percent complete */
	GTimer *start;		/* Start time */
} VipsProgress;

#define VIPS_TYPE_IMAGE (vips_image_get_type())
#define VIPS_IMAGE( obj ) \
	(G_TYPE_CHECK_INSTANCE_CAST( (obj), \
	VIPS_TYPE_IMAGE, VipsImage ))
#define VIPS_IMAGE_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_CAST( (klass), \
	VIPS_TYPE_IMAGE, VipsImageClass))
#define VIPS_IS_IMAGE( obj ) \
	(G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_IMAGE ))
#define VIPS_IS_IMAGE_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_IMAGE ))
#define VIPS_IMAGE_GET_CLASS( obj ) \
	(G_TYPE_INSTANCE_GET_CLASS( (obj), \
	VIPS_TYPE_IMAGE, VipsImageClass ))

typedef struct _VipsImage {
	VipsObject parent_object;

	/*< private >*/

	/* We have to keep these names for compatibility with the old API.
	 * Don't use them though, use vips_image_get_width() and friends.
	 */

	int Xsize;		/* image width, in pixels */
	int Ysize;		/* image height, in pixels */
	int Bands;		/* number of image bands */

	VipsBandFormat BandFmt;	/* pixel format */
	VipsCoding Coding;	/* pixel coding */
	VipsInterpretation Type;/* pixel interpretation */
	float Xres;		/* horizontal pixels per millimetre */
	float Yres;		/* vertical pixels per millimetre */

	int Xoffset;		/* image origin hint */
	int Yoffset;		/* image origin hint */

	/* No longer used, the names are here for compat with very, very old 
	 * code.
	 */
	int Length;
	short Compression;
	short Level;
	int Bbits;		/* was number of bits in this format */

	/* Derived fields that some code can fiddle with. New code should use
	 * vips_image_get_history() and friends.
	 */
	char *Hist;		/* don't use ... call im_history_get() */
	char *filename;		/* pointer to copy of filename */
	char *data;		/* start of image data for WIO */
	int kill;		/* set to non-zero to block eval */

	/* Everything below this private and only used internally by
	 * VipsImage.
	 */

	char *mode;		/* mode string passed to _new() */
	VipsImageType dtype;	/* descriptor type */
	int fd;         	/* file descriptor */
	char *baseaddr;     	/* pointer to the start of an mmap file */
	size_t length;		/* size of mmap area */
	guint32 magic;		/* magic from header, endian-ness of image */

	/* Partial image stuff. All private! All these fields are initialised 
	 * to NULL and ignored unless set by im_generate() or im_partial().
	 */
	void *(*start)();	/* user-supplied start function */
	int (*generate)();	/* user-supplied generate function */
	int (*stop)();		/* user-supplied stop function */
	void *client1;		/* user arguments */
	void *client2;
	GMutex *sslock;		/* start-stop lock */
	GSList *regions; 	/* list of regions current for this image */
	VipsDemandStyle dhint;	/* demand style hint */

	/* Extra user-defined fields ... see im_meta_get_int() etc.
	 */
	GHashTable *Meta;	/* GhashTable of GValue */
	GSList *Meta_traverse;	/* traverse order for Meta */

	/* Part of mmap() read ... the sizeof() the header we skip from the
	 * file start. Usually IM_SIZEOF_HEADER, but can be something else
	 * for binary file read.
	 */
	int sizeof_header;

	/* If this is a large disc image, don't map the whole thing, instead
	 * have a set of windows shared between the regions active on the
	 * image. List of im_window_t.
	 */
	GSList *windows;

	/* Upstream/downstream relationships, built from args to 
	 * im_demand_hint().
	 *
	 * We use these to invalidate downstream pixel buffers on 
	 * im_invalidate(). Use 'serial' to spot circular dependencies.
	 *
	 * See also hint_set below.
	 */
	GSList *upstream;
	GSList *downstream;
	int serial;

	/* Keep a list of recounted GValue strings so we can share hist
	 * efficiently.
	 */
	GSList *history_list;

	/* The VipsImage (if any) we should signal eval progress on.
	 */
	struct _VipsImage *progress;

	/* Record the file length here. We use this to stop ourselves mapping
	 * things beyond the end of the file in the case that the file has
	 * been truncated.
	 *
	 * gint64 so that we can guarantee to work even on systems with
	 * strange ideas about large files.
	 */
	gint64 file_length;

	/* Set this when im_demand_hint_array() is called, and check in any
	 * operation that will demand pixels from the image.
	 *
	 * We use im_demand_hint_array() to build the tree of
	 * upstream/downstream relationships, so it's a mandatory thing.
	 */
	gboolean hint_set;

} VipsImage;

typedef struct _VipsImageClass {
	VipsObjectClass parent_class;

	/* Signals we emit.
	 */

	/* Evaluation is starting.
	 */
	void (*preeval)( VipsImage *image );

	/* Evaluation progress.
	 */
	void (*eval)( VipsImage *image, VipsProgress *progress );

	/* Evaluation is ending.
	 */
	void (*posteval)( VipsImage *image );

	/* An image has been written to. 
	 * Used by eg. im_open("x.jpg", "w") to do the final write to jpeg.
	 */
	void (*written)( VipsImage *image );

	/* An image has been modified in some way and downstream caches all
	 * need dropping. 
	 */
	int (*invalidate)( VipsImage *image );

} VipsImageClass;

extern const size_t vips__sizeof_bandfmt[];

/* Pixel address calculation macros.
 */
#define VIPS_IMAGE_SIZEOF_ELEMENT( I ) \
	(vips__sizeof_bandfmt[(I)->BandFmt])
#define VIPS_IMAGE_SIZEOF_PEL( I ) \
	(IM_IMAGE_SIZEOF_ELEMENT( I ) * (I)->Bands)
#define VIPS_IMAGE_SIZEOF_LINE( I ) \
	(IM_IMAGE_SIZEOF_PEL( I ) * (I)->Xsize)
#define VIPS_IMAGE_N_ELEMENTS( I ) \
	((I)->Bands * (I)->Xsize)

/* If VIPS_DEBUG is defined, add bounds checking.
 */
#ifdef VIPS_DEBUG
#define VIPS_IMAGE_ADDR( I, X, Y ) \
	( ((X) >= 0 && (X) < (I)->Xsize && \
	   (Y) >= 0 && (Y) < (I)->Ysize) ? \
	     ((I)->data + \
	       (Y) * VIPS_IMAGE_SIZEOF_LINE( I ) + \
	       (X) * VIPS_IMAGE_SIZEOF_PEL( I )) : \
	     (fprintf( stderr, \
		"IM_IMAGE_ADDR: point out of bounds, " \
		"file \"%s\", line %d\n" \
		"(point x=%d, y=%d\n" \
		" should have been within Rect left=%d, top=%d, " \
		"width=%d, height=%d)\n", \
		__FILE__, __LINE__, \
		(X), (Y), \
		0, 0, \
		(I)->Xsize, \
		(I)->Ysize ), abort(), (char *) NULL) \
	)
#else /*!VIPS_DEBUG*/
#define VIPS_IMAGE_ADDR( I, X, Y ) \
	((I)->data + \
	 (Y) * VIPS_IMAGE_SIZEOF_LINE( I ) + \
	 (X) * VIPS_IMAGE_SIZEOF_PEL( I ))
#endif /*VIPS_DEBUG*/

int vips_image_get_width( VipsImage *image );
int vips_image_get_height( VipsImage *image );
int vips_image_get_bands( VipsImage *image );
VipsBandFormat vips_image_get_format( VipsImage *image );
VipsCoding vips_image_get_coding( VipsImage *image );
VipsInterpretation vips_image_get_interpretation( VipsImage *image );
double vips_image_get_xres( VipsImage *image );
double vips_image_get_yres( VipsImage *image );
int vips_image_get_xoffset( VipsImage *image );
int vips_image_get_yoffset( VipsImage *image );




const char *vips_get_argv0( void );
int vips_init_world( const char *argv0 );
GOptionGroup *vips_get_option_group( void );

const char *vips_version_string( void );
int vips_version( int flag );

const char *vips_guess_prefix( const char *argv0, const char *env_name );
const char *vips_guess_libdir( const char *argv0, const char *env_name );

VipsImage *vips_open( const char *filename, const char *mode );

#define vips_open_local( IM, NAME, MODE ) \
	((IMAGE *) vips_local( (IM), \
		(vips_construct_fn) im_open, (im_callback_fn) im_close, \
		(void *) (NAME), (void *) (MODE), NULL ))

/* Strange double cast stops bogus warnings from gcc 4.1
 */
#define vips_open_local_array( IM, OUT, N, NAME, MODE ) \
	(vips_local_array( (IM), (void **)((void*)(OUT)), (N),\
		(im_construct_fn) im_open, (im_callback_fn) im_close, \
		(void *) (NAME), (void *) (MODE), NULL ))

int vips_close( VipsImage *im );

void vips_invalidate( VipsImage *im );

void vips_initdesc( VipsImage *image, 
	int xsize, int ysize, int bands, int bandbits, 
	VipsBandFormat format, VipsCoding coding, 
	VipsInterpretation interpretation, 
	float xres, float yres,
	int xo, int yo );

int vips_cp_desc( VipsImage *out, VipsImage *in );
int vips_cp_descv( VipsImage *out, VipsImage *in1, ... )
	__attribute__((sentinel));
int vips_cp_desc_array( VipsImage *out, VipsImage *in[] );

VipsImage *vips_binfile( const char *name, 
	int xsize, int ysize, int bands, int offset );
VipsImage *vips_image( void *buffer, 
	int xsize, int ysize, int bands, VipsBandFormat bandfmt );

typedef void *(*im_construct_fn)( void *, void *, void * );

/* Also used for im_add_close_callback() etc.
 */
typedef int (*im_callback_fn)( void *a, void *b );

void *im_local( VipsImage *im, 
	im_construct_fn cons, im_callback_fn dest, void *a, void *b, void *c );
int im_local_array( VipsImage *im, void **out, int n,
	im_construct_fn cons, im_callback_fn dest, void *a, void *b, void *c );
char *im_strdup( VipsImage *im, const char *str );
VipsImage *im__open_temp( const char *format );

int im_bits_of_fmt( VipsBandFormat fmt );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*IM_IMAGE_H*/
