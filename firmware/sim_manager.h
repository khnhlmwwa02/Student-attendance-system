#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "globals.h"
#include "google_sheets.h"

struct SimSmsJob {
  char sdt[16];
  char ho[24];
  char ten[16];
  char maSV[16];
};

static HardwareSerial simSerial(2);
static QueueHandle_t gSimQueue = nullptr;
static TaskHandle_t gSimTask = nullptr;
static const uint8_t SIM_QUEUE_LEN = 10;
static bool gSimReady = false;

inline bool simWaitForToken(const char *token, unsigned long timeoutMs) {
  String response;
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (simSerial.available()) {
      char c = (char)simSerial.read();
      response += c;
      if (response.indexOf(token) >= 0) {
        return true;
      }
      if (response.indexOf("ERROR") >= 0) {
        Serial.print("[SIM] ERROR response: ");
        Serial.println(response);
        return false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  if (response.length() > 0) {
    Serial.print("[SIM] Timeout response: ");
    Serial.println(response);
  }
  return false;
}

inline bool simSendCommand(const char *cmd, const char *token, unsigned long timeoutMs) {
  while (simSerial.available()) simSerial.read();
  simSerial.println(cmd);
  return simWaitForToken(token, timeoutMs);
}

inline String buildAttendanceSms(const StudentInfo &student) {
  String fullName = student.ho;
  if (fullName.length() > 0 && student.ten.length() > 0) fullName += " ";
  fullName += student.ten;
  fullName = vnToAsciiUpper(fullName);

  String message = "Xac nhan diem danh sinh vien\r\n";
  message += "Ho va ten: " + fullName + "\r\n";
  message += "MSSV: " + student.maSV + "\r\n";
  message += "Thoi gian: " + getFormattedTime();
  return message;
}

inline bool sendSmsNow(const StudentInfo &student) {
  String phone = student.sdt;
  phone.trim();
  if (phone.length() == 0) {
    return false;
  }

  if (!simSendCommand("AT", "OK", 1000)) return false;
  if (!simSendCommand("AT+CMGF=1", "OK", 1000)) return false;

  while (simSerial.available()) simSerial.read();
  String cmd = "AT+CMGS=\"" + phone + "\"";
  simSerial.println(cmd);
  if (!simWaitForToken(">", 3000)) return false;

  String message = buildAttendanceSms(student);
  simSerial.print(message);
  simSerial.write((char)26);

  Serial.println("[SIM] Noi dung SMS:");
  Serial.println(message);

  return simWaitForToken("OK", 15000);
}

inline StudentInfo simJobToStudent(const SimSmsJob &job) {
  StudentInfo student;
  student.sdt = String(job.sdt);
  student.ho = String(job.ho);
  student.ten = String(job.ten);
  student.maSV = String(job.maSV);
  return student;
}

inline void simSenderTask(void *parameter) {
  SimSmsJob job;
  for (;;) {
    if (xQueueReceive(gSimQueue, &job, portMAX_DELAY) == pdTRUE) {
      StudentInfo student = simJobToStudent(job);
      if (sendSmsNow(student)) {
        Serial.println("[SIM] Da gui SMS thanh cong.");
      } else {
        Serial.println("[SIM] Gui SMS that bai.");
      }
    }
  }
}

inline void initSimModule() {
  if (gSimQueue != nullptr) return;

  gSimReady = false;
  simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  delay(500);

  for (int attempt = 0; attempt < 5; attempt++) {
    if (simSendCommand("AT", "OK", 1000)) {
      gSimReady = true;
      break;
    }
    delay(200);
  }

  if (!gSimReady) return;

  gSimQueue = xQueueCreate(SIM_QUEUE_LEN, sizeof(SimSmsJob));
  if (gSimQueue == nullptr) {
    gSimReady = false;
    return;
  }

  BaseType_t ok = xTaskCreatePinnedToCore(
    simSenderTask,
    "sim_sender",
    6144,
    nullptr,
    1,
    &gSimTask,
    0
  );

  if (ok != pdPASS) {
    vQueueDelete(gSimQueue);
    gSimQueue = nullptr;
    gSimTask = nullptr;
    gSimReady = false;
    return;
  }
}

inline bool isSimModuleReady() {
  return gSimReady;
}

inline void enqueueAttendanceSms(const StudentInfo &student) {
  if (gSimQueue == nullptr) {
    return;
  }

  String phone = student.sdt;
  phone.trim();
  if (phone.length() == 0) {
    return;
  }

  SimSmsJob job = {};
  safeCopy(job.sdt, sizeof(job.sdt), phone);
  safeCopy(job.ho, sizeof(job.ho), student.ho);
  safeCopy(job.ten, sizeof(job.ten), student.ten);
  safeCopy(job.maSV, sizeof(job.maSV), student.maSV);

  if (xQueueSend(gSimQueue, &job, 0) != pdTRUE) {
    Serial.println("[SIM] Hang doi SMS dang day.");
  }
}
