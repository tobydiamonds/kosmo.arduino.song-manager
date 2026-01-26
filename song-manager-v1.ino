#include "shared.h"
#include "DebounceButton165.h"
#include "kosmo-comm-master.h"
#include "channel.h"
#include "song-repository.h"
#include "AnalogMuxScanner.h"
#include "integration-tests.h"
#include "serial-song-parser.h"


// input bit mask
#define PROGRAM_BTN 0
#define LOAD_BTN 1
#define NEXT_SONG_BTN 2
#define PREV_SONG_BTN 3

// led bit mask
#define CLOCK_IN_LED 1 // DIG0
#define PROGRAMMER_LED_R 2 // DIG1
#define PROGRAMMER_LED_G 3 // DIG2
#define PROGRAMMER_LED_B 4 // DIG3

// 7-segment digits
#define BLANK 10
#define DASH 11

#define SCAN_INTERVAL 50
#define MUX_SCAN_INTERVAL 10

// CLOCK IN
const byte CLOCK_IN_PIN = 19;

// 74HC165
const byte INPUT_LOAD = 8;  // => 165/1
const byte INPUT_CLK  = 9;  // => 165/2
const byte INPUT_DATA = 10; // => 165/9
const byte INPUT_LATCH= 11; // => 165/15

// 74HC595
const byte LED_CLOCK = 2; // => 595//11
const byte LED_LATCH = 3; // => 595/12
const byte LED_DATA  = 4; // => 595/14

// 74HC4051//analog mux
const byte MUX_S0 = 30; // => 4051/9
const byte MUX_S1 = 32; // => 4051/10
const byte MUX_S2 = 34; // => 4051/11

const byte MUX_M1 = A0; // => 1st 4051/3 (channel 1[1,2,3], 2[1,2,3], 3[1,2])
const byte MUX_M2 = A1; // => 2nd 4051/3 (channel 3[3], 4[1,2,3], 5[1,2,3], 6[1])
const byte MUX_M3 = A2; // => 3rd 4051/3 (channel 6[2,3], 7[1,2,3], 8[1,2,3])

unsigned long now = 0;
unsigned long lastClockPulse = 0;
unsigned long lastInputScan = 0;
unsigned long lastProgrammingLed = 0;
unsigned long lastSongLoadingLed = 0;
unsigned long lastSongLoading = 0;
unsigned long lastSongBlink = 0;
unsigned long lastClockInLed = 0;

Song currentSong;
uint8_t currentSongNumber = 1;
uint8_t selectedSongNumber = 1;
uint8_t currentChannel = 0;
bool songIsLoading = false;
bool songIsPlaying = false;
bool programming = false;
bool programmingLed = false;
bool songLoadingLed = false;
bool blinkSongNumber = false;

bool clockInLed = false;
volatile bool edgeDetected = false;
volatile bool reset = false;
volatile bool hasPulse = false;

DebounceButton165 programBtn(PROGRAM_BTN);
DebounceButton165 loadBtn(LOAD_BTN);
DebounceButton165 nextSongBtn(NEXT_SONG_BTN);
DebounceButton165 prevSongBtn(PREV_SONG_BTN);

AnalogMuxScanner analogPotBank1(MUX_S0, MUX_S1, MUX_S2, A0, A1, A2, CHANNELS);

Channel channels[CHANNELS];

SerialSongParser songParser(currentSong);

void setup() {
  Serial.begin(115200);

  // 74HC165
  pinMode(INPUT_LOAD, OUTPUT);
  pinMode(INPUT_CLK, OUTPUT);
  pinMode(INPUT_LATCH, OUTPUT);
  pinMode(INPUT_DATA, INPUT);

  // 74HC595
  pinMode(LED_CLOCK, OUTPUT);
  pinMode(LED_LATCH, OUTPUT);
  pinMode(LED_DATA, OUTPUT);

  // // 74HC4051/analog mux
  analogPotBank1.onChange(onAnalogPotChangedHandler);
  analogPotBank1.setHysteresis(10);
  analogPotBank1.setSamplesPerRead(5);
  analogPotBank1.begin();

  // channels
  for(int i=0; i<CHANNELS; i++) {
    channels[i] = Channel(i, 7-i); // first parameter: channel nummber, second parameter: bit index of the channel button from 595
    channels[i].OnPartCompleted(onPartCompleted);
    channels[i].OnBeforePartCompleted(onBeforePartCompleted);
    channels[i].OnPartStarted(onPartStarted);
    channels[i].OnPartStopped(onPartStopped);
  }

  // i2c comm
  setupMaster();

  // clock in
  pinMode(CLOCK_IN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CLOCK_IN_PIN), onClockPulse, RISING);  

  // song repository
  SetupSongRepository();

  Serial.println("Song Manager ready!");

  //LoadSongAndUpdateChannels(1);
  songIsLoading = true;


}

void LoadSongAndUpdateChannels(int index) {
  // load song from SD card, send values to channels 
  Serial.print("Loading song: ");
  Serial.println(index);  

  Song prev = currentSong;
  resetSong(currentSong);

  bool success;
  currentSong = LoadSong(index, success);
  if(!success) {
    char s[100];
    sprintf(s, "Error loading song %d", index);
    Serial.println(s);
    currentSong = prev;
    songIsLoading = false; 
    songLoadingLed = false;
    selectedSongNumber = currentSongNumber;
  } else {
    applyCurrentSongToChannels();
    Serial.println("###SONG LOADED###");

    songIsLoading = false; 
    songLoadingLed = false;
    currentSongNumber = index;
    selectedSongNumber = index;

    //PeekSong(index);

  }  
}

void onPartCompleted(uint8_t channelNumber, int8_t chainToChannel) {
  char s[100];
  sprintf(s, "part %d ended - ", channelNumber);
  Serial.println(s);
  channels[channelNumber].Stop();
  if(chainToChannel == -1) {
    stopClock();
    Serial.println("no chain - stopping the clock");
  } else if(chainToChannel < CHANNELS) {
    channels[chainToChannel].Start();
    sprintf(s, "chaining to part %d", chainToChannel);
    Serial.println(s);    
  }
}

void onBeforePartCompleted(uint8_t channelNumber, int8_t chainToChannel) {
  if(chainToChannel >= 0 && chainToChannel < 8)
    setSlaveRegisters(now, currentSong.parts[chainToChannel]); 
}

void onPartStarted(uint8_t channelNumber) {
  // if(currentChannel != channelNumber)
  //   setSlaveRegisters(now, currentSong.parts[channelNumber]);  
  currentChannel = channelNumber;
  songIsPlaying = true;
}

void onPartStopped(uint8_t channelNumber) {
  songIsPlaying = false;
}

void onClockPulse() {
  lastClockPulse = now;  
  edgeDetected = true;
  hasPulse = true;
}

void setCurrentSongDrumSequencerLastStep(int channel, int value) {
  for(int i=0; i<5; i++) {
    currentSong.parts[channel].drumSequencer.channel[channel].lastStep = value;
  }
}

void onAnalogPotChangedHandler(int channel, int pot, uint16_t value) {
  // ###handle bad pots###
  if(channel == 2 && pot == 0) return;
  if(channel == 4 && pot == 1) return;
  if(channel == 6 && pot == 0) return;

  if(pot==0) {
    channels[channel].SetPageCountRaw(value);
    setCurrentSongDrumSequencerLastStep(channel, channels[channel].PageCount()*16);
  } else if(pot==1) {
    channels[channel].SetRepeatsRaw(value);
    currentSong.parts[channel].repeats = channels[channel].Repeats();
  } else if(pot==2) {
    channels[channel].SetChainToRaw(value);
    currentSong.parts[channel].chainTo = channels[channel].ChainTo();    
  }

  // // ###handle bad pots###
  // // channel 2,pot 0 always read max value so we set it to 1 page to make it usefull
  // if(channel == 2 && pot == 0) {
  //   channels[channel].SetPageCount(1);
  //   setCurrentSongDrumSequencerLastStep(channel, 16);
  // }
  // // channel 4,pot 1 always read max value so we set it to 0 repeats to make it usefull
  // if(channel == 4 && pot == 1) {
  //   channels[channel].SetRepeats(0);
  //   currentSong.parts[channel].repeats = 0;   
  // }
  // // channel 6,pot 0 is unstable so we set it to 1 page to make it usefull
  // if(channel == 6 && pot == 0) {
  //   channels[channel].SetPageCount(1);
  //   setCurrentSongDrumSequencerLastStep(channel, 16);
  // }

}

uint8_t read165byte() {
  uint8_t value = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(INPUT_CLK, LOW);      // prepare falling edge
    if (digitalRead(INPUT_DATA)) {
      value |= (1 << i);               // store bit in LSB first
    }
    digitalWrite(INPUT_CLK, HIGH);     // shift register updates here
  }
  return value;
}

void write595byte(uint8_t data, uint8_t bitOrder = LSBFIRST) {
    shiftOut(LED_DATA, LED_CLOCK, bitOrder, data);
}

void scanOperationsBoard() {
  uint8_t incoming = read165byte();
  //printByteln(incoming);
  if(incoming == 0xFF) return;

  programBtn.update(incoming, now);
  loadBtn.update(incoming, now);
  if(!programming) {
    nextSongBtn.update(incoming, now);
    prevSongBtn.update(incoming, now);
  }
}

void scanChannelBoards() {
  uint8_t incoming = 0;
  for(int i=0; i<CHANNELS; i++) {
    if(i % 8 == 0) {// only read 1 165 pr 8 channels
      incoming = read165byte();
      //printByteln(incoming);
      if(incoming == 0xFF) return;
    }

    channels[i].Button()->update(incoming, now);

    if(channels[i].Button()->wasPressed()) {
      Serial.print("Button pressed: ");
      Serial.println(i);
      if(programming) {
        while(!getSlaveRegisters(now));
        currentSong.parts[i].tempo = sharedTempoRegisters;
        currentSong.parts[i].drumSequencer = sharedDrumSequencerRegisters;
        currentSong.parts[i].sampler = sharedSamplerRegisters;
        currentSong.parts[i].repeats = channels[i].Repeats();
        currentSong.parts[i].chainTo = channels[i].ChainTo();
        //channels[i].SetLastStep(getPartLastStep(currentSong.parts[i]));
      } else {
        setSlaveRegisters(now, currentSong.parts[i]);  
        // if(channels[currentChannel].IsStarted()) {
        //   int chainTo = channels[currentChannel].ChainTo();
        //   channels[currentChannel].SetChainTo(i);
        //   if(chainTo != -1 && channels[i].ChainTo() == -1) {
        //     channels[i].SetChainTo(chainTo);
        //   }
        // } else {
        //   startClock();  
        //   channels[i].Start();
        // }
      }
    }
    
  }
}

void scanAnalogInputMux() {
  if(!programming) return;
  analogPotBank1.scan(now);
}

void scanInputs() {
  digitalWrite(INPUT_LOAD, LOW);
  delayMicroseconds(5);
  digitalWrite(INPUT_LOAD, HIGH);
  delayMicroseconds(5);
  digitalWrite(INPUT_CLK, HIGH);
  digitalWrite(INPUT_LATCH, LOW);

  // read data here
  scanOperationsBoard();
  scanChannelBoards();

  digitalWrite(INPUT_LATCH, HIGH);

}


// 2-digit display
const byte digitEnable[2] = {
  ~(B01000000), // QG -> digit1 (MSD)
  ~(B10000000)  // QH -> digit2
};


const byte digitToSegment[14] = {
  // GFEDCBAdp
  B01111110, // 0
  B00001100, // 1
  B10110110, // 2
  B10011110, // 3
  B11001100, // 4
  B11011010, // 5
  B11111010, // 6
  B00001110, // 7
  B11111110, // 8
  B11011110, // 9
  B00000000,  // reset display
  B11100110, // P
  B10100000, // r
  B11111010  // G
};


const byte digitToSegment28[12] = {
  // dpGFEDCBA
  B00111111, // 0
  B00110000, // 1
  B01011011, // 2
  B01111001, // 3
  B01110100, // 4
  B01101101, // 5
  B01101111, // 6
  B00111000, // 7
  B01111111, // 8
  B01111100, // 9
  B00000000,  // reset display
  B01000000, // dash
};

int getDigit(int number, int position) {
  // position = 0 => least significant digit
  for (int i = 0; i < position; i++) {
    number /= 10;
  }
  return number % 10;
}


void updateOperationsBoardDigit(int digit, int songNumber) {

  /*
  * 1st 595:          2nd 595:
  * QA => SEG G       QA => digit 1 enable (active low) 0x01
  * QB => SEG F       QB => digit 2 enable (Active low) 0x02
  * ...               QC => RGB R (active low)          0x08
  * QG => SEG A       QD => RGB G (active low)          0x10
  * QH => dp          QE => RGB B (active low)          0x20
  *                   QF => clock in led                0x04
  */


  uint8_t data[2] = {0};

  int index = (digit % 2 == 0) ? 1 : 0; // we have only 2 digits in the ops board, so whatever digit value comes in, we want to determine which of the 2 digits we are updating

  int digitVal = getDigit(songNumber, index);
  data[0] = (blinkSongNumber) ? 0x00 : digitToSegment[digitVal];
  data[1] = digitEnable[index];

  if(programmingLed)  // red led blinking while loading
    data[1] &= ~0x08;
  else
    data[1] |= 0x08;

  if(songIsLoading) {
    if(songLoadingLed)  // green led blinking while loading
      data[1] &= ~0x10;
    else
      data[1] |= 0x10;
  } else if(!programming) {
    data[1] &= ~0x10;  // green led static when song is loaded
  }

  if(clockInLed)
    data[1] &= ~0x04;
  else
    data[1] |= 0x04;
  

  write595byte(data[1]);
  write595byte(data[0]);

}

void updateChannelDigit(int channel, int digit) {
  /*
  * 1st 595:                                        2nd 595:
  * QA => DIG_0 enable (LEDS)                       QA => SEG_A
  * QB => DIG_1 enable (no. of repeats left digit)  QB => SEG_B
  * QC => DIG_2 enable (no. of repeats right digit) QC => SEG_C
  * QD => DIG_3 enable (chain ch left digit)        QD => SEG_D
  * QE => DIG_4 enable (chain ch right digit)       QE => SEG_E
  * QF => nc                                        QF => SEG_F
  * QG => nc                                        QG => SEG_F
  * QH => nc                                        QH => SEG_DP
  */


  uint8_t digitData = (1 << digit); // enable the digit
  uint8_t segmentData = 0;
  uint8_t pc = 1; // declare here since it is not allowed to declare inside the switch-statement
  int8_t chainTo = channels[channel].ChainTo();

  switch(digit) {
    case 0:
      for(int i=0; i<4; i++) {
        if(channels[channel].PageLedState(i))
          segmentData |= (1 << i);
        else
          segmentData &= ~(1 << i);
      }
      break;
    case 1:
      if(channels[channel].IsStarted())
        segmentData = digitToSegment28[channels[channel].RemainingRepeats() % 10];
      else
        segmentData = digitToSegment28[channels[channel].Repeats() % 10];
      break;
    case 2:
      if(channels[channel].IsStarted())
        segmentData = digitToSegment28[channels[channel].RemainingRepeats() / 10];
      else
        segmentData = digitToSegment28[channels[channel].Repeats() / 10];
      break;      
    case 3:
      segmentData = (chainTo==-1) ? digitToSegment28[11] : digitToSegment28[(chainTo+1) % 10];
      break;
    case 4:
      segmentData = (chainTo==-1) ? digitToSegment28[11] : digitToSegment28[(chainTo+1) / 10];
      break;          
  }
  write595byte(digitData, MSBFIRST);
  write595byte(segmentData, MSBFIRST);
}

void updateChannelProgramming(int channel, int digit) {
  // when programming we want the 4 leds to indicate something... 7-segments should be off
  
  uint8_t firstByte = (1 << digit); // enable the digit
  uint8_t secondByte = digitToSegment28[10]; // turn off 7-segments
  if(digit==0) {


  }
}


const int DIGITS = 5;
void updateUI() {

  for(int digit=0; digit<DIGITS; digit++) {

    digitalWrite(LED_LATCH, LOW);
    updateOperationsBoardDigit(digit, selectedSongNumber);
    for(int channel=0; channel<CHANNELS; channel++) {
      // if(programming)
      //   updateChannelProgramming(channel, digit);
      // else

      // char s[100];
      // sprintf(s, "ch%d: pages=%d  repeats=%d  chain-to=%d", channel, channels[channel].PageCount(), channels[channel].Repeats(), channels[channel].ChainTo());
      // Serial.println(s);

      updateChannelDigit(channel, digit);
    }
    digitalWrite(LED_LATCH, HIGH);
  }
}

uint8_t getPartLastStep(Part part) {
  uint8_t lastStep = 0;
  for(int i=0; i<5; i++) {
    lastStep = max(lastStep, part.drumSequencer.channel[i].lastStep);
  }
  return lastStep;
}

void applyCurrentSongToChannel(int index) {
  uint8_t lastStep = getPartLastStep(currentSong.parts[index]);
  channels[index].SetLastStep(lastStep);
  channels[index].SetChainTo(currentSong.parts[index].chainTo);
  channels[index].SetRepeats(currentSong.parts[index].repeats);  
}

void applyCurrentSongToChannels() {
  for(int i=0; i<CHANNELS; i++) {
    applyCurrentSongToChannel(i);
  }
}


int ppqnCounter = 0;
void triggerClockPulse() {
  ppqnCounter = (ppqnCounter + 1) % 24;
  for(int i=0; i<CHANNELS; i++)
    channels[i].Pulse(ppqnCounter);
  if(ppqnCounter == 0) {
    clockInLed = true;
    lastClockInLed = now;
  }
}

void loop() {
  now = millis();
 
  // handle reset
  if(now > (lastClockPulse + 2000) && hasPulse) {
    reset = true;
    hasPulse = false;
  }

  if(reset) {
    reset = false;
    ppqnCounter = 0;
    songIsPlaying = false;
    for(int i=0; i<CHANNELS; i++) {
      channels[i].Reset();
    }
    currentChannel = 0;
    Serial.println("reset!");
  }

  // handle clock in
  if(edgeDetected) {
    edgeDetected = false;
    if(!songIsPlaying)
      channels[currentChannel].Start();
    triggerClockPulse();
  }

  if (now > (lastInputScan + SCAN_INTERVAL)) {
    lastInputScan = now;
    scanInputs();
  }

  scanAnalogInputMux();

 
// NEXT SONG INDEX
  if(!programming && nextSongBtn.wasPressed() && !prevSongBtn.isDown()) {
    if(selectedSongNumber < MAX_SONGS-2)
      selectedSongNumber++;
    else
      selectedSongNumber=1;

    Serial.print("Selected song: ");
    Serial.println(selectedSongNumber);
  }
// PREV SONG INDEX
  if(!programming && prevSongBtn.wasPressed() && !nextSongBtn.isDown()) {
    if(selectedSongNumber > 1)
      selectedSongNumber--;
    else
      selectedSongNumber = MAX_SONGS;

    Serial.print("Selected song: ");
    Serial.println(selectedSongNumber);    
  }
// LOAD SONG
  if(!programming && loadBtn.wasPressed()) { //&& selectedSongNumber != currentSongNumber) {
    songIsLoading = true;
    songLoadingLed = true;
    lastSongLoading = now;
    LoadSongAndUpdateChannels(selectedSongNumber);
  }
// CANCEL LOAD SONG
  if(songIsLoading && loadBtn.wasPressed()) {
    songIsLoading = false;
    songLoadingLed = true;
    selectedSongNumber = currentSongNumber;
    LoadSongAndUpdateChannels(currentSongNumber);
  }  
// CANCEL PROGRAMMING
  if(programming && loadBtn.wasPressed()) { 
      programming = false;
      programmingLed = false;
      resetTempoRegisters(sharedTempoRegisters);
      resetDrumSequencerRegisters(sharedDrumSequencerRegisters);      
      Serial.println("programming cancelled - song not saved");
  }
  if(!songIsLoading && programBtn.wasPressed()) { 
// START PROGRAMMING
    if(!programming) {                      
      programming = true;
      Serial.println("programming...");
    } else { 
// END PROGRAMMING                          
      SaveSong(currentSong, selectedSongNumber);
      currentChannel = 0;
      ppqnCounter = 0;

      Serial.println("###SONG SAVED###");

      //PeekSong(selectedSongNumber);

      programming = false;
      programmingLed = false;
      resetTempoRegisters(sharedTempoRegisters);
      resetDrumSequencerRegisters(sharedDrumSequencerRegisters);      
      Serial.println("programming end");
    }
  }  

  if(currentSongNumber != selectedSongNumber && now > (lastSongBlink + LED_SHORT_PULSE)) {
    blinkSongNumber = !blinkSongNumber;
    lastSongBlink = now;
  }

  if(currentSongNumber == selectedSongNumber)
    blinkSongNumber = false;

  if(programming && now > (lastProgrammingLed + LED_SHORT_PULSE)) {
    programmingLed = !programmingLed;
    lastProgrammingLed = now;
  }

  if(songIsLoading && now > (lastSongLoadingLed + LED_SHORT_PULSE)) {
    songLoadingLed = !songLoadingLed;
    lastSongLoadingLed = now;
  }

  if(now >(lastClockInLed + LED_SHORT_PULSE)) {
    clockInLed = false;
  }



  for(int i=0; i<CHANNELS; i++)
    channels[i].Run(now);

  updateUI();  

  if(Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if(command=="init") {
      resetSong(currentSong);
      applyCurrentSongToChannels();
      Serial.println("Song initialized");
    } else if (command=="apply") {
      applyCurrentSongToChannels();
      int index = firstSongPart(currentSong);
      if(index >= 0 && index < CHANNELS) {
        setSlaveRegisters(now, currentSong.parts[index]); 
      }
      Serial.print("Song loaded from serial and modules loaded from part: ");
      Serial.println(index);
    } else {
      SlaveEnum target;
      int index = songParser.parseCommand(command, target);    
      if(programming && index >= 0 && index < CHANNELS) {
        applyCurrentSongToChannel(index);
      }    
      if(programming && index >= 0 && index < CHANNELS && target >= 0 && target < 100) {
        setSlaveRegisters(now, currentSong.parts[index], target); 
      }      
    }
    // if(programming) {
    //   String command = Serial.readStringUntil('\n');
    //   SlaveEnum target;
    //   int index = songParser.parseCommand(command, target);    
    //   if(index >= 0 && index < CHANNELS) {
    //     applyCurrentSongToChannel(index);
    //     setSlaveRegisters(now, currentSong.parts[index], target); 
    //   }
    // } else {

    //   int test;
    //   int channel;
    //   if(getSerialData(test, channel)) {
    //     runIntegrationTest(test, channel, currentSong);
    //   }
    // }
  } 
}

