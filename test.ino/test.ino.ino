
// ===========라이브러리============

// DFRobotDFPlayerMini 버전 1.0.6

// ===========라이브러리============

#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

#define FPSerial Serial1

// ESP32-S3 UART 핀 (DFPlayer 연결)
static const int RX_PIN = 44;  // DFPlayer TX -> ESP32 RX
static const int TX_PIN = 43;  // DFPlayer RX <- ESP32 TX

DFRobotDFPlayerMini mp3;

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:               Serial.println(F("[DFP] Time Out")); break;
    case WrongStack:            Serial.println(F("[DFP] Wrong Stack")); break;
    case DFPlayerCardInserted:  Serial.println(F("[DFP] Card Inserted")); break;
    case DFPlayerCardRemoved:   Serial.println(F("[DFP] Card Removed")); break;
    case DFPlayerCardOnline:    Serial.println(F("[DFP] Card Online")); break;
    case DFPlayerUSBInserted:   Serial.println(F("[DFP] USB Inserted")); break;
    case DFPlayerUSBRemoved:    Serial.println(F("[DFP] USB Removed")); break;
    case DFPlayerPlayFinished:  Serial.printf("[DFP] Play Finished: %d\n", value); break;
    case DFPlayerError:
      Serial.print(F("[DFP] Error: "));
      switch (value) {
        case Busy:               Serial.println(F("Card not found")); break;
        case Sleeping:           Serial.println(F("Sleeping")); break;
        case SerialWrongStack:   Serial.println(F("Wrong Stack")); break;
        case CheckSumNotMatch:   Serial.println(F("Checksum Not Match")); break;
        case FileIndexOut:       Serial.println(F("File Index Out")); break;
        case FileMismatch:       Serial.println(F("Cannot Find File")); break;
        case Advertise:          Serial.println(F("In Advertise")); break;
        default:                 Serial.println(F("Unknown")); break;
      }
      break;
    default: break;
  }
}

void showHelp() {
  Serial.println(F("=== DFPlayer UART Command Help ==="));
  Serial.println(F("p<N>        : play track N (e.g., p1)"));
  Serial.println(F("n           : next"));
  Serial.println(F("b           : previous (back)"));
  Serial.println(F("pause       : pause"));
  Serial.println(F("start       : start/resume"));
  Serial.println(F("stop        : stop"));
  Serial.println(F("vol <0-30>  : set volume (e.g., vol 20)"));
  Serial.println(F("eq <mode>   : normal|pop|rock|jazz|classic|bass"));
  Serial.println(F("mp3 <N>     : play /MP3/000N.mp3 (e.g., mp3 4)"));
  Serial.println(F("f <F> <N>   : play /FF/NNN.mp3 (e.g., f 2 7 -> /02/007.mp3)"));
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

  // 소문자 통일
  String lower = line;
  lower.toLowerCase();

  if (lower == "help") { showHelp(); return; }
  if (lower == "n")    { mp3.next(); Serial.println(F("OK: next")); return; }
  if (lower == "b")    { mp3.previous(); Serial.println(F("OK: previous")); return; }
  if (lower == "pause"){ mp3.pause(); Serial.println(F("OK: pause")); return; }
  if (lower == "start"){ mp3.start(); Serial.println(F("OK: start")); return; }
  if (lower == "stop") { mp3.stop();  Serial.println(F("OK: stop"));  return; }

  // p<N>
  if (lower.charAt(0) == 'p') {
    long n = lower.substring(1).toInt();
    if (n > 0) {
      mp3.play(n);
      Serial.printf("OK: play %ld\n", n);
    } else {
      Serial.println(F("ERR: usage p<N> (e.g., p1)"));
    }
    return;
  }

  // vol <0-30>
  if (lower.startsWith("vol ")) {
    int v = lower.substring(4).toInt();
   /* if(v > 20){
      Serial.printf("Max Volume is 20\n");
      return;
    }
    */
    //v = constrain(v, 0, 20);
    v = constrain(v, 0, 30);
    mp3.volume(v);
    Serial.printf("OK: volume %d\n", v);
    return;
  }

  // eq <mode>
  if (lower.startsWith("eq ")) {
    String mode = lower.substring(3);
    mp3.EQ(eqFromString(mode));
    Serial.printf("OK: eq %s\n", mode.c_str());
    return;
  }

  // mp3 <N> -> /MP3/000N.mp3
  if (lower.startsWith("mp3 ")) {
    long n = lower.substring(4).toInt();
    if (n >= 0) {
      mp3.playMp3Folder(n);
      Serial.printf("OK: mp3 %ld\n", n);
    } else {
      Serial.println(F("ERR: usage mp3 <N>"));
    }
    return;
  }

  // f <F> <N> -> /FF/NNN.mp3
  if (lower.startsWith("f ")) {
    // 공백 토큰 분리
    int sp1 = lower.indexOf(' ');
    if (sp1 > 0) {
      int sp2 = lower.indexOf(' ', sp1 + 1);
      if (sp2 > 0) {
        int folder = lower.substring(sp1 + 1, sp2).toInt();
        int file   = lower.substring(sp2 + 1).toInt();
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
    return;
  }

  if (lower == "state") {
    Serial.printf("state=%d\n", mp3.readState());
    Serial.printf("volume=%d\n", mp3.readVolume());
    Serial.printf("eq=%d\n", mp3.readEQ());
    Serial.printf("total_files=%d\n", mp3.readFileCounts());
    Serial.printf("current_file=%d\n", mp3.readCurrentFileNumber());
    // 예: 특정 폴더(3)의 파일 수
    Serial.printf("folder3_files=%d\n", mp3.readFileCountsInFolder(3));
    return;
  }

  Serial.println(F("ERR: unknown command (help to list)"));
}

void setup() {
Serial.begin(115200);
  delay(1000);  // 충분한 지연시간

  Serial.println("\n[ESP32S3] booting...");
  
  // UART 초기화 전에 핀 상태 확인
  Serial.printf("TX Pin: %d, RX Pin: %d\n", TX_PIN, RX_PIN);
  
  FPSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(500);  // UART 안정화 대기

  Serial.println("[ESP32S3] DFPlayer init...");
  
  // 더 긴 타임아웃으로 재시도
  mp3.setTimeOut(3000);
  
  if (!mp3.begin(FPSerial, true, true)) {
    Serial.println("DFPlayer begin failed. Retrying...");
    delay(1000);
    
    // 재시도
    if (!mp3.begin(FPSerial, false, false)) {
      Serial.println("DFPlayer init failed completely.");
      Serial.println("Check: 1)Wiring 2)SD card 3)Power");
      while (true) {
        Serial.println("Stuck in error loop...");
        delay(5000);
      }
    }
  }
  
  Serial.println("DFPlayer online!");

  mp3.setTimeOut(500);
  mp3.outputDevice(DFPLAYER_DEVICE_SD);
  mp3.EQ(DFPLAYER_EQ_NORMAL);
  mp3.volume(20);

  showHelp();
}

void loop() {
  // 터미널에서 한 줄 단위로 읽기
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;   // CR 무시(윈도우 CRLF 대응)
    if (c == '\n') {           // LF에서 커맨드 실행
      handleLine(line);
      line = "";
    } else {
      line += c;
      if (line.length() > 100) line = ""; // 간단한 오버런 방어
    }
  }

  // DFPlayer 이벤트 처리
  if (mp3.available()) {
    printDetail(mp3.readType(), mp3.read());
  }
}