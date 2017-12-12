# CRTS
Cognitive Radio Test System


## Ports

Currently being developed on Ubuntu 16.04


## Dependencies

### libconfig


```console
apt-get install libconfig-dev libconfig-doc

```


### USRP Hardware Driver

USRP Hardware Driver is not a kernel driver.  It's just middle-ware.

Reference: http://files.ettus.com/manual/page_install.html

The following may install a lot of dependencies:
```console
sudo apt-get install libuhd-dev libuhd003 uhd-host
```


### liquid-dsp

TODO: CRTS depends on a very old version of liquid-dsp, and
the latest tagged release of liquid-dsp will not work without
some edits to CRTS.

Reference: https://github.com/jgaeddert/liquid-dsp/

We wrote a script to download and install it:
```console
./Install_liquid-dsp --prefix=/LIQUID-DSP_PREFIX
```
where */LIQUID-DSP_PREFIX* is where *lib/libliquid.so* and
*include/liquid.h* will be installed.



## Development Notes


### Notes: Changing IP address of USRP N210

Plug Ethernet cable from the USRP N210 directly to your computer
and get your computer to see it via

```console
uhd_usrp_probe
```

```console
uhd_find_devices
```

Next (or before the above) get the linux network interface name

```console
/sbin/ifconf
```

You'll need to setup your network so that you can connect directly to the
USRP ethernet port directly.  One at a time plug each USRP ethernet port
directly into your computer.  Setup the networking the way you want on
each USRP.  For example:

```console
/usr/lib/uhd/utils/usrp_burn_mb_eeprom --values="subnet=255.255.0.0, gateway=192.168.1.1, ip-addr=192.168.10.2"
```
Power-cycle the USRP device for the changes to take effect.

I setup 3 USRP N210 units with a NetGear WNDR3700v4 router the ip
addresses were: *192.168.10.2*, *192.168.10.3*, and *192.168.10.4*.
The 4th router port was used by the controlling computer (laptop).


Next confirm that we can probe each USRP via:

```console
uhd_usrp_probe --args="addr=192.168.10.2"
uhd_usrp_probe --args="addr=192.168.10.3"
uhd_usrp_probe --args="addr=192.168.10.4"
```

Another test which displays FFT scope GUIs:

```console
uhd_fft -f 865e6 -s 0.5e6 --args="addr=192.168.10.2" & 
uhd_fft -f 865e6 -s 0.5e6 --args="addr=192.168.10.3" & 
uhd_fft -f 865e6 -s 0.5e6 --args="addr=192.168.10.4" & 
```
