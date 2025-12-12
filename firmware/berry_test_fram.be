# MB85RC FRAM raw test (minimal)
# Tasmota berry implementation to write and read 2 bytes from FRAM chip MB85RC04V
import string

var dev = 0x50
var addr = 0x0001
var data = 0xCEDA

var wire = tasmota.wire_scan(dev)
if !wire
  print("FRAM not found")
  return
end

print("FRAM found at", dev)

# split address into two bytes
var addr_hi = (addr >> 8) & 0x7F
var addr_lo = addr & 0xFF
var data_hi = (data >> 8)
var data_lo = data & 0xFF

print(string.format("Write %x %x data to %x %x", data_hi, data_lo, addr_hi, addr_lo))

# ---------------- WRITE ----------------
wire._begin_transmission(dev)

wire._write(addr_hi)
wire._write(addr_lo)
wire._write(data_hi)
wire._write(data_lo)

wire._end_transmission()

tasmota.delay(2)

# ---------------- READ ----------------
wire._begin_transmission(dev)

wire._write(addr_hi)
wire._write(addr_lo)

wire._end_transmission(true)

wire._request_from(dev, 2);

var value_hi = wire._read()
var value_lo = wire._read()
var value = (value_hi << 8) | value_lo

# value may be int or bytes array
var read_val = value

print(string.format("Read back: %x", read_val))
print("Test OK =", read_val == data)
