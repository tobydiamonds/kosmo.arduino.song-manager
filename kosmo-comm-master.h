#ifndef KosmoCommMaster_h
#define KosmoCommMaster_h

#include <Wire.h>
#include "shared.h"

#define SLAVE_ADDR_TEMPO 8
#define SLAVE_ADDR_DRUM_SEQUENCER 9
#define SLAVE_ADDR_SAMPLER 10

#define RETRY_INTERVAL 3000
#define RETRY_LIMIT 10

#define MAX_CHUNK_SIZE 32 // i2c has this limitation



const int numberOfSlaves = 3;
KosmoSlave slaves[numberOfSlaves] = {
  {SLAVE_ADDR_TEMPO, false, false, 0,0, sizeof(TempoRegisters)},
  {SLAVE_ADDR_DRUM_SEQUENCER, false, false, 0,0, sizeof(DrumSequencer)},
  {SLAVE_ADDR_SAMPLER, false, false, 0,0, sizeof(SamplerRegisters)}
};

void setupMaster() {
  Wire.begin();
  Wire.setClock(400000);
}

bool getKosmoTempoRegisters(unsigned long now, int slaveIndex) {
  Wire.requestFrom(slaves[slaveIndex].address, slaves[slaveIndex].registerSize);

  if(Wire.available() == 0) {
    Serial.println("Error while retrieving status from slave");
    return false;
  }
  if(Wire.available() != slaves[slaveIndex].registerSize) {
    char s[100];
    sprintf(s, "Error data size from slave - expected: %d, actual: %d", slaves[slaveIndex].registerSize, Wire.available());
    Serial.println(s);
    return false;
  }
  Wire.readBytes((char*)&sharedTempoRegisters, slaves[slaveIndex].registerSize);
  return true;
}

bool getKosmoDrumSequencerRegisters(unsigned long now, int slaveIndex) {
  size_t totalSize = slaves[slaveIndex].registerSize;
  int currentChunk = 0;
  int totalChunks = (totalSize + 31) / 32;
  int chunkIndex = 0;

  // act 
  for(int i=0; i<totalChunks; i++) {
    size_t offset = chunkIndex * 32;
    size_t chunkSize = min(32, totalSize - offset);
    Wire.requestFrom(slaves[slaveIndex].address, chunkSize);    
    uint8_t buffer[chunkSize];
    size_t bytesRead = 0;

    while(Wire.available()) {
      buffer[bytesRead++] = Wire.read();
    }

    memcpy((uint8_t*)&sharedDrumSequencerRegisters + offset, buffer, bytesRead);
    chunkIndex++;    
  }  
  return true;
}

bool getKosmoSampleRegisters(unsigned long now, int slaveIndex) {
  Wire.requestFrom(slaves[slaveIndex].address, slaves[slaveIndex].registerSize);

  if(Wire.available() == 0) {
    Serial.println("Error while retrieving status from slave");
    return false;
  }
  if(Wire.available() != slaves[slaveIndex].registerSize) {
    char s[100];
    sprintf(s, "Error data size from slave - expected: %d, actual: %d", slaves[slaveIndex].registerSize, Wire.available());
    Serial.println(s);
    return false;
  }
  Wire.readBytes((char*)&sharedSamplerRegisters, sizeof(SamplerRegisters));  
  return true;  
}

bool getSlaveRegisters(unsigned long now) {
  // Check if we are in programming mode and if a request is in progress
  bool results[numberOfSlaves] = {false};
  for(int i=0; i<numberOfSlaves; i++) {

    if(!slaves[i].requestInProgress && now > (slaves[i].lastGetRequest + RETRY_INTERVAL)) {
      Serial.print("Starting get request to slave ");
      Serial.println(i);
      slaves[i].requestInProgress = true;
      slaves[i].lastGetRequest = now;

      bool status = false;
      if(i==0) {
        status = getKosmoTempoRegisters(now, 0);
      } else if(i==1) {
        status = getKosmoDrumSequencerRegisters(now, 1);
      } else if(i==2) {
        status = getKosmoSampleRegisters(now, 2);
      }
    
      if (status) {
        results[i] = true;
      } else if(slaves[i].retries < RETRY_LIMIT) {
        slaves[i].retries++;
        Serial.print("No registers received from slave ");
        Serial.print(i);
        Serial.println(". Retrying in 1s");
      }

      slaves[i].requestInProgress = false;
    }
  }
  for(int i=0; i<numberOfSlaves; i++) {
    if(!results[i])
      return false;
  }
  return true;
}

void setKosmoTempoRegisters(unsigned long now, int slaveIndex, TempoRegisters regs) {
  const size_t totalSize = slaves[slaveIndex].registerSize;
  uint8_t buffer[totalSize];
  memcpy(buffer, &regs, totalSize); 

  Wire.beginTransmission(slaves[slaveIndex].address);
  Wire.write("set");
  Wire.write(&buffer[0], totalSize);
  int result = Wire.endTransmission(); 
  if(result != 0) {
    Serial.println("Error while sending data to slave");
    return false;    
  }
  return true;
}

bool setKosmoDrumSequencerRegisters(unsigned long now, int slaveIndex, DrumSequencer drums) {
  size_t totalSize = slaves[slaveIndex].registerSize;
  int totalChunks = (totalSize + 31) / 32;
  int chunkIndex = 0;

  uint8_t buffer[totalSize];
  memcpy(buffer, &drums, totalSize);  

  for(int i=0; i<totalChunks; i++) {
    size_t offset = chunkIndex * 32;
    size_t chunkSize = min(32, totalSize - offset);
    Wire.beginTransmission(slaves[slaveIndex].address);
    Wire.write(&buffer[offset], chunkSize);
    int result = Wire.endTransmission(); 
    if(result != 0) {
      Serial.println("Error while sending data to slave");
      return false;    
    }
    chunkIndex++;    
  }

  return true;
}

bool setSamplerRegisters(unsigned long now, int slaveIndex, SamplerRegisters sampler) {
  const size_t totalSize = slaves[slaveIndex].registerSize;
  uint8_t buffer[totalSize];
  memcpy(buffer, &sampler, totalSize); 

  Wire.beginTransmission(slaves[slaveIndex].address);
  Wire.write(&buffer[0], totalSize);
  int result = Wire.endTransmission(); 
  if(result != 0) {
    Serial.println("Error while sending data to slave");
    return false;    
  }
  return true;
}

void setSlaveRegisters(unsigned long now, Part part) {
  for(int i=0; i<numberOfSlaves; i++) {
    Serial.print("Starting SET request to slave ");
    Serial.println(i);
    slaves[i].requestInProgress = true;

    if(i==0)
      setKosmoTempoRegisters(now, 0, part.tempo);
    else if(i==1)
      setKosmoDrumSequencerRegisters(now, 1, part.drumSequencer);
    else if(i==2)
      setSamplerRegisters(now, 2, part.sampler);

    slaves[i].requestInProgress = false;
  }
}

void startClock() {
  slaves[0].requestInProgress = true;  
  Wire.beginTransmission(slaves[0].address);
  Wire.write("start"); 
  int result = Wire.endTransmission(); 
  slaves[0].requestInProgress = false;    
}

void stopClock() {
  slaves[0].requestInProgress = true;  
  Wire.beginTransmission(slaves[0].address);
  Wire.write("stop"); 
  int result = Wire.endTransmission(); 
  slaves[0].requestInProgress = false;     
}
#endif