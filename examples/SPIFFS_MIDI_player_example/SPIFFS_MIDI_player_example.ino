#include <FS.h>
#include <SPIFFS.h>
#include <vector>
#include <string>

#include <M5NanoC6.h>
#include "MIDI_Player.hpp"

MIDI_Player MidiPlayer;

int currentFileIndex = 0;  // 現在のMIDIファイルのインデックス
bool isPlaying = false;

void setup() {
  NanoC6.begin();
  MidiPlayer.begin();

  pinMode(M5NANO_C6_BTN_PIN, INPUT_PULLUP);  // プルアップでボタンの状態を監視
}

//**********************************
// 拡張子を指定してファイルを探す
//**********************************
std::vector<std::string> getFilesWithExtension(const char* directoryPath, const char* extension) {
  std::vector<std::string> files;

  File root = SPIFFS.open(directoryPath);
  if (!root || !root.isDirectory()) {
    Serial.println("ディレクトリを開けませんでした");
    return files;
  }

  File file = root.openNextFile();
  while (file) {
    std::string fileName = file.name();

    // 拡張子をチェック
    if (fileName.length() > strlen(extension)) {
      if (fileName.substr(fileName.length() - strlen(extension)) == extension) {
        files.push_back(fileName);
      }
    }

    file = root.openNextFile();
  }

  return files;
}

void playMidiFile() {
  //拡張子が.mid のファイルをFFSから取り出します。
  const char* directoryPath = "/";
  const char* extension = ".mid";
  std::vector<std::string> midFiles = getFilesWithExtension(directoryPath, extension);
  int numMidFiles = midFiles.size();
  if (numMidFiles == 0) {
    Serial.println(".mid Files Non!");
    return;
  }
  currentFileIndex = currentFileIndex % numMidFiles;

  Serial.print(midFiles[currentFileIndex].c_str());
  Serial.println(" play.");
  std::string midFile = "/" + midFiles[currentFileIndex];

  //ノーマル音量
  {
    MidiPlayer.play(String(midFile.c_str()));

    while (MidiPlayer.isRunning()) {
      MidiPlayer.loop();
      if (digitalRead(M5NANO_C6_BTN_PIN) == LOW) {
        while (digitalRead(M5NANO_C6_BTN_PIN) == LOW)
          ;
        break;
      }
    }
    MidiPlayer.stop();
  }

  //音量を大小させる
  {
    Serial.println("音量を大小させる");
    float step = 1;
    float volume = 1.0;
    MidiPlayer.play(String(midFile.c_str()));

    while (MidiPlayer.isRunning()) {
      MidiPlayer.loop();
      if (digitalRead(M5NANO_C6_BTN_PIN) == LOW) {
        while (digitalRead(M5NANO_C6_BTN_PIN) == LOW)
          ;
        break;
      }

      volume += 0.01 * step;
      if (volume > 1.0) {
        step = -1;
        volume = 1;
      } else if (volume < 0) {
        step = 1;
        volume = 0;
      }
      MidiPlayer.setVolume(volume);
    }
    MidiPlayer.stop();
    MidiPlayer.setVolume(1.0);
  }

  //速度を変化させる
  {
    Serial.println("速度を変化させる");
    float step = 1;
    float tempo = 1.0;
    MidiPlayer.play(String(midFile.c_str()));

    while (MidiPlayer.isRunning()) {
      MidiPlayer.loop();
      if (digitalRead(M5NANO_C6_BTN_PIN) == LOW) {
        while (digitalRead(M5NANO_C6_BTN_PIN) == LOW)
          ;
        break;
      }

      tempo += 0.001 * step;
      if (tempo > 2.0) {
        step = -1;
        tempo = 2;
      } else if (tempo < 0.125) {
        step = 1;
        tempo = 0.125;
      }
      MidiPlayer.setTempo(tempo);
    }
    MidiPlayer.stop();
    MidiPlayer.setTempo(1.0);
  }

  currentFileIndex = (currentFileIndex + 1) % numMidFiles;
}


void loop() {
  // ボタンが押された場合
  if (digitalRead(M5NANO_C6_BTN_PIN) == LOW && !isPlaying) {
    delay(50);                                    // デバウンス用の遅延
    if (digitalRead(M5NANO_C6_BTN_PIN) == LOW) {  // 再確認
      while (digitalRead(M5NANO_C6_BTN_PIN) == LOW)
        ;

      isPlaying = true;  // 演奏フラグをセット
      playMidiFile();
      isPlaying = false;  // 演奏完了後にフラグをリセット
      while (digitalRead(M5NANO_C6_BTN_PIN) == LOW)
        ;
    }
  }
}
