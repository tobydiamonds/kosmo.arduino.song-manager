#ifndef SerialFunctions_h
#define SerialFunctions_h

#include "shared.h"


bool getSerialData(int& test, int& channel) {
  while(Serial.available()) {
      String data = Serial.readString();
      data.trim();

      // find seperators
      int seperators[4] = {};
      int length = 0;
      for(int i=0; i<data.length(); i++) {
        if(data.charAt(i) == ' ') {
          seperators[length] = i;
          length++;
        }
      }

      // extract data values
      int values[5] = {};      
      int offset = 0;
      int valueCount = 0;
      for(int i=0; i<length; i++) {
        int value;
        if(tryGetInt(data, offset, seperators[i], value))
          values[valueCount++] = value;
        offset = seperators[i];
      }
      // get the last value
      if(seperators[length-1]<data.length()) {
        int value;
        if(tryGetInt(data, seperators[length-1]+1, data.length(), value))
          values[valueCount++] = value;
      }

      if(valueCount!=2) return false;
      test = values[0];
      channel = values[1];
      return true;
    }
}

#endif