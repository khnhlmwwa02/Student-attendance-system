#pragma once

#include <WiFi.h>
#include <Audio.h>

// ESP32-S3 Dev Module + MAX98357A
// DIN   -> GPIO40
// BCLK  -> GPIO39
// LRC   -> GPIO38
// SD pin on MAX98357A can leave floating or tie depending on mono channel need.
// This sketch expects an ESP32-compatible Audio library that provides:
// setPinout(), setVolume(), loop(), isRunning(), stopSong(), connecttospeech().

static const int SPK_DIN_PIN  = 40;
static const int SPK_BCLK_PIN = 39;
static const int SPK_LRC_PIN  = 38;
static const int SPK_VOLUME   = 14;   // 0..21

static Audio gAudio;
static bool gAudioReady = false;
static bool gSpeechBusy = false;
static unsigned long gLastSpeechMs = 0;

inline String speakerUrlEncode(const String &input) {
  String out;
  char hex[4];
  for (size_t i = 0; i < input.length(); i++) {
    uint8_t c = (uint8_t)input[i];
    bool safe =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' || c == '_' || c == '.' || c == '~';

    if (safe) {
      out += (char)c;
    } else if (c == ' ') {
      out += "%20";
    } else {
      snprintf(hex, sizeof(hex), "%%%02X", c);
      out += hex;
    }
  }
  return out;
}

inline void initSpeaker() {
  gAudio.setPinout(SPK_BCLK_PIN, SPK_LRC_PIN, SPK_DIN_PIN);
  gAudio.setVolume(SPK_VOLUME);
  gAudioReady = true;
  gSpeechBusy = false;
}

inline void speakerLoop() {
  if (!gAudioReady) return;
  gAudio.loop();
  if (gSpeechBusy && !gAudio.isRunning()) {
    gSpeechBusy = false;
  }
}

inline void speakerStop() {
  if (!gAudioReady) return;
  gAudio.stopSong();
  gSpeechBusy = false;
}

inline bool speakerIsBusy() {
  return gSpeechBusy;
}

inline bool isSpeakerReady() {
  return gAudioReady;
}

inline bool speakerCanSpeak() {
  if (!gAudioReady) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (gSpeechBusy) return false;
  return true;
}

inline bool speakTextVi(const String &text) {
  if (!speakerCanSpeak()) return false;

  // Thu vien Audio dung tren ESP32 se dung text goc co dau de doc tieng Viet.
  bool ok = gAudio.connecttospeech(text.c_str(), "vi");
  if (ok) {
    gSpeechBusy = true;
    gLastSpeechMs = millis();
  }
  return ok;
}

inline bool speakAttendance(const String &ho, const String &ten, const String &maSV) {
  String fullName = ho;
  if (fullName.length() > 0 && ten.length() > 0) fullName += " ";
  fullName += ten;

  String speech = "Xác nhận điểm danh sinh viên " + fullName;
  return speakTextVi(speech);
}

inline bool speakRegisterSuccess(const String &ho, const String &ten, const String &maSV) {
  String fullName = ho;
  if (fullName.length() > 0 && ten.length() > 0) fullName += " ";
  fullName += ten;

  String speech = "Đã thêm sinh viên " + fullName + " thành công";
  return speakTextVi(speech);
}
