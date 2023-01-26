/*
 * Copyright 2011 NextWindow
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NextWindow
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission. 
 * NextWindow makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright 2007 Peter Hutterer
 * Copyright 2009 Przemysław Firszt
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __XF86_NEXTWINDOW_H__
#define __XF86_NEXTWINDOW_H__

#include <linux/input.h>

#include "touchhelp.h"

#define SYSCALL(call) while (((call) == -1) && (errno == EINTR))

typedef struct _NextwindowDeviceRec
{
    char *device;
    int version;
    Atom* labels;
    int num_vals;
    int axes;
    int update_x;
    int update_y;
    int update_btn_left;
    int update_btn_right;
    int new_x_val;
    int new_y_val;
    int new_btn_left_val;
    int new_btn_right_val;
    TouchHelpRec touch_help;
    int use_touch_help;
    int th_drag_threshold;
    int th_rightclick_timeout;
    int th_doubleclick_timeout;
} NextwindowDeviceRec, *NextwindowDevicePtr;

void nwProcessEvent(InputInfoPtr pInfo, struct input_event *ev);

#endif

