//#pragma once
#ifndef _MIDI_PLAYER_HPP_
#define _MIDI_PLAYER_HPP_

#include <Arduino.h>
#include <SoftwareSerial.h>

#define PIN_RXD 1
#define PIN_TXD 2
#define MIDI_BAURATE 31250
#define VOLUME_CHANNEL 16

class MIDI_Player {
public:
  MIDI_Player();
  MIDI_Player(int txPin, int rxPin);  // コンストラクタ
  ~MIDI_Player();                     // デストラクタでメモリ解放

  void begin();
  void begin(int maxTrack);
  void begin(float volumeScale, float tempoScale);
  void begin(int maxTrack, float volumeScale, float tempoScale);

  void play(const String spiffs_midiFilename);
  bool loop();
  void stop();
  bool isRunning();

  void setTempo(float value);
  void setVolume(float value);

private:
  SoftwareSerial midiSerial;
  int MaxTrack = 16;                     // トラックの最大数
  unsigned int* startIndex = nullptr;    // トラックの開始インデックス
  unsigned int* endIndex = nullptr;      // トラックの終了インデックス
  unsigned int* currentIndex = nullptr;  // 現在の再生インデックス
  unsigned long* nextTime = nullptr;     // 次のイベント時間
  uint8_t* runningStatus = nullptr;      // 各トラックのステータス
  uint32_t* tempos = nullptr;            // 各トラックのテンポ

  void play(const uint8_t* midiData);
  bool play_type1(const uint8_t* midiData, int trackNum);
  uint32_t readVarLen(const uint8_t* midiData, unsigned int& index);
  void sendMidiMessage(uint8_t statusByte, uint8_t data1, uint8_t data2);
  void sendGMReset();
  void setNextTime(const uint8_t* midiData, int trackNum, unsigned long nowTime);
  bool hasTracksNotReachedEnd(int trackCount);
};

// グローバルインスタンスの宣言
//extern MIDI_Player MidiPlayer;

#endif  // _MIDI_PLAYER_HPP_