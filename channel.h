#ifndef Channel_h
#define Channel_h

#include "DebounceButton165.h"

typedef void (*PartCompleted)(uint8_t, uint8_t); // channel number, chainTo
typedef void (*PartStarted)(uint8_t); // channel number
typedef void (*PartStopped)(uint8_t); // channel number

class Channel {
private:
  uint8_t _channelNumber;
  DebounceButton165 *_button;

  uint16_t _pageCountRaw = 0;
  uint16_t _repeatsRaw = 0;
  uint16_t _chainToRaw = 0;
  uint8_t _pageCount = 0;
  uint8_t _repeats = 0;
  int8_t _chainTo = -1;
  uint8_t _remainingRepeats = 0;
  bool _started = false;

  bool _hasPulse = false;
  uint8_t _currentStep = 0;
  uint8_t _lastStep = 0;
  uint8_t _currentPage = 0;
  bool _pageLedState[4] = {false};
  unsigned long _lastCurrentPageLedTime = 0;
  bool _quaterNodeEdge = false;

  // events
  PartCompleted _onPartCompleted = nullptr;
  PartStarted _onPartStarted = nullptr;
  PartStopped _onPartStopped = nullptr;

public:
  Channel() {
    _channelNumber = 0;
  }

  Channel(uint8_t channelNumber, uint8_t bitIndex) : _channelNumber(channelNumber) {
    _button = new DebounceButton165(bitIndex);
  }

  DebounceButton165* Button() {
    return _button;
  }

  void OnPartCompleted(PartCompleted handler) {
    _onPartCompleted = handler;
  }

  void OnPartStarted(PartStarted handler) {
    _onPartStarted = handler;
  }

  void OnPartStopped(PartStopped handler) {
    _onPartStopped = handler;
  }

  void Start() {
    _remainingRepeats = (_repeats > 0) ? _repeats-1 : 0;
    _started = true;
    if(_onPartStarted)
      _onPartStarted(_channelNumber);
  }

  void Stop() {
    _started = false;
    for(int i=0; i<4; i++) {
      _pageLedState[i] = i < _pageCount;
    }        
    if(_onPartStopped)
      _onPartStopped(_channelNumber);
  }

  bool IsStarted() {
    return _started;
  }

  void Reset() {
    _started = false;
    for(int i=0; i<4; i++) {
      _pageLedState[i] = i < _pageCount;
    }     
    _currentPage = 0;
    _currentStep = 0;
    _remainingRepeats = _repeats;
  }

  void Pulse(uint8_t pulses) {
    if(!_started) return;
    // happens 24 times pr quarter node 
    _hasPulse = true;
    if(pulses % 6 == 0) { // every 16th note
      if(_currentStep == _lastStep && _remainingRepeats == 0) {
        if(_onPartCompleted)
          _onPartCompleted(_channelNumber, _chainTo);
        else
          Stop();
      }
      
      if(_currentStep < _lastStep)
        _currentStep++;
      else {
        _currentStep=0;
        _remainingRepeats--;
      }      

      _currentPage = _currentStep / 16;
    }
    _quaterNodeEdge = pulses % 12 == 0;
  }

  void Run(unsigned long now) {
    if(!_started) return;
    if(_quaterNodeEdge) {
      _quaterNodeEdge = false;
      for(int i=0; i<4; i++) {
        if(i != _currentPage)
          _pageLedState[i] = i < _pageCount;
        else
          _pageLedState[i] = !_pageLedState[i];
      }
    }
  }

  bool PageLedState(int page) {
    return _pageLedState[page];
  }

  uint8_t CurrentPage() {
    return _currentPage;
  }

  uint8_t PageCount() {
    return _pageCount;
  }

  uint8_t Repeats() {
    return _repeats;
  }

  uint8_t RemainingRepeats() {
    return _remainingRepeats;
  }

  uint8_t ChainTo() {
    return _chainTo;
  }

  void SetPageCount(uint8_t pageCount) {
    _pageCount = pageCount;
    _lastStep = pageCount * 16;
    for(int i=0; i<4; i++) {
      _pageLedState[i] = i < _pageCount;
    }    
  }

  void SetPageCountRaw(uint16_t raw) {
    _pageCountRaw = raw;
    SetPageCount(map(raw, 0, 1000, 0, 4));
  }

  void SetRepeats(uint8_t repeats) {
    _repeats = repeats;
    _repeatsRaw = map(_repeats, 0, 32, 0, 1023);
  }

  void SetRepeatsRaw(uint16_t raw) {
    _repeatsRaw = raw;
    SetRepeats(map(raw, 0, 1023, 0, 32));
  }

  void SetChainTo(int8_t chainTo) {
    _chainTo = chainTo;
    _chainToRaw = map(_chainTo, -1, 15, 0, 1023);
  }

  void SetChainToRaw(uint16_t raw) {
    _chainToRaw = raw;
    SetChainTo(map(raw, 0, 1023, -1, 15));
  }

  void SetLastStep(uint8_t value) {
    _lastStep = value;
    int pageCount = (_lastStep == 0) ? 0 : ((_lastStep < 16) ? 1 : (_lastStep+1) / 16);
    SetPageCount(pageCount);
  }
};


#endif