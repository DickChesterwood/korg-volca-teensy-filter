# korg-volca-teensy-filter
Sketch for a MIDI filter to drive Korg Volcas. Fixes the channels problem for Volca Sample and allows Volca FM to receive volume data.

The plan is to package this into a nice hardware box, implementing 3 MIDI ins and 5 MIDI outs. Will be using a Teensy LC to host this firmware, using the 3 hardware serial lines.


# Schematic

I'm heavily indebted to the [blog from Morocco Dave](https://moroccodave.com/2017/02/06/diy-midi-thru-box/) where he describes how to build a DIY through box. With my very basic electronics knowledge, I've expanded that design to route the signal through a Teensy LC, where the MIDI filtering can take place. At the same time, I'm taking advantage of the 3 hardware serial lines to do a MIDI merge through software.

Note the LC's pins run at 3.3v, hence my clumsy bolt on of a second Voltage Regulator. I'm sure there must be better ways to do this but it seems to work for me. The caps on the inputs and output are as specified in the datasheet; I've also modified the caps on the 7805 according to the datasheet - I'm not sure about this, but again it seems to run stably.

TODO My next job is to transition this from breadboard to veroboard, where it probably all overheat and blow up.

![Schematic](https://github.com/DickChesterwood/korg-volca-teensy-filter/blob/master/Schematic.png?raw=true)
