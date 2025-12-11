# MB85RC FRAM raw test (minimal)

var dev = 0x50
var addr = 0x0001
var test_byte = 0xCE

var wire = tasmota.wire_scan(dev)
if !wire
  print("FRAM not found")
  return
end

print("FRAM found at", dev)

# split address into two bytes
var addr_hi = (addr >> 8) & 0x7F
var addr_lo = addr & 0xFF

print("Write hi lo data:", addr_hi, addr_lo, test_byte)

# ---------------- WRITE ----------------
wire._begin_transmission(dev)

wire._write(addr_hi)
wire._write(addr_lo)
wire._write(test_byte)

wire._end_transmission()

tasmota.delay(2)

# ---------------- READ ----------------
wire._begin_transmission(dev)

wire._write(addr_hi)
wire._write(addr_lo)

wire._end_transmission(true)

wire._request_from(dev, 1);

var value = wire._read()

# value may be int or bytes array
var read_val = value

print("Read back:", read_val)
print("Test OK =", read_val == test_byte)
