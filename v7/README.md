# NextWindow Touchscreen on Ubuntu 22.04

The files in this subrepo will install nwfermi 0.7.0.1 instead of 0.6.0.5 that is in the main repo.

It is based on the files provided by HP as SUSE drivers for the "HP Compaq Elite 8300 All-in-One Desktop PC".

https://support.hp.com/us-en/drivers/selfservice/hp-compaq-elite-8300-all-in-one-desktop-pc/5272027
https://ftp.hp.com/pub/softpaq/sp63501-64000/sp63501.tgz

The HP driver has not been updated anymore since 2013, I've made following enhancements:

- Fix driver compilation, fix work_struct/delayed_work structs usage
- Fix startup of user space deamon via udev/systemd implementation as you cannot launch 32 bit applications on 64 bits OSes via udev (blocked via seccomp in systemd-udevd)
- Xorg config example

For models 1926:1846 and 1926:1878 there seeems to be an additional "fwprod" daemon. Its usage is unclear and is untested.

# Install required packages

```
apt-get install dkms build-essential autoconf xutils-dev libtool xserver-xorg-dev libc6-i386 pkg-config
```

# Install additional packages (models 1926:1846 and 1926:1878)

Install libudev1 and symlink it to libudev0 (no longer available as i386)

```
apt-get install libudev1:i386
ln -s /usr/lib/i386-linux-gnu/libudev.so.1 /usr/lib/i386-linux-gnu/libudev.so.0
```

# Build and install nwfermi driver

- Remove nwfermi/0.6.5.0 if installed
```
# dkms unbuild nwfermi/0.6.5.0 --all
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

# Rebuild xf86-input-nextwindow Xorg module

- Download xf86-input-nextwindow_0.3.4~precise1.tar.gz from https://launchpad.net/~djpnewton/+archive/ubuntu/xf86-input-nextwindow/+packages
- Extract and compile
```
# chmod +x autogen.sh ; ./autogen.sh
# make
# make install
```
- Install module
```
# cp /usr/local/lib/xorg/modules/input/nextwindow_drv.* /usr/lib/xorg/modules/input/
```

# Add your local user to the input group

To be able to read the input device your local user *must* be part of the *input* group.
You should add the *gdm* user to this group as well.

```
# usermod -a -G input gdm
# usermod -a -G input your_username
```

# Reboot

Reboot and everything should work! Enjoy.
