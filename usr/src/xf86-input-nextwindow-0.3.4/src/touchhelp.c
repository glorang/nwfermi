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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <xf86.h>
#include <xf86Xinput.h>

#include "touchhelp.h"

// function prototypes

static CARD32 rightclick_func(OsTimerPtr timer, CARD32 now, pointer _priv);
static CARD32 doubleclick_func(OsTimerPtr timer, CARD32 now, pointer _priv);
void _play_sample(TouchHelpPtr th, TouchPosPtr tp);
void _play_buffer(TouchHelpPtr th);
void _push_buffer(TouchHelpPtr th, TouchPosPtr tp);
void _trash_buffer(TouchHelpPtr th);
int _is_touch_outside_drag_threshold(TouchHelpPtr th, TouchPosPtr tp);
void _kill_rightclick_timer(TouchHelpPtr th);
void _kill_doubleclick_timer(TouchHelpPtr th);
void _to_pretouch(TouchHelpPtr th, TouchPosPtr tp);
void _transition(TouchHelpPtr th, int new_state);
void _idle(TouchHelpPtr th, int signal, TouchPosPtr tp);
void _pretouch(TouchHelpPtr th, int signal, TouchPosPtr tp);
void _predoubletouch(TouchHelpPtr th, int signal, TouchPosPtr tp);
void _prerelease(TouchHelpPtr th, int signal, TouchPosPtr tp);

// timer callbacks

static CARD32
rightclick_func(OsTimerPtr timer, CARD32 now, pointer _priv)
{
    TouchHelpPtr th = (TouchHelpPtr)_priv;
    TouchPosRec tp;
    TH_Signal(th, TH_RIGHTCLICK_TIMER_SIG, tp);
    return 0;
}

static CARD32
doubleclick_func(OsTimerPtr timer, CARD32 now, pointer _priv)
{
    TouchHelpPtr th = (TouchHelpPtr)_priv;
    TouchPosRec tp;
    TH_Signal(th, TH_DOUBLECLICK_TIMER_SIG, tp);
    return 0;
}

// private methods

void _play_sample(TouchHelpPtr th, TouchPosPtr tp)
{
    switch (tp->state)
    {
        case TOUCH_DOWN:
            xf86PostMotionEvent(th->dev, TRUE, 0, 2, tp->x, tp->y);
            xf86PostButtonEvent(th->dev, TRUE, 1, 1, 0, 0);
            break;
        case TOUCH_MOVE:
            xf86PostMotionEvent(th->dev, TRUE, 0, 2, tp->x, tp->y);
            break;
        case TOUCH_UP:
            xf86PostMotionEvent(th->dev, TRUE, 0, 2, tp->x, tp->y);
            xf86PostButtonEvent(th->dev, TRUE, 1, 0, 0, 0);
            break;
        case TOUCH_RIGHTCLICK:
            xf86PostMotionEvent(th->dev, TRUE, 0, 2, tp->x, tp->y);
            xf86PostButtonEvent(th->dev, TRUE, 3, 1, 0, 0);
            xf86PostButtonEvent(th->dev, TRUE, 3, 0, 0, 0);
            break;
    }
}

#include <mi.h>

void _play_buffer(TouchHelpPtr th)
{
    int i;
    for (i = 0; i < th->play_buffer.sample_count; i++)
        _play_sample(th, &th->play_buffer.buffer[i]);
    _trash_buffer(th);
}

void _push_buffer(TouchHelpPtr th, TouchPosPtr tp)
{
    if (th->play_buffer.sample_count >= TH_BUFFER_SIZE)
    {
        // show warning
        xf86MsgVerb(X_NONE, 0, "TH: buffer overflow\n"); 
    }
    else if (th->play_buffer.sample_count < 0)
    {
        // show warning
        xf86MsgVerb(X_NONE, 0, "TH: buffer underflow\n"); 
    }
    else
    {
        th->play_buffer.buffer[th->play_buffer.sample_count++] = *tp;
    }
}

void _trash_buffer(TouchHelpPtr th)
{
    th->play_buffer.sample_count = 0;
}

int abs(int a)
{
    if (a >= 0)
        return a;
    return -a;
}

int _is_touch_outside_drag_threshold(TouchHelpPtr th, TouchPosPtr tp)
{
    if (abs(tp->x - th->initial_touch_pos.x) > th->drag_threshold ||
        abs(tp->y - th->initial_touch_pos.y) > th->drag_threshold) 
        return TRUE;
    return FALSE;
}

void _kill_rightclick_timer(TouchHelpPtr th)
{
    if (th->rightclick_timer) 
        TimerFree(th->rightclick_timer);
    th->rightclick_timer = NULL;
}

void _kill_doubleclick_timer(TouchHelpPtr th)
{
    if (th->doubleclick_timer) 
        TimerFree(th->doubleclick_timer);
    th->doubleclick_timer = NULL;
}

void _to_pretouch(TouchHelpPtr th, TouchPosPtr tp)
{
    // set right click timer
    th->rightclick_timer = TimerSet(th->rightclick_timer, 0,
                       th->rightclick_timeout,
                       rightclick_func, th);
    // save initial touch                   
    th->initial_touch_pos = *tp;
    // hover at current location
    tp->state = TOUCH_MOVE;
    _play_sample(th, tp);
    // transition to pretouch state
    _transition(th, TH_PRETOUCH_STATE);
}

void _transition(TouchHelpPtr th, int new_state)
{
    th->current_state = new_state;
}

// state functions

void _idle(TouchHelpPtr th, int signal, TouchPosPtr tp)
{
    switch (signal)
    {
        case TH_TOUCH_SIG:
            if (tp->state == TOUCH_DOWN)
                _to_pretouch(th, tp);
            else
                _play_sample(th, tp);
            break;
    }
}

void _pretouch(TouchHelpPtr th, int signal, TouchPosPtr tp)
{
    switch (signal)
    {
        case TH_TOUCH_SIG:
            switch (tp->state)
            {
                case TOUCH_MOVE:
                    if (_is_touch_outside_drag_threshold(th, tp))
                    {
                        //xf86MsgVerb(X_NONE, 0, "TH: *play buffer (%d), to idle\n", th->play_buffer.sample_count); 
                        _kill_rightclick_timer(th);
                        _push_buffer(th, tp);
                        _play_sample(th, &th->initial_touch_pos);
                        _play_buffer(th);
                        _transition(th, TH_IDLE_STATE);
                    }
                    else
                    {
                        //xf86MsgVerb(X_NONE, 0, "TH: push buffer\n"); 
                        _push_buffer(th, tp);
                    }
                    break;
                case TOUCH_UP:
                    _kill_rightclick_timer(th);
                    _trash_buffer(th);
                    _play_sample(th, &th->initial_touch_pos);
                    th->initial_touch_pos.state = TOUCH_UP;
                    _play_sample(th, &th->initial_touch_pos);
                    // set double click timer
                    th->doubleclick_timer = TimerSet(th->doubleclick_timer, 0,
                                       th->doubleclick_timeout,
                                       doubleclick_func, th);
                                   
                    _transition(th, TH_PREDOUBLETOUCH_STATE);
            }
            break;

        case TH_RIGHTCLICK_TIMER_SIG:
            //xf86MsgVerb(X_NONE, 0, "TH: right click, to prerelease\n"); 
            _kill_rightclick_timer(th);
            _trash_buffer(th);
            th->initial_touch_pos.state = TOUCH_RIGHTCLICK;
            _play_sample(th, &th->initial_touch_pos);
            _transition(th, TH_PRERELEASE_STATE);
            break;
    }
}

void _predoubletouch(TouchHelpPtr th, int signal, TouchPosPtr tp)
{
    switch (signal)
    {
        case TH_TOUCH_SIG:
            if (tp->state == TOUCH_DOWN)
            {
                _kill_doubleclick_timer(th);
                if (_is_touch_outside_drag_threshold(th, tp))
                {
                    //xf86MsgVerb(X_NONE, 0, "TH: predoubletouch, to pretouch \n"); 
                    _to_pretouch(th, tp);
                }
                else
                {
                    //xf86MsgVerb(X_NONE, 0, "TH: doubleclick, to idle\n"); 
                    th->initial_touch_pos.state = TOUCH_DOWN;
                    _play_sample(th, &th->initial_touch_pos);
                    _transition(th, TH_IDLE_STATE);
                }
            }
            break;
        case TH_DOUBLECLICK_TIMER_SIG:
             _kill_doubleclick_timer(th);
            _transition(th, TH_IDLE_STATE);
            break;
    }
}

void _prerelease(TouchHelpPtr th, int signal, TouchPosPtr tp)
{
    switch (signal)
    {
        case TH_TOUCH_SIG:
            if (tp->state == TOUCH_UP)
                _transition(th, TH_IDLE_STATE);
            else if (tp->state == TOUCH_MOVE)
                _play_sample(th, tp);
            break;
    }
}

// public methods

void TH_Init(TouchHelpPtr th, DeviceIntPtr device, int drag_threshold, int rightclick_timeout, int doubleclick_timeout)
{
    th->play_buffer.sample_count = 0;
    th->current_state = TH_IDLE_STATE;
    th->dev = device;
    th->drag_threshold = drag_threshold;
    th->rightclick_timeout = rightclick_timeout;
    th->doubleclick_timeout = doubleclick_timeout;
}

void TH_Signal(TouchHelpPtr th, int signal, TouchPosRec tp)
{
    switch (th->current_state)
    {
        case TH_IDLE_STATE:
            _idle(th, signal, &tp);
            break;
        case TH_PRETOUCH_STATE:
            _pretouch(th, signal, &tp);
            break;
        case TH_PREDOUBLETOUCH_STATE:
            _predoubletouch(th, signal, &tp);
            break;
        case TH_PRERELEASE_STATE:
            _prerelease(th, signal, &tp);
            break;
    }
}
