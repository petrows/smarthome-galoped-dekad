class TWINKLE
    var strip, matrix
    var fast_loop_closure
    var tick, frame_div
    var W, H
    var hue_buf, val_buf

    def init()
        self.W = 40
        self.H = 1
        self.strip = Leds(self.W * self.H, gpio.pin(gpio.WS2812, 32))
        var bpp = self.strip.pixel_size()
        var buf = self.strip.pixels_buffer()
        self.matrix = pixmat(buf, self.W, self.H, bpp, true)

        self.hue_buf = bytes(-(self.W * self.H))
        self.val_buf = bytes(-(self.W * self.H))

        self.tick = 0
        self.frame_div = 2  # adjust for speed

        self.fast_loop_closure = def () self.fast_loop() end
        tasmota.add_fast_loop(self.fast_loop_closure)
    end

    def deinit()
        self.strip.clear()
        tasmota.remove_fast_loop(self.fast_loop_closure)
        tasmota.remove_driver(self)
    end

    def idx(x, y)
        return y * self.W + x
    end

    def fast_loop()
        self.tick += 1
        if self.tick % self.frame_div != 0 return end
        self.update_twinkle()
        self.draw_twinkle()
    end

    def update_twinkle()
        import crypto
        var total = self.W * self.H

        # Fade all pixels slightly by reducing brightness
        var i = 0
        while i < total
            var v = self.val_buf[i]
            if v > 5
                v -= 5
            else
                v = 0
            end
            self.val_buf[i] = v
            i += 1
        end

        # Randomly light up a few new twinkles
        var new_count = 3
        var n = 0
        while n < new_count
            var idx = crypto.random(1)[0] % total
            self.hue_buf[idx] = crypto.random(1)[0]  # random hue
            self.val_buf[idx] = 255                   # full brightness
            n += 1
        end
    end

    def draw_twinkle()
        var w = self.W
        var h = self.H
        var hue = self.hue_buf
        var val = self.val_buf
        var y = 0
        while y < h
            var x = 0
            while x < w
                var k = y * w + x
                var v = val[k]
                if v > 0
                    self.matrix.set(x, y, hue[k], 200, v)  # fixed saturation=200
                else
                    self.matrix.set(x, y, 0, 0, 0)          # off
                end
                x += 1
            end
            y += 1
        end
        self.strip.show()
    end
end

var anim = TWINKLE()
