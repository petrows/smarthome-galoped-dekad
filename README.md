

Tasmota template:

```json
{"NAME":"Galoped-dekad","GPIO":[1,1,1,1,1,1,1,1,1,1,416,1,1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0,1,1,32,1,1,0,0,1],"FLAG":0,"BASE":1}

{"NAME":"Galoped-dekad","GPIO":[1,1,1,1,1,1,1,1,1,1,416,1,1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0,1,1,1,1,1,0,0,1],"FLAG":0,"BASE":1}
```


Berry script to drive Gauge:
```
var last_gauge_pos = 0

def co2_update(value, trigger)
 # print("value of ", trigger , " is ", value)
 var drivePos = 180 + ((int(value) - 400) * 2)
 # print("DrivePos: ", drivePos)
 if last_gauge_pos != drivePos
  tasmota.cmd("GaugeSet " + str(drivePos))
  last_gauge_pos = drivePos
 end
end

tasmota.add_rule("S8#CarbonDioxide", co2_update)

```

var co2Int = int(value)
 print("IntValue is ", str(co2Int))
 var drivePos = 180 + ((co2Int - 400) * 2)
 print("DrivePos: ", drivePos)
 tasmota.cmd("GaugePercent " + drivePos)



 ```
var drivePos = 37
tasmota.cmd("GaugePercent " + str(drivePos))
var strip = Leds(40, gpio.pin(gpio.WS2812))
var color = 0x00FF00
strip.clear()
for i: 22 .. 28
    strip.set_pixel_color(i, color)
end
strip.show()
 ```