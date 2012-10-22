/* Private include file ... if we've been configured without gthread, we need
 * to point the g_thread_*() and g_mutex_*() functions at our own stubs.
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

#ifndef VIPS_THREAD_H
#define VIPS_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/* Stack size for each thread. We need to set this explicitly because some
 * systems have a very low default.

 	FIXME ...  should have an environment variable for this?

 */
#define VIPS__DEFAULT_STACK_SIZE (2 * 1024 * 1024)

/* We need wrappers over g_mutex_new(), it was replaced by g_mutex_init() in
 * glib 2.32+
 */
GMutex *vips_mutex_new( void );
void vips_mutex_free( GMutex * );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_THREAD_H*/
