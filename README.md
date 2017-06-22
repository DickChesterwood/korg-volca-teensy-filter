# korg-volca-teensy-filter

WARNING - this is a work in progress - don't assume any of this works. I'll be manufacturing the board for this in mid July.

22 June 2017 - added a Midi Panic section.

This is a hardware schematic a MIDI merge and thru box, together with a software filter to drive Korg Volcas. This fixes the channels problem for Volca Sample and allows Volca FM to receive volume data (and the software can of course be updated over time as new problems emerge).

# Schematic

I'm heavily indebted to the [blog from Morocco Dave](https://moroccodave.com/2017/02/06/diy-midi-thru-box/) where he describes how to build a DIY through box. With my very basic electronics knowledge, I've expanded that design to route the signal through a Teensy LC, where the MIDI filtering can take place. At the same time, I'm taking advantage of the 3 hardware serial lines to do a MIDI merge through software.

Note the LC's pins run at 3.3v, hence the Hex Inverter has been changed for a 74HCT14 which can run at logic level. The caps on the inputs and output are as specified in the datasheet; I've also modified the caps on the 7805 according to the datasheet - I'm not sure about this, but again it seems to run stably.

This schematic is due to be sent off for manufacture by the end of June so hopefully I'll know if it works by end of July.

![Schematic](https://github.com/DickChesterwood/korg-volca-teensy-filter/blob/master/Schematic.png?raw=true)
