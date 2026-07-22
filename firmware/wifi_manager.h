#pragma once

#include <WiFi.h>
#include <WiFiManager.h>
#include "globals.h"
#include "lcd_display.h"

static WiFiManager wm;

static const uint8_t BOOT_BUTTON_PIN = 0;
static const unsigned long BOOT_HOLD_RESET_MS = 5000;

static bool bootPressedLast = false;
static bool bootResetTriggered = false;
static unsigned long bootPressStartMs = 0;

inline void clearSavedWiFiAndRestart() {
  showBootWifiScreen("XOA CAU HINH WIFI", "DANG KHOI DONG LAI", "", "");
  wm.resetSettings();
  WiFi.disconnect(true, true);
  delay(600);
  ESP.restart();
}

inline void handleBootButtonWiFiReset() {
  bool pressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);

  if (pressed && !bootPressedLast) {
    bootPressStartMs = millis();
    bootResetTriggered = false;
  }

  if (pressed && !bootResetTriggered) {
    unsigned long held = millis() - bootPressStartMs;
    if (held >= BOOT_HOLD_RESET_MS) {
      bootResetTriggered = true;
      clearSavedWiFiAndRestart();
      return;
    }
  }

  bootPressedLast = pressed;
}

inline void initWiFi() {
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  wm.setDebugOutput(false);
  wm.setConfigPortalTimeout(180);

  showBootWifiScreen("KHOI DONG HE THONG", "WIFI: Diem danh SV", "Mo AP cai dat WIFI", "IP: 192.168.4.1");

  bool ok = wm.autoConnect("Điểm danh SV");

  if (ok && WiFi.status() == WL_CONNECTED) {
    localIP = WiFi.localIP().toString();
    showBootWifiScreen("WIFI DA KET NOI", vnToAsciiUpper(WiFi.SSID()), "IP: " + localIP, "");
    delay(1200);
  } else {
    localIP = "0.0.0.0";
    showBootWifiScreen("WIFI CHUA SAN SANG", "MO AP CAI DAT", "TEN AP:", "   Diem danh SV");
    delay(1200);
  }
}

inline void handleWiFiReconnect() {
  handleBootButtonWiFiReset();

  if (WiFi.status() == WL_CONNECTED) {
    localIP = WiFi.localIP().toString();
    return;
  }

  localIP = "0.0.0.0";
}
