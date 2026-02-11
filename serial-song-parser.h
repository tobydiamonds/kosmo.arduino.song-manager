#ifndef SerialSongParser_h
#define SerialSongParser_h


class SerialSongParser {
  private:
    Song& _song;
    const int allowedDividers[7] = {3,6,8,9,12,15,24};

    bool isDividerAllowed(int divider) {
      for (int i = 0; i < 7; i++) {
        if (allowedDividers[i] == divider) {
          return true;
        }
      }
      return false;
    }

    bool parseDrumSequencerCommand(int partIndex, String path, String data) {
      bool error = false;
      int pathSize;
      String* paths = splitString(path, '.', pathSize);
      
      int channel = -1;
      String function = "";
      if(pathSize==1) {
        channel = paths[0].toInt();
      } else if(pathSize==2) {
        channel = paths[0].toInt();
        function = paths[1];
      }
      delete[] paths;

      if(channel == -1) {
        Serial.println("Invalid channel");
        return false;
      }

      int valueSize;
      String* values = splitString(data, ' ', valueSize);
      
      if(function=="div") {
        int divider;
        if(valueSize == 1 && tryGetInt(values[0], divider) && isDividerAllowed(divider)) {
          _song.parts[partIndex].drumSequencer.channel[channel].divider = divider;
        } else {
          Serial.println("Invalid argument setting divider");          
          error = true;
        }
      } else if(function=="ena") {
        if(valueSize == 1) {
          _song.parts[partIndex].drumSequencer.channel[channel].enabled = values[0] == "1";
        } else {
          Serial.println("Invalid argument setting enabled");          
          error = true;
        }
      } else if(function=="last") {
        int laststep;
        if(valueSize == 1 && tryGetInt(values[0], laststep) && laststep >= 0 && laststep <= 63) {
          _song.parts[partIndex].drumSequencer.channel[channel].lastStep = laststep;
        } else {
          Serial.println("Invalid argument setting laststep");          
          error = true;
        }
      } else {
        // we are setting the steps - each value part corresponds to a page
        uint16_t steps;
        for(int i=0; i<valueSize; i++) {
          if(!tryParseInt(values[i], steps)) {
            steps = 0;
          }
          _song.parts[partIndex].drumSequencer.channel[channel].page[i] = steps;
        }
      }

      delete[] values;

      //printDrumSequencerChannel(_song.parts[partIndex].drumSequencer.channel[channel], channel);
      return !error;
    }

    bool parseTempoCommand(int partIndex, String path, String values) {
      bool error = false;
      int bpm;
      if(tryGetInt(values, bpm)) {
        _song.parts[partIndex].tempo.bpm = bpm;
      } else {
        Serial.println("Invalid bpm-value");
        error = true;
      }
      return !error;
    }

    bool parseSamplerCommand(int partIndex, String path, String values) {
      bool error = false;
      int pathSize;
      String* paths = splitString(path, '.', pathSize);
      
      int channel = -1;
      String function = "";
      if(path.length()==0) {
        function = "bank";
      } else if(pathSize==2) {
        channel = paths[0].toInt();
        function = paths[1];
      }
      delete[] paths;

      if(function == "mix" && channel == -1) {
        Serial.println("Invalid channel");
        return false;
      }

      if (function == "mix") {
        int mix;
        if(tryGetInt(values, mix) && mix >= 0 && mix <= 1023) {
          _song.parts[partIndex].sampler.mix[channel] = mix;
        } else {
          Serial.println("Invalid mix-value");
          error = true;
        }
      } else {
        int bank;
        if(tryGetInt(values, bank) && bank >= 0 && bank <= 99) {
          _song.parts[partIndex].sampler.bank = bank;
        } else {
          Serial.println("Invalid bank-value");
          error = true;
        }
      }
      return !error;
    }

    bool parseSongProgrammerCommand(int partIndex, String values) {
      bool error = false;

      int pages;
      int repeats;
      int chainTo;

      int size;
      String* parts = splitString(values, ' ', size);
      if(size != 3) {
        Serial.print("Invalid arguments for song programmer: ");
        Serial.println(values);
        error = true;
      }

      if(!tryGetInt(parts[0], pages)) {
        Serial.print("Invalid value for pos 0/pages: ");
        Serial.println(parts[0]);
        error = true;
      }
      if(!tryGetInt(parts[1], repeats)) {
        Serial.print("Invalid value for pos 1/repeats: ");
        Serial.println(parts[1]);
        error = true;
      }
      if(!tryGetInt(parts[2], chainTo)) {
        Serial.print("Invalid value for pos 2/chainTo: ");
        Serial.println(parts[2]);
        error = true;
      }      

      delete[] parts;

      // Validate ranges
      if (pages < 0 || pages > 4) {
        Serial.println("Invalid pages value!");
        error = true;
      }
      if (repeats < 0 || repeats > 32) {
        Serial.println("Invalid repeats value!");
        error = true;
      }
      if (chainTo < -1 || chainTo > 15) {
        Serial.println("Invalid chainTo value!");
        error = true;
      }      

      if(error) return false;

      _song.parts[partIndex].pages = pages;
      _song.parts[partIndex].repeats = repeats;
      _song.parts[partIndex].chainTo = chainTo;

      return true;
    }

  public:
    SerialSongParser(Song& song) : _song(song) {}

    int parseCommand(String command, SlaveEnum &target) {
      target = NONE;
      command.trim();

      if(command=="init") return -1;
      if(command=="apply") return -1;
      if(command.indexOf("#")==0) return -1;


      int partIndex;
      String module;
      String path;
      String values;

      // Split the command into parts
      if(command.indexOf('=') == -1) {
        Serial.print("Invalid command - missing '=' : ");
        Serial.println(command);
        return -1; // invalid command
      }
      
      int size=0;
      String* parts = splitString(command, ':', size);

      // char s[100];
      // sprintf(s, "command: %s  (%d parts) => ", command.c_str(), size);
      // Serial.print(s);      
      // for(int i=0; i<size; i++) {
      //   Serial.print(parts[i]);
      //   if(i<size-1) {
      //     Serial.print(",");
      //   }
      // }
      // Serial.println();

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
        Serial.print("Invalid command: ");
        Serial.println(command);
        return -1;
      }

      delete[] parts;

      // sprintf(s, "partIndex: %d | module: %s | path: %s | values: %s", partIndex, module.c_str(), path.c_str(), values.c_str());
      // Serial.println(s);

      module.trim();
      path.trim();
      values.trim();

      bool result = true;

      if (module == "seq") {
        result = parseDrumSequencerCommand(partIndex, path, values);
        target = DRUM_SEQUENCER;
      } else if (module == "tempo") {
        result = parseTempoCommand(partIndex, path, values);
        target = TEMPO;
      } else if (module == "sampler") {
        result = parseSamplerCommand(partIndex, path, values);
        target = SAMPLER;
      } else {
        result = parseSongProgrammerCommand(partIndex, values);
        target = PROGRAMMER;
      }

      if(!result) return -1;

      return partIndex;
    }
};

#endif