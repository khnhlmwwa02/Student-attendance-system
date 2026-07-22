#pragma once

#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "globals.h"

static const char* GG_SHEET_URL = "https://script.google.com/macros/s/AKfycbyUkxsVBYpsWJiNR6HHuAZyb8BqRiUOaHQ8Rx7UVZOQYb7YfGCyFVDwL6wrLMiGeBwK/exec";

struct GoogleSheetJob {
  char maSV[16];
  char ho[24];
  char ten[16];
  char lop[16];
  char sdt[16];
  char trangThai[8];
};

static QueueHandle_t gGoogleSheetQueue = nullptr;
static TaskHandle_t gGoogleSheetTask = nullptr;
static const uint8_t GOOGLE_SHEET_QUEUE_LEN = 10;

inline String getFormattedTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return String(buf);
  }
  return "N/A";
}

inline void initNTPTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

inline String urlEncode(const String &str) {
  String encoded = "";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

inline void sendToGoogleSheet(const StudentInfo &student) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String thoiGian = getFormattedTime();
  String trangThai = student.ghiChu;
  if (trangThai != "IN" && trangThai != "OUT") {
    trangThai = isCurrentAttendanceIn() ? "IN" : "OUT";
  }
  String postData = "mssv=" + urlEncode(student.maSV)
                  + "&ho=" + urlEncode(student.ho)
                  + "&ten=" + urlEncode(student.ten)
                  + "&lop=" + urlEncode(student.lop)
                  + "&sdt=" + urlEncode(student.sdt)
                  + "&thoigian=" + urlEncode(thoiGian)
                  + "&trangthai=" + urlEncode(trangThai);

  Serial.println("[GSheet] Noi dung gui:");
  Serial.println("mssv: " + student.maSV);
  Serial.println("ho va ten: " + student.ho + " " + student.ten);
  Serial.println("lop: " + student.lop);
  Serial.println("sdt: " + student.sdt);
  Serial.println("thoi gian: " + thoiGian);
  Serial.println("trang thai: " + trangThai);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(10000);

  if (!http.begin(client, String(GG_SHEET_URL))) {
    Serial.println("[GSheet] Khong ket noi duoc den server.");
    return;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("[GSheet] Da gui Google Sheet. HTTP Code: " + String(httpCode));
    if (httpCode >= 400) {
      Serial.println("[GSheet] Response: " + response);
    }
  } else {
    Serial.println("[GSheet] Loi HTTP: " + http.errorToString(httpCode));
  }

  http.end();
}

inline StudentInfo googleSheetJobToStudent(const GoogleSheetJob &job) {
  StudentInfo student;
  student.maSV = String(job.maSV);
  student.ho = String(job.ho);
  student.ten = String(job.ten);
  student.lop = String(job.lop);
  student.sdt = String(job.sdt);
  student.ghiChu = String(job.trangThai);
  return student;
}

inline void googleSheetSenderTask(void *parameter) {
  GoogleSheetJob job;
  for (;;) {
    if (xQueueReceive(gGoogleSheetQueue, &job, portMAX_DELAY) == pdTRUE) {
      sendToGoogleSheet(googleSheetJobToStudent(job));
    }
  }
}

inline void initGoogleSheetSender() {
  if (gGoogleSheetQueue != nullptr) return;

  gGoogleSheetQueue = xQueueCreate(GOOGLE_SHEET_QUEUE_LEN, sizeof(GoogleSheetJob));
  if (gGoogleSheetQueue == nullptr) {
    return;
  }

  BaseType_t ok = xTaskCreatePinnedToCore(
    googleSheetSenderTask,
    "gsheet_sender",
    6144,
    nullptr,
    1,
    &gGoogleSheetTask,
    0
  );

  if (ok != pdPASS) {
    vQueueDelete(gGoogleSheetQueue);
    gGoogleSheetQueue = nullptr;
    gGoogleSheetTask = nullptr;
    return;
  }
}

inline void enqueueToGoogleSheet(const StudentInfo &student) {
  if (gGoogleSheetQueue == nullptr) {
    return;
  }

  GoogleSheetJob job = {};
  safeCopy(job.maSV, sizeof(job.maSV), student.maSV);
  safeCopy(job.ho, sizeof(job.ho), student.ho);
  safeCopy(job.ten, sizeof(job.ten), student.ten);
  safeCopy(job.lop, sizeof(job.lop), student.lop);
  safeCopy(job.sdt, sizeof(job.sdt), student.sdt);
  safeCopy(job.trangThai, sizeof(job.trangThai), isCurrentAttendanceIn() ? "IN" : "OUT");

  if (xQueueSend(gGoogleSheetQueue, &job, 0) != pdTRUE) {
    Serial.println("[GSheet] Hang doi Google Sheet dang day.");
  }
}
