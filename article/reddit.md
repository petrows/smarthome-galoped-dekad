Hello everyone and happy upcoming holidays! I wanted to make a couple of gadgets for friends' smart homes. And suddenly I thought - why are they all so boring? Let's make a sensor in a form factor that you definitely can't buy in a store today - it will be a great gift for New Year or Christmas. And something you won't be ashamed to give.

I'll clarify right away, the device is of course not analog, but fully digital. "Analog" here is only the design, inside there's nothing supernatural at the moment. It uses the popular SenseAir S8 sensor and an instrument stepper motor for automotive dashboards.

![](https://habrastorage.org/webt/lv/3s/81/lv3s81o7ysr2wmwmkmm0ux0bmh4.jpeg)

**Disclaimer:** electronics is just a hobby for me, so there may be inaccuracies and errors. Write comments, we'll fix them. Also, I'm an expat living in Germany, which leads to some specifics of this post: prices in euros + during development we'll only use free software on Linux.

![](https://habrastorage.org/webt/4u/5p/m5/4u5pm5pwkjyycnailinjmrccvpg.jpeg)

Creating this little guy took about a month and a half. The review is compiled from photos taken at different times when I was still actively searching - the photos may show different versions of devices, prototypes, there may be differences in details, imperfections and visible dust. AI assistance was not used in this article - accordingly there may be typos or strange wording. Please be understanding :)

![](https://habrastorage.org/webt/eg/r1/ou/egr1ou0ieftjfw8hlug1t-7bhls.jpeg)

Everything published here and in the repository is licensed under [GNU GPLv3](https://github.com/petrows/smarthome-galoped-dekad/blob/master/LICENSE).

# Technical Requirements

Our device was conceived as something you could give as a gift and the user could handle it themselves - without using a programmer, while having integration with popular Smart Home systems (but as an option). The second key requirement: the device must be technologically accessible, so that someone else who liked it could assemble and gift it without hassles like searching for Soviet radio tubes or cutting bronze with a jigsaw.

Let's formalize the requirements:

- CO2 level measurement with good accuracy and responsiveness;
- Measurement of standard climate parameters: temperature, humidity and pressure;
- Transmission of all this measured data to the smart home;
- Display of the current level on the device's analog display (with a needle);
- The device should have backlighting;
- Web interface with status display, settings, firmware updates, etc. - this is important so users can figure it out themselves;
- Preferably shouldn't look like scary DIY;
- The device should be easy to replicate and not contain unique components; Components should be available worldwide;

Initially I wanted to make an "aviation instrument", but in the process someone quickly asked me to also make a retro version. Accordingly, our device will have configuration options, each can be chosen independently:

- Scale style: "aviation" and "retro", with corresponding pointer;
- Backlighting: no backlight, "lamp-style" and "RGB";
- Sensors: CO₂, climate - can be installed in any combination, also without sensors - in this case the device will be controlled only by external commands, for example from the Smart Home system;

Requirements for the firmware specifically: it should have documentation, community and updates; Should be minimum reinventing the wheel and normal (for regular people) usability; Integration into smart home without complicated procedures and flashing;

# Naming and Marketing

I don't like that many good devices are often hard to find. Often nobody bothers with naming and makes something like "Advanced Co2 home sensor module", which is impossible to find or understand - where is what.

I came up with a name for the sensor series - Galoped. The name has no meaning, I was just standing outside smoking and thought - I need something in the IKEA style, consisting of the words "Haloperidol" and "Pediatrician". That's how I came up with it.

Later I regretted this name, because people constantly asked - what does it mean and what's the hidden meaning, I probably could have come up with something more obvious. But what's done is done.

# Concept Development

A bit of backstory - I've had the idea to make an "analog" indicator for a very long time. Somewhere around 2012 I bought a dual-pointer tachometer from a turboprop aircraft that I wanted to "revive". Naturally, nobody revived anything in 10 years :) But the idea didn't go anywhere, the device moved with me to Germany in 2017 and still sits on my desk:

![](https://habrastorage.org/webt/iu/b2/uk/iub2ukctqkvwcq4db9hkwi0wvca.jpeg)

This device is difficult to revive, it needs two three-phase signals that rotate control disks inside, which deflect the needles through a magnetic coupling. The device is large and probably noisy. But in principle it's possible, it can indicate something in percentages. In this case it would be better to use modern technologies and make everything from scratch - I seem to have everything for this: an old Ender-3 3D printer (the very first model) and my hands are itching.

Of course there are various indicators on the market, for example on AliExpress there are plenty of devices like this:

<details>
<summary>Devices from AliExpress</summary>

![](https://habrastorage.org/webt/9q/sd/lz/9qsdlz05crnhwmsllltl7cnnhyk.jpeg)

![](https://habrastorage.org/webt/93/jp/q6/93jpq6gmlitrnmfmgn5c6aagmyy.jpeg)

</details>

, but they're all made for street racers and not suitable for reasonable use. Until I understood what such devices run on - I thought to order one to try, disassemble and look. But it wasn't necessary, as it turned out that making from scratch would generally be easier.

I also considered using an analog volt- or ammeter. They show what's needed, the signal is easy to generate through a DAC. Here I got stuck on the fact that rotation angles out of the box are too small - out of the box it's usually about 90° for most models. Using real aviation voltmeters or some "special" ones is also out for us (see requirements for technological accessibility). Besides, I thought it would be much harder to make a beautiful pointer and everything else for a voltmeter, since they're usually very delicate creatures, because we can only control low voltage directly. Various voltmeters from VAZ or Soviet instruments are also out, as they're exotic outside the CIS.

In the end I decided to use automotive dashboard technology. They somehow rotate needles there, right? They have everything beautiful and a large angle. So, we'll look in this direction.

The next question - what exactly to display? You can display various smart home metrics, from work or home infrastructure, but that's too specific. In the end we choose CO2 level - fits perfectly: there are simply no such sensor designs (I didn't find a single one), this metric changes slowly, is autonomous (can be measured on site), besides, it's useful - you can see when it's time to ventilate. In our office, by the way, people constantly argue - whether to open windows or not, and such a device can resolve such disputes through direct instrumental measurement. I won't describe for the hundredth time - what CO₂ measurement is and if the topic is unclear to you - [you can read, for example, here](https://www.iqair.com/newsroom/indoor-carbon-dioxide-co2). As a meter we choose SenseAir S8, as the most authoritative among popular ones. You can read about it and comparison with others [here](https://mysku.club/blog/china-stores/75322.html). And of course, this device can display anything, strictly speaking CO₂ is just an example to get a complete device.

When the idea of creating a new device came to mind - it seemed that everything was simple, take a stepper motor (servo?) and drive it in one of a million ways. But it quickly became clear that a servo doesn't fit - noisy, inaccurate and has too small a rotation angle.

Pretty quickly I googled that there exists a real popular stepper motor X27-168, more precisely - a whole family of them. Pretty quickly I found [a post by the wonderful guy.carpenter](https://guy.carpenter.id.au/gaugette/2017/04/29/switecx25-quad-driver-tests/) with experiments with drivers for these motors. These motors are widely used in various devices, for example [popular with sim racers](https://simvimx.com/gau_stepper_x27.html). Also, it turned out there's [a whole big family](https://github.com/petrows/smarthome-galoped-dekad/blob/master/datasheet/juken-stepper-motors-for-dashboards-datasheet_vers.3.0.pdf) of them - for example there are variants with 2 needles or zero sensors (variants for clocks). These and similar motors are installed in practically all cars and widely available everywhere. Used for example in OPEL or VAZ dashboards (for example in VAZ-2110).

There's a whole family and multiple clones of special drivers that implement the microstepping algorithm: when one command from the controller rotates the drive not by one division (usually 1/3°), but by 1/12°. Also the datasheet says this method is preferable - it increases accuracy and reduces drive noise. With great difficulty I found an [English datasheet for VID6608](https://github.com/petrows/smarthome-galoped-dekad/blob/master/datasheet/VID6608.pdf) and uploaded it to the repo, then ordered a batch of 10 pieces.

First I ordered six motors right away (the set cost 12€), they arrived first. I didn't have drivers yet, and I tested them through random H-Bridge drivers that I found in my junk. In principle it works, but looks very so-so - poor accuracy, strong noise. The needle moves unnaturally. Yes, in principle you can already say - it works like this, but it still looks little like a car dashboard.

![](https://habrastorage.org/webt/rj/4h/a1/rj4ha1ydbapcopmw4hp5y_qaium.jpeg)

While I was doing other things - the drivers arrived (and I immediately got breakout pcb for SOP-28). I assembled the first prototype on breadboards and started testing. In principle, something already worked, but here was the first disappointment: assembling the firmware on my knee in platform.io with the [SwitecX25](https://github.com/clearwater/SwitecX25) library from the respected guy.carpenter I ran into a bunch of bugs and problems. Had to fix them on the fly, which slowed down the process quite a bit.

![](https://habrastorage.org/webt/dt/lw/ge/dtlwgerckyvat5vn22331qkb_k8.jpeg)

Let's start with the fact that the library doesn't work under ESP32 at all - there's careless use of `char`, which under ESP32 has type `unsigned` and we get integer overflow. Moreover, this specific bug was fixed, but they didn't release a version - if you pull the library as dependencies in platform.io, you'll get bugs. I poked around with a logic analyzer and found more problems - for example the library doesn't care about the necessary timeouts that the VID6608 datasheet requires.

At some point I came to the conclusion that it's easier to write my own from scratch than to figure out the original - the author clearly poked around a bit and then lost interest in the topic. Pull requests have been hanging for a long time. But still - huge thanks to the author! He laid the foundation for implementation.

From the very beginning I planned to use [Tasmota](https://tasmota.github.io/docs/) firmware as the basis for the device, since it already contains everything needed and is widely known. But here I was met with the next disappointment that Tasmota only has support for H-Bridge drivers for "ordinary" stepper motors.

So, we form our plan - how do we assemble everything together?

- Add support for instrument stepper motors to Tasmota
- Find a suitable donor case (alarm clock)
- Design and order a printed circuit board
- Design and print case parts
- Come up with backlighting somehow
- Assemble this together and figure out something so it all works together :-)

## Adding Support for Instrument Motors to Tasmota

So, if we decided - we need to do it :-) Clone the project and dive into the documentation. In practice everything turned out to be not simple: documentation in principle exists, but quite fragmentary, I had to choose a couple of simple drivers and use them as a live example. I looked at how support for other libraries is done - they're just committed directly into the repository, not as submodules or dependencies. At this point I decided it's easier to write my own library and add it there + driver.

I won't go into much detail about development, everything is done in the image and likeness of the existing SwitecX25 - just without bugs)) Differences from the old one:

- Support for control only through VID6608 driver and similar - direct control is pointless, imho
- Support for acceleration curve settings
- Support for setting zero from saved position
- Fewer calculations per step, simpler control algorithm
- More precise compliance with datasheet, the old one didn't care about some timings required by the datasheet

As a result, a new library appeared: [arduino-vid6608](https://github.com/petrows/arduino-vid6608).

The library uses a simpler acceleration calculation algorithm, where there's a static distance-delay array, and up to the first 1/2 of the path distance comparison and speed increase occurs, in the second half deceleration occurs. Position setting functions are asynchronous, the vid6608::loop() function is sensitive to the accuracy of calling the update function.

<oembed>https://youtu.be/dTKQrNPrnPg</oembed>

The video shows an example of the algorithm working - the device just shows random values. In the end I left it like this - in my opinion it turned out quite realistic with minimum calculations.

In this mode the engineering sample worked for a little more than two days without stopping - I didn't detect any missed steps or reading drift.

Next we choose the next free Driver-ID in Tasmota and occupy it. I got number 92, as a result the driver [xdrv_92_vid6608](https://github.com/arendst/Tasmota/blob/development/tasmota/tasmota_xdrv_driver/xdrv_92_vid6608.ino) was born. Note that the implementation is specifically as a general-purpose driver, initially I thought to make it as a [display](https://tasmota.github.io/docs/Displays/), but this class of driver doesn't have sufficiently accurate timing to ensure smooth needle movement.

After making the first version of the driver - we immediately face two new problems:

- Needle movement is very slow - due to the fact that the loop function is not called in Tasmota with sufficient resolution
- The zero setting function on startup causes Watchdog triggering in some cases - due to blocking the main thread for too long

The solution to the first problem is obvious: for ESP32 in Tasmota FreeRTOS from the ESP-IDF development package is used, which means we can use primitives and functions of the real-time operating system. Actually the solution is simple as a log: we'll just start update processing in a separate thread, which by default will go to the second core (we'll put a dual-core ESP32 version on the board):

```cpp
// Start background RTOS thread -> required for precision timing
xTaskCreate(
    VID6608XvTask,                /* Function to implement the task */
    "VID6608XvTask",              /* Name of the task */
    1024,                         /* Stack size in words */
    NULL,                         /* Task input parameter */
    0,                            /* Priority of the task, lowest */
    NULL                          /* Task handle. */
);
...
void VID6608XvTask(void *) {
    while(true) {
        ...
        driver->loop();
        ...
        /*
            If we dont need to move any -> go sleep.
            This will delay next move begin up to 500ms, but freeds up CPU a lot.
        */
        if (!needToMove) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}
```

Good, but this trick will only work for ESP32, and for the old ESP8266 this solution doesn't work. In the end I left it as is - I didn't plan to use ESP8266, and in principle the driver still works there - just quite slowly: about 20 seconds for every 10 degrees of movement. This is still enough for displaying slowly changing parameters (such as temperature or number of likes on this post). Note that the loop in `VID6608XvTask()` sleeps for a long time if the motor finished moving: this slows down the start of the next movement by 0...500 ms, but significantly unloads the core with its sleep.

The solution to the second problem is less obvious: you just need to add a call to the `yield()` function in the procedure for setting the required step:

```cpp
void vid6608::step(vid6608::MoveDirection direction, uint16_t delayUs) {
    ...
    // We should keep resources reserved by others
    yield();
}
```

In this case even multiple calls in a loop won't block our program and we'll get rid of Watchdog crashes.

We add FreeRTOS synchronization primitives - mutexes. Initially I didn't bother with them, because I decided the problem shouldn't be big, at worst something extra will get into the MQTT output, and we'll save on resources. But no: multithreading synchronization problems quickly emerged on the assembled device, so we make ourselves a macro, wrapping all this in ESP-32-only code:

```cpp
#ifdef VID6608_RTOS
  SemaphoreHandle_t vid6608Mutex;
  #define VID6608_MUTEX_TAKE   xSemaphoreTake(vid6608Mutex, portMAX_DELAY);
  #define VID6608_MUTEX_GIVE   xSemaphoreGive(vid6608Mutex);
#else
  #define VID6608_MUTEX_TAKE
  #define VID6608_MUTEX_GIVE
#endif
```
, which can be safely put everywhere needed, and it all compiles for esp8266 too. In general, driver operation under esp8266 is not strictly mandatory, Tasmota's guideline only requires that it also compiles for esp8266, in principle the feature can be left as an exclusive for ESP32, but I decided to leave it - we're not Apple to make artificial limitations for people.

I also want to touch on another problem: timings. For beautiful rendering of needle movement we must maintain timings down to 0.3ms (300 μs), which is already impossible with FreeRTOS timers. Yes, they have their own implementation of `delayMicroseconds()`, but looking at the source code we're met with frustration: there's just a loop of `nop`s for the required time. Which simply turns waiting time into heat instead of timers. The problem is currently not solved - if someone knows a good solution - please write in the comments. On the bright side: the problem is not that big, it only manifests during active needle movement and only in some sections.

In any case: patches were successfully accepted into Tasmota and bugs fixed:

- [Adding the driver](https://github.com/arendst/Tasmota/pull/24153) - here you can see an example of a complete package of changes for adding drivers, pins to them and commands
- [Fixing multithreading bugs (I didn't bother creating synchronization objects until I assembled a real device)](https://github.com/arendst/Tasmota/pull/24189)
- [Adding support for reset from saved state and fixing smaller bugs](https://github.com/arendst/Tasmota/pull/24218)

At the time of writing this review, the driver is already included in release [Tasmota v15.2.0 Stephan](https://github.com/arendst/Tasmota/releases/tag/v15.2.0). Note that the third package of changes (reset from saved state) didn't make it into the release, so the base for the firmware is still `development`.

The driver adds the following commands:

- `GaugeZero N` - calibration procedure, where `N` is optional initial position (see next section)
- `GaugeSet N` - set position in absolute motor steps, from 0 to 3840 (320*12)
- `GaugePercent N` - set position in percent, from 0 to 100

The driver supports up to 4 motors. All commands by default control the first motor, can have a motor suffix (from 1 to 4), with 0 being all motors. Example: `GaugeSet0 520` - will set all motors to value 520, `GaugeZero3` - reset only motor #3.

On firmware startup all motors are reset one by one, which somewhat slows down initialization, but making it parallel is quite difficult, we'd need to complicate our small driver, which is also generally undesirable. I left it like this.

# Drive Calibration

This was one of the main mysteries for me: when I first ordered and was waiting for the motors, I tried to google or find out - how do others do it? On sim racer forums they suggested all sorts of bulky solutions like optical sensors, and everyone else - just didn't write anything about this. In the old library in the source code I found an explanation - there simply was backward movement for the entire scale, which apparently led to the drive braking in the extreme position, after which the current position was taken as zero. This really works, but leads to pounding our motor against the end stops - not good.

Actually the solution: let's just remember the current value "somewhere" and then use it for calibration. In this case we can simply advance the needle forward by the remainder from the last value, and then a full circle backward. If the needle was out of sync with what we had recorded, at worst pounding will occur either at the beginning or at the end (depending on where the real value went), but in the end the needle will be guaranteed in the zero position.

The only question is - where to write? Flash doesn't work, because it has a severely limited number of writes. But everything has already been invented before us: this is Ferroelectric RAM (FeRAM or FRAM) — RAM similar in its design to DRAM, using a ferroelectric layer instead of a dielectric layer to provide non-volatility. The chip is inexpensive and provides up to 10¹² write cycles, this should be enough for a little more than 31 thousand years of continuous operation with writing once per second. Implementation is very simple - let's just put an inexpensive [Fujitsu MB85RC04V FRAM](https://github.com/petrows/smarthome-galoped-dekad/blob/master/datasheet/MB85RC04V.pdf) chip on the board and write there every time the needle receives a command.

<oembed>https://youtu.be/uMyvno2kHuc</oembed>

Example of restoration from saved state. Everything works and doesn't pound where it shouldn't :-)

In general, besides FRAM there are other solutions, for example [DRAM with backup to flash](https://github.com/petrows/smarthome-galoped-dekad/blob/master/datasheet/47L04-EERAM.pdf) when power is disconnected and similar, but in this case it's excessive. We don't need to remember much, it's inexpensive. In automotive dashboards I think there's no special procedure at all - since the system is under constant battery backup there, I don't have that option. In my car the dashboard does a needle test - exactly like I got it, but it's purely decorative, can be disabled with a jumper.

# Hardware Implementation

So, we've worked out the main ideas and can proceed to designing the electronics of our device.

**Attention**: I will use the schematic with corrections of the original design. You may notice differences from photos of the real device, but the schematic already contains bug fixes. This is done for ease of replication. Original schematics can be found in the [repository](https://github.com/petrows/smarthome-galoped-dekad/tree/master/shematic), if you're interested in the original implementation specifically.

Let's go to EasyEDA and draw the schematic:

![](https://habrastorage.org/webt/lk/ij/nf/lkijnftihogm8jtys3wcjl-yqzc.jpeg)

The basis for the entire device is the ESP32-WROOM-32 module, dual-core, 4MB flash. You can of course put any other in the same form factor.

The most important thing at this stage is to choose the right GPIO. If you use the wrong pin that has dual purpose - this can lead to a bunch of unexpected effects, for example the chip won't boot if some pins have pullup at that moment. I used this guide: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/ - everything at hand and understandable.

## Reset Circuit

![](https://habrastorage.org/webt/uf/2z/oo/uf2zoo6avka4oeloxnaltsf6mim.jpeg)

This circuit is needed for remote chip reset from esptool without the need to press reset/boot buttons. The circuit is copied from the ESP32-Devkit-v1 module. Implements a NOR element to ensure reset signal sequence from DTR or RST. This provides flashing through a suitable USB-UART module in fully automatic mode.

## Drive Driver

![](https://habrastorage.org/webt/ok/zw/kf/okzwkflxzj0dbwmtmwd9tlpnw-y.jpeg)

Actually - the most important thing :) Drive driver, dual-channel. Stock device is assembled with one needle, but can be equipped with double if necessary. The chip is not controlled by reset circuit from MCU, as the datasheet recommends, because my AliExpress VID6608 chips reacted inadequately - possibly not originals :) In any case, pulling reset to power and putting recommended capacitors is the basic minimum, works as it should then.

## Extension Connector

![](https://habrastorage.org/webt/is/r3/dt/isr3dt_bzgzd4cvoqqaafr4ocbq.jpeg)

GPIO 4, 13 and 25 are brought out to this connector as well as the I²C bus, to which additional sensors or actuators can be connected, for example a clock. Power and ground are present. Also on the board there are two more GPIOs (18 and 23), brought out just to pads on the board, you can solder to them.

## Drive Position Memory Circuit

![](https://habrastorage.org/webt/8x/k0/ic/8xk0icxr_efwsexeskfukv8sh6w.jpeg)

Implemented on I²C FRAM memory chip MB85RC04V, manufactured by Fujitsu. Has 4KB of memory on board - quite modest by our standards, but inexpensive and works :)

## Everything Else

Nothing particularly interesting further, from the module the following is brought out:

- Reset button for manual EN drive. Resets the chip when necessary. To avoid unnecessary resets there are pullup circuits and anti-bounce capacitor.
- GPIO34 button: configured as `Button 0` in Tasmota. Single press turns backlight on/off, holding for 45 seconds resets settings and activates initial setup through access point.
- Boot button: pulls GPIO0 low, enters flash mode if pressed simultaneously with EN. _This button is not in the photos, added later_.
- SenseAir S8 sensor, connected to GPIO 16 and 17 for communication, calibration port connected to GPIO 19
- ANT20+BMP280 module, connected to I²C, measures temperature, humidity and pressure. Is an option, if absent there will simply be no telemetry.
- "Retro" backlight control is implemented on a bipolar transistor circuit with open collector - can control something else of course.

Software calibration trigger can help if you want to calibrate the sensor forcibly (otherwise it performs auto-calibration every 7 days). But in the firmware I haven't yet figured out how to make a normal UI for this. Nevertheless, the sensor can be stuck out the window, hold the pin for 5 seconds - the sensor will then take the current value as 400 ppm.

## Printed Circuit Boards

We order and wait :) Something happened with delivery and I received the boards only three weeks later with apologies. Boards ordered with component installation on one side, without ESP32 module (I had them separately, plus I wasn't sure I didn't make mistakes).

![](https://habrastorage.org/webt/9f/xp/pt/9fxpptyo5ybgv2lluiuiuut9izs.jpeg)

Popular x27-168 and similar are recommended to be fixed by soldering. Good idea, in general, but requires a separate printed circuit board. For installing the motor in the case I made a simple variant with pins for both connector and soldering.

We assemble and check - almost everything works :) The first version had one bug - I didn't put a pullup for button 0, because [Tasmota documentation claimed](https://tasmota.github.io/docs/Buttons-and-Switches/) that with this configuration the internal pullup resistor in the MCU is used. But no, nothing. If you leave it floating then the button triggers, which can lead to settings reset (on "pressing" for 45 seconds). But fortunately it's very easy to fix - we throw a through-hole resistor on top and done.

![](https://habrastorage.org/webt/1j/xl/my/1jxlmyddv8d2th9fsktst7yjqpm.jpeg)

![](https://habrastorage.org/webt/xc/hq/-r/xchq-rmsjrssleq3br0wdsnve70.jpeg)

Links to projects in EasyEDA Web:
 - [Main board](https://oshwlab.com/petrows/galoped-dekad)
 - [Breakout board for drive](https://oshwlab.com/petrows/galoped-dekad-drive)

# Device Case

This is probably the most important. In this section we'll determine - whether our device will look like a hack or like something decent. Of course, the case can be made completely on a 3D printer, but I rejected this idea immediately, the sides will show that the case is printed, and I wanted something nicer, which means - purchased and factory-made. Besides, our requirements have a point that the device should be simple to replicate. Therefore, Aliexpress is poorly suited here - sellers often have actual products that differ depending on time and may not match the picture. And the internals can also be different revisions, and we need everything to fit well.

![](https://habrastorage.org/webt/s8/_m/f1/s8_mf1aomgchcfuyi9z1ew41y0a.jpeg)

I decided to start with IKEA. The reasons are as follows: IKEA is distributed all over the world and their products are strictly the same everywhere. Therefore, you can make such a device for yourself practically anywhere, you can buy an IKEA thing even where there's no IKEA. I looked at what they have and chose [Ikea Dekad](https://www.ikea.com/de/de/p/dekad-wecker-schwarz-30540479/). This is the only thing that fit more or less.

![](https://habrastorage.org/webt/s2/zg/9v/s2zg9vqsdldh0xf0f7tmangjmp0.jpeg)

![](https://habrastorage.org/webt/no/0k/30/no0k30h93vi79g2jh4zuebahnic.jpeg)

I want to say that I didn't really like this lot from the very beginning, I thought there would be difficulties. But how wrong I was! The device is just as if invented for DIY. Beautiful and even case, very technological - minimum excess, everything inside on screws. Dial, frame and glass - separate parts, which allows for flight of imagination. On top there's a hole for the bell drive - which is electric and driven by an electric motor. It can be left and for example surprise the user with a loud bell if CO₂ is too high. But I decided this would be overkill :)

![](https://habrastorage.org/webt/gb/yu/hh/gbyuhhynkswz2xp8w3miqhkrrlm.jpeg)

From the original alarm clock we'll need: actually the case, glass, frame (I thought so then, but actually - not quite), stainless steel nuts and feet, as well as the needle tip - we'll press it into ours.

What we learned at this step:

- Device dimensions: cylinder approximately 95 by 50 mm
- Dial diameter
- The name acquires a suffix from its essence: `Galoped-dekad`

# Dial

The face of our device - everything here should be maximally beautiful. It took me more time than planned. I started by finding a [video](https://www.youtube.com/watch?v=kPpti6-jHaQ) where a guy makes a circular scale in Inkscape. Did it similarly - I just have a newer version, there were small differences. It took me a long time to figure out - for the "Pattern along path" modifier to work - your scale piece sample must be one whole curve, then everything is done correctly. We make the document immediately for a photo printing kiosk, for 10x15.

![](https://habrastorage.org/webt/-r/4z/13/-r4z13e7f6ll_i5aczvy-m8uadk.jpeg)

We open a photo of an aviation instrument - our mascot and get inspired :) Actually the design took several days, plus I changed it several times in the process - changed the dead zone display, made the green and red sectoral lines more similar to a real instrument.

![](https://habrastorage.org/webt/-d/ge/i3/-dgei3gfxry3mbekp3copekw9zs.png)

And, immediately we make a retro version:

![](https://habrastorage.org/webt/7i/ju/l7/7ijul7l_p3hfngbbfiyvvxcqe6q.png)

For scale design we must determine minimum and maximum values. We look at the datasheet for our x27.168 motor and see that a full rotation is 315° (actually 320°, but at the time of scale design I didn't know this). Now we estimate: we have 315*12 steps, need to capture approximately a suitable range and minimize calculations (especially division). Also need to make a small "dead zone" (to indicate device not-ready state). We calculate: leave 15° for dead zone display, so we have 300° of significant range left. Divide it into 18 pieces, turns out we can display from 400 to 2200 ppm, and most importantly, we get even correspondence: `(300*12) / (2200-400) = 2`, meaning we get exactly two drive steps per 1 ppm. This is very convenient, because then converting one value to another can be done simply by shifting 1 bit left or right, which is much faster than other mathematical operations in the MCU. And we also have an extra 5° beyond the scale (and datasheet) - but I just didn't bother with this, because I learned about this much later and didn't redo it because of this. But that value must definitely be taken into account during calibration.

![](https://habrastorage.org/webt/hm/zm/fc/hmzmfc4y2us1dxwe-t-0ur1wavo.jpeg)

We go to DM and print our wonder-printing in a kiosk. For us this pleasure costs about 30 cents for small batches. We only have glossy photo paper for kiosk printing, matte needs to be ordered. I thought this was a problem, planned to spray with matte varnish - but it's not needed, everything looks super as is.

![](https://habrastorage.org/webt/q5/bl/vm/q5blvmmalv8huas16yr8djw3qh8.jpeg)

Next we face the problem - how do we make a beautiful hole in the center so it doesn't look like a hack? I tried drilling or cutting, turned out so-so. In the end the solution came like this - I ordered leather punches on Amazon. Just punching with a hammer turns out poorly, but if you clamp the punch in a drill chuck and "drill", you get a perfectly even and beautiful hole.

# Backlighting

Here I fiddled most in the end. Initially the plan was simple - in the IKEA alarm clock the standard frame is as if made specifically for backlighting, it's semi-transparent, knurled on the inside - looks super. But in practice everything turned out not so simple.

![](https://habrastorage.org/webt/su/j_/ks/suj_ks5zcq3st99buta1vqjhsyy.jpeg)

It immediately became obvious that an ordinary LED strip is poorly suited - I wanted the backlight to look "lamp-like", that is without visible diodes. For this it was decided to purchase flexible filament threads at 3 volts. They look great, glow too. I took the variant where the leads are on one side of the tube - don't do that, mounting is then very inconvenient. Next versions I'll do on strips with connection from different ends.

![](https://habrastorage.org/webt/hu/cv/ii/hucvii4yuzbtwlwpxpoahrmdfng.jpeg)

We glue with transparent tape to the original frame and look. Looks bad - due to the fact that the frame is pressed directly to the glass - we get a bright halo around the dial. In the photo it doesn't look so bad, but in reality everything is much worse.

![](https://habrastorage.org/webt/_e/lm/mm/_elmmm4nctagibaf4z0gkih1avu.jpeg)

For RGB backlighting I purchased 2.7mm strip with addressable WS2812b LEDs. With the original frame the problem is the same - they also give a sharp halo around the dial, and also look even worse - the backlight from the diodes looks very spotty, little like real instruments.

![](https://habrastorage.org/webt/s_/8k/gi/s_8kgimzar-kzccjr7utj09jfnk.jpeg)

I tried printing my own frame from white plastic to get a diffuser, but it helps little. The photo shows an example of such a variant. In the end the halo can be reduced a bit if you glue the LED strip with diodes outward - but they still shine through the strip.

![](https://habrastorage.org/webt/oa/pi/qs/oapiqsvnkdsapz5khbqng_yg37w.jpeg)

Example of a frame from "transparent" plastic, as you can see also - so-so. In this section it's quite brief, but I printed and tested a bunch of variants, with diodes inside the ring, outside, closer to the glass and dial - poor result.

![](https://habrastorage.org/webt/kb/uo/pe/kbuopexenx7tlv27ux2vih_9_wc.jpeg)

In the end I decided like this: made a frame from two parts - a thin black ring under the glass to kill the halo, wrapped the semi-transparent part from inside with white electrical tape to dim the diodes. I don't like this variant either, but it already looks generally acceptable.

![](https://habrastorage.org/webt/io/ex/u2/ioexu2pcxsoirticplw1cocwhfi.jpeg)

The photo shows an example with RGB strip, electrical tape and intermediate black frame. You can see details on 3D models. Filament strip fits in the same frame without modifications.

![](https://habrastorage.org/webt/j4/70/nv/j470nvszptsgo9udkq-0kkhmj44.jpeg)

Addressable diodes also allow making different effects, for example sectoral backlighting or changing color depending on readings. Sectoral looks generally bad - it gives a bright glare on the opposite side of the device.

![](https://habrastorage.org/webt/ss/j6/hb/ssj6hbji0gkjlxbnzaynbtpnd40.jpeg)

In the end we put both backlighting variants in a two-piece ring, we won't provide any effects in stock, just on-off and brightness adjustment. Here of course there's still much to think about, but I think I'll postpone until next versions.

![](https://habrastorage.org/webt/gp/w7/hc/gpw7hcdeisjhn1us_dr_zhcwnt0.jpeg)

We get two parts: the semi-transparent frame itself and a black ring against glare on the glass.

# Case Parts

Now it's time to make the plastic part of our wonder-device. Most of the visible component (case, dial) already look good, we just need to take care of the back wall, needle and cover the hole on top of the case (from the bell mechanism). We open FreeCAD and proceed to drawing our parts. By the way, I thought I was quite familiar with this CAD program, but I decided to watch training videos before starting, found [Dima Gog's channel](https://youtube.com/playlist?list=PLrwLdIBuPZQYLgEido7oWpHCu6io0WywY&si=4EqxRqk2pRcRh05R) also from Germany, and just was amazed - I didn't know practically anything. Big thanks to him, I did everything needed much faster. Highly recommend to everyone who wants to learn to make beautiful parts quickly and efficiently.

## Needle

So, we made the scale, but what should ride on it? I thought about this for a long time, googled forums. They suggest many things - cut from an old card, punch, file by hand from metal. I decided to still try to print carefully on a 3D printer with maximum resolution. If you print upside down on a good substrate the surface turns out smooth, you can't even really see - that it's printed. In the process of searching for optimal form and technology I made probably a hundred of these needles, they're now lying around everywhere.

![](https://habrastorage.org/webt/9o/xf/r4/9oxfr43_bdmbucgovzdfluucrja.jpeg)

I have a favorite varnish from Lidl - muffler paint. It gives a cool matte surface, I immediately decided to spray the black part of the needle specifically with it. We print, paint - first the butt, then the needle itself with white varnish that I borrowed from my wife.

![](https://habrastorage.org/webt/7p/ua/lq/7pualqmfbik5c6qeys8qbva2w-q.jpeg)

Turns out approximately like this. I initially wanted to mess with glow-in-the-dark paint, but then didn't - better to make backlighting for the whole device. So we just paint with available paints/varnish and it turns out good.

![](https://habrastorage.org/webt/vr/si/ig/vrsiiga4smjgvlfmwv633zgqsyu.jpeg)

Now an important moment - we take the original tip from the IKEA alarm clock, drill a 1.8mm hole in the needle and press the tip into our needle in a vice. Turns out great - our needle already looks almost like a real one in an airplane. Besides - it fits perfectly tightly on the motor shaft, don't need to figure anything out with fastening. The retro version needle looks almost the same and isn't painted at all, I print immediately with black plastic.

## Interior

In general, the device consists of parts: frame (if configuration with backlighting, otherwise we use original IKEA), front panel with saddle for motor and board, back cover, covers for S8 and Climate sensors (if installed).

![](https://habrastorage.org/webt/rb/dd/hl/rbddhlslwh4nethv2xe_t5_b1uq.jpeg)

![](https://habrastorage.org/webt/7s/vr/sc/7svrsctjstyp-yu1lppza250-e0.jpeg)

This is what the assembly drawing looks like. We combine parts, rotate and look - so all holes match, nothing interferes with each other. The tricky part was only perhaps to calculate - where exactly the mounting holes are located in the IKEA case: they're spaced at 30° and 34° from the vertical axis.

![](https://habrastorage.org/webt/vs/mo/eu/vsmoeutmlbqzrykscl8tnzbqavi.jpeg)

Had to fiddle separately with covers - for S8 and climate sensors. In the end I settled on this - the S8 cover is inserted from inside and held just by pressure (or you can drop glue for certainty), the climate sensor is mounted on screws to the cover. Since both sensors are options, the covers may not be installed (I haven't had anyone request a version without CO₂ measurement on board yet, in this case we'll need to make a special cover - or just remove the hole for it).

![](https://habrastorage.org/webt/m2/6l/-s/m26l-sjx268z4h27u3bybxhwnue.jpeg)

Also at this stage we generate a 2D drawing, for instructions for future users :) I created a Page in FreeCad for this and placed the needed View with isometric view of the product there, just had to export where needed.

![](https://habrastorage.org/webt/xa/fr/lk/xafrlkew2fa9ay3-9jmiawk8k4a.jpeg)

We print all parts, remove supports and clean. I made prototypes from PLA. I printed all parts of the final device with black PETG plastic, backlight frame from "transparent". Case parts and frames - 0.2 mm layer, 20% infill, 20% fan. Sensor covers and needle - 0.1 mm layer. Be sure to enable ironing to get beautiful flat surfaces, especially those that stick out.

All models (CAD and STL) can be seen in the repo: https://github.com/petrows/smarthome-galoped-dekad/tree/master/case

## Fasteners

Initially I assembled everything on M2.3 self-tapping screws for plastic, but quickly got tired - looks worse, questions about strength, adds "cheapness" to the look of our serious special product. Specifically for the device I got M2 and M3 screws with hex head from stainless steel. Mounting in plastic will be on heat-set inserts - just an amazing thing turned out, how did I live without them before?

![](https://habrastorage.org/webt/xn/e2/jn/xne2jnbbfj2m3ymfv9yyoraubem.jpeg)

Screws look much more solid, adds a bit more "aviation" style. Some things need to be taken from the IKEA alarm clock (M3 nuts and washers, a couple of M2 self-tapping screws for mounting the USB-C port if you don't have your own).

## Final Product Assembly

So, let's now connect everything together! Finally we'll see what kind of Frankenstein we've got here. We choose a configuration for assembly: Aviation style, lamp backlight, both sensors installed.

![](https://habrastorage.org/webt/da/lz/vl/dalzvl7sba6dmlwg1awbbkvzxhs.jpeg)

For assembly we should already have all the parts that remained from previous steps or findings. Or just print new ones on 3D and 2D printers, order boards, connectors, wires. We lay all this out on the table and admire. For assembly we generally need the following:

- IKEA DEKAD alarm clock
- Printed parts: front and back covers, top cover, CO₂ and climate sensor covers, two backlight frame parts
- Fasteners: M3 screws (2 pcs), M2 screws (10 pcs), Heat-set inserts for plastic M2 (10 pcs)
- [Filament backlight strip](https://www.aliexpress.com/item/1005009437951627.html) (26 cm or longer) and wires for it
- Printed circuit boards: main and motor
- [X27-168 motor](https://www.aliexpress.com/item/1005008385301163.html), leads on back side
- [ESP32-WROOM32E module (4Mb)](https://www.aliexpress.com/item/1005002515949841.html)
- [VID6608 chip](https://www.aliexpress.com/item/1005007050530920.html)
- [SenseAir S8 sensor](https://www.aliexpress.com/item/1005009122286447.html)
- [Climate sensor - AHT20 / BMP280 module](https://www.aliexpress.com/item/1005005486181411.html)
- [SMD buttons 3x6](https://www.aliexpress.com/item/1005009645342892.html), any height - I have 4.5 mm
- [JST-PH 2.54 connectors](https://www.aliexpress.com/item/1005008339053787.html) and wires: 2-pin (2 pcs), 4-pin (1 pc). I'm too lazy to crimp, got pre-crimped
- [USB-C power connector](https://www.aliexpress.com/item/1005010142667928.html)
- Wires, pin headers, soldering iron, good mood and determination :)

![](https://habrastorage.org/webt/bn/ib/lz/bniblzzee7vcunouvrouetklqds.jpeg)

First of all we install the ESP32 chip. I ordered without installation of it, because I had them and didn't want to risk possible defects. Then I cursed everything - installation is a brutal hassle, I ruined one module. Order installation at the factory together with the board - it will be the same price and much less hassle.

![](https://habrastorage.org/webt/jy/pf/o0/jypfo0prg4qk6er8smvnyqhma1u.jpeg)

Immediately after installation it's important to check operability - flash the firmware and check that the device comes to consciousness. You can flash any Tasmota, then we'll flash what's needed over the air. I use [this adapter](https://www.aliexpress.com/item/1005008554998627.html), it has the needed RST and DTR outputs (note that DTR is brought out on the side header - I was stupid at first and confused it with CTS and ruined one board, thinking I did something improper). In this case the chip flashes directly through esptool or platform.io. If you don't have such an adapter, then any will do, main thing is it's compatible with 3.3V, short the contacts to enter flash mode manually then. It's better to supply external power - 3.3V in my version of the debug connector from the adapter was enough for flashing, but not enough for first boot anymore. In your board version this problem is solved (5V is brought out), but in any case external power is recommended - just in case. In this case connect only ground from the adapter, otherwise you can burn something.

![](https://habrastorage.org/webt/ym/qg/k5/ymqgk5qdsvhv9c9gj0uyfmemau4.jpeg)

Hooray! Seems we didn't ruin anything. Yet :)

![](https://habrastorage.org/webt/ls/k3/eq/lsk3eq4km3q6azrz2_d4m6lviy0.jpeg)

We solder the rest - connectors and actually our hero of the day - VID6608. It's much easier to install than ESP32, harder to mess up. Be sure to check pins after installation! Everything looked super for me, but half the pins actually weren't making contact. If poorly soldered it will look like this: the drive will jerk chaotically instead of smooth movement. We install everything else - button connectors and such.

![](https://habrastorage.org/webt/9j/is/bo/9jisbombe_n-cifgmuwexxuycnc.jpeg)

We install sleeves for M2 screws. Installation is very simple - place on the hole and press in with soldering iron, everything turns out even and beautiful.

![](https://habrastorage.org/webt/_f/8x/3z/_f8x3zgjo_vtq9525hlo6tfzfsu.jpeg)

We solder our motor to the mounting board and put a connector. Holes for the header can be drilled out and push wires through there for better retention.

![](https://habrastorage.org/webt/ez/c0/0a/ezc00akgncexh1uru6yh1te1kxw.jpeg)

After installing the drive we can cut out and glue the front panel. We prepare the needle, press the tip into it and paint. After this we can assemble everything together.

![](https://habrastorage.org/webt/dh/hg/ti/dhhgtiyv2uzwewibp1pozysp-kw.jpeg)

At this stage it's important to set the needle exactly to zero. For this we pull the needle on as we can (it goes on very tight, don't be shy), then rotate the motor to the stop (without signal it rotates freely) to zero and carefully rotate the needle until it's exactly at the beginning of the dead zone, this is the true zero of the scale.

![](https://habrastorage.org/webt/kk/an/2h/kkan2htlmb-5v_mknunlsjq8ni8.jpeg)

We disassemble the alarm clock, remove everything valuable from it, remove the sticker and clean. We get a beautiful glossy cylinder. We carefully remove the glass and preferably don't smudge it - greasy fingerprints will be hard to remove later.

![](https://habrastorage.org/webt/1e/z_/dg/1ez_dgk7r_tw_qakb1lujadgfoo.jpeg)

We prepare the filament strip - solder to the leads at right angles and make a connector. I also have a correction circuit soldered: 220 Ohm resistor and ripple suppression capacitor. In the board version you'll see in EasyEDA these parts are already installed (at the time of creating the very first version I was still in doubt about the backlighting).

![](https://habrastorage.org/webt/dt/v4/ye/dtv4yekdsn3-baogdnb1dkohtsi.jpeg)

We install the strip in the frame, it's best to turn it with the "matte" part inward. For convenience you can glue with transparent tape. Also put a bit of tape between the contact petals for insulation, there's no room to properly fit in heat shrink. We trim the strip so it meets evenly - then we'll have minimal dark piece of backlighting.

![](https://habrastorage.org/webt/7r/nz/lf/7rnzlfcc3-dc4z9xmchsrg0puno.jpeg)

We insert the glass, black frame and our frame with strip into the case. It's important to put in correct orientation - parts are made to enter tight fit (so nothing rattles) and rotating later can be difficult. Wires should exit exactly at the bottom center to get into the cutout of the main part.

![](https://habrastorage.org/webt/pw/wv/zu/pwwvzuy3l26h31brnx-8jtwh75a.jpeg)

We install the main case part, pass backlight wires through. Now we can screw in our main fastening lines: feet and M3 screws through the top cover.

![](https://habrastorage.org/webt/ql/a4/gh/qla4ghdpbsxvfvpjfn9mbw_jiqm.jpeg)

We connect all connectors and install the main board, orient it with sensors down. We screw everything with beautiful M2 screws.

![](https://habrastorage.org/webt/8g/e8/eq/8ge8eqwxj9-2g_zaouzzfhq0aao.jpeg)

In the back cover we screw the USB-C connector on small screws, pushing it from inside. You can use original silver IKEA ones from the alarm clock, but I have anodized black ones, they look better. We install the S8 sensor grille, the sensor itself and place the cover in place, screw everything on two M2 screws.

![](https://habrastorage.org/webt/6o/5d/nr/6o5dnrujcwa-mrzwhmjtr41smlw.jpeg)

Our climate sensor turns out sticking out of the case. This is good - then heat from the MCU won't affect its readings.

![](https://habrastorage.org/webt/ko/x_/f5/kox_f5rcpditesm7kz5d5ax7ms8.jpeg)

We poke our climate sensor on the connector and screw the cover on top with two more M2 screws. Assembly complete :-)

![](https://habrastorage.org/webt/ak/id/bg/akidbggeu1coocgqykcwapj8pxo.jpeg)

We move to arm's length distance, turn on and check that everything works. Note that without initial configuration the device won't even call calibration "out of the box", we need to configure Tasmota and prepare the unit for work.

## Configuration

First of all we need to configure our "module". Which pins sit where and what they do. To simplify life I made templates (version for RGB and Lamp backlights): differences in one pin - it's either on WS2812B or on lamp dimmer transistor. The template can be applied manually, but it's more convenient to do it right in the firmware. Besides, we'll need a couple of `#define`s: by default Tasmota has no support for either AHT20 or my wonderful driver. We create file `tasmota/user_config_override.h`:

```cpp
// Required features for this project:
// [I2cDriver43] Enable AHT20/AM2301B instead of AHT1x humidity and temperature sensor (I2C address 0x38) (+0k8 code)
#ifndef USE_AHT2x
    #define USE_AHT2x
#endif
// [I2cDriver10] Enable BMP085/BMP180/BMP280/BME280 sensors (I2C addresses 0x76 and 0x77) (+4k4 code)
#ifndef USE_BMP
    #define USE_BMP
#endif
// Add support for SenseAir K30, K70 and S8 CO2 sensor (+2k3 code)
#ifndef USE_SENSEAIR
    #define USE_SENSEAIR
#endif
// Add support for VID6608 Automotive analog gauge driver (+0k7 code)
#define USE_VID6608
// Reset VID6608 on init (default: true), change if you control this manually
#define VID6608_RESET_ON_INIT false

// Enable WS2812 leds number (any HW model)
#undef WS2812_LEDS
// If RGB backlight is used, it will have 40 LED's
#define WS2812_LEDS 40

// Template defaults, apply on flash reset.
// You may skip this template application, in this case please apply manually (see README.md)
// Read from file, generated by ./bin/build.sh or CI
#include "user/user_config_hw.h"
// Activate template on reset/flash new
#undef MODULE
#define MODULE USER_MODULE
```

Also, to choose a specific configuration we choose one of two variants as file `tasmota/user/user_config_hw.h`:

```cpp
/*
  Part for Galoped-dekad hw version:

    * Backlight: Retro
*/

#undef FRIENDLY_NAME
#define FRIENDLY_NAME "Galoped-dekad-retro"

#define USER_TEMPLATE "{\"NAME\":\"Galoped-dekad-retro\",\"GPIO\":[1,1,1,1,1,1,1,1,1,1,416,0,1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0,1,1,32,1,1,0,0,1],\"FLAG\":0,\"BASE\":1}"
```

, this was for lamp, for RGB:

```cpp
/*
  Part for Galoped-dekad hw version:

    * Backlight: RGB
*/

#undef FRIENDLY_NAME
#define FRIENDLY_NAME "Galoped-dekad-rgb"

#define USER_TEMPLATE "{\"NAME\":\"Galoped-dekad-rgb\",\"GPIO\":[1,1,1,1,1,1,1,1,1,1,0,1376,1600,1632,1,1,0,640,608,1,0,1,12160,12192,0,0,0,0,1,1,32,1,1,0,0,1],\"FLAG\":0,\"BASE\":1}"
```

As you can see - difference in one pin and redefine `FRIENDLY_NAME` of the module name - so as not to forget :) Now we can build the firmware.

For the lazy: go to the [Releases](https://github.com/petrows/smarthome-galoped-dekad/releases) section of my project and download the ready one there.

Only very small things remain: need to check everything and connect the needle to CO₂ readings. Fortunately, nothing even needs to be modified in the firmware - Tasmota has [Berry script](https://tasmota.github.io/docs/Berry/) in the version for ESP32, so we'll just write a script and throw it into the unit. For autostart we need to name this wonder-file as autoexec.be and put it through the built-in file manager:

```python
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
# Read last gauge position from FRAM
var last_gauge_pos = fram_read_u16(addr_pos)
if last_gauge_pos
 print("FRAM gauge pos read:", last_gauge_pos)
end
# Call Reset option from saved position, and save zero
tasmota.cmd("GaugeZero " + str(last_gauge_pos))
fram_write_u16(addr_pos, 0)
# Function to update Gauge position on CO2 change
def co2_update(value, trigger)
 var drivePos = 180 + ((int(value) - 400) * 2)
 if last_gauge_pos != drivePos
  tasmota.cmd("GaugeSet " + str(drivePos))
  last_gauge_pos = drivePos
  # Save current position into FRAM
  fram_write_u16(addr_pos, int(drivePos))
 end
end
# Add rule to monitor CO2 changes
tasmota.add_rule("S8#CarbonDioxide", co2_update)
```

[This file in the repo](https://github.com/petrows/smarthome-galoped-dekad/blob/master/firmware/autoexec.be). Functionally consists of two parts - reading/writing to FRAM and the CO₂ reading function:

```python
tasmota.add_rule("S8#CarbonDioxide", co2_update)
```

This call hooks the co2_update callback on update. As you can see - couldn't be simpler. Now our device has come to life and will display something.

For reading and writing values to FRAM we look at the [Datasheet](https://github.com/petrows/smarthome-galoped-dekad/blob/master/datasheet/MB85RC04V.pdf). Our data is `uint16_t` (must fit the full range, i.e. from 0 to 3840), therefore two bytes. Our address will simply be `0` - we don't store anything else there yet. As well as two address bytes. For writing we split the address and data byte-wise and send the write command: first address, then data:

```python
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
 ```

Reading is done the same way. We check - everything works! Let's add some code for state restoration and updating:

```python
# Read last gauge position from FRAM
var last_gauge_pos = fram_read_u16(addr_pos)
if last_gauge_pos
 print("FRAM gauge pos read:", last_gauge_pos)
end
# Call Reset option from saved position, and save zero
tasmota.cmd("GaugeZero " + str(last_gauge_pos))
fram_write_u16(addr_pos, 0)
```

After resetting the drive don't forget to immediately write the new value - otherwise we'll have an incorrect starting position when power is disconnected before the CO₂ sensor reaches operating mode.

We connect our device to WiFi following instructions through the standard Tasmota procedure - access point, open the device's IP address and enjoy the web interface:

![](https://habrastorage.org/webt/vj/rn/60/vjrn60kopxioe8f7bqcj0p_ykgk.jpeg)

The device should also show sensor readings and needle position in the web interface. Depending on configuration there will be either a dimmer for "lamp" backlighting or color selection for RGB. Then it's just a matter of technique - follow Tasmota instructions to configure Smart Home integration and so on, our firmware differs only in basic configuration and all features are available:

- MQTT support;
- HTTP-API support;
- Support for [native and cloud integrations](https://tasmota.github.io/docs/Integrations/), such as AWS IoT or ioBroker;

And much more that you didn't forget to enable in config ;)

I have quite a large [smart home in OpenHab](https://github.com/petrows/smarthome-openhab) (about 150 devices), integration there is done standard way, like for any Tasmota. In the example we declare device `sz_climate`:

```java
Thing mqtt:topic:openhab:sz_climate "SZ Climate" (mqtt:broker:openhab) {
    Channels:
        Type number : rssi [stateTopic="tele/sz_climate/STATE", transformationPattern="JSONPATH:$.Wifi.RSSI"]
        Type string : bssid [stateTopic="tele/sz_climate/STATE", transformationPattern="JSONPATH:$.Wifi.BSSId"]
        Type number : la [stateTopic="tele/sz_climate/STATE", transformationPattern="JSONPATH:$.LoadAvg"]
        Type datetime : activity [stateTopic="tele/sz_climate/STATE", transformationPattern="JS:codegen-activity.js"]
        Type switch : light [stateTopic="stat/sz_climate/RESULT", transformationPattern="REGEX:(.*\"POWER1\".*)∩JSONPATH:$.POWER1", commandTopic="cmnd/sz_climate/POWER1", on="ON", off="OFF"]
        Type number : co2 [stateTopic="tele/sz_climate/SENSOR", transformationPattern="JSONPATH:$.S8.CarbonDioxide", unit="ppm"]
        Type number : temperature [stateTopic="tele/sz_climate/SENSOR", transformationPattern="JSONPATH:$.AHT2X.Temperature", unit="C°"]
        Type number : humidity [stateTopic="tele/sz_climate/SENSOR", transformationPattern="JSONPATH:$.AHT2X.Humidity", unit="%"]
        Type number : dewpoint [stateTopic="tele/sz_climate/SENSOR", transformationPattern="JSONPATH:$.AHT2X.DewPoint", unit="C°"]
        Type number : pressure [stateTopic="tele/sz_climate/SENSOR", transformationPattern="JSONPATH:$.BMP280.Pressure", unit="hPa"]
}
...
Switch sz_climate_backlight "SZ Climate Backlight" <light> (g_all_sw) {channel="mqtt:topic:openhab:sz_climate:light"}
Number:Dimensionless sz_climate_co2 "SZ Climate CO₂ [%d ppm]" <co2> (g_all_co2) {channel="mqtt:topic:openhab:sz_climate:co2"}
Number:Temperature sz_climate_temperature "SZ Climate temp [%.0f %unit%]" <temperature> (g_all_temperature,sz_hz_temperature_sensor) {channel="mqtt:topic:openhab:sz_climate:temperature"}
Number:Dimensionless sz_climate_humidity "SZ Climate humidity  [%.0f %%]" <humidity> (g_all_humidity) {channel="mqtt:topic:openhab:sz_climate:humidity"}
Number:Temperature sz_climate_dewpoint "SZ Climate dewpoint [%.0f %unit%]" <temperature> (g_all_dewpoint) {channel="mqtt:topic:openhab:sz_climate:dewpoint"}
Number:Pressure sz_climate_pressure "SZ Climate pressure  [%.0f %unit%]" <pressure> (g_all_pressure) {channel="mqtt:topic:openhab:sz_climate:pressure"}
Number:Dimensionless sz_climate_rssi "SZ Climate RSSI [%.0f]" <network> (g_all_rssi) {channel="mqtt:topic:openhab:sz_climate:rssi"}
String sz_climate_bssid "SZ Climate BSSID [%s]" <network> (g_all_bssid) {channel="mqtt:topic:openhab:sz_climate:bssid"}
DateTime sz_climate_activity "SZ Climate activity [JS(codegen-display-activity.js):%s]" <time> (g_all_activity) {channel="mqtt:topic:openhab:sz_climate:activity"}
```

In this example we connected our items to onboard sensor readings (CO₂, temperature, humidity and pressure), and also added backlight on/off. You can also control backlight dimming and/or color (for RGB version), for this you need to add an item with type `Dimmer`.

And of course - you can disconnect the needle from the sensor and control directly by Smart Home command, for this just send MQTT command `cmnd/sz_climate/GaugeSet` with payload in absolute steps. You can add a dimmer to the smart home panel and rotate the needle manually.

# Device Cost

Exact cost heavily depends on the number of units manufactured and what you already have on hand. In my case purely in hardware it came out to about 50€ per device, and this is provided that I have my own 3D printer and case parts are conditionally free. The main cost is the sensor, it alone is more expensive than all other electronics combined. If someone needs an exact calculation write in comments - I'll add it.

# Conclusions and Future Thoughts

So this is the wonder of technology we've got - what do you think about it?

![](https://habrastorage.org/webt/nc/42/8b/nc428bmxdadoubhyffmj4sziuqu.jpeg)

Stretched our hands and not ashamed to give as a gift. Well and also added a good driver to Tasmota.

![](https://habrastorage.org/webt/je/s7/5q/jes75q25lo3ngqjuh439vjacgbw.jpeg)

Old and new devices together - gestalt closed.

In general, the device is of course easily customizable and adaptable to harsh reality:

- Appearance: I wrote people's names on the dial (for those I gifted to) - turns out a cool gift in the form of a unique gadget that you can't buy in a store;
- The device can of course display anything, both from sensors and from external control; Here it's up to imagination - temperature, humidity, devils in a mortar or remaining work day for today;
- On the board there's an expansion connector that can accept basically anything - I²C sensors, displays and so on;
- The board can control two motors - both one dual-pointer and two separate;
- You can add another VID6608 or replace with VID6606 - in this case you'll be able to control 4 drives at once;
- You can control the original alarm motor and ring loudly when needed :-)

What I think should be improved in the future (possibly when you read this review something is already done, see updates in repo):

- I think we should add FRAM work to Tasmota itself, it's inconvenient that we read-write in script, I think automatic writing and reading during calibration can be done. But not sure if such would be accepted - probably such a patch is too specific;
- Need to split one 8-pin connector for two motors into two 4-pin: such solution turned out very inconvenient;
- All connectors need to be flipped 180°, turned out impossible to install angled ones with current layout - would be more convenient that way;
- The breakout board idea for the motor turned out not very good - it greatly complicates construction and is generally a pointless solution. I think a good universal printed cover with direct wire mounting would be better;
- Should add some status indication on the board, otherwise it's impossible for user to understand - is it alive at all or not;
- The USB-C connector used only works with USB-A-C cable, USB-C-C doesn't fit. Not critical, but certainly a minus;

Project sources: https://github.com/petrows/smarthome-galoped-dekad . There's currently a small mess in the repo from test parts and no proper Readme - didn't have time to clean up, will tidy up a bit later. But everything is nevertheless there.

If you have more good ideas - share them, we'll add. Have a good day everyone ;)
