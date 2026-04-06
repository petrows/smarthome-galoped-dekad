# Galoped (CO2 version)

![Device](assets/device-co2-all.jpg)

## Device specs

Device has customizations on top of base Galoped model:

* On-board sensors: SenseAir S8 for Co₂, BMP280+AHTO20 for Temperature, Pressure and Humidity readings
* Backlight RGB can follow CO2 readings and indicate current green-yell-red state

## RGB backlight for status

To enable RGB backlight auto-color option, navigate to "Configuration" -> "Configure Galoped" and select
"RGB Mode" to "Dynamic".

This will enable Backlight auto color, driven by current CO2 readings.

*Note:* Color can be still changed manually or via command, it will be updated
on next changed value received from sensor.
