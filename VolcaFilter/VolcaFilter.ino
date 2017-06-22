
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
 * TODO sysex is not supported yet
 * TODO need to filter out song start messages as the volcas keep playing the sequencers when I run the keyboard's Arp
 * TODO sample is ignoring velocity as well. Same fix as the FM?
 * TODO consider chromiatic playing for the sample
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

const byte NUMBER_OF_INPUT_CHANNELS = 3;

// Teensy LC pin. Just used to flash on startup.
const byte ledPin = 13;

const byte panicButtonPin = 11;     // the number of the pushbutton pin

bool awaitingDataByte2[] = {false, false, false};
byte lastStatusReceived[] = {0,0,0};
byte dataByte1[NUMBER_OF_INPUT_CHANNELS];
byte dataByte2[NUMBER_OF_INPUT_CHANNELS];
bool inPanicMode = false;

void setup() {

    // Regular serial for debugging
    Serial.begin(115200);

    // Hardware serial connected to Midi Thru section of circuit.
    Serial1.begin(31250);      
    Serial2.begin(31250);      
    Serial3.begin(31250);      

    Serial.println("Korg Volca Firmware v0.2 (Multi In) Ready.");

    pinMode(ledPin, OUTPUT);
    pinMode(panicButtonPin, INPUT);

    for (int i=0; i<1000; i+=100)
    {
      digitalWrite(ledPin, HIGH);
      delay(50);
      digitalWrite(ledPin, LOW);
      delay(50);
    }

}

// Nb - in this hardware, all broadcasts are done through Serial1 as Midi Thru is
// Handled in hardware.
void midiBroadcast(byte status, byte dataByte1)
{  
  Serial1.write(status); 
  Serial1.write(dataByte1);
}

void midiBroadcast(byte status, byte dataByte1, byte dataByte2)
{
  // Incoming Channels 1-10 are not going to be sent Thru as they would trigger the Sample. 
  if ((status & B00001111) <= 9) return;    
  
  midiFilter(status, dataByte1, dataByte2);
  midiBroadcast(status, dataByte1);
  Serial1.write(dataByte2);
}

void midiFilter(byte &status, byte &dataByte1, byte &dataByte2)
{
  byte channel = (status & B00001111) + 1;
  byte command = (status & B11110000);

  if (channel == VOLCA_SAMPLE_CHANNEL)
  {
    if (command == STATUS_NOTE_ON || command == STATUS_NOTE_OFF)
    {
      channel = dataByte1 % 12;
      if (channel >= 10) return; // "Only" 10 channels on Sample.
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

void processStatusByte(byte midiByte, byte midiInputChannel)
{  
  if ((midiByte & B10000000) == B10000000)
  {
     lastStatusReceived[midiInputChannel] = midiByte;
     awaitingDataByte2[midiInputChannel] = false;
  }
}

void processDataByte(byte midiByte, byte midiInputChannel)
{   
   if ((midiByte & B10000000) == B00000000)
   {
      if (!awaitingDataByte2[midiInputChannel])
      {
        dataByte1[midiInputChannel] = midiByte;
      }
      else
      {
        dataByte2[midiInputChannel] = midiByte;
      }

      // handle the state transition
      switch(lastStatusReceived[midiInputChannel] & B11110000)
      {
        case STATUS_NOTE_ON:
        case STATUS_NOTE_OFF:
        case STATUS_POLY_PRESSURE:
        case STATUS_CONTROL_CHANGE:
        case STATUS_PITCH_BEND:
        case STATUS_SONG_POSITION: 
                   if (!awaitingDataByte2[midiInputChannel]) 
                   {
                     awaitingDataByte2[midiInputChannel] = true;
                     break;
                   }
                   else
                   {
                     // Done. Broadcast.
                     midiBroadcast(lastStatusReceived[midiInputChannel], dataByte1[midiInputChannel], dataByte2[midiInputChannel]);
                     awaitingDataByte2[midiInputChannel] = false;
                     break;
                   }
        case STATUS_PROGRAM_CHANGE:
        case STATUS_CHANNEL_PRESSURE:
        case STATUS_MIDI_TIME_CODE:
        case STATUS_SONG_SELECT:
                   midiBroadcast(lastStatusReceived[midiInputChannel], dataByte1[midiInputChannel]);
                   awaitingDataByte2[midiInputChannel] = false;
                   break;
     }
   }
}

void processMidiByte(byte midiByte, byte midiInputChannel)
{
   processStatusByte(midiByte,midiInputChannel);
   processDataByte(midiByte,midiInputChannel);
}

// Basic implementation just for testing before fabrication.
// We think the volcas respond to CC 123. It may be necessary to 
// TODO change this to a full set of note OFFs on all channels if working with other hardware.      
void midiPanic()
{
  for (int channel = 0; channel < 16; channel ++)
  {
    byte status = 176 | channel;
    Serial1.write(status);
    Serial1.write(123);
    Serial1.write(0);    
  }
      
  delay(1000);
}

void loop () {

       int panicButtonState = digitalRead(panicButtonPin);

       if ((panicButtonState == HIGH) && !inPanicMode)
       {
         inPanicMode = true;
         digitalWrite(ledPin, HIGH);

         // Don't panic!!!!!!!!
         // Oh, actually - do panic
         Serial.println("PANIC!!!!!!!");
         midiPanic();
       }
       else if ((panicButtonState == LOW) && inPanicMode)
       {
          // All right, calm down.
          digitalWrite(ledPin, LOW);          
          inPanicMode = false;
          Serial.println("PANIC over. Carry on playing.");
       }


       if (!inPanicMode)
       {
          if (Serial1.available() > 0) {
              Serial.println("Received on input channel 0");
              byte midiByte = Serial1.read();
              processMidiByte(midiByte, 0);       
          }              

          if (Serial2.available() > 0) {
              Serial.println("Received on input channel 1");
              byte midiByte = Serial2.read();
              processMidiByte(midiByte, 1);       
          }              

          if (Serial3.available() > 0) {
              Serial.println("Received on input channel 2");
              byte midiByte = Serial2.read();
              processMidiByte(midiByte, 2);       
          }              
      }
}
