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
  {SLAVE_ADDR_DRUM_SEQUENCER, false, false, 0,0, sizeof(DrumSequencerRegisters)},
  {SLAVE_ADDR_SAMPLER, false, false, 0,0, sizeof(SamplerRegisters)}
};

void setupMaster() {
  Wire.begin();
  Wire.setClock(400000);
}

void startProgrammingMode(unsigned long now) {
  // loop over slave array here
  Serial.println("set slaves in programming mode");

  for(int i=0; i<numberOfSlaves; i++) {
    Wire.beginTransmission(slaves[i].address);
    Wire.write("prg");
    int result = Wire.endTransmission();
    if (result == 0) {
      slaves[i].inProgrammingMode = true;
      Serial.println("Successfully sent command to slave.");
    } else {
      Serial.print("Error sending to slave: ");
      Serial.println(result); // Print the error code
    }
    //slaves[i].lastGetRequest = now;
  }

  Serial.println("END set slaves in programming mode");

}

void endProgrammingMode(unsigned long now, int slaveIndex) {
  if(slaves[slaveIndex].inProgrammingMode) {
    Wire.beginTransmission(slaves[slaveIndex].address);
    Wire.write("end");
    Wire.endTransmission();
    slaves[slaveIndex].inProgrammingMode = false;
    slaves[slaveIndex].retries = 0;
    slaves[slaveIndex].lastGetRequest = now;
  }
}

void endProgrammingModeForAllSlaves(unsigned long now) {
  for(int i=0; i<numberOfSlaves; i++) {
    endProgrammingMode(now, i);
  }
}

bool getKosmoTempoRegisters(unsigned long now, int slaveIndex) {
  Wire.requestFrom(slaves[slaveIndex].address, sizeof(TempoRegisters)+1); // Request 1 byte for the new data flag

  if(Wire.available() > 0) {
    int status = Wire.read();
    Serial.print("Status from slave 0: ");
    Serial.println(status);

    if(status==1 && Wire.available() == sizeof(TempoRegisters)) {
      Wire.readBytes((char*)&sharedTempoRegisters, sizeof(sharedTempoRegisters));
      Serial.print("Received TempoRegisters: BPM = ");
      Serial.print(sharedTempoRegisters.bpm);
      Serial.print(", Morph Target BPM = ");
      Serial.print(sharedTempoRegisters.morphTargetBpm);
      Serial.print(", Morph Bars = ");
      Serial.print(sharedTempoRegisters.morphBars);      
      Serial.print(", Morph Enabled = ");
      Serial.println(sharedTempoRegisters.morphEnabled);
      //endProgrammingMode(now, slaveIndex);        
      return true;  
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool getKosmoDrumSequencerRegisters(unsigned long now, int slaveIndex) {
  size_t expectedChunks = (sizeof(DrumSequencerRegisters) + 30) / 31;
  size_t expectedSize = (sizeof(DrumSequencerRegisters) + expectedChunks); // the struct + 1 status byte pr chunk
  Serial.println("Requesting chunk 0 from drum sequencer");
  Wire.beginTransmission(slaves[slaveIndex].address);
  Wire.write("get 0");
  Wire.endTransmission();
  Wire.requestFrom(slaves[slaveIndex].address, min(expectedSize, 32));

  bool result = false;

  if(Wire.available() > 0) {
    bool more = true; // read at least once
    int chunkIndex = 0;

    
    while(more && Wire.available()) {
      int status = Wire.read(); 
      expectedSize -= 1;

      if(status == 0) {
        more = false;
        result = false;
      } else if(status > 0) {
        uint8_t buffer[31];
        size_t bytesRead = 0;

        while(Wire.available() && bytesRead < 31) {
          buffer[bytesRead++] = Wire.read();
        }

        size_t offset = chunkIndex * 31;
        memcpy((uint8_t*)&sharedDrumSequencerRegisters + offset, buffer, bytesRead);
        expectedSize -= bytesRead;

        chunkIndex++;
        more = (status & 0x80) == 0x80;
        if(more) {
          char s[100];
          sprintf(s, "Requesting chunk %d from drum sequencer. Expected size: %d", chunkIndex, min(expectedSize, 32));
          Serial.println(s);

          Wire.beginTransmission(slaves[slaveIndex].address);
          Wire.write("get ");
          Wire.write(((char)'0' + chunkIndex));
          Wire.endTransmission();
          Wire.requestFrom(slaves[slaveIndex].address, min(expectedSize, 32));        
        } else {
          Serial.println("Received drum registers");
          printDrumSequencerRegisters(sharedDrumSequencerRegisters);
          //endProgrammingMode(now, slaveIndex);  
          result = true;
        }
      }
    }
    return result;
  } else {
    return false;
  }
}

bool getKosmoSampleRegisters(unsigned long now, int slaveIndex) {
  Wire.requestFrom(slaves[slaveIndex].address, sizeof(SamplerRegisters)+1); // Request 1 byte for the new data flag

  if(Wire.available() > 0) {
    int status = Wire.read();
    Serial.print("Status from slave 2: ");
    Serial.println(status);

    if(status==1 && Wire.available() == sizeof(SamplerRegisters)) {
      Wire.readBytes((char*)&sharedSamplerRegisters, sizeof(sharedSamplerRegisters));

      char s[100];
      sprintf(s, "Received SamplerRegisters: Bank => %d", sharedSamplerRegisters.bank);
      Serial.println(s);
      for(int i=0; i<5; i++) {
        sprintf(s, "ch%d mix => %d", i, sharedSamplerRegisters.mix[i]);
        Serial.println(s);
      }
      return true;  
    } else {
      return false;
    }
  } else {
    return false;
  }
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

      // Begin transmission to the slave
      Wire.beginTransmission(slaves[i].address);
      Wire.write("get"); // Send the request command
      int result = Wire.endTransmission();

      // Check for transmission success
      if(result == 0) {

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
        } else {
          endProgrammingMode(now, i);
          Serial.print("Max retries. Ending programming mode for slave ");
          Serial.println(i);
          results[i] = true;
        }
      } else {
        Serial.print("Transmission error, code: ");
        Serial.println(result); // Print the error code
        results[i] = true;
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
  const size_t totalSize = sizeof(TempoRegisters);
  uint8_t buffer[totalSize];
  memcpy(buffer, &regs, totalSize); 

  Wire.beginTransmission(slaves[slaveIndex].address);
  Wire.write(&buffer[0], totalSize);
  Wire.endTransmission(); 
}

void stopKosmoTempo() {
  Wire.beginTransmission(slaves[0].address);
  Wire.write("stop"); 
  Wire.endTransmission();      
}

void setKosmoDrumSequencerRegisters(unsigned long now, int slaveIndex, DrumSequencerRegisters drums) {
  const size_t totalSize = sizeof(DrumSequencerRegisters);
  uint8_t buffer[totalSize];
  memcpy(buffer, &drums, totalSize);  

  int chunkIndex = 0;
  
  size_t totalChunks = (totalSize + (MAX_CHUNK_SIZE-1)) / MAX_CHUNK_SIZE;
  while(chunkIndex < totalChunks) {
    int offset = chunkIndex * MAX_CHUNK_SIZE;
    int chunkSize = min(MAX_CHUNK_SIZE, totalSize-offset);
    Wire.beginTransmission(slaves[slaveIndex].address);
    Wire.write(&buffer[offset], chunkSize);
    Wire.endTransmission(); 
    chunkIndex++;
  }
  
}

void setSamplerRegisters(unsigned long now, int slaveIndex, SamplerRegisters sampler) {
  const size_t totalSize = sizeof(SamplerRegisters);
  uint8_t buffer[totalSize];
  memcpy(buffer, &sampler, totalSize); 

  Wire.beginTransmission(slaves[slaveIndex].address);
  Wire.write(&buffer[0], totalSize);
  Wire.endTransmission(); 
}

void setSlaveRegisters(unsigned long now, Part part) {
  for(int i=0; i<numberOfSlaves; i++) {
    Serial.print("Starting SET request to slave ");
    Serial.println(i);
    slaves[i].requestInProgress = true;


    Wire.beginTransmission(slaves[i].address);
    Wire.write("set 0"); 
    int result = Wire.endTransmission();    
    if(result == 0) {
      if(i==0)
        setKosmoTempoRegisters(now, 0, part.tempo);
      else if(i==1)
        setKosmoDrumSequencerRegisters(now, 1, part.drumSequencer);
      else if(i==2)
        setSamplerRegisters(now, 2, part.sampler);
    }
    Wire.beginTransmission(slaves[i].address);
    Wire.write("endset"); 
    Wire.endTransmission(); 

    slaves[i].requestInProgress = false;
  }
}



void startClock() {
  slaves[0].requestInProgress = true;  
  Wire.beginTransmission(slaves[0].address);
  Wire.write("start"); 
  Serial.println("here");
  int result = Wire.endTransmission(); 
  Serial.print("start status: ");
  Serial.println(result);
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