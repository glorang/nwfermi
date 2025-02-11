# NextWindow Touchscreen on Ubuntu 22.04

This is a guide for installing / configuring a NextWindow Touchscreen on Ubuntu 22.04.

The files in this repo will install nwfermi 0.7.0.1.

It is based on the files provided by HP as SUSE drivers for the "HP Compaq Elite 8300 All-in-One Desktop PC". [HP Support Link](https://support.hp.com/us-en/drivers/selfservice/hp-compaq-elite-8300-all-in-one-desktop-pc/5272027) | [Direct Link](https://ftp.hp.com/pub/softpaq/sp63501-64000/sp63501.tgz)

The HP driver has not been updated anymore since 2013, I've made following enhancements:

- Fix driver compilation (fix work_struct/delayed_work structs usage, fix some minor compiler warnings)
- Fix startup of user space deamon via udev/systemd implementation as you cannot launch 32 bit applications on 64 bits OSes via udev (blocked via seccomp in systemd-udevd)
- Xorg config example

For models 1926:1846 and 1926:1878 you need to install the additional "fwprod" daemon. Its usage is unclear but seems to work according to #4. See additional steps as required outlined below.

# Licensing

- The kernel driver has been released under a GPL license by NextWindow
- The user space daemons and xf86-input-nextwindow Xorg module are copyrighted by NextWindow and released under [following license](LICENSE-NW)

# Install required packages

```
apt-get install dkms build-essential autoconf xutils-dev libtool xserver-xorg-dev libc6-i386 pkg-config evtest
```

# Install additional packages (models 1926:1846 and 1926:1878)

Install libudev1 and symlink it to libudev0 (no longer available as i386)

```
apt-get install libudev1:i386
ln -s /usr/lib/i386-linux-gnu/libudev.so.1 /usr/lib/i386-linux-gnu/libudev.so.0
```

# Get source files from this Git repo

```
# wget https://github.com/glorang/nwfermi/archive/refs/heads/master.zip
# unzip master.zip
# cd nwfermi-master
```

All following chapters assume you are executing the steps as outlined from within the `nwfermi-master` folder.

# Build and install nwfermi driver

- Remove nwfermi/0.6.5.0 if installed
```
# dkms unbuild nwfermi/0.6.5.0 --all
```
- Install nwfermi source to /usr/src
```
# cp -p -r usr/src/nwfermi-0.7.0.1 /usr/src
```
- Build nwfermi/0.7.0.1
```
# dkms build nwfermi/0.7.0.1
```
- Install module
```
# dkms install nwfermi/0.7.0.1
```

# Install required nwfermi files
- Copy Xorg config
```
# cp etc/X11/xorg.conf.d/10-nwfermi.conf /etc/X11/xorg.conf.d/
```
- Copy nwfermi_daemon to /usr/sbin
```
# cp usr/sbin/nwfermi_daemon /usr/sbin
```
- Install udev rules from this git repo
```
# cp etc/udev/rules.d/40-nw-fermi.rules /etc/udev/rules.d/
```
- Install systemd service files from this git repo
```
# cp etc/systemd/system/nwfermi@.service /etc/systemd/system/
```
- Reload systemd
```
# systemctl daemon-reload
```

# Install optional nwfermi files (models 1926:1846 and 1926:1878)

- Copy fwprod to /usr/sbin
```
# cp usr/sbin/fwprod /usr/sbin
```
- Install systemd service files from this git repo
```
# cp etc/systemd/system/fwprod-1926-1846.service /etc/systemd/system/
# cp etc/systemd/system/fwprod-1926-1878.service /etc/systemd/system/
```
- Reload systemd
```
# systemctl daemon-reload
```

# Build xf86-input-nextwindow Xorg module

```
# cd usr/src/xf86-input-nextwindow-0.3.4
# chmod +x autogen.sh ; ./autogen.sh
# make
# make install
```
- Install module
```
# cp /usr/local/lib/xorg/modules/input/nextwindow_drv.* /usr/lib/xorg/modules/input/
```

# Disable Wayland / Enable Xorg

Edit `/etc/gdm3/custom.conf` and set `WaylandEnable=false` in the `[daemon]` section.

# Add your local user to the input group

To be able to read the input device your local user *must* be part of the *input* group.
You should add the *gdm* user to this group as well.
On Linux Mint 22 Xfce Edition, replace gdm for 'lightdm'. Everything else stays the same.

```
# usermod -a -G input gdm 
# usermod -a -G input your_username
```

# Reboot

Reboot and everything should work! Enjoy.

# Debugging

Use following steps for some general debugging hints:

- Run `lsusb` and check if your model (1926:XXXX) is supported by the driver [nw-fermi.c, line #49 and onwards](usr/src/nwfermi-0.7.0.1/nw-fermi.c#L49)
- Check if kernel module is loaded: `lsmod |grep -i fermi`
- Enter `strings /dev/nwfermi1` (or nwfermi2) it should produce garbage text output when you touch the screen
- Start nwfermi daemon manually in foreground `/usr/sbin/nwfermi_daemon /instanceId 1`, expected output:

```
# /usr/sbin/nwfermi_daemon /instanceid 1
StartThreads
Starting bulk thread
Starting tl thread
```

After touching the screen:

```
state: 1, X: 14893, Y: 22044
state: 2, X: 14893, Y: 22044
state: 2, X: 14893, Y: 22044
```

- For model 1926:1846 : Start fwproc manually `/usr/sbin/fwprod /product_id 0x1846 /message 004302F300 /padding_size 65 /timeout 300 /daemon`
- For model 1926:1878 : Start fwproc manually `/usr/sbin/fwprod /product_id 0x1878 /message 004302F001 /padding_size 65 /timeout 300 /daemon`

- Run evtest and select "Nextwindow Fermi Touchscreen" and touch your screen, you should see it generate events. Expected output:

```
# evtest
No device specified, trying to scan all of /dev/input/event*
Available devices:

...
/dev/input/event8:      Nextwindow Fermi Touchscreen
...

Select the device event number [0-21]: 8
Input driver version is 1.0.1
Input device ID: bus 0x0 vendor 0x0 product 0x0 version 0x0
Input device name: "Nextwindow Fermi Touchscreen"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 261 (BTN_5)
    Event code 272 (BTN_LEFT)
    Event code 273 (BTN_RIGHT)
  Event type 3 (EV_ABS)
    Event code 0 (ABS_X)
      Value  15028
      Min        0
      Max    32767
    Event code 1 (ABS_Y)
      Value  18035
      Min        0
      Max    32767
    Event code 48 (ABS_MT_TOUCH_MAJOR)
      Value      0
      Min        0
      Max    32767
    Event code 49 (ABS_MT_TOUCH_MINOR)
      Value      0
      Min        0
      Max    32767
    Event code 53 (ABS_MT_POSITION_X)
      Value      0
      Min        0
      Max    32767
    Event code 54 (ABS_MT_POSITION_Y)
      Value      0
      Min        0
      Max    32767
    Event code 57 (ABS_MT_TRACKING_ID)
      Value      0
      Min        0
      Max      255
Properties:
Testing ... (interrupt to exit)
```

After touching the screen:

```
Event: time 1673258489.763882, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 0
Event: time 1673258489.763882, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 16527
Event: time 1673258489.763882, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 12131
Event: time 1673258489.763882, type 3 (EV_ABS), code 48 (ABS_MT_TOUCH_MAJOR), value 1000
Event: time 1673258489.763882, type 3 (EV_ABS), code 49 (ABS_MT_TOUCH_MINOR), value 1000
Event: time 1673258489.763882, ++++++++++++++ SYN_MT_REPORT ++++++++++++
Event: time 1673258489.763882, type 1 (EV_KEY), code 272 (BTN_LEFT), value 1
Event: time 1673258489.763882, type 3 (EV_ABS), code 0 (ABS_X), value 16527
Event: time 1673258489.763882, type 3 (EV_ABS), code 1 (ABS_Y), value 12131
Event: time 1673258489.763882, -------------- SYN_REPORT ------------
Event: time 1673258489.784821, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 0
Event: time 1673258489.784821, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 16527
Event: time 1673258489.784821, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 12131
Event: time 1673258489.784821, type 3 (EV_ABS), code 48 (ABS_MT_TOUCH_MAJOR), value 1000
Event: time 1673258489.784821, type 3 (EV_ABS), code 49 (ABS_MT_TOUCH_MINOR), value 1000
```

- Check Xorg log in `/home/<username>/.local/share/xorg/Xorg.0.log` (or Xorg.1.log), expected output:

```
$ grep -i fermi /home/glorang/.local/share/xorg/Xorg.1.log
[    52.662] (II) config/udev: Adding input device Nextwindow Fermi Touchscreen (/dev/input/event9)
[    52.662] (**) Nextwindow Fermi Touchscreen: Applying InputClass "libinput pointer catchall"
[    52.662] (**) Nextwindow Fermi Touchscreen: Applying InputClass "Nextwindow Fermi Touchscreen"
[    52.662] (II) Using input driver 'nextwindow' for 'Nextwindow Fermi Touchscreen'
[    52.662] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.662] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.662] (II) Nextwindow Fermi Touchscreen: Using device /dev/input/event9.
[    52.662] (II) Nextwindow Fermi Touchscreen: Using touch help.
[    52.718] (II) XINPUT: Adding extended input device "Nextwindow Fermi Touchscreen" (type: UNKNOWN, id 16)
[    52.718] (**) Nextwindow Fermi Touchscreen: (accel) keeping acceleration scheme 1
[    52.718] (**) Nextwindow Fermi Touchscreen: (accel) acceleration profile 0
[    52.718] (**) Nextwindow Fermi Touchscreen: (accel) acceleration factor: 2.000
[    52.718] (**) Nextwindow Fermi Touchscreen: (accel) acceleration threshold: 4
[    52.718] (II) Nextwindow Fermi Touchscreen: On.
[    52.718] (II) config/udev: Adding input device Nextwindow Fermi Touchscreen (/dev/input/js0)
[    52.718] (**) Nextwindow Fermi Touchscreen: Applying InputClass "Nextwindow Fermi Touchscreen"
[    52.718] (II) Using input driver 'nextwindow' for 'Nextwindow Fermi Touchscreen'
[    52.718] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.718] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.718] (II) Nextwindow Fermi Touchscreen: Using device /dev/input/js0.
[    52.718] (II) Nextwindow Fermi Touchscreen: Using touch help.
[    52.770] (II) XINPUT: Adding extended input device "Nextwindow Fermi Touchscreen" (type: UNKNOWN, id 17)
[    52.770] (**) Nextwindow Fermi Touchscreen: (accel) keeping acceleration scheme 1
[    52.770] (**) Nextwindow Fermi Touchscreen: (accel) acceleration profile 0
[    52.770] (**) Nextwindow Fermi Touchscreen: (accel) acceleration factor: 2.000
[    52.770] (**) Nextwindow Fermi Touchscreen: (accel) acceleration threshold: 4
[    52.770] (II) Nextwindow Fermi Touchscreen: On.
[    52.770] (II) config/udev: Adding input device Nextwindow Fermi Touchscreen (/dev/input/mouse3)
[    52.770] (**) Nextwindow Fermi Touchscreen: Applying InputClass "Nextwindow Fermi Touchscreen"
[    52.770] (II) Using input driver 'nextwindow' for 'Nextwindow Fermi Touchscreen'
[    52.770] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.770] (**) Nextwindow Fermi Touchscreen: always reports core events
[    52.770] (II) Nextwindow Fermi Touchscreen: Using device /dev/input/mouse3.
[    52.770] (II) Nextwindow Fermi Touchscreen: Using touch help.
[    52.826] (II) XINPUT: Adding extended input device "Nextwindow Fermi Touchscreen" (type: UNKNOWN, id 18)
[    52.826] (**) Nextwindow Fermi Touchscreen: (accel) keeping acceleration scheme 1
[    52.826] (**) Nextwindow Fermi Touchscreen: (accel) acceleration profile 0
[    52.826] (**) Nextwindow Fermi Touchscreen: (accel) acceleration factor: 2.000
[    52.826] (**) Nextwindow Fermi Touchscreen: (accel) acceleration threshold: 4
[    52.826] (II) Nextwindow Fermi Touchscreen: On.
```
