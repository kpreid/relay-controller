`relay.ino` is Arduino firmware for a computer interface for up to three latching relays.

## Protocol

The protocol really ought to be replaced, perhaps with a standard (industrial) automation protocol, but I haven't gotten around to researching what is simple enough. For now, it is this:

115200 baud serial over the standard Arduino serial/USB interface.

Each message consists of ASCII characters and is terminated by ";".
The defined messages are:

* `A1;` (or variants using `B`, `C`, or `2`): Set (host to interface) or report (interface to host) relay position.
    On set, the changed or unchanged state is always reported in response.
* `AU;` (or `B`, `C`): Reports relay is disconnected or indicator is faulty (both lines inactive).
* `AF;` (or `B`, `C`): Reports indicator is faulty (both lines active).
* `BOOTING;`: Sent first on firmware startup.
* `READY;`: Sent last on firmware startup. The current states of the relays will have been reported before this message.
* `INCOMPLETE_COMMAND;`: Sent when a `;` is seen without the expected preceding characters.
* `BAD_CHARACTER;`: Sent when a character not part of any command is seen.
* `BAD_DRIVE;`: Internal error.

## Pinout

The firmware is configured so that the six Arduino pins 8-13 are active-high relay coil driver outputs and pins 2-7 are active-low indicator inputs (reporting the physical state of the relay).
(Note that this means that the Arduino bootloader will trigger the third relay unless it is modified to not flash pin 13 on startup.)

This pinout is compatible with a Velleman KA05 / VMA05 I/O Shield for Arduino. 
This would seem to be overkill in the sense that it contains not just driver transistors but non-latching relays,
but it means that the latching relays may be additionally controlled with manual switches: the shield's relays' NC pins go to ground, NO to the manual switches, and COM to the latching relays.
Visible LED indicators mounted next to the switches can be connected in parallel on the inputs.

## Additional construction details

The following mechanical/electrical details are completely independent of the firmware and exist only because I might as well write them down somewhere.

I chose to attach the relays using DE-9 connectors with the following pinout (which happens to be symmetric so that accidental which-side-am-I-looking-at mirror-reversal will only make the control backwards, not connect the wrong things together):

1. NC
2. Indicator contact for position 1 (to Arduino digital input)
3. Indicator common (to Arduino 5V rail)
4. Indicator contact for position 2 (to Arduino digital input)
5. NC
6. Coil for position 1, positive (relay coil voltage supply)
7. Coil for position 1, negative (to coil driver)
8. Coil for position 2, negative (to coil driver)
9. Coil for position 2, positive (relay coil voltage supply)

Given the use of the KA05 shield, it is feasible but not trivial to do all of the wiring with no additional PCBs or breadboard.
For a pure computer interface, the only junction that is needed is the unswitched positive side of the coils all going to supply.
Adding manual switches requires their connections to ground. Adding indicator LEDs requires supply voltage and resistors, and paralleling them with the digital inputs.

If the latching relays are 12V, then the entire system can be powered from 12V.
If they are 24V, then both 24V and 12V supplies are needed due to the KA05 shield's 12V relays.
If a more customized relay driver circuit is used then a single supply will again suffice.

I also included a fuse on the +12V input (immediately after taking it from the Arduino V<sub>in</sub> pin. (The Arduino has a polyfuse which protects only its +5V.) This protects against any wiring fault in the external relay connections.