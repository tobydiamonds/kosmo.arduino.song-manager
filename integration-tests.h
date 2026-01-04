#ifndef IntegrationTests_h
#define IntegrationTests_h

#include <Wire.h>
#include "shared.h"

#define SLAVE_ADDR_TEMPO 8
#define SLAVE_ADDR_DRUM_SEQUENCER 9
#define SLAVE_ADDR_SAMPLER 10

struct TestContext {
  int index;
  Part part;
};

bool SamplerTest_GetDataFromSlave(TestContext context) {
  Serial.print("Running test SamplerTest_GetDataFromSlave:");  
  // arrange
  SamplerRegisters samplerRegisters;

  // act
  Wire.requestFrom(SLAVE_ADDR_SAMPLER, sizeof(SamplerRegisters));

  // assert
  if(Wire.available() == 0) {
    Serial.println("Error while retrieving status from slave");
    return false;
  }
  if(Wire.available() != sizeof(SamplerRegisters)) {
    char s[100];
    sprintf(s, "Error data size from slave - expected: %d, actual: %d", sizeof(SamplerRegisters), Wire.available());
    Serial.println(s);
    return false;
  }
  Wire.readBytes((char*)&samplerRegisters, sizeof(SamplerRegisters));  

  printSamplerRegisters(samplerRegisters);

  return true;
}

bool SamplerTest_SetDataFromMaster(TestContext context) {
  Serial.println("Running test SamplerTest_SetDataFromMaster:");

  // arrange
  SamplerRegisters samplerRegisters;

  const size_t totalSize = sizeof(SamplerRegisters);
  SamplerRegisters regs;
  regs.bank = 10;
  regs.mix[0] = 100;
  regs.mix[1] = 200;
  regs.mix[2] = 300;
  regs.mix[3] = 400;
  regs.mix[4] = 500;
  uint8_t buffer[totalSize];
  memcpy(buffer, &regs, totalSize); 

  // act
  Wire.beginTransmission(SLAVE_ADDR_SAMPLER);
  Wire.write(&buffer[0], totalSize);
  int result = Wire.endTransmission(); 
  if(result != 0) {
    Serial.println("Error while sending data to slave");
    return false;    
  }

  // assert

  return true;
}

void runIntegrationTest(int testIndex, int channel, Song song) {
  char s[100];
  sprintf(s, "testIndex: %d  |  channel: %d", testIndex, channel);
  Serial.println(s);
  
  if (channel < 0 || channel >= CHANNELS) {
      Serial.println("Invalid channel");
      return;
  }

  Part part = song.parts[channel];

  TestContext context = {channel, part};
  bool result = true;

  switch(testIndex) {
    case 0: 
      result = SamplerTest_GetDataFromSlave(context); break;
    case 1:
      result = SamplerTest_SetDataFromMaster(context); break;
    default:
      result = false; break;
  }
  

  if(result)
    Serial.println("INTEGRATION TEST PASSED SUCCESSFULLY");
  else
    Serial.println("INTEGRATION TEST COMPLETED WITH ERRORS!!!");  
}

String allowedChars = "0123456789";
bool isIntValue(String s) {
  if(s.length()==0)
    return false;

  for(int i=0; i<s.length(); i++) {
    if(allowedChars.indexOf(s.charAt(i)) == -1)
      return false;
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