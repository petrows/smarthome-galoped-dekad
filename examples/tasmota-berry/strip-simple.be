# === Setup ===
leds = Leds(40, gpio.pin(gpio.WS2812, 0))
buf = leds.pixels_buffer()
m = pixmat(buf, 40, 1, 3, true)   # serpentine = true

# === Helper: show and pause ===
def show_pause(ms)
    leds.show()
    tasmota.delay(ms)
end

# === 1. Fill background ===
m.clear(0x101010)   # dim grey background
show_pause(500)

# === 2. Draw a perfect blue diagonal ===
for i: 0..7
    m.set(i, i, 0x0000FF)   # blue
end
show_pause(1000)

# === 3. Moving vertical bar ===
for x: 0..31
    # Clear previous bar
    m.clear(0x101010)
    # Redraw diagonal
    for i: 0..7
        m.set(i, i, 0x0000FF)
    end
    # Draw vertical bar in yellow
    for y: 0..7
        m.set(x, y, 0xFFFF00)
    end
    show_pause(50)
end

# === 4. Scroll test ===
# Scroll right
for s: 1..5
    m.scroll(3, 1)   # dir=3 (right), steps=1
    show_pause(100)
end

# Scroll down
for s: 1..3
    m.scroll(2, 1)   # dir=2 (down), steps=1
    show_pause(100)
end

# Scroll left
for s: 1..5
    m.scroll(1, 1)   # dir=1 (left), steps=1
    show_pause(100)
end

# Scroll up
for s: 1..3
    m.scroll(0, 1)   # dir=0 (up), steps=1
    show_pause(100)
end

# === End pattern ===
m.clear(0x000000)    # clear to black
leds.show()
