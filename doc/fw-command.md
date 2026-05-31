# How to command device

This section describes, how to send commands to device.

## Primary gauge command

Original firmware defines new command `GalopedSetX Y`, where
`X` - gauge number starting from `1`, `Y` - target value *in gauge unuts*.

To use this command, device [Guage configuration](fw-config.md) is required.

Important: this command saves the state into FRAM memory (if present),
this is importnant to re-run correct zero procedure on power cycle.

Examples:

```ruby
# Set Gauge 1 to display 77% (units, i.e. percent, as defined in galoped.ini)
GalopedSet1 77

# Set Gauge 2 to display 32°C (units, i.e. degrees, as defined in galoped.ini)
GalopedSet2 32
```

Command can be issues via:

* Console (**Tools** -> **Console** or Serial console)
* Via HTTP, for example `http://<ip>/cm?cmnd=GalopedSet1%2023`
(use `%20` instead of space)
* MQTT

And other methods, see full list:
<https://tasmota.github.io/docs/Commands/#how-to-use-commands>

## Raw commands

Each unit has also raw commands to move: in steps or in procent from physical drive scale,
defined by VID6608 Tasmota driver. This commands ignore gauge settings in `galoped.ini`
and the **not** updating gauge position in FRAM - use with cauton.

Use following commands to control raw drive movement:

| Console Commands | Description                                      | Values      |
|------------------|--------------------------------------------------|-------------|
| `GaugeSet`       | Set position in absolute steps                   | `0` - `3840`|
| `GaugeSetPercent`| Set position in percent                          | `0` - `100` |
| `GaugeZero`      | Reset drive; optional argument is saved position | `0` - `3840`|

All commands have Drive suffix. If not set, it drives first motor. If set, it uses Drive number.
If suffix is `0` - it drives all motors. Examples:

* `GaugeZero3`: Reset drive number 3 with default reset routine;
* `GaugeZero2 751`: Reset drive number 2 with expected last position as `751`;
* `GaugeSet0 366`: Set all drives absolute position 366;
* `GaugeSetPercent 30`: Set drive number 1 position to 30%;

See full driver documentation here: <https://tasmota.github.io/docs/vid6608/> .
