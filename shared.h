#ifndef Shared_h
#define Shared_h

#define MAX_SONGS 99

#define PPQN 24.0
#define PPQN_PR_BEAT 6
#define LED_SHORT_PULSE 300
#define LED_VERY_SHORT_PULSE 25

#define CHANNELS 8


enum SlaveEnum {
  TEMPO = 0,
  DRUM_SEQUENCER = 1,
  SAMPLER = 2,
  PROGRAMMER = 100,
  ALL = -1,
  NONE = -2
};

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

String* splitString(String data, char delimiter, int &size) {
    int count = 0;
    for (int i = 0; i < data.length(); i++) {
        if (data.charAt(i) == delimiter) {
            count++;
        }
    }

    size = count + 1; // Number of substrings
    String* result = new String[size];

    int startIndex = 0;
    int resultIndex = 0;
    for (int i = 0; i <= data.length(); i++) {
        if (i == data.length() || data.charAt(i) == delimiter) {
            result[resultIndex] = data.substring(startIndex, i);
            startIndex = i + 1;
            resultIndex++;
        }
    }

    return result;
}

//String allowedChars = "0123456789";
bool isIntValue(String s) {
    if (s.length() == 0) return false;
    int start = 0;
    
    // Check for a leading minus sign
    if (s.charAt(0) == '-') {
        start = 1;
        if (s.length() == 1) return false; // Just a minus sign is not valid
    }

    for (int i = start; i < s.length(); i++) {
        if (!isDigit(s.charAt(i))) {
            return false;
        }
    }
    return true;
}

bool tryGetInt(String data, int offset, int end, int& value) {
  value = 0;
  if(data.length()==0) return false;
  if(offset < 0) return false;
  if(offset > end) return false;
  if(end > data.length()) return false;

  String v = data.substring(offset, end);
  v.trim();
  if(isIntValue(v)) {
    value = v.toInt();
    return true;
  }
  return false;
}


bool tryGetInt(String data, int& value) {
    return tryGetInt(data, 0, data.length(), value);
}

bool tryParseInt(String data, uint16_t& value) {
    data.trim();
    
    // Check for binary format
    if (data.startsWith("0b") || data.length() == 16) {
        value = 0;
        for (int i = 0; i < data.length(); i++) {
            char c = data.charAt(i);
            if (c == '0' || c == '1') {
                value = (value << 1) | (c - '0');
            } else if (c != 'b') {
                return false; // Invalid character for binary
            }
        }
        return true;
    }
    
    // Check for hexadecimal format
    if (data.startsWith("0x")) {
        value = strtol(data.c_str(), nullptr, 16);
        return true;
    }

    return false; // Unsupported format
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

struct SamplerRegisters {
  uint8_t bank = 0;
  uint16_t mix[5] = {0};
};

struct DrumSequencerChannel {
  uint16_t page[4] = {0};
  int divider = 6;
  int lastStep = 0;
  bool enabled = false;  
};

struct DrumSequencer {
  DrumSequencerChannel channel[5];
  bool chainModeEnabled;  
};

struct Part {
  uint8_t repeats = 0;
  int8_t chainTo = -1;
  TempoRegisters tempo;
  DrumSequencer drumSequencer;
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
DrumSequencer sharedDrumSequencerRegisters;
SamplerRegisters sharedSamplerRegisters;

int firstSongPart(Song song) {
  int index = -1;
  for(int i=0; i<CHANNELS; i++) {
    if(index == -1) {
      for(int j=0; j<5; j++) {
        if(song.parts[i].drumSequencer.channel[j].lastStep > 0) {
          index = i;
        }
      }
    }
  }
}

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
void resetDrumSequencerRegisters(DrumSequencer &regs) {
    regs.chainModeEnabled = false;
    for(int i=0; i<5; i++) {
      regs.channel[i].divider = 6;
      regs.channel[i].lastStep = 0;
      regs.channel[i].enabled = false;
      for(int p=0; p<4; p++) {
        regs.channel[i].page[p] = 0;
      }
    }
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

void printDrumSequencerChannel(DrumSequencerChannel channel, int index) {
  char s[100];
  sprintf(s, "ch%d => laststep: %d | divider: %d | output enabled: ", index, channel.lastStep, channel.divider);
  Serial.print(s);
  Serial.println(channel.enabled);
  Serial.print("steps: ");  
  for(int i=0; i<4; i++) {
    printInt(channel.page[i]);
  }
  Serial.println();
}

void printDrumSequencer(DrumSequencer drums) {
  for(int i=0; i<5; i++) {
    printDrumSequencerChannel(drums.channel[i], i);
  }
  Serial.print("chain mode enabled: ");
  Serial.println(drums.chainModeEnabled);
}

void printSongPart(Part part, int index) {
  char s[100];
  sprintf(s, "part %d => repeats: %d | chainTo: %d", index, part.repeats, part.chainTo);
  Serial.println(s);
  printTempoRegisters(part.tempo);
  printDrumSequencer(part.drumSequencer);
  printSamplerRegisters(part.sampler);
}

void printSong(Song song) {
  Serial.println("SONG:");

  for(int i=0; i<CHANNELS; i++) {
    printSongPart(song.parts[i], i);
  }
}

#endif