# Connect and preare i2c FRAM MB85RC04V
var fram_addr = 0x50
var wire = tasmota.wire_scan(fram_addr)

var addr_pos = 0x0000

if !wire
   print("FRAM not found")
end

def fram_write_u16(addr, data)
 if !wire
  return 0
 end
 # split address into two bytes
 var addr_hi = (addr >> 8) & 0x7F
 var addr_lo = addr & 0xFF
 var data_hi = (data >> 8)
 var data_lo = data & 0xFF
 # ---------------- READ ----------------
 wire._begin_transmission(fram_addr)
 wire._write(addr_hi)
 wire._write(addr_lo)
 wire._write(data_hi)
 wire._write(data_lo)
 wire._end_transmission(true)
end

def fram_read_u16(addr)
 if !wire
  return 0
 end
 # split address into two bytes
 var addr_hi = (addr >> 8) & 0x7F
 var addr_lo = addr & 0xFF
 # ---------------- READ ----------------
 wire._begin_transmission(fram_addr)
 wire._write(addr_hi)
 wire._write(addr_lo)
 wire._end_transmission(true)
 wire._request_from(fram_addr, 2)
 var value_hi = wire._read()
 var value_lo = wire._read()
 var value = (value_hi << 8) | value_lo
 return value
end

var last_gauge_pos = fram_read_u16(addr_pos)

if last_gauge_pos
 print("FRAM gauge pos read:", last_gauge_pos)
end

tasmota.cmd("GaugeZero " + str(last_gauge_pos))

def co2_update(value, trigger)
 # print("value of ", trigger , " is ", value)
 var drivePos = 180 + ((int(value) - 400) * 2)
 # print("DrivePos: ", drivePos)
 if last_gauge_pos != drivePos
  tasmota.cmd("GaugeSet " + str(drivePos))
  last_gauge_pos = drivePos
  fram_write_u16(addr_pos, int(drivePos))
 end
end

tasmota.add_rule("S8#CarbonDioxide", co2_update)


