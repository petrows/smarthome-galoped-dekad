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


