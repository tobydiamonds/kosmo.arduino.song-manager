#ifndef Song1_h
#define Song1_h

#include "shared.h"
#include <SPI.h>
#include <SD.h>
#include "serial-song-parser.h"
#include "song-writer.h"

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

  SongWriter writer;
  writer.Save(filename, song);

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
  SerialSongParser parser(song);

  Serial.println("### RAW SONG CONTENT AT LOAD SONG ###");

  while (file.available()) {
    String command = file.readStringUntil('\n');
    Serial.println(command);
    SlaveEnum target;
    int partIndex = parser.parseCommand(command, target);
    if (partIndex == -1) {
      Serial.println("Error parsing command.");
      success = false;
      return Song();
    }
  }

  if(!success) {
    file.close();
    Serial.print("Song loaded from: ");
    Serial.println(filename);  
    return Song();
  }
  success = true;

  //printSong(song);

  return song;
}

void PeekSong(int index) {
  Song song;
  char filename[15];
  snprintf(filename, sizeof(filename), "song_%d.dat", index);
  if(!SD.exists(filename)) {
    Serial.print("File does not exist: ");
    Serial.println(filename);
    return;
  }

  File file = SD.open(filename);
  if(!file) {
    Serial.print("error accessing file: ");
    Serial.println(filename);
    return;    
  }  

  while (file.available()) {
    String command = file.readStringUntil('\n');
    Serial.println(command);
  }
  file.close();  
}



#endif