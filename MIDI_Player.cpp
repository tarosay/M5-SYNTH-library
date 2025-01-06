#include <FS.h>
#include <SPIFFS.h>
#include "MIDI_Player.hpp"

// グローバルインスタンスの宣言
//MIDI_Player MidiPlayer;

float VolumeScale = 1.0;
float TempoScale = 1.0;
uint8_t Volumes[VOLUME_CHANNEL];
// MIDIのタイムベース（ヘッダから取得するか、既知の場合は直接設定）
uint16_t ppqn = 96;  // パルス毎四分音符
// 初期テンポ（マイクロ秒毎四分音符）
uint32_t tempo = 500000;  // 120 BPM

uint8_t* midi_Data = nullptr;
uint16_t TrackCount = 0;
uint16_t FormatType = 0;

// コンストラクタ実装
MIDI_Player::MIDI_Player()
  : midiSerial(PIN_RXD, PIN_TXD)  // 定義済みのピンを使用
{
}

// コンストラクタ実装
MIDI_Player::MIDI_Player(int rxPin, int txPin)
  : midiSerial(rxPin, txPin)  // SoftwareSerial を初期化
{
}

template<typename T>
void SafeDelete(T*& ptr) {
  if (ptr != nullptr) {
    delete[] ptr;
    ptr = nullptr;
  }
}

// デストラクタで動的メモリを解放
MIDI_Player::~MIDI_Player() {
  SafeDelete(midi_Data);
  SafeDelete(startIndex);
  SafeDelete(endIndex);
  SafeDelete(currentIndex);
  SafeDelete(nextTime);
  SafeDelete(runningStatus);
  SafeDelete(tempos);
}

void MIDI_Player::begin() {
  begin(MaxTrack, VolumeScale, TempoScale);
}
void MIDI_Player::begin(int maxTrack) {
  begin(maxTrack, VolumeScale, TempoScale);
}
void MIDI_Player::begin(float volumeScale, float tempoScale) {
  begin(MaxTrack, volumeScale, tempoScale);
}
void MIDI_Player::begin(int maxTrack, float volumeScale, float tempoScale) {
  MaxTrack = maxTrack;
  VolumeScale = volumeScale;
  TempoScale = tempoScale;
  midiSerial.begin(MIDI_BAURATE);
  SPIFFS.begin(true);

  // 既存の配列を解放
  SafeDelete(startIndex);
  SafeDelete(endIndex);
  SafeDelete(currentIndex);
  SafeDelete(nextTime);
  SafeDelete(runningStatus);
  SafeDelete(tempos);

  // 新しいサイズで配列を確保
  startIndex = new unsigned int[MaxTrack];
  endIndex = new unsigned int[MaxTrack];
  currentIndex = new unsigned int[MaxTrack];
  nextTime = new unsigned long[MaxTrack];
  runningStatus = new uint8_t[MaxTrack];
  tempos = new uint32_t[MaxTrack];

  for (int i = 0; i < VOLUME_CHANNEL; i++) {
    Volumes[i] = 0;
  }
}

void MIDI_Player::play(const String spiffs_midiFilename) {
  File file = SPIFFS.open(spiffs_midiFilename.c_str(), FILE_READ);
  if (!file) {
    Serial.println("..Open Error!");
    return;
  }
  if (file.isDirectory()) {
    Serial.println("..Read Error!");
    file.close();
    return;
  }
  size_t size = file.size();

  SafeDelete(midi_Data);
  midi_Data = new uint8_t[size];

  if (midi_Data == nullptr) {
    Serial.println("out of memory!");
    file.close();
    return;
  }
  // ファイル内容を一気にバッファに読み込み
  file.read(midi_Data, size);
  file.close();

  play(midi_Data);
}

void MIDI_Player::play(const uint8_t* midiData) {
  unsigned int index = 0;

  // ヘッダチャンクの解析
  if (midiData[index] != 'M' || midiData[index + 1] != 'T' || midiData[index + 2] != 'h' || midiData[index + 3] != 'd') {
    // ヘッダチャンクが見つからない
    Serial.println("Invalid MIDI header");
    return;
  }
  index += 8;  // 'MThd'とチャンクサイズ（6バイト）をスキップ

  // フォーマットタイプ、トラック数、タイムディビジョンを読み込む
  FormatType = (midiData[index] << 8) | midiData[index + 1];
  index += 2;
  TrackCount = (midiData[index] << 8) | midiData[index + 1];
  index += 2;
  ppqn = (midiData[index] << 8) | midiData[index + 1];
  index += 2;

  // Serial.print("FormatType= ");
  // Serial.println(FormatType);
  // Serial.print("TrackCount= ");
  // Serial.println(TrackCount);
  // Serial.print("ppqn= ");
  // Serial.println(ppqn);
  // Serial.print("index= 0x");
  // Serial.println(index, HEX);

  if (TrackCount > MaxTrack) {
    TrackCount = MaxTrack;
  }

  startIndex[0] = index;
  unsigned long nowTime = millis();

  for (int i = 0; i < TrackCount; i++) {
    if (i > 0) {
      startIndex[i] = endIndex[i - 1];
    }
    nextTime[i] = 0;
    tempos[i] = 500000;
    runningStatus[i] = 0;

    index = startIndex[i];
    // トラックチャンクの解析
    if (midiData[index] != 'M' || midiData[index + 1] != 'T' || midiData[index + 2] != 'r' || midiData[index + 3] != 'k') {
      // トラックチャンクが見つからない
      Serial.println("Invalid Track header");
      currentIndex[i] = 0;
      endIndex[i] = 0;
    } else {
      currentIndex[i] = index + 4;
    }

    if (currentIndex[i] > 0) {
      index = currentIndex[i];
      // トラックチャンクサイズを読み込む
      uint32_t trackSize = (midiData[index] << 24) | (midiData[index + 1] << 16) | (midiData[index + 2] << 8) | midiData[index + 3];
      //Serial.print("trackSize= ");
      //Serial.println(trackSize);

      index += 4;
      endIndex[i] = index + trackSize;
      currentIndex[i] = index;
      setNextTime(midiData, i, nowTime);
    }

    // Serial.print("startIndex= 0x");
    // Serial.println(startIndex[i], HEX);
    //Serial.print("endIndex= 0x");
    //Serial.println(endIndex[i], HEX);
  }
  // Serial.print("endIndex= 0x");
  // Serial.println(endIndex[TrackCount - 1], HEX);
}

bool MIDI_Player::loop() {
  if (!hasTracksNotReachedEnd(TrackCount)) {
    SafeDelete(midi_Data);
    sendGMReset();
    return false;
  }

  for (int i = 0; i < TrackCount; i++) {
    if (nextTime[i] < millis()) {
      if (currentIndex[i] < endIndex[i]) {

        if (play_type1(midi_Data, i) == false) {
          currentIndex[i] = endIndex[i];
        }
        setNextTime(midi_Data, i, nextTime[i]);
      }
    }
  }

  return true;
}

void MIDI_Player::stop() {
  for (int i = 0; i < TrackCount; i++) {
    currentIndex[i] = endIndex[i] = 0;
  }
  SafeDelete(midi_Data);
  sendGMReset();
}

uint32_t MIDI_Player::readVarLen(const uint8_t* midiData, unsigned int& index) {
  uint32_t value = 0;
  uint8_t c;

  do {
    c = midiData[index++];
    value = (value << 7) | (c & 0x7F);
  } while (c & 0x80);

  return value;
}

void MIDI_Player::sendMidiMessage(uint8_t statusByte, uint8_t data1, uint8_t data2) {
  if (statusByte >= 0xB0 && statusByte < 0xB0 + 16 && data1 == 0x07) {
    Volumes[statusByte - 0xB0] = data2;
    float vol = (float)data2 * VolumeScale;
    vol = vol > 0x7F ? 0x7F : vol;
    data2 = ((uint8_t)vol) % 0x80;

    // Serial.print(statusByte, HEX);
    // Serial.print(" ");
    // Serial.print(data1, HEX);
    // Serial.print(" ");
    // Serial.println(data2, HEX);
  }
  midiSerial.write(statusByte);
  midiSerial.write(data1);
  if ((statusByte & 0xF0) != 0xC0 && (statusByte & 0xF0) != 0xD0) {
    midiSerial.write(data2);

    // Serial.print(statusByte, HEX);
    // Serial.print(" ");
    // Serial.print(data1, HEX);
    // Serial.print(" ");
    // Serial.println(data2, HEX);
  } else {
    // Serial.print(statusByte, HEX);
    // Serial.print(" ");
    //Serial.println(data1, HEX);
  }
}
void MIDI_Player::sendGMReset() {
  uint8_t gmReset[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
  for (uint8_t i = 0; i < sizeof(gmReset); i++) {
    midiSerial.write(gmReset[i]);
  }
  //Serial.println("GM Reset.");
}

void MIDI_Player::setNextTime(const uint8_t* midiData, int trackNum, unsigned long nowTime) {
  unsigned int index = currentIndex[trackNum];

  // 遅延時間の計算
  float delayTime = (float)(readVarLen(midiData, index) * tempos[trackNum]) / (ppqn * 1000.0 * TempoScale);
  // Serial.print("delayTime= ");
  // Serial.println(delayTime);
  nextTime[trackNum] = nowTime + (unsigned long)delayTime;
  currentIndex[trackNum] = index;
}

void MIDI_Player::setTempo(float value) {
  TempoScale = value;
  TempoScale = TempoScale < 0.125 ? 0.125 : TempoScale;
  TempoScale = TempoScale > 5 ? 5 : TempoScale;
}

void MIDI_Player::setVolume(float value) {
  VolumeScale = value;
  for (int i = 0; i < VOLUME_CHANNEL; i++) {
    sendMidiMessage(i + 0xB0, 7, Volumes[i]);
  }
}

bool MIDI_Player::play_type1(const uint8_t* midiData, int trackNum) {
  unsigned int index = currentIndex[trackNum];
  //Serial.print("index= 0x");
  //Serial.println(index, HEX);

  // イベントの読み込み（indexはここでインクリメントしない）
  uint8_t eventType = midiData[index];

  if (eventType == 0xFF) {
    index++;
    // メタイベント
    uint8_t metaType = midiData[index++];
    uint32_t length = readVarLen(midiData, index);
    if (metaType == 0x51) {
      // テンポ変更イベント
      tempos[trackNum] = (midiData[index] << 16) | (midiData[index + 1] << 8) | midiData[index + 2];
      index += 3;
    } else if (metaType == 0x2F) {
      // エンドオブトラック
      return false;
    } else {
      // その他のメタイベントをスキップ
      index += length;
    }
  } else if ((eventType & 0xF0) == 0xF0) {
    index++;
    // システムエクスクルーシブイベントをスキップ
    uint32_t length = readVarLen(midiData, index);
    index += length;
  } else {
    uint8_t statusByte;
    if (eventType & 0x80) {
      // ステータスバイトを持っている
      statusByte = midiData[index++];
      runningStatus[trackNum] = statusByte;
    } else {
      // ランニングステータスを使用
      statusByte = runningStatus[trackNum];
    }

    uint8_t command = statusByte & 0xF0;
    uint8_t data1 = midiData[index++];
    uint8_t data2 = 0;

    if (command != 0xC0 && command != 0xD0) {
      // データバイト2を読み込む
      data2 = midiData[index++];
    }

    // MIDIメッセージを送信
    sendMidiMessage(statusByte, data1, data2);
  }
  currentIndex[trackNum] = index;
  return true;
}

bool MIDI_Player::isRunning() {
  return hasTracksNotReachedEnd(TrackCount);
}

bool MIDI_Player::hasTracksNotReachedEnd(int trackCount) {
  for (int i = 0; i < trackCount; i++) {
    if (currentIndex[i] < endIndex[i]) {
      return true;  // 1つでも見つかったら true を返す
    }
  }
  return false;  // 全部達していたら false
}