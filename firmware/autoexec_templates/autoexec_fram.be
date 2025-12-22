# Functions to read and write FRAM memory
# to store data during power off
# Connect and preare i2c FRAM MB85RC04V
var fram_addr = 0x50
var wire = tasmota.wire_scan(fram_addr)
# Address in FRAM to store last gauge position
var addr_pos = 0x0000
# Check initialization
if !wire
   print("FRAM not found")
end
# Function to write FRAM memory, 2 bytes
def fram_write_u16(addr, data)
 if !wire
  return 0
 end
 # Split address and data into two bytes
 var addr_hi = (addr >> 8) & 0x7F
 var addr_lo = addr & 0xFF
 var data_hi = (data >> 8)
 var data_lo = data & 0xFF
 # ---------------- WRITE ----------------
 wire._begin_transmission(fram_addr)
 wire._write(addr_hi)
 wire._write(addr_lo)
 wire._write(data_hi)
 wire._write(data_lo)
 wire._end_transmission(true)
end
# Function to read FRAM memory, 2 bytes
def fram_read_u16(addr)
 if !wire
  return 0
 end
 # Split address and data into two bytes
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
