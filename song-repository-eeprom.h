#ifndef SongRepositoryEEPROM_h
#define SongRepositoryEEPROM_h

#include <Arduino.h>
#include <EEPROM.h>
#include "shared.h"
#include "song-serializer.h"

#define SONG_SIZE 2048

int address = 0;

void writeLineToEEPROM(const String& line) {
  Serial.print(address);
  Serial.print(":");
  Serial.println(line);
  for (int i = 0; i < line.length(); i++) {
    EEPROM.write(address++, line[i]);
  }
  EEPROM.write(address++, '\n'); // Add newline character
}

class SongRepositoryEEPROM {
  private:
    int calculateAddress(int index) {
      if(index < 1) return -1;
      // first song is index 1 => we want to start at address 0
      return (index-1) * SONG_SIZE; // Assume each song takes 512 bytes, adjust if needed
    }

  public:
    bool SaveSong(const Song& song, int index) {
      int offset = calculateAddress(index);
      if(offset < 0) return false;
      address = offset;

      SongSerializer writer;
      writer.serialize(song, writeLineToEEPROM);

      writeLineToEEPROM("EOS\n");

      Serial.print("Song saved successfully. Size: ");
      Serial.println(address - offset);
      return true;
    }

    Song LoadSong(int index, bool &success) {
      int offset = calculateAddress(index);
      address = offset;
      int endAddress = address + SONG_SIZE + 1000;
      Song song = Song();
      SerialSongParser parser(song);      
      String line;
      while (address < endAddress) {
        char c = EEPROM.read(address++);
        if (c == '\n' || address >= EEPROM.length()) {
          // Process line
          if (line.length() > 0) {
            Serial.println(line);
            
            if(line=="EOS") break;

            SlaveEnum target;
            int partIndex = parser.parseCommand(line, target);
          }
          line = "";
          if (address >= EEPROM.length()) break;
        } else {
          line += c;
        }
      }
      Serial.print("Song loaded successfully. Size: ");
      Serial.println(address - offset);
      success = true;
      return song;
    }
};

#endif