var i2c_addr = 0x50
var mb85rc_slaveid = 0xF8

wire = tasmota.wire_scan(i2c_addr)
print(wire)

wire._begin_transmission(mb85rc_slaveid >> 1)
wire._write(i2c_addr << 1)
result = wire._end_transmission(false)

wire._request_from(mb85rc_slaveid >> 1, 3)
print(wire._read())
print(wire._read())
print(wire._read())

wire._end_transmission(true)
