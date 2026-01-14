#ifndef SerialSongParser_h
#define SerialSongParser_h


class SerialSongParser {
  private:
    Song& currentSong;

    void parseDrumSequencerCommand(int partIndex, String details) {
      Serial.println("Parsing Drum Sequencer Command:");
      Serial.print("Details: ");
      Serial.println(details);

      int channelIndex = details.substring(0, details.indexOf(':')).toInt();
      details = details.substring(details.indexOf(':') + 1);

      if (details.indexOf('=') != -1) {
        String value = details.substring(details.indexOf('=') + 1);
        String function = details.substring(0, details.indexOf('='));

        Serial.print("Channel Index: ");
        Serial.println(channelIndex);
        Serial.print("Function: ");
        Serial.println(function);
        Serial.print("Value: ");
        Serial.println(value);

        if (function == "div") {
          currentSong.parts[partIndex].drumSequencer.channel[channelIndex].divider = value.toInt();
          Serial.print("Divider set to: ");
          Serial.println(value.toInt());
        } else if (function == "ena") {
          currentSong.parts[partIndex].drumSequencer.channel[channelIndex].enabled = (value.toInt() != 0);
          Serial.print("Enabled set to: ");
          Serial.println(value.toInt() != 0);
        } else if (function == "lst") {
          currentSong.parts[partIndex].drumSequencer.channel[channelIndex].lastStep = value.toInt();
          Serial.print("Last Step set to: ");
          Serial.println(value.toInt());
        }
      } else {
        // Handle page patterns
        int pageIndex = 0;
        while (details.length() > 0) {
          int spaceIndex = details.indexOf(' ');
          String pattern = (spaceIndex == -1) ? details : details.substring(0, spaceIndex);
          currentSong.parts[partIndex].drumSequencer.channel[channelIndex].page[pageIndex] = strtol(pattern.c_str(), NULL, 2);
          Serial.print("Page ");
          Serial.print(pageIndex);
          Serial.print(" pattern set to: ");
          Serial.println(pattern);
          details = (spaceIndex == -1) ? "" : details.substring(spaceIndex + 1);
          pageIndex++;
        }
      }
    }

    void parseSamplerCommand(int partIndex, String details) {
      int channelIndex = details.substring(0, details.indexOf('.')).toInt();
      String function = details.substring(details.indexOf('.') + 1, details.indexOf('='));
      String value = details.substring(details.indexOf('=') + 1);

      if (function == "mix") {
        currentSong.parts[partIndex].sampler.mix[channelIndex] = value.toInt();
      } else {
        currentSong.parts[partIndex].sampler.bank = value.toInt();
      }
    }

    void parseSongProgrammerCommand(int partIndex, String details) {
      Serial.println("Parsing Song Programmer Command:");
      Serial.print("Details: ");
      Serial.println(details);

      int commaIndex = details.indexOf(',');
      int pages = details.substring(0, commaIndex).toInt();
      details = details.substring(commaIndex + 1);

      Serial.print("Pages: ");
      Serial.println(pages);

      commaIndex = details.indexOf(',');
      int repeats = details.substring(0, commaIndex).toInt();
      details = details.substring(commaIndex + 1);

      Serial.print("Repeats: ");
      Serial.println(repeats);

      commaIndex = details.indexOf(',');
      int chainTo = details.substring(0, commaIndex).toInt();
      details = details.substring(commaIndex + 1);

      Serial.print("Chain To: ");
      Serial.println(chainTo);

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
      String details;
      String value;

      // Split the command into parts
      int equalPos = command.indexOf('=');
      if(equalPos == -1) return -1; // invalid command
      if(!tryGetInt(command, 0, equalPos, partIndex)) return -1; // invalid part index

      int firstColon = command.indexOf(':');
      int secondColon = command.indexOf(':', firstColon + 1);

      if (firstColon == -1) { // no module
        module = "song";
        details = "";
        value = command.substring(equalPos + 1);
      }
      if (secondColon == -1) {
        module = command.substring(firstColon + 1, equalPos);
        details = "";
        value = command.substring(equalPos + 1);
      } else {
        module = command.substring(firstColon + 1, secondColon);
        details = command.substring(secondColon + 1, equalPos);
        value = command.substring(equalPos + 1);
      }

      if (module == "seq") {
        parseDrumSequencerCommand(partIndex, details);
      } else if (module == "tempo") {
        currentSong.parts[partIndex].tempo.bpm = details.toInt();
      } else if (module == "sampler") {
        parseSamplerCommand(partIndex, details);
      } else {
        parseSongProgrammerCommand(partIndex, module);
      }

      return partIndex;
    }
};

#endif