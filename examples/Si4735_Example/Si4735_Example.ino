/*
* Si4735 Example Sketch
* Written by Ryan Owens for SparkFun Electronics
* Updated for the current state of the library by Radu - Eosif Mihailescu
*
* This example sketch illustrates how to use some of the basic commands in the
* Si4735 Library. The sketch will start the Si4735 in FM mode and then wait
* for user commands via the serial port.
* More information on the Si4735 chip can be found in the datasheet.
*
* HARDWARE SETUP:
* This sketch assumes you are using the Si4735 Shield from SparkFun
* Electronics. 
* The shield should be plugged into an Arduino Main Board (Uno, Mega etc.).
* You will need an audio amplifier as the Si4735 is too weak to directly drive
* a pair of headphones. Immediate candidates would be a pair of active
* (multimedia) speakers that you're probably already using with your computer.
* You will also need a proper antenna connected to the Shield. Luckily
* for you, this is a very forgiving chip when it comes to "proper" antennas,
* so for FM only you will be able to get away with just a 2ft (60cm) length of
* wire connected to the FM antenna pad on the shield. Decent results can
* probably be obtained using a 6" breadboard jumper wire too.
* For SW/AM/LW, you will also need a ferrite bar (loopstick) antenna (aerial)
* connected to the AM antenna pads on the Shield. You can get that from any
* old AM/LW radio or from your local electronics shop. For example, Maplin
* sells such an antenna as P/N LB12N. If you scavenge an old AM/LW radio, the
* ferrite stick in it may have up to 4 windings, grouped on two bobbins. Find
* the bobbin that seems to host the lesser turn count coil (one should look
* like single stranded and the other like multiple strands, the difference
* should be obvious to the naked eye). If there's just one pair of wires
* coming from that winding, you're done. If there's two, use the larger one
* (if visible, the turns ratio should be something like 7:1 so, again, obvious
* to the naked eye; if not, you can use an ohmmeter to spot the longer coil
* since it will have marginally higher resistance).
*
* USING THE SKETCH:
* Once you've plugged the Si4735 Shield into your Arduino board (and
* antenna(s), as appropriate), connect the Arduino to your computer
* and select the corresponding board and COM port from the Tools menu and
* upload the sketch. After the sketch has been updated, open the serial
* terminal using a 9600 baud speed. The sketch accepts single character
* commands (just enter the character and press 'send'). Here is a list of the
* acceptable commands:
*   v/V     - decrease/increase the volume
*   s/S     - seek down/up with band wrap-around
*   m/M     - mute/unmute audio output
*   f       - display currently tuned frequency and mode
*   L/A/W/F - switch mode to LW/AM/SW/FM
*   q       - display signal quality metrics
*   t       - display decoded status byte
*   r       - display chip and firmware revision
*   R       - display RDS data, if available
*   T       - display RDS time, if available
*   ?       - display this list
*
*/

//Due to a bug in Arduino, these need to be included here too/first
#include <SPI.h>
#include <Wire.h>

//Add the Si4735 Library to the sketch.
#include <Si4735.h>

//Create an instance of the Si4735 named radio.
Si4735 radio;
Si4735RDSDecoder decoder;
char command;
byte mode, status;
word frequency, rdsblock[4];
Si4735_RX_Metrics RSQ;
Si4735_RDS_Data station;
Si4735_RDS_Time rdstime;
char FW[3], REV;

void setup()
{
  //Create a serial connection
  Serial.begin(9600);

  //Initialize the radio to the FM mode. (see SI4735_MODE_* constants).
  //The mode will set the proper receiver bandwidth. Ensure that the antenna
  //switch on the shield is configured for the desired mode.
  radio.begin(SI4735_MODE_FM);
}

void loop()
{
  //Attempt to update RDS information if any surfaced
  if(!(millis() % 250)) {
    radio.sendCommand(SI4735_CMD_GET_INT_STATUS);
    if(radio.readRDSBlock(rdsblock)) decoder.decodeRDSBlock(rdsblock);
  }
  
  //Wait until a character comes in on the Serial port.
  if(Serial.available() > 0){
    //Decide what to do based on the character received.
    command = Serial.read();
    switch(command){
      case 'v': 
        if(radio.volumeDown()) Serial.println("Volume decreased");
        else Serial.println("ERROR: already at minimum volume");
        break;
      case 'V':
        if(radio.volumeUp()) Serial.println("Volume increased");
        else Serial.println("ERROR: already at maximum volume");
        break;
      case 's': 
        radio.seekDown();
        Serial.println("Seeking down with band wrap-around");
        break;
      case 'S': 
        radio.seekUp();
        Serial.println("Seeking up with band wrap-around");
        break;
      case 'm': 
        radio.mute();
        Serial.println("Audio muted");
        break;
      case 'M': 
        radio.unMute();
        Serial.println("Audio unmuted");
        break;
      case 'f': 
        frequency = radio.getFrequency();
        mode = radio.getMode();
        Serial.print("Currently tuned to ");
        switch(mode) {
          case SI4735_MODE_FM: 
            Serial.print(frequency / 100);
            Serial.print(".");
            Serial.print(frequency % 100);
            Serial.println("MHz FM");
            break;
          case SI4735_MODE_SW: 
            Serial.print(frequency / 1000);
            Serial.print(".");
            Serial.print(frequency % 1000);
            Serial.println("MHz SW");
            break;
          case SI4735_MODE_AM:
          case SI4735_MODE_LW: 
            Serial.print(frequency);
            Serial.print("kHz ");
            Serial.println((mode == SI4735_MODE_AM) ? "AM" : "LW");
            break;
        }
        break;
      case 'L':
      case 'A':
      case 'W':
      case 'F': 
        Serial.print("Switching mode to ");
        switch(command) {
          case 'L': 
            Serial.println("LW");
            radio.setMode(SI4735_MODE_LW);
            break;
          case 'A': 
            Serial.println("AM");
            radio.setMode(SI4735_MODE_AM);
            break;
          case 'W': 
            Serial.println("SW");
            radio.setMode(SI4735_MODE_SW);
            break;
          case 'F': 
            Serial.println("FM");
            radio.setMode(SI4735_MODE_FM);
            break;
        }
        break;
      case 'q': 
        radio.getRSQ(&RSQ);
        Serial.println("Signal quality metrics {");
        Serial.print("RSSI = ");
        Serial.print(RSQ.RSSI);
        Serial.println("dBuV");
        Serial.print("SNR = ");
        Serial.print(RSQ.SNR);
        Serial.println("dB");
        if(radio.getMode() == SI4735_MODE_FM) {
          Serial.println((RSQ.PILOT ? "Stereo" : "Mono"));
          Serial.print("f Offset = ");
          Serial.print(RSQ.FREQOFF);
          Serial.println("kHz");
        }
        Serial.println("}");
        break;
      case 't': 
        status = radio.getStatus();
        Serial.println("Status byte {");
        if(status & SI4735_STATUS_CTS) Serial.println("* Clear To Send");
        if(status & SI4735_STATUS_ERR) 
          Serial.println("* Error on last command");
        if(status & SI4735_STATUS_RSQINT)
          Serial.println("* Received Signal Quality interrupt");
        if(status & SI4735_STATUS_RDSINT)
          Serial.println("* RDS/RDBS interrupt");            
        if(status & SI4735_STATUS_STCINT)
          Serial.println("* Seek/Tune Complete interrupt");
        Serial.println("}");
        break;
      case 'r': 
        radio.getRevision(FW, NULL, &REV);
        Serial.print("This is a Si4735-");
        Serial.print(REV);
        Serial.println(FW);
        break;
      case 'R': 
        if(radio.isRDSCapable()) {
          decoder.getRDSData(&station);
          Serial.println("RDS information {");
          Serial.print("PI: ");
          Serial.println(station.programIdentifier, HEX);
          Serial.print("PTY: ");
          Serial.println(station.PTY);
          Serial.println("DI {");
          if(station.DICC & SI4735_RDS_DI_DYNAMIC_PTY)
            Serial.println("* Dynamic PTY");
          if(station.DICC & SI4735_RDS_DI_COMPRESSED)
            Serial.println("* Audio Compression");
          if(station.DICC & SI4735_RDS_DI_ARTIFICIAL_HEAD)
            Serial.println("* Artificial Head Recording");
          if(station.DICC & SI4735_RDS_DI_STEREO)
            Serial.println("* Stereo Encoding");
          Serial.println("}");
          if(station.TP) Serial.println("Traffic programme carried");
          if(station.TA) Serial.println("Traffic announcement underway");
          Serial.print("Currently broadcasting ");
          if(station.MS) Serial.println("music");
          else Serial.println("speech");
          Serial.print("PS: ");
          Serial.println(station.programService);
          Serial.print("PTYN: ");
          Serial.println(station.programTypeName);
          Serial.print("RT: ");
          Serial.println(station.radioText);
          Serial.println("}");
        } else Serial.println("RDS not available.");
        break;
      case 'T': 
        if(decoder.getRDSTime(&rdstime)) {
          Serial.println("RDS CT (group 4A) information:");
          Serial.print(rdstime.tm_hour);
          Serial.print(":");
          Serial.print(rdstime.tm_min);
          Serial.print("UTC ");
          Serial.print(rdstime.tm_year);
          Serial.print("-");
          Serial.print(rdstime.tm_mon);
          Serial.print("-");
          Serial.println(rdstime.tm_mday);
        } else Serial.println("RDS CT not available.");
        break;
      case '?': 
        Serial.println("Available commands:");
        Serial.println("* v/V     - decrease/increase the volume");
        Serial.println("* s/S     - seek down/up with band wrap-around");
        Serial.println("* m/M     - mute/unmute audio output");
        Serial.println("* f       - display currently tuned frequency and mode");
        Serial.println("* L/A/W/F - switch mode to LW/AM/SW/FM");
        Serial.println("* q       - display signal quality metrics");
        Serial.println("* t       - display decoded status byte");
        Serial.println("* r       - display chip and firmware revision");
        Serial.println("* R       - display RDS data, if available");
        Serial.println("* T       - display RDS time, if available");
        Serial.println("* ?       - display this list");
        break;
    }
  }   
}
