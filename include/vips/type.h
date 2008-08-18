/* VIPS argument types
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

#ifndef IM_TYPE_H
#define IM_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* Type names. Old code might use "doublevec" etc. from before we had the
 * "array" type.
 */
#define IM_TYPE_NAME_DOUBLE "double"	/* im_object is ptr to double */
#define IM_TYPE_NAME_INT "integer"	/* 32-bit integer */
#define IM_TYPE_NAME_COMPLEX "complex"	/* Pair of doubles */
#define IM_TYPE_NAME_STRING "string"	/* Zero-terminated char array */
#define IM_TYPE_NAME_IMASK "intmask"	/* Integer mask type */
#define IM_TYPE_NAME_DMASK "doublemask"	/* Double mask type */
#define IM_TYPE_NAME_IMAGE "image"	/* IMAGE descriptor */
#define IM_TYPE_NAME_DISPLAY "display"	/* Display descriptor */
#define IM_TYPE_NAME_GVALUE "gvalue"	/* GValue wrapper */
#define IM_TYPE_NAME_ARRAY "array"	/* Array of other values of some type */

/* The arg to the init function is a pointer to the object.
 */
typedef void (*im_type_init_fn)( im_object *obj );
typedef void (*im_type_free_fn)( im_object obj );

/* A VIPS type. 
 */
typedef struct im__type_t {
	const char *name; 		/* Name of type, eg. "double" */
	size_t size;			/* sizeof( im_object repres. ) */
	im_type_init_fn init; 		/* Init memory */
	im_type_free_fn free;		/* Destroy object */
} im_type_t;

/* A 'subclass' of im_type_t for array objects, eg. array-of-double.
 */
typedef struct im__type_array_t {
	im_type_t parent;		/* "array" */
	im_type_t *type;		/* What this is an array of */
} im_type_array_t;

/* Various im_object values.
 */
typedef struct {
	char *name;			/* Command-line name in */
	void *mask;			/* Mask --- DOUBLE or INT */
} im_object_mask_t;

typedef struct {
	int n;				/* Array length */
	im_object *array;		/* Array */
} im_object_array_t;

/* An argument to a VIPS operation.
 */
typedef struct im__argument_t {
	const char *name; 		/* Eg. "in2" */
	im_type_t *type;		/* Argument type */
	gboolean input;			/* TRUE means arg to operation */
} im_argument_t;

/* Flags for operations. Various hints for UIs about the behaviour of the
 * operation,
 */
typedef enum {
	IM_OPERATION_NONE = 0,		/* No flags set */
	IM_OPERATION_PIO = 0x1,		/* Is a partial function */
	IM_OPERATION_TRANSFORM = 0x2,	/* Performs coord transformations */
	IM_OPERATION_PTOP = 0x4,	/* Point-to-point ... can be LUTted */
	IM_OPERATION_NOCACHE = 0x8	/* Result should not be cached */
} im_operation_flags;

/* Type of a VIPS dispatch funtion.
 */
typedef int (*im_operation_dispatch_fn)( im_object *argv );

/* A VIPS operation.
 */
typedef struct im__operation_t {
	const char *name;		/* eg "im_invert" */
	const char *desc;		/* One line description */
	im_operation_flags flags;	/* Flags for this function */
	im_operation_dispatch_fn disp;	/* Dispatch */
	int argc;			/* Number of args */
	im_argument_t **argv; 		/* Arg list */
} im_operation_t;

/* Register/iterate over types.
 */
im_type_t *im_type_register( const char *name, size_t size,
	im_type_init_fn init, im_type_free_fn free );
void *im_type_map( VSListMap2Fn fn, void *a, void *b );
im_type_t *im_type_lookup( const char *name );

/* Create arguments.
 */
im_argument_t *im_argument_new( const char *name, 
	im_type_t *type, gboolean input );

/* Register/iterate/lookup operations.
 */
im_operation_t *im_operation_register( const char *name, const char *desc, 
	im_operation_flags flags, im_operation_dispatch_fn disp, int argc );
void *im_operation_map( VSListMap2Fn fn, void *a, void *b );
im_operation_t *im_operation_lookup( const char *name );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*IM_TYPE_H*/
