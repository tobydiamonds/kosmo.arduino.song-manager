#ifndef SongWriter_h
#define SongWriter_h

#include <SD.h>
#include <Arduino.h>
#include "shared.h"


class SongWriter {
  private:
    File file;

    void writeLine(const String& line) {
      Serial.println(line);
      if (file) {
        file.println(line);
      } else {
        Serial.println("Error writing line to file.");
      }
    }

    void convertSongToLines(const Song& song) {
      for (int partIndex = 0; partIndex < CHANNELS; partIndex++) {
        const Part& part = song.parts[partIndex];

        // Song Programmer Command
        String line = String(partIndex) + "=" + 
                      String(part.drumSequencer.channel[0].lastStep / 16 + 1) + " " + 
                      String(part.repeats) + " " + 
                      String(part.chainTo);
        writeLine(line);

        // Tempo Command
        line = String(partIndex) + ":tempo=" + String(part.tempo.bpm);
        writeLine(line);

        // Sampler Command
        line = String(partIndex) + ":sampler=" + String(part.sampler.bank);
        writeLine(line);
        for (int i = 0; i < 5; i++) {
          line = String(partIndex) + ":sampler:" + String(i) + ".mix=" + String(part.sampler.mix[i]);
          writeLine(line);
        }

        // Drum Sequencer Commands
        for (int channelIndex = 0; channelIndex < 5; channelIndex++) {
          const DrumSequencerChannel& channel = part.drumSequencer.channel[channelIndex];
          
          line = String(partIndex) + ":seq:" + String(channelIndex) + "=";
          for (int pageIndex = 0; pageIndex < 4; pageIndex++) {
            line += String(channel.page[pageIndex], BIN);
            if (pageIndex < 3) {
              line += " ";
            }
          }
          writeLine(line);

          line = String(partIndex) + ":seq:" + String(channelIndex) + ".div=" + String(channel.divider);
          writeLine(line);

          line = String(partIndex) + ":seq:" + String(channelIndex) + ".ena=" + String(channel.enabled ? "1" : "0");
          writeLine(line);

          line = String(partIndex) + ":seq:" + String(channelIndex) + ".last=" + String(channel.lastStep);
          writeLine(line);
        }
      }
    }

  public:
    // SongWriter() { }

    // ~SongWriter() { }

    void Save(const char* filename, const Song& song) {
      file = SD.open(filename, FILE_WRITE);
      if (!file) {
        Serial.print("Error opening file: ");
        Serial.println(filename);
      }
      convertSongToLines(song);
      Serial.println("Song saved successfully.");
      if (file) {
        file.close();
      }      
    }
};


#endif