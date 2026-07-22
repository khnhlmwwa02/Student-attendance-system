#pragma once
#include <Arduino.h>

struct StudentInfo {
  String maSV;
  String ho;
  String ten;
  String lop;
  String khoa;
  String khoaHoc;
  String sdt;
  String ghiChu;
  String rfidUid;
  int fingerId = -1;
};

struct StoredStudentRecord {
  uint8_t active;
  char maSV[16];
  char ho[24];
  char ten[16];
  char lop[16];
  char khoa[24];
  char khoaHoc[12];
  char sdt[16];
  char ghiChu[32];
  char rfidUid[16];
  int16_t fingerId;
};

struct StorageHeader {
  uint32_t magic;
  uint16_t count;
};

struct AttendanceLogEntry {
  uint16_t stt;
  char dateKey[12];
  char ho[24];
  char ten[16];
  char maSV[16];
  char lop[16];
  char timeText[12];
  int16_t diffMinutesFrom7am;
  uint8_t onTime;
  uint8_t direction;
};

enum AttendanceDirection {
  ATTENDANCE_DIR_IN = 1,
  ATTENDANCE_DIR_OUT = 2
};

enum SystemMode {
  MODE_ATTENDANCE_IDLE,
  MODE_WEBSERVER_OPEN,
  MODE_REGISTER_CARD_STEP,
  MODE_REGISTER_FINGER_STEP,
  MODE_REGISTER_SUMMARY,
  MODE_SAVE_SUCCESS
};

extern StudentInfo currentStudent;
extern SystemMode currentMode;

extern bool webServerEnabled;
extern bool formValid;
extern bool cardCaptured;
extern bool fingerCaptured;
extern bool readyToSave;
extern bool fingerSensorReady;
extern bool rfidReaderReady;

extern String localIP;
extern String currentCardStatus;
extern String currentFingerStatus;
extern String lastSavedDisplayName;
extern String lastAttendanceStudentId;
extern bool lastAttendanceWasIn;

extern unsigned long saveSuccessMillis;
extern unsigned long stepStartMillis;
extern unsigned long attendanceNameMillis;
extern bool attendanceNameShowing;
extern unsigned long portalSessionId;

static const char* MASTER_UID = "E3 44 6A 05";
static const unsigned long STEP_TIMEOUT_MS = 15000;
static const unsigned long SAVE_SUCCESS_SHOW_MS = 5000;
static const unsigned long ATTENDANCE_NAME_SHOW_MS = 3000;

static const uint8_t LCD_SDA_PIN = 2;
static const uint8_t LCD_SCL_PIN = 1;
static const uint8_t LCD_I2C_ADDR = 0x27;

static const uint8_t RFID_SS_PIN   = 10;
static const uint8_t RFID_RST_PIN  = 8;
static const uint8_t RFID_SCK_PIN  = 12;
static const uint8_t RFID_MISO_PIN = 13;
static const uint8_t RFID_MOSI_PIN = 11;

static const uint8_t FINGER_RX_PIN = 5;
static const uint8_t FINGER_TX_PIN = 4;
static const uint8_t FINGER_TCH_PIN = 6;
static const uint8_t FINGER_VA_PIN  = 7;
static const uint32_t FINGER_BAUD = 57600;

// SIM Module pins (UART2)
// Wiring: SIM RX -> GPIO15, SIM TX -> GPIO16
static const uint8_t SIM_RX_PIN = 16;   // ESP32 RX <- SIM TX
static const uint8_t SIM_TX_PIN = 15;   // ESP32 TX -> SIM RX
static const uint32_t SIM_BAUD = 115200;
static const uint8_t ATTENDANCE_SWITCH_PIN = 42;

static const uint32_t STORAGE_MAGIC = 0x53564442; // SVDB
static const int MAX_STUDENTS = 40;
static const int MAX_ATTENDANCE_LOGS = 200;
static const unsigned long ATTENDANCE_DUPLICATE_WINDOW_MS = 60000;

inline String vnToAsciiUpper(String s) {
  const char* from[] = {
    "à","á","ạ","ả","ã","â","ầ","ấ","ậ","ẩ","ẫ","ă","ằ","ắ","ặ","ẳ","ẵ",
    "À","Á","Ạ","Ả","Ã","Â","Ầ","Ấ","Ậ","Ẩ","Ẫ","Ă","Ằ","Ắ","Ặ","Ẳ","Ẵ",

    "è","é","ẹ","ẻ","ẽ","ê","ề","ế","ệ","ể","ễ",
    "È","É","Ẹ","Ẻ","Ẽ","Ê","Ề","Ế","Ệ","Ể","Ễ",

    "ì","í","ị","ỉ","ĩ",
    "Ì","Í","Ị","Ỉ","Ĩ",

    "ò","ó","ọ","ỏ","õ","ô","ồ","ố","ộ","ổ","ỗ","ơ","ờ","ớ","ợ","ở","ỡ",
    "Ò","Ó","Ọ","Ỏ","Õ","Ô","Ồ","Ố","Ộ","Ổ","Ỗ","Ơ","Ờ","Ớ","Ợ","Ở","Ỡ",

    "ù","ú","ụ","ủ","ũ","ư","ừ","ứ","ự","ử","ữ",
    "Ù","Ú","Ụ","Ủ","Ũ","Ư","Ừ","Ứ","Ự","Ử","Ữ",

    "ỳ","ý","ỵ","ỷ","ỹ",
    "Ỳ","Ý","Ỵ","Ỷ","Ỹ",

    "đ","Đ"
  };

  const char* to[] = {
    "a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a",
    "a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a","a",

    "e","e","e","e","e","e","e","e","e","e","e",
    "e","e","e","e","e","e","e","e","e","e","e",

    "i","i","i","i","i",
    "i","i","i","i","i",

    "o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o",
    "o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o",

    "u","u","u","u","u","u","u","u","u","u","u",
    "u","u","u","u","u","u","u","u","u","u","u",

    "y","y","y","y","y",
    "y","y","y","y","y",

    "d","d"
  };

  const size_t n = sizeof(from) / sizeof(from[0]);
  for (size_t i = 0; i < n; i++) {
    s.replace(from[i], to[i]);
  }

  s.toUpperCase();
  return s;
}

inline bool isFormComplete() {
  return currentStudent.maSV.length() > 0 &&
         currentStudent.ho.length() > 0 &&
         currentStudent.ten.length() > 0 &&
         currentStudent.lop.length() > 0 &&
         currentStudent.khoa.length() > 0 &&
         currentStudent.khoaHoc.length() > 0 &&
         currentStudent.sdt.length() > 0 &&
         currentStudent.ghiChu.length() > 0;
}

inline void updateReadyToSave() {
  formValid = isFormComplete();
  readyToSave = formValid && (cardCaptured || fingerCaptured);
}

inline void resetTempRegistration() {
  currentStudent = StudentInfo();
  formValid = false;
  cardCaptured = false;
  fingerCaptured = false;
  readyToSave = false;
  currentCardStatus = "Chua quet the moi";
  currentFingerStatus = "Chua dang ky";
  stepStartMillis = 0;
}

inline void safeCopy(char *dest, size_t destSize, const String &src) {
  if (destSize == 0) return;
  size_t len = min(destSize - 1, src.length());
  memcpy(dest, src.c_str(), len);
  dest[len] = '\0';
}

inline String modeToText(SystemMode mode) {
  switch (mode) {
    case MODE_ATTENDANCE_IDLE: return "Diem danh";
    case MODE_WEBSERVER_OPEN: return "Webserver dang mo";
    case MODE_REGISTER_CARD_STEP: return "Dang ky the";
    case MODE_REGISTER_FINGER_STEP: return "Dang ky van tay";
    case MODE_REGISTER_SUMMARY: return "Them sinh vien";
    case MODE_SAVE_SUCCESS: return "Luu thanh cong";
  }
  return "Khong ro";
}

bool startRegistrationFromWeb(String &message);
bool saveStudentFromWeb(String &message);
bool lookupStudent(const String &type, const String &value, StudentInfo &out, bool &hasCard, bool &hasFinger);
void openPortal();
void closePortal();
void renderCurrentScreen();
void fillLcdPreview(String &l1, String &l2, String &l3, String &l4);
int getStoredStudentCount();
bool getStoredStudentAt(int index, StudentInfo &out, bool &hasCard, bool &hasFinger);
bool deleteStudentByStudentId(const String &studentId, String &message);
void recordAttendanceEvent(const StudentInfo &student);
int getAttendanceLogCount();
bool getAttendanceLogAt(int index, AttendanceLogEntry &out);
String getAttendanceLogDateLabel();
String buildAttendanceStatusText(const AttendanceLogEntry &entry);
String buildAttendanceNoteText(const AttendanceLogEntry &entry);
AttendanceDirection getCurrentAttendanceDirection();
bool isCurrentAttendanceIn();
