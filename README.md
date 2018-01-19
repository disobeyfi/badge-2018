# Sources for Disobey 2018 badge

![Image of USER badge](https://raw.githubusercontent.com/disobeyfi/badge-2018/master/web/badge.jpg)

Disobey Badge is an electronic device that was given for all Disobey visitors. It is part of the badge challenge.
The badge uses most of it's time on sleeping and playing simple animations using its leds as a 14-pixel display.
It also includes a game in which the user has to collect keys from all 9 different badge types, to obtain access to further puzzles on the "cloud". Badge is designed to resemble Nintendo style gamepad, and behaves as one when it's plugged to a computer using USB.

## Functional Description

Technically the badge is a device build around STM32F0 microcontroller. It ticks at 8MHz normally with an internal RC oscillator.
Each badge type has it's own binary, and each one of them has a different hardcoded key and led brightness correction parameter.
Communication with other badges is handled through a half-duplex uart. Badge normally operates with a coin battery. When USB power line is connected, the USB power is used instead, and the mcu can detect this by checking a GPIO that's connected to the USB power.

#### Keys

During the boot badge hashes it's current key accumulator using SHA-256. It compares first 32 bits of the hash to list of hashes of all valid key combinations.
(if hash is not found, the accumulator is resetted to badge's internal code) If reset was user-initiated, the key combination bitmask is flashed on the leds for a little period. At the same time uart is polled for incoming communication (fondling) and button gpio's are polled for the first press of the konami code.

#### Data Transfer

If button is pressed during boot, badge goes to a code input mode. In this mode user is expected to press next button in the sequence.
If any other button is pressed, the sequence is resetted to the first step. If no key is pressed for some time, badge goes back to the animation mode. When the code sequence is completed, a pilot signal is sent from the uart. After each pilot signal iteration, badge checks for real incoming transfer.

Badges check if an incoming pilot signal is detected on the boot. If this happens, badge goes directly to the code send mode.
Badge sends its key in a packet that roughly follows the format of [sync][key][key2]. The receiving badge (the one that initiated the exchange) has a sequencer that checks if the sync string has been completely received. After that it tries to receive a key two times. If received keys match, the key accumulator is xorred with the received key. After this the badge reboots as to show the new state of the accumulator, but usually the transfer connection still exists and code send is triggered. This is a bug, but since it didn't seem to cause any (electrical or other) damage, an acute wave of laziness killed any attempts to fix it.

#### USB

If USB power is detected during boot sequence, badge goes to the gamepad mode. In this mode the badge acts as a USB HID Gamepad device with two axes and four buttons. The HID descriptor was generated using Frank Zhao's excellent "USB Descriptor and Request Parser" (http://eleccelerator.com/usbdescreqparser/). USB code is based on Gareth McMullin's example.

If user has pressed keys during boot and gotten to the konami code input, USB power is also polled simultaneously. If its detected, badge presents itself as a HID keyboard instead. After initialization the current status of the key accumulator is written and after this buttons emit keypresses instead.

#### Animations

In animation mode, badge uses random generator to select an animation sequence. Most of the animation sequences are written like two-dimensional functions which are evaluated only on the coordinates of the led lights. This resembles a bit how GPU fragment programs work.

Once "pixel color values" or luminances are evaluated, they are passed to the light driver. Light driver first applies per-board brightness correction, global brightness control (for fade-in/-outs) and gamma correction for the values. Since multiple leds might be connected to some timers and some timers don't control any leds, only the appropriate timer clocks are enabled as to conserve power.

Animations play for a random time. After this RTC wakeup interrupt is set up, and MCU goes back to the lowest STANDBY mode. Waking from this mode is pretty much same as a system reset, and that's why reset is used as a natural part of the device's usage cycle in a pure data-ish way.


## Building

1. get source
2. make symlink called "libopencm3" that points to LibOpenCM3 base directory (http://libopencm3.org/)
3. install arm-none-eabi-gcc (One from Ubuntu repo f.ex. works, no CodeSourcery needed)
4. "make"
5. If you have Texane's STLINK tools, you can program badges directly by running "make badge-type-ledcolor.stlink-flash"

## Bonus
### Taranis Receiver

This repo also features source for a SBUS-to-USB adapter, which can be used (with an appropriate RX hardware such as XM+ or D4R-II) to remotely connect Taranis or Futaba (untested) RC controller to a computer as a game controller.

The resulting adapter controller works with atleast LiftOff quadrotor game/simulator. Adapter currently provides all 16 SBUS channels to the computer as axes.

You can program this to your badge by running "make taranis.stlink-flash"

#### Hardware modification
To prepare your badge for SBUS adapter usage, you'll just need to solder some wires from your RX to the badge.
![Wiring diagram](https://raw.githubusercontent.com/disobeyfi/badge-2018/master/web/badge-sbus.jpg)

## Credits

### Product Team
* Heikki Juva - HW design, manufacturing and managering
* Markus Pyykkö - Embedded software, assembly line software
* Valtteri Raila - Idea for the gamepad, all cloud service parts of the puzzle, forcing of people to meet each other

### Thanks
* Authors of excellent (and free) LibOpenCM3 (http://libopencm3.org/)
* NotMyNick - The Father of Disobey
* Guttula - Help in early brainstorming sessions
* Hacklab Helsinki - Providing a electronics lab during development
