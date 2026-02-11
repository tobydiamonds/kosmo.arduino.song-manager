#ifndef IntegrationTests_h
#define IntegrationTests_h

#include <Wire.h>
#include "shared.h"
#include "serial-functions.h"

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

bool TempoTest_GetDataFromSlave(TestContext context) {
  Serial.print("Running test TempoTest_GetDataFromSlave:");  
  // arrange
  TempoRegisters tempoRegisters;

  // act
  Wire.requestFrom(SLAVE_ADDR_TEMPO, sizeof(tempoRegisters));

  // assert
  if(Wire.available() == 0) {
    Serial.println("Error while retrieving status from slave");
    return false;
  }
  if(Wire.available() != sizeof(TempoRegisters)) {
    char s[100];
    sprintf(s, "Error data size from slave - expected: %d, actual: %d", sizeof(TempoRegisters), Wire.available());
    Serial.println(s);
    return false;
  }
  Wire.readBytes((char*)&tempoRegisters, sizeof(TempoRegisters));  

  printTempoRegisters(tempoRegisters);

  return true;
}

bool TempoTest_SetDataFromMaster(TestContext context, char* command) {
  Serial.println("Running test TempoTest_SetDataFromMaster:");

  // arrange
  TempoRegisters tempoRegisters;

  const size_t totalSize = sizeof(TempoRegisters);
  TempoRegisters regs;
  regs.bpm = 130;
  regs.morphBars = 1;
  regs.morphEnabled = true;
  regs.morphTargetBpm = 170;

  uint8_t buffer[totalSize];
  memcpy(buffer, &regs, totalSize); 

  // act
  Wire.beginTransmission(SLAVE_ADDR_TEMPO);
  Wire.write(command);
  Wire.write(&buffer[0], totalSize);
  int result = Wire.endTransmission(); 
  if(result != 0) {
    Serial.println("Error while sending data to slave");
    return false;    
  }

  // assert

  return true;
}

bool SequencerTest_GetDataFromSlave(TestContext context) {
  Serial.println("Running test SequencerTest_GetDataFromSlave:");  

  // arrange
  DrumSequencer registers;
  size_t totalSize = sizeof(DrumSequencer);
  int currentChunk = 0;
  int totalChunks = (totalSize + 31) / 32;
  int chunkIndex = 0;

  // act 
  for(int i=0; i<totalChunks; i++) {
    size_t offset = chunkIndex * 32;
    size_t chunkSize = min(32, totalSize - offset);
    Wire.requestFrom(SLAVE_ADDR_DRUM_SEQUENCER, chunkSize);    
    uint8_t buffer[chunkSize];
    size_t bytesRead = 0;

    while(Wire.available()) {
      buffer[bytesRead++] = Wire.read();
    }

    char s[100];
    sprintf(s, "Reading %d/%d => ", i, totalChunks);
    Serial.print(s);
    for(int j=0; j<bytesRead; j++) {
      Serial.print(buffer[j], HEX);
      Serial.print(" ");
    }
    Serial.println();

    memcpy((uint8_t*)&registers + offset, buffer, bytesRead);
    chunkIndex++;    
  }

  // assert
  printDrumSequencer(registers);
  return true;
}

bool SequencerTest_SetDataFromMaster(TestContext context) {
  Serial.println("Running test SequencerTest_SetDataFromMaster:");

  // arrange
  bool ena[5] = {true, true, false, false, false};
  int steps[5] = {31, 31, 0, 0, 0};

  DrumSequencer regs[8];
  for(int part=0; part<8; part++) {
    for(int i=0; i<5; i++) {
      regs[part].channel[i].divider = 6;
      regs[part].channel[i].enabled = ena[i];
      regs[part].channel[i].lastStep = steps[i];
      regs[part].channel[i].page[0] = 0x8888;
      regs[part].channel[i].page[1] = 0x888D;
      regs[part].channel[i].page[2] = 0;
      regs[part].channel[i].page[3] = 0;
    }
    regs[part].chainModeEnabled = false;
  }
  size_t totalSize = sizeof(DrumSequencer);
  int totalChunks = (totalSize + 31) / 32;


  for(int part=0; part<8; part++) {
    int chunkIndex = 0;
    uint8_t buffer[totalSize];
    memcpy(buffer, &regs[part], totalSize);  

    // act
    for(int i=0; i<totalChunks; i++) {
      size_t offset = chunkIndex * 32;
      size_t chunkSize = min(32, totalSize - offset);
      Wire.beginTransmission(SLAVE_ADDR_DRUM_SEQUENCER);
      Wire.write(&buffer[offset], chunkSize);
      int result = Wire.endTransmission(); 
      if(result != 0) {
        Serial.println("Error while sending data to slave");
        return false;    
      }
      chunkIndex++;    
    }
  }
  // assert

  return true;
}

bool SequencerTest_SetPartIndexFromMaster(TestContext context) {
  Serial.println("Running test SequencerTest_SetPartIndexFromMaster:");

  // arrange
  int partIndex = context.index;

  // act
  Wire.beginTransmission(SLAVE_ADDR_DRUM_SEQUENCER);
  Wire.write(partIndex);
  int result = Wire.endTransmission(); 
  if(result != 0) {
    Serial.print("Error while sending part-index to drum sequencer: ");
    Serial.println(result);
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
    case 2:
      result = TempoTest_GetDataFromSlave(context); break;
    case 3:
      result = TempoTest_SetDataFromMaster(context, "set"); break;
    case 4:
      result = TempoTest_SetDataFromMaster(context, "stop"); break;
    case 5:
      result = TempoTest_SetDataFromMaster(context, "start"); break;            
    case 6:
      result = SequencerTest_GetDataFromSlave(context); break;
    case 7:
      result = SequencerTest_SetDataFromMaster(context); break;
    case 8:
      result = SequencerTest_SetPartIndexFromMaster(context); break;
    default:
      result = false; break;
  }
  

  if(result)
    Serial.println("INTEGRATION TEST PASSED SUCCESSFULLY");
  else
    Serial.println("INTEGRATION TEST COMPLETED WITH ERRORS!!!");  
}




#endif