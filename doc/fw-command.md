# How to command device

This section describes, how to send commands to device.

Original firmware defines new command `GalopedSetX Y`, where
`X` - gauge number starting from `1`, `Y` - target value *in gauge unuts*.

To use this command, device [Guage configuration](fw-config.md) is required.

Examples:

``` bash
# Set Gauge 1 tpo display 77 (units, i.e. percent)
GalopedSet1 77
# Set Gauge 1 tpo display 32 (units, i.e. degrees)
GalopedSet2 32
```

Command can be issues via:

* Console (**Tools** -> **Console** or Serial console)
* Via HTTP, for example `http://<ip>/cm?cmnd=GalopedSet1%2023` (use `%20` instead of space)
* MQTT

And other methods, see full list: https://tasmota.github.io/docs/Commands/#how-to-use-commands
