/**
 * Korg Volca Filter firmware.
 * 
 * Richard Chesterwood 8 June 2017. 
 * 
 * Channel 16 is assumed to be addressing the Volca Sample. These notes are analysed and re-broadcast on channels 1-10 accordingly.
 * 
 * Channel 15 is assumed to be addressing the Volca FM. The velocity data is converted to a form the FM can understand over MIDI.
 * 
 * Channels 11-14 are assumed to be any other device and are rebroadcast. (We don't Midi-Thru channels 1-10 as these would trigger the Volca Sample)
 * 
 * TODO midi implementation chart.
 * TODO sysex is not supported.
 * TODO this only runs on Arduino Mega right now (need Hardware Serial). Why can't we run on pins 0 and 1 on an Uno? Need to port to a small board mountable device.
 * TODO sample is ignoring velocity as well. Same fix as the FM?
 */

const byte VOLCA_SAMPLE_CHANNEL = 16;
const byte VOLCA_FM_CHANNEL = 15;

const byte STATUS_NOTE_OFF = 0x80;
const byte STATUS_NOTE_ON  = 0x90;
const byte STATUS_POLY_PRESSURE = 0xA0;
const byte STATUS_CONTROL_CHANGE = 0xB0;
const byte STATUS_PITCH_BEND = 0xE0;
const byte STATUS_SONG_POSITION = 0xF2;
const byte STATUS_PROGRAM_CHANGE = 0xC0;
const byte STATUS_CHANNEL_PRESSURE = 0xD0;
const byte STATUS_MIDI_TIME_CODE = 0xF1;
const byte STATUS_SONG_SELECT = 0xF3;

bool awaitingVolcaSampleData = false;
bool awaitingDataByte2 = false;
byte lastStatusReceived = 0;
byte dataByte1;
byte dataByte2;

void setup() {

    // Regular serial for debugging
    Serial.begin(115200);

    // Hardware serial connected to Midi Thru section of circuit.
    
    Serial1.begin(31250);

    Serial.println("Korg Volca Firmware v0.1 Ready.");
}

void midiBroadcast(byte status, byte dataByte1)
{  
  Serial1.write(status); 
  Serial1.write(dataByte1);
  byte channel = (status & B00001111) + 1;
  Serial.print("Ch "); Serial.print(channel);
  Serial.print(" Sent "); Serial.print(status);   Serial.print(":"); Serial.print(dataByte1);  
}

void midiBroadcast(byte status, byte dataByte1, byte dataByte2)
{
  // Incoming Channels 1-10 are not going to be sent Thru as they would trigger the Sample. 
  if ((status & B00001111) <= 9) return;    
  
  midiFilter(status, dataByte1, dataByte2);
  midiBroadcast(status, dataByte1);
  Serial1.write(dataByte2);
  Serial.print(":"); Serial.print(dataByte2);
}

void midiFilter(byte &status, byte &dataByte1, byte &dataByte2)
{
  byte channel = (status & B00001111) + 1;
  byte command = (status & B11110000);

  if (channel == VOLCA_SAMPLE_CHANNEL)
  {
    if (command == STATUS_NOTE_ON || command == STATUS_NOTE_OFF)
    {
      channel = dataByte1 % 12; // TODO a proper mapping here maybe?
      Serial.print("as databyte is "); Serial.println(dataByte1); Serial.print(" the channel is going to be "); Serial.println(channel);
      if (channel > 10) return; // "Only" 10 channels on Sample.
      status = command | channel;
    }
  }

  if (channel == VOLCA_FM_CHANNEL)
  {
    if (command == STATUS_NOTE_ON || command == STATUS_NOTE_OFF)
    {
      Serial1.write(0xB0 | (VOLCA_FM_CHANNEL-1)); // CC 
      Serial1.write(41); // Volume
      Serial1.write(dataByte2);
    }    
  }
}

void processStatusByte(byte midiByte)
{  
  if ((midiByte & B10000000) == B10000000)
  {
     lastStatusReceived = midiByte;

   Serial.print("rx "); Serial.println(midiByte);
   byte nChan = (midiByte & B00001111) + 1;
   
   Serial.print("chan "); Serial.println(nChan);
   byte command = (midiByte & B11110000);
   Serial.print("command "); Serial.println(command);

   awaitingDataByte2 = false;
  }
}

void processDataByte(byte midiByte)
{   
   if ((midiByte & B10000000) == B00000000)
   {
      if (!awaitingDataByte2)
      {
        dataByte1 = midiByte;

        Serial.print("note "); Serial.println(dataByte1);

        
      }
      else
      {
        dataByte2 = midiByte;
   Serial.print("second byte "); Serial.println(dataByte2);
        
      }

      // handle the state transition
      switch(lastStatusReceived & B11110000)
      {
        case STATUS_NOTE_ON:
        case STATUS_NOTE_OFF:
        case STATUS_POLY_PRESSURE:
        case STATUS_CONTROL_CHANGE:
        case STATUS_PITCH_BEND:
        case STATUS_SONG_POSITION: 
                   if (!awaitingDataByte2) 
                   {
                     awaitingDataByte2 = true;
                     break;
                   }
                   else
                   {
                     // Done. Broadcast.
                     midiBroadcast(lastStatusReceived, dataByte1, dataByte2);
                     awaitingDataByte2 = false;
                     break;
                   }
        case STATUS_PROGRAM_CHANGE:
        case STATUS_CHANNEL_PRESSURE:
        case STATUS_MIDI_TIME_CODE:
        case STATUS_SONG_SELECT:
                   midiBroadcast(lastStatusReceived, dataByte1);
                   awaitingDataByte2 = false;
                   break;
     }
   }
   Serial.println();
}

void processMidiByte(byte midiByte)
{
   processStatusByte(midiByte);
   processDataByte(midiByte);
}



void loop () {
    if (Serial1.available() > 0) {
        byte midiByte = Serial1.read();
        Serial.print("RAW MIDI IN : "); Serial.println(midiByte);
        processMidiByte(midiByte);       
    }        
}
