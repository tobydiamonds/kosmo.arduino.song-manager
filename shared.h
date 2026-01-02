#ifndef Shared_h
#define Shared_h

#define MAX_SONGS 99

#define PPQN 24.0
#define PPQN_PR_BEAT 6
#define LED_SHORT_PULSE 300
#define LED_VERY_SHORT_PULSE 25

#define CHANNELS 8

void printByte(uint8_t b, uint8_t bitOrder = LSBFIRST) {
  if(bitOrder == LSBFIRST) {
    for(int j=7; j>=0; j--) {
      if((b >> j) & 1)
        Serial.print("1");
      else
        Serial.print("0");
    }    
  } else {
    for(int j=0; j<8; j++) {
      if((b >> j) & 1)
        Serial.print("1");
      else
        Serial.print("0");
    }        
  }
  Serial.print(" ");
}

void printByteln(uint8_t b, uint8_t bitOrder = LSBFIRST) {
  printByte(b, bitOrder);
  Serial.println();
}

void printInt(uint16_t value) {
  for(int i=0; i<16; i++) {
    if((value >> i) & 1) Serial.print("1");
    else Serial.print("0");
  }
  Serial.print(" ");
}

void printIntln(uint16_t value) {
  printInt(value);
  Serial.println();
}

uint8_t byteToBitMask(uint8_t b, uint8_t bitOrder = LSBFIRST) {
  uint8_t mask = 0;
  if(bitOrder == LSBFIRST) {
    for(int i=0; i<8; i++) {
      uint8_t bit = (1 << i);
      if((b & bit) == bit)
        mask |= bit;
    }
  } else {
    for(int i = 0; i < 8; i++) {
      uint8_t bit = (1 << (7 - i));
      if((b & bit) == bit) { // Change the bit position
        mask |= bit;
      }
    }    
  }
  return mask;
}



struct KosmoSlave {
  int address;
  bool inProgrammingMode;
  bool requestInProgress;
  unsigned long lastGetRequest;
  int retries;
  size_t registerSize;
};

struct TempoRegisters {
  uint8_t bpm = 120; // Default value for bpm
  uint8_t morphTargetBpm = 100; // Default value for morphTargetBpm
  uint8_t morphBars = 4; // Default value for morphBars
  bool morphEnabled = false; // Default value for morphEnabled
};

struct DrumSequencerChannel {
  uint16_t page1;
  uint16_t page2;
  uint16_t page3;
  uint16_t page4;
  int divider;
  int lastStep;
  bool enabled;
};

struct DrumSequencerRegisters {
    bool chainModeEnabled = false; // Default value for chainModeEnabled

    uint16_t ch1_page1 = 0;
    uint16_t ch1_page2 = 0;
    uint16_t ch1_page3 = 0;
    uint16_t ch1_page4 = 0;  
    int ch1_divider = 6; // 16th notes
    int ch1_lastStep = 15; 
    bool ch1_enabled = false;  

    uint16_t ch2_page1 = 0;
    uint16_t ch2_page2 = 0;
    uint16_t ch2_page3 = 0;
    uint16_t ch2_page4 = 0;  
    int ch2_divider = 6; 
    int ch2_lastStep = 15; 
    bool ch2_enabled = false;   

    uint16_t ch3_page1 = 0;
    uint16_t ch3_page2 = 0;
    uint16_t ch3_page3 = 0;
    uint16_t ch3_page4 = 0;  
    int ch3_divider = 6;
    int ch3_lastStep = 15;
    bool ch3_enabled = false;  

    uint16_t ch4_page1 = 0;
    uint16_t ch4_page2 = 0;
    uint16_t ch4_page3 = 0;
    uint16_t ch4_page4 = 0;  
    int ch4_divider = 6;
    int ch4_lastStep = 15;
    bool ch4_enabled = false; 

    uint16_t ch5_page1 = 0;
    uint16_t ch5_page2 = 0;
    uint16_t ch5_page3 = 0;
    uint16_t ch5_page4 = 0;  
    int ch5_divider = 6;
    int ch5_lastStep = 15;
    bool ch5_enabled = false; 
};

struct SamplerRegisters {
  uint8_t bank = 0;
  uint16_t mix[5] = {0};
};

struct Part {
  uint8_t repeats = 0;
  int8_t chainTo = -1;
  TempoRegisters tempo;
  DrumSequencerRegisters drumSequencer;
  SamplerRegisters sampler;
};

struct Song {
  Part parts[CHANNELS];

  Song() {
    for(int i=0; i<CHANNELS; i++) {
      parts[i] = Part();
    }
  }
};


TempoRegisters sharedTempoRegisters;
DrumSequencerRegisters sharedDrumSequencerRegisters;
SamplerRegisters sharedSamplerRegisters;

void resetSamplerRegisters(SamplerRegisters &regs) {
  regs.bank = 0;
  for(int i=0; i<5; i++)
    regs.mix[i] = 0;
}

void resetTempoRegisters(TempoRegisters &regs) {
  regs.bpm = 0;
  regs.morphTargetBpm = 0;
  regs.morphBars = 0;
  regs.morphEnabled = false;
}
void resetDrumSequencerRegisters(DrumSequencerRegisters &regs) {
    regs.chainModeEnabled = false;

    regs.ch1_page1 = 0; regs.ch1_page2 = 0; regs.ch1_page3 = 0; regs.ch1_page4 = 0;  
    regs.ch1_divider = 0; regs.ch1_lastStep = 0; regs.ch1_enabled = false;  

    regs.ch2_page1 = 0; regs.ch2_page2 = 0; regs.ch2_page3 = 0; regs.ch2_page4 = 0;  
    regs.ch2_divider = 0; regs.ch2_lastStep = 0; regs.ch2_enabled = false;    

    regs.ch3_page1 = 0; regs.ch3_page2 = 0; regs.ch3_page3 = 0; regs.ch3_page4 = 0;  
    regs.ch3_divider = 0; regs.ch3_lastStep = 0; regs.ch3_enabled = false;    

    regs.ch4_page1 = 0; regs.ch4_page2 = 0; regs.ch4_page3 = 0; regs.ch4_page4 = 0;  
    regs.ch4_divider = 0; regs.ch4_lastStep = 0; regs.ch4_enabled = false;    

    regs.ch5_page1 = 0; regs.ch5_page2 = 0; regs.ch5_page3 = 0; regs.ch5_page4 = 0;  
    regs.ch5_divider = 0; regs.ch5_lastStep = 0; regs.ch5_enabled = false;          
}

void resetPart(Part &part) {
    part.repeats = 0;
    part.chainTo = -1; 
    
    resetTempoRegisters(part.tempo);
    resetDrumSequencerRegisters(part.drumSequencer);
    resetSamplerRegisters(part.sampler);
}

void resetSong(Song &song) {
    for (uint8_t i = 0; i < CHANNELS; i++) {
        resetPart(song.parts[i]);
    }
}

void printSamplerRegisters(SamplerRegisters reg) {
  char s[100];
  sprintf(s, "sampler => bank: %d", reg.bank);
  Serial.println(s);
  for(int i=0; i<5; i++) {
    sprintf(s, "ch%d => mix: %d", i, reg.mix[i]);
    Serial.println(s);
  }  
}

void printTempoRegisters(TempoRegisters reg) {
  char s[100];
  sprintf(s, "tempo => bpm: %d | target bpm: %d | morph bars: %d | morph enabled: ", reg.bpm, reg.morphTargetBpm, reg.morphBars);
  Serial.print(s);
  Serial.println(reg.morphEnabled);
}

void printDrumSequencerRegisters(DrumSequencerRegisters reg) {
  char s[100];
  sprintf(s, "ch1 => laststep: %d | divider: %d | output enabled: ", reg.ch1_lastStep, reg.ch1_divider);
  Serial.print(s);
  Serial.println(reg.ch1_enabled);
  Serial.print("steps: ");
  printInt(reg.ch1_page1);
  printInt(reg.ch1_page2);
  printInt(reg.ch1_page3);
  printIntln(reg.ch1_page4);      

  sprintf(s, "ch2 => laststep: %d | divider: %d | output enabled: ", reg.ch2_lastStep, reg.ch2_divider);
  Serial.print(s);
  Serial.println(reg.ch2_enabled);
  Serial.print("steps: ");
  printInt(reg.ch2_page1);
  printInt(reg.ch2_page2);
  printInt(reg.ch2_page3);
  printIntln(reg.ch2_page4);

  sprintf(s, "ch3 => laststep: %d | divider: %d | output enabled: ", reg.ch3_lastStep, reg.ch3_divider);
  Serial.print(s);
  Serial.println(reg.ch3_enabled);
  Serial.print("steps: ");
  printInt(reg.ch3_page1);
  printInt(reg.ch3_page2);
  printInt(reg.ch3_page3);
  printIntln(reg.ch3_page4);    

  sprintf(s, "ch4 => laststep: %d | divider: %d | output enabled: ", reg.ch4_lastStep, reg.ch4_divider);
  Serial.print(s);
  Serial.println(reg.ch4_enabled);
  Serial.print("steps: ");
  printInt(reg.ch4_page1);
  printInt(reg.ch4_page2);
  printInt(reg.ch4_page3);
  printIntln(reg.ch4_page4);    

  sprintf(s, "ch5 => laststep: %d | divider: %d | output enabled: ", reg.ch5_lastStep, reg.ch5_divider);
  Serial.print(s);
  Serial.println(reg.ch5_enabled);
  Serial.print("steps: ");
  printInt(reg.ch5_page1);
  printInt(reg.ch5_page2);
  printInt(reg.ch5_page3);
  printIntln(reg.ch5_page4);              
}

void printSongPart(Part part, int index) {
  char s[100];
  sprintf(s, "part %d => repeats: %d | chainTo: %d", index, part.repeats, part.chainTo);
  Serial.println(s);
  printTempoRegisters(part.tempo);
  printDrumSequencerRegisters(part.drumSequencer);
  printSamplerRegisters(part.sampler);
}

void printSong(Song song) {
  Serial.println("SONG:");

  for(int i=0; i<CHANNELS; i++) {
    printSongPart(song.parts[i], i);
  }
}


#endif