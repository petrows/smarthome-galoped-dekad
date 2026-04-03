import animation

# try
#  anim.stop()
# except 'syntax_error'
#   # do nothing - normal first run
# end

var leds_count = 40

var duration = 10000
var strip = Leds(5 * 5, gpio.pin(gpio.WS2812, 0))
var anim = animation.core(strip)
anim.add_background_animator(animation.palette(animation.PALETTE_RAINBOW_WHITE, duration))
anim.start()
