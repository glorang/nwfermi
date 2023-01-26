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
 * Copyright 2009 Przemyslaw Firszt
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <linux/input.h>
#include <linux/types.h>

#include <xf86_OSproc.h>

#include <unistd.h>

#include <xf86.h>
#include <xf86Xinput.h>
#include <exevents.h>
#include <xorgVersion.h>
#include <xkbsrv.h>


#ifdef HAVE_PROPERTIES
#include <xserver-properties.h>
/* 1.6 has properties, but no labels */
#ifdef AXIS_LABEL_PROP
#define HAVE_LABELS
#else
#undef HAVE_LABELS
#endif

#endif


#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <xorg-server.h>
#include <xorgVersion.h>
#include <xf86Module.h>
#include <X11/Xatom.h>

#include "nextwindow.h"

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
/* removed from server, purge when dropping support for server 1.10 */
#define XI86_OPEN_ON_INIT 0x01
#define XI86_CONFIGURED 0x02
#endif

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
static InputInfoPtr nwPreInit(InputDriverPtr drv, IDevPtr dev, int flags);
#else
static int nwPreInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags);
#endif
static void nwUnInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags);
static pointer nwPlug(pointer module, pointer options, int *errmaj, int *errmin);
static void nwUnplug(pointer p);
static void nwReadInput(InputInfoPtr pInfo);
static int nwControl(DeviceIntPtr device, int what);
static int _nw_init_buttons(DeviceIntPtr device);
static int _nw_init_axes(DeviceIntPtr device);

_X_EXPORT InputDriverRec NEXTWINDOW = {
    1,
    "nextwindow",
    NULL,
    nwPreInit,
    nwUnInit,
    NULL,
    0,
};

static XF86ModuleVersionInfo nwVersionRec =
{
    "nextwindow",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR, PACKAGE_VERSION_MINOR,
    PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData nextwindowModuleData =
{
    &nwVersionRec,
    &nwPlug,
    &nwUnplug
};

static void
nwUnplug(pointer p)
{
};

static pointer
nwPlug(pointer module,
       pointer options,
       int *errmaj,
       int *errmin)
{
    xf86AddInputDriver(&NEXTWINDOW, module, 0);
    return module;
};

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
static int
NewNwPreInit(InputDriverPtr drv,
             InputInfoPtr pInfo,
             int flags);

static InputInfoPtr
nwPreInit(InputDriverPtr drv,
          IDevPtr dev,
          int flags)
{
    InputInfoPtr pInfo;

    if (!(pInfo = xf86AllocateInput(drv, 0)))
        return NULL;

    pInfo->name = xstrdup(dev->identifier);
    pInfo->flags = 0;
    pInfo->type_name = XI_TOUCHSCREEN; /* see XI.h */
    pInfo->conf_idev = dev;

    if (NewNwPreInit(drv, pInfo, flags) == Success)
    {
        return pInfo;
    }

    xf86DeleteInput(pInfo, 0);
    return NULL;
}

static int
NewNwPreInit(InputDriverPtr drv,
             InputInfoPtr pInfo,
             int flags)
#else
static int
nwPreInit(InputDriverPtr drv,
          InputInfoPtr pInfo,
          int flags)
#endif
{
    int ret = Success;
    NextwindowDevicePtr pNW;

    pNW = calloc(1, sizeof(NextwindowDeviceRec));
    if (!pNW)
    {
        ret = BadAlloc;
        pInfo->private = NULL;
        goto cleanup;;
    }

    pInfo->private = pNW;
    pInfo->read_input = nwReadInput; /* new data avl */
    pInfo->switch_mode = NULL; /* toggle absolute/relative mode */
    pInfo->device_control = nwControl; /* enable/disable dev */

    /* populate pInfo->options */
    xf86CollectInputOptions(pInfo, NULL
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
    , pInfo->options
#endif
    );

    /* process generic options */
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* process driver specific options */
    pNW->device = xf86SetStrOption(pInfo->options,
                                         "Device",
                                         "/dev/input/nwfermi_event");
    xf86Msg(X_INFO, "%s: Using device %s.\n", pInfo->name, pNW->device);
    pNW->use_touch_help = xf86SetBoolOption(pInfo->options,
					 "UseTouchHelp",
					 1);
    if (pNW->use_touch_help)
    	xf86Msg(X_INFO, "%s: Using touch help.\n", pInfo->name);
    pNW->th_drag_threshold = xf86SetIntOption(pInfo->options,
					 "DragThreshold",
					 TH_DRAG_THRESHOLD);
    pNW->th_rightclick_timeout = xf86SetIntOption(pInfo->options,
					 "RightClickTimeout",
					 TH_RIGHT_CLICK_TIMEOUT);
    pNW->th_doubleclick_timeout = xf86SetIntOption(pInfo->options,
					 "DoubleClickTimeout",
					 TH_DOUBLE_CLICK_TIMEOUT);

    /* Open sockets, init device files, etc. */
    SYSCALL(pInfo->fd = open(pNW->device, O_RDWR | O_NONBLOCK));
    if (pInfo->fd == -1)
    {
        ret = BadImplementation;
        xf86Msg(X_ERROR, "%s: failed to open %s.",
                pInfo->name, pNW->device);
        pInfo->private = NULL;
        goto cleanup;
    }
    /* do more funky stuff */
    close(pInfo->fd);
    pInfo->fd = -1;
    pInfo->flags |= XI86_OPEN_ON_INIT;
    pInfo->flags |= XI86_CONFIGURED;

cleanup:

    if (ret != Success)
    {
         if (pNW != NULL)
             free(pNW);
    }

    return ret;
}

static void nwUnInit(InputDriverPtr drv,
                         InputInfoPtr pInfo,
                         int flags)
{
    NextwindowDevicePtr pNW = pInfo->private;
    if (pNW->device)
    {
        free(pNW->device);
        pNW->device = NULL;
        /* Common error - pInfo->private must be NULL or valid memoy before
         * passing into xf86DeleteInput */
        pInfo->private = NULL;
    }
    xf86DeleteInput(pInfo, 0);
}


static int
_nw_init_buttons(DeviceIntPtr device)
{
    InputInfoPtr        pInfo = device->public.devicePrivate;
    NextwindowDevicePtr pNW = pInfo->private;
    CARD8               *map;
    int                 i;
    int                 ret = Success;
    const int           num_buttons = 2;

    map = calloc(num_buttons, sizeof(CARD8));

    for (i = 0; i < num_buttons; i++)
        map[i] = i;

    pNW->labels = malloc(sizeof(Atom));

    if (!InitButtonClassDeviceStruct(device, num_buttons, pNW->labels, map))
    {
        xf86Msg(X_ERROR, "%s: Failed to register buttons.\n", pInfo->name);
        ret = BadAlloc;
    }

    free(map);
    return ret;
}

static void nwInitAxesLabels(NextwindowDevicePtr pNW, int natoms, Atom *atoms)
{
#ifdef HAVE_LABELS
    Atom atom;
    int axis;
    char **labels;
    int labels_len = 0;
    char *misc_label;

    labels     = rel_labels;
    labels_len = ArrayLength(rel_labels);
    misc_label = AXIS_LABEL_PROP_REL_MISC;

    memset(atoms, 0, natoms * sizeof(Atom));

    /* Now fill the ones we know */
    for (axis = 0; axis < labels_len; axis++)
    {
        if (pNW->axis_map[axis] == -1)
            continue;

        atom = XIGetKnownProperty(labels[axis]);
        if (!atom) /* Should not happen */
            continue;

        atoms[pNW->axis_map[axis]] = atom;
    }
#endif
}


static int
_nw_init_axes(DeviceIntPtr device)
{
    InputInfoPtr        pInfo = device->public.devicePrivate;
    NextwindowDevicePtr pNW = pInfo->private;
    int                 i;
    const int           num_axes = 2;
    Atom                * atoms;

    pNW->num_vals = num_axes;
    atoms = malloc(pNW->num_vals * sizeof(Atom));

    nwInitAxesLabels(pNW, pNW->num_vals, atoms);
    if (!InitValuatorClassDeviceStruct(device,
                num_axes,
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                atoms,
#endif	
                GetMotionHistorySize(),
                0))
        return BadAlloc;

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 12
    pInfo->dev->valuator->mode = Relative;
#endif
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 13
    if (!InitAbsoluteClassDeviceStruct(device))
        return BadAlloc;
#endif

    for (i = 0; i < pNW->axes; i++)
    {
        xf86InitValuatorAxisStruct(device, i, *pNW->labels, -1, -1, 1, 1, 1
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 12
            , Absolute
#endif
            );
        xf86InitValuatorDefaults(device, i);
    }
    free(atoms);
    return Success;
}

static void _nwPtrControl(DeviceIntPtr dev, PtrCtrl* ctrl)
{
}

static int nwControl(DeviceIntPtr device,
                         int what)
{
    InputInfoPtr        pInfo = device->public.devicePrivate;
    NextwindowDevicePtr pNW = pInfo->private;

    switch(what)
    {
        case DEVICE_INIT:
            _nw_init_buttons(device);
            _nw_init_axes(device);
            if (!InitPtrFeedbackClassDeviceStruct(device, _nwPtrControl))
            {
                xf86Msg(X_ERROR, "%s: cannot init ptr feedback.\n", pInfo->name);
                return !Success;
            }
            break;

        /* Switch device on.  Establish socket, start event delivery.  */
        case DEVICE_ON:
            xf86Msg(X_INFO, "%s: On.\n", pInfo->name);
            if (device->public.on)
                    break;

            SYSCALL(pInfo->fd = open(pNW->device, O_RDONLY | O_NONBLOCK));
            if (pInfo->fd < 0)
            {
                xf86Msg(X_ERROR, "%s: cannot open device.\n", pInfo->name);
                return BadRequest;
            }

            xf86FlushInput(pInfo->fd);
            xf86AddEnabledDevice(pInfo);
            device->public.on = TRUE;

            // init touch helper
            if (pNW->use_touch_help)
                TH_Init(&pNW->touch_help, pInfo->dev, pNW->th_drag_threshold, pNW->th_rightclick_timeout, pNW->th_doubleclick_timeout);

            break;
       case DEVICE_OFF:
            xf86Msg(X_INFO, "%s: Off.\n", pInfo->name);
            if (!device->public.on)
                break;
            xf86RemoveEnabledDevice(pInfo);
            close(pInfo->fd);
            pInfo->fd = -1;
            device->public.on = FALSE;
            break;
      case DEVICE_CLOSE:
            /* free what we have to free */
            break;
    }
    return Success;
}

void nwProcessEvent(InputInfoPtr pInfo, struct input_event *ev)
{
    NextwindowDevicePtr pNW = pInfo->private;

    switch (ev->type)
    {
        case EV_REL:
            break;
        case EV_ABS:
            if (ev->code == ABS_X)
            {
                pNW->update_x = TRUE;
                pNW->new_x_val = ev->value;
            }
            else if (ev->code == ABS_Y)
            {
                pNW->update_y = TRUE;
                pNW->new_y_val = ev->value;
            }
            break;
        case EV_KEY:
            if (ev->code == BTN_LEFT)
            {
                pNW->update_btn_left = TRUE;
                pNW->new_btn_left_val = ev->value;
            }
            else if (ev->code == BTN_RIGHT)
            {
                pNW->update_btn_right = TRUE;
                pNW->new_btn_right_val = ev->value;
            }
            break;
        case EV_SYN:
        {
            int x = pNW->new_x_val / 32767.0f * screenInfo.screens[0]->width;
            int y = pNW->new_y_val / 32767.0f * screenInfo.screens[0]->height;

            if (pNW->use_touch_help)
            {
                TouchPosRec tp;
                tp.state = TOUCH_NONE;
                if (pNW->update_x || pNW->update_y)
                    tp.state = TOUCH_MOVE;
                if (pNW->update_btn_left && pNW->new_btn_left_val)
                    tp.state = TOUCH_DOWN;
                if (pNW->update_btn_left && !pNW->new_btn_left_val)
                    tp.state = TOUCH_UP;
                if (pNW->update_btn_right && pNW->new_btn_right_val)
                    tp.state = TOUCH_RIGHTCLICK;
                tp.x = x;
                tp.y = y;
                TH_Signal(&pNW->touch_help, TH_TOUCH_SIG, tp);             
            }
            else
            {
                if (pNW->update_x || pNW->update_y)
                {
                    xf86PostMotionEvent(pInfo->dev, TRUE, 0, 2, x, y);
                }
                if (pNW->update_btn_left)
                    xf86PostButtonEvent(pInfo->dev, TRUE, 1, pNW->new_btn_left_val, 0, 0);
                if (pNW->update_btn_right)
                    xf86PostButtonEvent(pInfo->dev, TRUE, 3, pNW->new_btn_right_val, 0, 0);
            }

            pNW->update_x = FALSE;
            pNW->update_y = FALSE;
            pNW->update_btn_left = FALSE;
            pNW->update_btn_right = FALSE;
            break;
        }
    }
}

// magic number to reduce reads
#define NUM_EVENTS 16

static void nwReadInput(InputInfoPtr pInfo)
{
    struct input_event ev[NUM_EVENTS];
    int i, len = sizeof(ev);

    while (len == sizeof(ev))
    {
        len = read(pInfo->fd, &ev, sizeof(ev));

        if (len < 0)
        {
            // X_NONE does not allocate memory
            xf86MsgVerb(X_NONE, 0, "%s: Read error: %s\n", pInfo->name, strerror(errno)); 
            break;
        }

        if (len % sizeof(ev[0]))
        {
            break;
        }

        for (i = 0; i < len / sizeof(ev[0]); i++)
        {
            nwProcessEvent(pInfo, &ev[i]);
        }
    }
}

