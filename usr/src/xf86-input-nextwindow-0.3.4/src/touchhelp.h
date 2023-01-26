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

#ifndef _TOUCH_HELP_H_
#define _TOUCH_HELP_H_

#define TOUCH_NONE       0
#define TOUCH_DOWN       1
#define TOUCH_MOVE       2
#define TOUCH_UP         3
#define TOUCH_RIGHTCLICK 4

typedef struct 
{
    int state;
    int x, y;
} TouchPosRec, *TouchPosPtr;

#define TH_BUFFER_SIZE 300

typedef struct
{
    int sample_count;
    TouchPosRec buffer[TH_BUFFER_SIZE];
} TouchHelpBuffer;

#define TH_IDLE_STATE 0
#define TH_PRETOUCH_STATE 1
#define TH_PREDOUBLETOUCH_STATE 2
#define TH_PRERELEASE_STATE 3

#define TH_TOUCH_SIG 0
#define TH_RIGHTCLICK_TIMER_SIG 1
#define TH_DOUBLECLICK_TIMER_SIG 2

#define TH_DRAG_THRESHOLD 10
#define TH_RIGHT_CLICK_TIMEOUT 1200
#define TH_DOUBLE_CLICK_TIMEOUT 500

typedef struct _TouchHelpRec
{
    TouchHelpBuffer play_buffer;
    DeviceIntPtr dev;
    int current_state;
    TouchPosRec initial_touch_pos;
    OsTimerPtr rightclick_timer;
    OsTimerPtr doubleclick_timer;
    int drag_threshold;
    int rightclick_timeout;
    int doubleclick_timeout;
} TouchHelpRec, *TouchHelpPtr;


void TH_Init(TouchHelpPtr th, DeviceIntPtr device, int drag_threshold, int rightclick_timeout, int doubleclick_timeout);
void TH_Signal(TouchHelpPtr th, int signal, TouchPosRec tp); 

#endif
