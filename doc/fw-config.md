# Firmware configuration

Device starting from version 2 uses gauge and primary features defined via
`galoped.ini` configuration file. Configuration files are located within
[Tasmota's UFS](https://tasmota.github.io/docs/UFS/) (Tools -> Manage File System menu).

Generic format is:

```ini
[galoped]
serial = GDA-001
mac = 05:05:12:34:56:78
display = Bi-Axial example
model = biax_test
backlight = RGB
color = black
assembled = 2026-04-08
[gauge-1]
name = Temp
unit = C°
deg = 270
min = 0
max = 300
[gauge-2]
name = Progress
unit = %
deg = 270
min = 0
max = 100
```

Section `galoped` defines primary device function and proprietary information (serial number and customizations).

Value `model` defines internal logic for some device variations. Possible values are:

* `co2`: device will indicate with gauge (and optionally RGB) readings of SenseAir S8
* `3dp_tp`: device will enable 3D Printer watch, currently temperature and progress

All other values in this section are used for informational purpose only.

Section `gauge-1` and `gauge-2` define current gauge display settings.

* `name`: display name in UI and metrics
* `unit`: display units in UI and metrics
* `deg`: gauge range in degrees (with dead-zone included)
* `deg_dz`: gauge dead-zone in degrees
* `min`: minimal gauge value (in units)
* `max`: maximal gauge value (in units)

Example for `CO2` meter:

```ini
[galoped]
serial = GDA-001
mac = 05:05:12:34:56:78
display = CO2
model = co2
backlight = RGB
color = black
assembled = 2026-04-08
[gauge-1]
name = CO2
unit = ppm
deg = 320
deg_dz = 15
min = 400
max = 2200
```

This defines single-axis drive with gauge of 320° range, 15° starting dead-zone, marked as 400 - 2200 ppm for full scale.

*Note:* Original devices, built by me, have this file signed. Signature is checked for informational page display only
and does not affect or disable any features used. Any device can be reconfigured by your own.
