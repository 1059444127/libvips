/* abstract base class for all vips objects
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

#ifndef VIPS_OBJECT_H
#define VIPS_OBJECT_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define VIPS_TYPE_OBJECT (vips_object_get_type())
#define VIPS_OBJECT( obj ) \
	(G_TYPE_CHECK_INSTANCE_CAST( (obj), VIPS_TYPE_OBJECT, VipsObject ))
#define VIPS_OBJECT_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_CAST( (klass), VIPS_TYPE_OBJECT, VipsObjectClass))
#define VIPS_IS_OBJECT( obj ) \
	(G_TYPE_CHECK_INSTANCE_TYPE( (obj), VIPS_TYPE_OBJECT ))
#define VIPS_IS_OBJECT_CLASS( klass ) \
	(G_TYPE_CHECK_CLASS_TYPE( (klass), VIPS_TYPE_OBJECT ))
#define VIPS_OBJECT_GET_CLASS( obj ) \
	(G_TYPE_INSTANCE_GET_CLASS( (obj), VIPS_TYPE_OBJECT, VipsObjectClass ))

/* Handy vips_object_destroy() shortcut.
 */
#define IDESTROY( O ) { \
	if( O ) { \
		(void) vips_object_destroy( VIPS_OBJECT( O ) ); \
		( O ) = NULL; \
	} \
}

typedef struct _VipsObject {
	GObject parent_object;

	/* True when created ... the 1 reference that gobject makes is
	 * 'floating' and not owned by anyone. Do _sink() after every _ref()
	 * to transfer ownership to the parent container. Upshot: no need to
	 * _unref() after _add() in _new().
	 */
	gboolean floating;

	/* Stop destroy loops with this.
	 */
	gboolean in_destruction;
} VipsObject;

typedef struct _VipsObjectClass {
	GObjectClass parent_class;

	/* End object's lifetime, just like gtk_object_destroy.
	 */
	void (*destroy)( VipsObject * );

	/* Something about the object has changed. Should use glib's properties
	 * but fix this later.
	 */
	void (*changed)( VipsObject * );
} VipsObjectClass;

void *vips_object_destroy( VipsObject *vips_object );
void *vips_object_changed( VipsObject *vips_object );
void vips_object_sink( VipsObject *vips_object );

GType vips_object_get_type( void );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_OBJECT_H*/


