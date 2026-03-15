import animate

var leds_count = 40

var duration = 10000
var strip = Leds(leds_count, gpio.pin(gpio.WS2812, 0))
var anim = animate.core(strip)
anim.set_back_color(0x2222AA)
anim.set_bri(60)                    # set brightness to 40%
var pulse = animate.pulse(0xFF4444, 2, 1)
var osc1 = animate.oscillator(-3, leds_count, 5000, animate.COSINE)
osc1.set_cb(pulse, pulse.set_pos)

# animate color of pulse
var palette = animate.palette(animate.PALETTE_STANDARD_VAL, 30000)
palette.set_cb(pulse, pulse.set_color)

anim.start()
