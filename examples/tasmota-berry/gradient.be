# Gradient green -> yellow -> red for 40 LEDs
# Green (0,255,0) -> Yellow (255,255,0) -> Red (255,0,0)

var strip = Leds(40)
var num_leds = 40

for i: 0 .. num_leds - 1
  var r, g, b
  if i < 20
    # Green to Yellow: increase red from 0 to 255
    r = (i * 255) / 19
    g = 255
    b = 0
  else
    # Yellow to Red: decrease green from 255 to 0
    r = 255
    g = 255 - ((i - 20) * 255) / 19
    b = 0
  end
  strip.set_pixel_color(i, (r << 16) | (g << 8) | b)
end

strip.show()
