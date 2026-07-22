#pragma once
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "globals.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

inline String fit20(const String &text) {
  String out = text;
  if (out.length() > 20) out = out.substring(0, 20);
  while (out.length() < 20) out += ' ';
  return out;
}

inline void lcdPrintAt(uint8_t row, const String &text) {
  String clean = vnToAsciiUpper(text);
  lcd.setCursor(0, row);
  lcd.print(fit20(clean));
}

inline String attendanceBottomLine(const String &mssv) {
  if (localIP == "0.0.0.0") return "IP: " + localIP;
  String value = mssv;
  value.trim();
  if (value.length() == 0) value = "--------";
  return "MSSV:" + value;
}

inline void initLCD() {
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
}

inline void showBootWifiScreen(const String &l1, const String &l2, const String &l3, const String &l4) {
  lcd.clear();
  lcdPrintAt(0, l1);
  lcdPrintAt(1, l2);
  lcdPrintAt(2, l3);
  lcdPrintAt(3, l4);
}

inline String deviceStatusLine(const String &device, bool ok) {
  return device + ": " + (ok ? "HOAT DONG" : "LOI");
}

inline void showHardwareCheckScreen(bool rfidOk, bool fingerOk, bool simOk, bool speakerOk) {
  lcd.clear();
  lcdPrintAt(0, deviceStatusLine("RFID522", rfidOk));
  lcdPrintAt(1, deviceStatusLine("AS608", fingerOk));
  lcdPrintAt(2, deviceStatusLine("SIM", simOk));
  lcdPrintAt(3, deviceStatusLine("MAX98357A", speakerOk));
}

inline void showAttendanceScreen() {
  lcd.clear();
  lcdPrintAt(0, "HE THONG DIEM DANH");
  lcdPrintAt(1, isCurrentAttendanceIn() ? "DIEM DANH SV VAO LOP" : "DIEM DANH SV RA VE");
  lcdPrintAt(2, "Ten: --------");
  lcdPrintAt(3, attendanceBottomLine(""));
}

inline void showAttendanceName(const String &ten, const String &maSV, bool isIn) {
  lcd.clear();
  lcdPrintAt(0, "HE THONG DIEM DANH");
  lcdPrintAt(1, isIn ? "DIEM DANH SV VAO LOP" : "DIEM DANH SV RA VE");
  lcdPrintAt(2, "Ten: " + ten);
  lcdPrintAt(3, attendanceBottomLine(maSV));
}

inline void showAttendanceError(const String &line2, const String &line3, const String &maSV = "--------") {
  lcd.clear();
  lcdPrintAt(0, "HE THONG DIEM DANH");
  lcdPrintAt(1, line2);
  lcdPrintAt(2, line3);
  lcdPrintAt(3, attendanceBottomLine(maSV));
}

inline void showWebServerOpenScreen() {
  lcd.clear();
  lcdPrintAt(0, "WEBSERVER DANG MO");
  lcdPrintAt(1, "THEM SINH VIEN");
  lcdPrintAt(2, "TRA CUU THONG TIN");
  lcdPrintAt(3, "IP: " + localIP);
}

inline void showRegisterCardStep(const String &uid, const String &status) {
  lcd.clear();
  lcdPrintAt(0, "BAT DAU DANG KY");
  lcdPrintAt(1, "VUI LONG QUET THE");
  lcdPrintAt(2, "ID: " + uid);
  lcdPrintAt(3, status);
}

inline void showRegisterFingerStep(const String &status) {
  lcd.clear();
  lcdPrintAt(0, "DANG KY VAN TAY");
  lcdPrintAt(1, "VUI LONG DAT TAY");
  lcdPrintAt(2, status);
  lcdPrintAt(3, "IP: " + localIP);
}

inline void showRegisterSummaryScreen() {
  lcd.clear();
  lcdPrintAt(0, "THEM SINH VIEN");
  lcdPrintAt(1, cardCaptured ? "Da quet the moi" : "Chua quet the moi");
  lcdPrintAt(2, fingerCaptured ? "Da dang ky tay" : "Chua dang ky tay");
  lcdPrintAt(3, readyToSave ? "San sang luu" : "Chua du dieu kien");
}

inline void showSaveSuccessScreen(const String &ten, const String &maSV) {
  lcd.clear();
  lcdPrintAt(0, "HE THONG DIEM DANH");
  lcdPrintAt(1, "Luu thanh cong");
  lcdPrintAt(2, ten);
  lcdPrintAt(3, attendanceBottomLine(maSV));
}
