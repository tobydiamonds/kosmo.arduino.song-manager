#ifndef Song1_h
#define Song1_h

#include "shared.h"
#include <SPI.h>
#include <SD.h>

const int SD_CS_PIN = 53;

void SetupSongRepository() {
  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card failed, or not present.");
    return;
  }
  Serial.println("Card initialized.");  
}

bool SaveSong(Song song, int index) {
  //printSong(song);

  char filename[15];
  snprintf(filename, sizeof(filename), "song_%d.dat", index);

  // delete existing file
  if(SD.exists(filename)) {
    SD.remove(filename);
  }

  // save the content of the struct in the file
  File file = SD.open(filename, FILE_WRITE);
  if(!file) {
    Serial.print("error accessing file: ");
    Serial.println(filename);
    return false;
  }
  size_t bytesWritten = file.write((const uint8_t*)&song, sizeof(Song));
  file.close();

  if(bytesWritten != sizeof(Song)) {
    Serial.print("error writing to file: ");
    Serial.println(filename);    
    return false;
  }

  Serial.print("Song saved to: ");
  Serial.println(filename);  
  return true;
}

Song LoadSong(int index, bool &success) {
  char filename[15];
  snprintf(filename, sizeof(filename), "song_%d.dat", index);
  if(!SD.exists(filename)) {
    Serial.print("File does not exist: ");
    Serial.println(filename);
    success = false;
    return;
  }

  File file = SD.open(filename);
  if(!file) {
    Serial.print("error accessing file: ");
    Serial.println(filename);
    success = false;
    return;    
  }

  Song song = Song();
  size_t bytesRead = file.read((uint8_t*)&song, sizeof(Song));
  file.close();
  if (bytesRead < sizeof(Song)) {
    // Pad the remainder of the struct with 0x00 if the read was short
    memset(reinterpret_cast<uint8_t*>(&song) + bytesRead, 0x00, sizeof(Song) - bytesRead);
    char s[100];
    sprintf(s, "Warning: Incomplete read. Read: %d. Padded: %d", bytesRead, (int)(sizeof(Song) - bytesRead));
    Serial.println(s);
  }
  Serial.print("Song loaded from: ");
  Serial.println(filename);  
  success = true;

  //printSong(song);

  return song;
}



#endif