#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

#define FPSerial Serial1

// ESP32-S3 UART 핀 (DFPlayer 연결)
static const int RX_PIN = 18;  // DFPlayer TX → ESP32 RX
static const int TX_PIN = 17;  // DFPlayer RX ← ESP32 TX

DFRobotDFPlayerMini mp3;

// SD카드 상태 정의
enum SDStatus {
  SD_NO_CARD = 0,     // SD카드 없음 (cnt = -1)
  SD_EMPTY = 1,       // SD카드 있지만 파일 없음 (cnt = 0)
  SD_HAS_FILES = 2    // SD카드 있고 파일도 있음 (cnt > 0)
};

// SD카드 상태 확인
SDStatus getSDStatus() {
    int cnt = mp3.readFileCounts();
    int state = mp3.readState();
    
    // 라이브러리 버그 우회: 둘 다 0이면 SD카드 없다고 판정
    if (cnt == 0 && state == 0) {
        return SD_NO_CARD;
    }
    
    if (cnt < 0) return SD_NO_CARD;
    if (cnt == 0) return SD_EMPTY;
    return SD_HAS_FILES;
}

// 재생 명령용: 파일이 있어야 함
bool requireFilesForPlayback() {
  SDStatus status = getSDStatus();
  if (status == SD_NO_CARD) {
    Serial.println(F("ERR: SD Card not detected"));
    return false;
  }
  if (status == SD_EMPTY) {
    Serial.println(F("ERR: SD Card empty (no MP3 files found)"));
    return false;
  }
  return true;
}

// 설정 명령용: SD카드만 있으면 됨
bool requireSDCard() {
  SDStatus status = getSDStatus();
  if (status == SD_NO_CARD) {
    Serial.println(F("ERR: SD Card not detected"));
    return false;
  }
  return true;
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:          Serial.println(F("[DFP] Time Out")); break;
    case WrongStack:       Serial.println(F("[DFP] Wrong Stack")); break;
    case DFPlayerCardInserted: Serial.println(F("[DFP] Card Inserted")); break;
    case DFPlayerCardRemoved:  Serial.println(F("[DFP] Card Removed")); break;
    case DFPlayerCardOnline:   Serial.println(F("[DFP] Card Online")); break;
    case DFPlayerUSBInserted:  Serial.println(F("[DFP] USB Inserted")); break;
    case DFPlayerUSBRemoved:   Serial.println(F("[DFP] USB Removed")); break;
    case DFPlayerPlayFinished: Serial.printf("[DFP] Play Finished: %d\n", value); break;
    case DFPlayerError:
      Serial.print(F("[DFP] Error: "));
      switch (value) {
        case Busy:              Serial.println(F("Card not found / Busy")); break;
        case Sleeping:          Serial.println(F("Sleeping")); break;
        case SerialWrongStack:  Serial.println(F("Wrong Stack")); break;
        case CheckSumNotMatch:  Serial.println(F("Checksum Not Match")); break;
        case FileIndexOut:      Serial.println(F("File Index Out")); break;
        case FileMismatch:      Serial.println(F("Cannot Find File")); break;
        case Advertise:         Serial.println(F("In Advertise")); break;
        default:                Serial.println(F("Unknown")); break;
      }
      break;
    default: break;
  }
}

void showHelp() {
  Serial.println(F("=== DFPlayer UART Command Help ==="));
  Serial.println(F("p<N>        : play global index N (e.g., p1)"));
  Serial.println(F("n           : next"));
  Serial.println(F("b           : previous"));
  Serial.println(F("pause       : pause"));
  Serial.println(F("start       : start/resume"));
  Serial.println(F("stop        : stop"));
  Serial.println(F("vol <0-30>  : set volume (e.g., vol 20)"));
  Serial.println(F("eq <mode>   : normal|pop|rock|jazz|classic|bass"));
  Serial.println(F("mp3 <N>     : play /MP3/000N.mp3"));
  Serial.println(F("f <F> <N>   : play /FF/NNN.mp3"));
  Serial.println(F("state       : print state/volume/EQ/current/filecounts"));
  Serial.println(F("help        : show this help"));
  Serial.println(F("=================================="));
}

uint8_t eqFromString(const String& s) {
  if (s == "normal")  return DFPLAYER_EQ_NORMAL;
  if (s == "pop")     return DFPLAYER_EQ_POP;
  if (s == "rock")    return DFPLAYER_EQ_ROCK;
  if (s == "jazz")    return DFPLAYER_EQ_JAZZ;
  if (s == "classic") return DFPLAYER_EQ_CLASSIC;
  if (s == "bass")    return DFPLAYER_EQ_BASS;
  return DFPLAYER_EQ_NORMAL;
}

void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  String lower = line;
  lower.toLowerCase();

  // 도움말은 항상 가능
  if (lower == "help") { showHelp(); return; }

  // 재생 관련 명령들 - 파일이 있어야 함
  if (lower == "n") {
    if (requireFilesForPlayback()) {
      mp3.next();
      Serial.println(F("OK: next"));
    }
    return;
  }
  
  if (lower == "b") {
    if (requireFilesForPlayback()) {
      mp3.previous();
      Serial.println(F("OK: previous"));
    }
    return;
  }
  
  if (lower == "pause") {
    if (requireFilesForPlayback()) {
      mp3.pause();
      Serial.println(F("OK: pause"));
    }
    return;
  }
  
  if (lower == "start") {
    if (requireFilesForPlayback()) {
      mp3.start();
      Serial.println(F("OK: start"));
    }
    return;
  }
  
  if (lower == "stop") {
    if (requireFilesForPlayback()) {
      mp3.stop();
      Serial.println(F("OK: stop"));
    }
    return;
  }

  // p<N> - 파일이 있어야 함
  if (lower.charAt(0) == 'p') {
    if (requireFilesForPlayback()) {
      long n = lower.substring(1).toInt();
      if (n > 0) {
        mp3.play(n);
        Serial.printf("OK: play %ld\n", n);
      } else {
        Serial.println(F("ERR: usage p<N> (e.g., p1)"));
      }
    }
    return;
  }

  // mp3 <N> - 파일이 있어야 함
  if (lower.startsWith("mp3 ")) {
    if (requireFilesForPlayback()) {
      long n = lower.substring(4).toInt();
      if (n >= 0) {
        mp3.playMp3Folder(n);
        Serial.printf("OK: mp3 %ld\n", n);
      } else {
        Serial.println(F("ERR: usage mp3 <N>"));
      }
    }
    return;
  }

  // f <F> <N> - 파일이 있어야 함
  if (lower.startsWith("f ")) {
    if (requireFilesForPlayback()) {
      int sp1 = lower.indexOf(' ');
      if (sp1 > 0) {
        int sp2 = lower.indexOf(' ', sp1 + 1);
        if (sp2 > 0) {
          int folder = lower.substring(sp1 + 1, sp2).toInt();
          int file = lower.substring(sp2 + 1).toInt();
          if (folder >= 1 && folder <= 99 && file >= 1 && file <= 255) {
            mp3.playFolder(folder, file);
            Serial.printf("OK: /%02d/%03d.mp3\n", folder, file);
          } else {
            Serial.println(F("ERR: f <folder 1..99> <file 1..255>"));
          }
          return;
        }
      }
      Serial.println(F("ERR: usage f <folder> <file>"));
    }
    return;
  }

  // 볼륨 설정 - SD카드만 있으면 됨 (파일 없어도 됨)
  if (lower.startsWith("vol ")) {
    if (requireSDCard()) {
      int v = lower.substring(4).toInt();
      v = constrain(v, 0, 30);
      mp3.volume(v);
      Serial.printf("OK: volume %d\n", v);
    }
    return;
  }

  // EQ 설정 - SD카드 없어도 가능
  if (lower.startsWith("eq ")) {
    String mode = lower.substring(3);
    mp3.EQ(eqFromString(mode));
    Serial.printf("OK: eq %s\n", mode.c_str());
    return;
  }

  // 상태 확인 - 항상 가능
  if (lower == "state") {
    int st = mp3.readState();
    int cnt = mp3.readFileCounts();
    SDStatus status = getSDStatus();
    
    Serial.printf("state=%d\n", st);
    Serial.printf("volume=%d\n", mp3.readVolume());
    Serial.printf("eq=%d\n", mp3.readEQ());
    Serial.printf("total_files=%d\n", cnt);
    Serial.printf("current_file=%d\n", mp3.readCurrentFileNumber());
    Serial.printf("folder3_files=%d\n", mp3.readFileCountsInFolder(3));
    
    // SD카드 상태 표시
    switch (status) {
      case SD_NO_CARD:
        Serial.println("SD_Status=NO_CARD");
        break;
      case SD_EMPTY:
        Serial.println("SD_Status=EMPTY");
        break;
      case SD_HAS_FILES:
        Serial.println("SD_Status=HAS_FILES");
        break;
    }
    Serial.println();
    return;
  }

  Serial.println(F("ERR: unknown command (help to list)"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n[ESP32S3] booting...");
  Serial.printf("TX Pin: %d, RX Pin: %d\n", TX_PIN, RX_PIN);

  FPSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(500);

  Serial.println("[ESP32S3] DFPlayer init...");
  mp3.begin(FPSerial, false, false); // 결과 체크 생략
  delay(1200);

  mp3.setTimeOut(500);
  mp3.outputDevice(DFPLAYER_DEVICE_SD);
  mp3.EQ(DFPLAYER_EQ_NORMAL);
  mp3.volume(20);

  Serial.println("DFPlayer initialized.");
  
  // 초기 SD카드 상태 확인
  SDStatus status = getSDStatus();
  switch (status) {
    case SD_NO_CARD:
      Serial.println("SD Status: No card detected");
      break;
    case SD_EMPTY:
      Serial.println("SD Status: Card detected but no MP3 files");
      break;
    case SD_HAS_FILES:
      Serial.printf("SD Status: Card detected with %d files\n", mp3.readFileCounts());
      break;
  }
  
  showHelp();
}

void loop() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { 
      handleLine(line); 
      line = ""; 
    } else {
      line += c;
      if (line.length() > 100) line = "";
    }
  }
  
  // 이벤트 처리
  if (mp3.available()) {
    printDetail(mp3.readType(), mp3.read());
  }
}