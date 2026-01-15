#ifndef SerialSongParser_h
#define SerialSongParser_h


class SerialSongParser {
  private:
    Song& currentSong;

    void parseDrumSequencerCommand(int partIndex, String path, String data) {
      int pathSize;
      String* paths = splitString(path, '.', pathSize);
      
      int channel = -1;
      String setting = "";
      if(pathSize==1) {
        channel = paths[0].toInt();
      } else if(pathSize==2) {
        channel = paths[0].toInt();
        setting = paths[1];
      }
      delete[] paths;

      if(channel == -1) {
        Serial.println("Invalid channel");
        return;
      }

      int valueSize;
      String* values = splitString(data, ' ', valueSize);
      
      if(setting=="div") {
        int divider;
        if(valueSize == 1 && tryGetInt(values[0], divider)) {
          currentSong.parts[partIndex].drumSequencer.channel[channel].divider = divider;
        } else {
          Serial.println("Invalid argument setting divider");          
        }
      } else if(setting=="ena") {
        if(valueSize == 1) {
          currentSong.parts[partIndex].drumSequencer.channel[channel].enabled = values[0] == "1";
        } else {
          Serial.println("Invalid argument setting enabled");          
        }
      } else if(setting=="last") {
        int laststep;
        if(valueSize == 1 && tryGetInt(values[0], laststep)) {
          currentSong.parts[partIndex].drumSequencer.channel[channel].lastStep = laststep;
        } else {
          Serial.println("Invalid argument setting laststep");          
        }
      } else {
        // we are setting the steps - each value part corresponds to a page
        uint16_t steps;
        for(int i=0; i<valueSize; i++) {
          if(!tryParseInt(values[i], steps)) {
            steps = 0;
          }
          currentSong.parts[partIndex].drumSequencer.channel[channel].page[i] = steps;
        }
      }

      delete[] values;

      printDrumSequencerChannel(currentSong.parts[partIndex].drumSequencer.channel[channel], channel);
    }

    void parseSamplerCommand(int partIndex, String details, String values) {
      int channelIndex = details.substring(0, details.indexOf('.')).toInt();
      String function = details.substring(details.indexOf('.') + 1, details.indexOf('='));
      String value = details.substring(details.indexOf('=') + 1);

      if (function == "mix") {
        currentSong.parts[partIndex].sampler.mix[channelIndex] = value.toInt();
      } else {
        currentSong.parts[partIndex].sampler.bank = value.toInt();
      }
    }

    void parseSongProgrammerCommand(int partIndex, String values) {
      Serial.println("Parsing Song Programmer Command:");
      Serial.print("Values: ");
      Serial.println(values);

      bool error = false;

      int pages;
      int repeats;
      int chainTo;

      int size;
      String* parts = splitString(values, ' ', size);
      if(size != 3) {
        Serial.println("Invalid arguments for song programmer!");
        error = true;
      }

      if(!tryGetInt(parts[0], pages)) {
        Serial.println("Invalid value for pos 0/pages");
        error = true;
      }
      if(!tryGetInt(parts[1], repeats)) {
        Serial.println("Invalid value for pos 1/repeats");
        error = true;
      }
      if(!tryGetInt(parts[2], chainTo)) {
        Serial.println("Invalid value for pos 2/chainTo");
        error = true;
      }      

      delete[] parts;

      // Validate ranges
      if (pages < 0 || pages > 4) {
        Serial.println("Invalid pages value!");
        return -1;
      }
      if (repeats < 0 || repeats > 32) {
        Serial.println("Invalid repeats value!");
        return -1;
      }
      if (chainTo < -1 || chainTo > 15) {
        Serial.println("Invalid chainTo value!");
        return -1;
      }      
      currentSong.parts[partIndex].repeats = repeats;
      currentSong.parts[partIndex].chainTo = chainTo;

       // Calculate lastStep based on pages
      int lastStep = pages * 16 - 1;
      if (pages == 0) lastStep = 0;

      // Set lastStep for all channels
      for (int i = 0; i < CHANNELS; i++) {
        currentSong.parts[partIndex].drumSequencer.channel[i].lastStep = lastStep;
      }

      Serial.print("Last Step set to: ");
      Serial.println(lastStep);
    }

  public:
    SerialSongParser(Song& song) : currentSong(song) {}

    int parseCommand(String command) {
      int partIndex;
      String module;
      String path;
      String values;

      // Split the command into parts
      if(command.indexOf('=') == -1) {
        Serial.println("Invalid command - missing =");
        return -1; // invalid command
      }
      
      int size=0;
      String* parts = splitString(command, ':', size);

      char s[100];
      sprintf(s, "command: %s  (%d parts) => ", command.c_str(), size);
      Serial.print(s);      
      for(int i=0; i<size; i++) {
        Serial.print(parts[i]);
        if(i<size-1) {
          Serial.print(",");
        }
      }
      Serial.println();

      if(size==1) { // no module
        // e.g. 0=2 0 2
        int pos = command.indexOf('=');
        partIndex = parts[0].toInt();
        module = "song";
        path = "";
        values = command.substring(pos + 1);
      } else if(size==2) {
        // e.g. 0:tempo=120
        //      0:sampler=1
        int pos = parts[1].indexOf('=');
        partIndex = parts[0].toInt();
        module = parts[1].substring(0, pos);
        path = "";
        values = parts[1].substring(pos + 1);
      } else if(size==3) {
        // e.g. 0:seq:0=1000100010001000
        //      0:seq:0.last=31
        //      0:sampler:0.mix=512
        int pos = parts[2].indexOf('=');
        partIndex = parts[0].toInt();
        module = parts[1];
        path = parts[2].substring(0, pos);
        values = parts[2].substring(pos + 1);
      } else {
        Serial.println("Invalid command!");
        return -1;
      }

      delete[] parts;

      sprintf(s, "partIndex: %d | module: %s | path: %s | values: %s", partIndex, module.c_str(), path.c_str(), values.c_str());
      Serial.println(s);

      if (module == "seq") {
        parseDrumSequencerCommand(partIndex, path, values);
      } else if (module == "tempo") {
        currentSong.parts[partIndex].tempo.bpm = values.toInt();
      } else if (module == "sampler") {
        parseSamplerCommand(partIndex, path, values);
      } else {
        parseSongProgrammerCommand(partIndex, values);
      }

      return partIndex;
    }
};

#endif