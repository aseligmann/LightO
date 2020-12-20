# LightO

LightO is an open remote-controlled light project.

The project is based around an ESP32 driving a string of addressable LEDs, in this case a custom PCB with 61 SK6812 RGBW LEDs.

The user is able to control the LightO through an intuitive web-based interface, consisting of color-wheel and slider control.
The brightness of the light is user-controlled and can be driven at up to roughly 15 W, for an eye-scorching brightness level.

The LightO has 3 modes which can be selected from the interface:

* **Colour** - User defines a colour to display
* **Candle** - Simulates a flickering candle
* **Cycle** - Cycles through the colour-spectrum

The light is powered by a single USB-C port. 


### Images

(https://github.com/aseligmann/LightO/blob/master/Images/front_off.png "Front")

(https://github.com/aseligmann/LightO/blob/master/Images/front.png "Front")

(https://github.com/aseligmann/LightO/blob/master/Images/left.png "Left side")

(https://github.com/aseligmann/LightO/blob/master/Images/back.png "Back")

(https://github.com/aseligmann/LightO/blob/master/Images/interface.png "Interface")


### Features
- Full control of colour and brightness (RGBW channels) for all LEDs
- USB-C powered
- WiFi based control with user-centered design
- Intuitive captive AP for setup


### Development
- [ ] Provide schematic for ESP32 piggyback board
- [ ] Wake-up mode simulating a sunrise at a userdefined time
- [ ] Migrate code to PlatformIO
- [ ] Integrate USB-C PD and ESP32 on LED PCB


### Bill of Materials

#### PCB
* 1x 160 mm LightO PCB (light_160mm_v1_0_2020-04-21)
* 61x SK6812 LEDs
* 61x 5050 LED lenses, wide angle (optional)
* 61x 0805 100 nF capacitors
* 2x JST-XH 2-pin straight connectors

#### ESP32
* 1x ESP32 development board
* 1x 100 uF capacitor
* 2x 330 Ohm resistors

#### Power-delivery
* 1x USB-C PD module (ZY12PDN)

#### Misc.
* 7x M3 bolts (16 mm)
* 7x M3 nuts
* 3x 2.2x9.5 self-tapping screws