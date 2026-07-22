#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Adafruit_Fingerprint.h>
#include <time.h>

#include "globals.h"
#include "lcd_display.h"
#include "wifi_manager.h"
#include "webserver_manager.h"
#include "max98357a_manager.h"
#include "google_sheets.h"
#include "sim_manager.h"

StudentInfo currentStudent;
SystemMode currentMode = MODE_ATTENDANCE_IDLE;

bool webServerEnabled = false;
bool formValid = false;
bool cardCaptured = false;
bool fingerCaptured = false;
bool readyToSave = false;
bool fingerSensorReady = false;
bool rfidReaderReady = false;

String localIP = "0.0.0.0";
String currentCardStatus = "Chua quet the moi";
String currentFingerStatus = "Chua dang ky";
String lastSavedDisplayName = "";
String lastAttendanceStudentId = "--------";
bool lastAttendanceWasIn = true;

unsigned long saveSuccessMillis = 0;
unsigned long stepStartMillis = 0;
unsigned long attendanceNameMillis = 0;
bool attendanceNameShowing = false;
unsigned long portalSessionId = 0;
bool systemHardwareReady = false;

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
HardwareSerial fingerSerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
Preferences prefs;

static const char* STORAGE_NAMESPACE = "svdb";
StoredStudentRecord gRecords[MAX_STUDENTS];
StorageHeader gHeader;
AttendanceLogEntry gAttendanceLogs[MAX_ATTENDANCE_LOGS];
int gAttendanceLogCount = 0;
uint16_t gAttendanceSequence = 0;
char gAttendanceDateKey[12] = "";

enum FingerEnrollStage {
  FE_WAIT_FIRST,
  FE_WAIT_REMOVE,
  FE_WAIT_SECOND
};

FingerEnrollStage fingerEnrollStage = FE_WAIT_FIRST;
int pendingFingerId = -1;
unsigned long lastFingerAttendanceCheck = 0;
int lastAttendanceSwitchLevel = HIGH;
String lastAcceptedAttendanceStudentId = "";
AttendanceDirection lastAcceptedAttendanceDirection = ATTENDANCE_DIR_IN;
unsigned long lastAcceptedAttendanceMillis = 0;
bool duplicateWarningActive = false;
bool duplicateWarningWaitingForSpeech = false;
unsigned long duplicateWarningStartMillis = 0;

const char* fingerStatusText(uint8_t code) {
  switch (code) {
    case FINGERPRINT_OK: return "OK";
    case FINGERPRINT_PACKETRECIEVEERR: return "PACKET_ERROR";
    case FINGERPRINT_NOFINGER: return "NO_FINGER";
    case FINGERPRINT_IMAGEFAIL: return "IMAGE_FAIL";
    case FINGERPRINT_IMAGEMESS: return "IMAGE_MESS";
    case FINGERPRINT_FEATUREFAIL: return "FEATURE_FAIL";
    case FINGERPRINT_INVALIDIMAGE: return "INVALID_IMAGE";
    case FINGERPRINT_NOTFOUND: return "NOT_FOUND";
    case FINGERPRINT_ENROLLMISMATCH: return "ENROLL_MISMATCH";
    case FINGERPRINT_BADLOCATION: return "BAD_LOCATION";
    case FINGERPRINT_FLASHERR: return "FLASH_ERROR";
    default: return "UNKNOWN";
  }
}

void clearAttendanceLogs() {
  memset(gAttendanceLogs, 0, sizeof(gAttendanceLogs));
  gAttendanceLogCount = 0;
  gAttendanceSequence = 0;
}

String attendanceMetaKey(const char *name) {
  return String("att_") + name;
}

String attendanceSlotKey(int index) {
  return "attrec_" + String(index);
}

void saveAttendanceMeta() {
  prefs.putString(attendanceMetaKey("date").c_str(), String(gAttendanceDateKey));
  prefs.putUInt(attendanceMetaKey("count").c_str(), gAttendanceLogCount);
  prefs.putUInt(attendanceMetaKey("seq").c_str(), gAttendanceSequence);
}

void clearAttendanceLogStorage() {
  for (int i = 0; i < MAX_ATTENDANCE_LOGS; i++) {
    prefs.remove(attendanceSlotKey(i).c_str());
  }
  prefs.remove(attendanceMetaKey("date").c_str());
  prefs.remove(attendanceMetaKey("count").c_str());
  prefs.remove(attendanceMetaKey("seq").c_str());
}

void saveAttendanceLogsToStorage() {
  for (int i = 0; i < MAX_ATTENDANCE_LOGS; i++) {
    if (i < gAttendanceLogCount) {
      prefs.putBytes(attendanceSlotKey(i).c_str(), &gAttendanceLogs[i], sizeof(AttendanceLogEntry));
    } else {
      prefs.remove(attendanceSlotKey(i).c_str());
    }
  }
  saveAttendanceMeta();
}

void loadAttendanceLogsFromStorage() {
  clearAttendanceLogs();

  String storedDate = prefs.getString(attendanceMetaKey("date").c_str(), "");
  if (storedDate.length() == 0) {
    gAttendanceDateKey[0] = '\0';
    return;
  }

  safeCopy(gAttendanceDateKey, sizeof(gAttendanceDateKey), storedDate);
  gAttendanceLogCount = min((int)prefs.getUInt(attendanceMetaKey("count").c_str(), 0), MAX_ATTENDANCE_LOGS);
  gAttendanceSequence = (uint16_t)prefs.getUInt(attendanceMetaKey("seq").c_str(), gAttendanceLogCount);

  for (int i = 0; i < gAttendanceLogCount; i++) {
    AttendanceLogEntry entry = {};
    size_t got = prefs.getBytes(attendanceSlotKey(i).c_str(), &entry, sizeof(AttendanceLogEntry));
    if (got != sizeof(AttendanceLogEntry)) {
      gAttendanceLogCount = i;
      break;
    }
    gAttendanceLogs[i] = entry;
  }

  if (gAttendanceSequence < gAttendanceLogCount) {
    gAttendanceSequence = gAttendanceLogCount;
  }
}

bool getCurrentTimeInfo(struct tm &timeinfo) {
  return getLocalTime(&timeinfo, 100);
}

String makeDateKey(const struct tm &timeinfo) {
  char buf[12];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
  return String(buf);
}

String makeDateLabel(const struct tm &timeinfo) {
  char buf[16];
  strftime(buf, sizeof(buf), "%d/%m/%Y", &timeinfo);
  return String(buf);
}

String makeTimeLabel(const struct tm &timeinfo) {
  char buf[12];
  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
  return String(buf);
}

void syncAttendanceLogDay() {
  struct tm timeinfo;
  if (!getCurrentTimeInfo(timeinfo)) return;

  String today = makeDateKey(timeinfo);
  if (strcmp(gAttendanceDateKey, today.c_str()) != 0) {
    clearAttendanceLogs();
    safeCopy(gAttendanceDateKey, sizeof(gAttendanceDateKey), today);
    clearAttendanceLogStorage();
    saveAttendanceMeta();
  }
}

String formatDurationMinutes(int totalMinutes) {
  int minutes = abs(totalMinutes);
  int hours = minutes / 60;
  int remain = minutes % 60;

  if (hours > 0 && remain > 0) {
    return String(hours) + " gio " + String(remain) + " phut";
  }
  if (hours > 0) {
    return String(hours) + " gio";
  }
  return String(remain) + " phut";
}

AttendanceDirection getCurrentAttendanceDirection() {
  return digitalRead(ATTENDANCE_SWITCH_PIN) == HIGH ? ATTENDANCE_DIR_IN : ATTENDANCE_DIR_OUT;
}

bool isCurrentAttendanceIn() {
  return getCurrentAttendanceDirection() == ATTENDANCE_DIR_IN;
}

String buildAttendanceStatusText(const AttendanceLogEntry &entry) {
  if (entry.direction == ATTENDANCE_DIR_OUT) return "Da ra";
  if (String(entry.timeText) == "N/A") return "Chua ro";
  return entry.onTime ? "Dung gio" : "Tre";
}

String buildAttendanceNoteText(const AttendanceLogEntry &entry) {
  if (entry.direction == ATTENDANCE_DIR_OUT) return "";
  if (String(entry.timeText) == "N/A") return "Chua dong bo gio he thong";
  if (entry.diffMinutesFrom7am == 0) return "Dung moc 07:00";
  if (entry.diffMinutesFrom7am < 0) {
    return "Som " + formatDurationMinutes(entry.diffMinutesFrom7am) + " so voi 07:00";
  }
  return "Tre " + formatDurationMinutes(entry.diffMinutesFrom7am) + " so voi 07:00";
}

void logFingerResult(const char *step, uint8_t code) {
  Serial.print("[Finger] ");
  Serial.print(step);
  Serial.print(" -> ");
  Serial.print(fingerStatusText(code));
  Serial.print(" (");
  Serial.print(code);
  Serial.println(")");
}

void updateBootHardwareScreen(bool rfidOk, bool fingerOk, bool simOk, bool speakerOk) {
  showHardwareCheckScreen(rfidOk, fingerOk, simOk, speakerOk);
  delay(1000);
}

bool runHardwareChecks() {
  bool rfidOk = false;
  bool fingerOk = false;
  bool simOk = false;
  bool speakerOk = false;

  showBootWifiScreen("DANG KIEM TRA", "PHAN CUNG HE THONG", "", "");
  delay(1000);

  initRFID();
  rfidOk = rfidReaderReady;
  updateBootHardwareScreen(rfidOk, fingerOk, simOk, speakerOk);

  initFinger();
  fingerOk = fingerSensorReady;
  updateBootHardwareScreen(rfidOk, fingerOk, simOk, speakerOk);

  initSimModule();
  simOk = isSimModuleReady();
  updateBootHardwareScreen(rfidOk, fingerOk, simOk, speakerOk);

  initSpeaker();
  speakerOk = isSpeakerReady();
  updateBootHardwareScreen(rfidOk, fingerOk, simOk, speakerOk);

  systemHardwareReady = rfidOk && fingerOk && simOk && speakerOk;
  return systemHardwareReady;
}

String attendanceBottomPreview(const String &mssv) {
  if (localIP == "0.0.0.0") return "IP: " + localIP;
  String value = mssv;
  value.trim();
  if (value.length() == 0) value = "--------";
  return "MSSV:" + value;
}

void showAttendanceErrorTemp(const String &line2, const String &line3, const String &maSV = "--------") {
  showAttendanceError(line2, line3, maSV);
  delay(1200);
  if (currentMode == MODE_ATTENDANCE_IDLE) {
    if (attendanceNameShowing) showAttendanceName(lastSavedDisplayName, lastAttendanceStudentId, lastAttendanceWasIn);
    else showAttendanceScreen();
  }
}

String slotKey(int index) {
  return "rec_" + String(index);
}

String slotValidKey(int index) {
  return "valid_" + String(index);
}

void clearLocalRecord(StoredStudentRecord &rec) {
  memset(&rec, 0, sizeof(rec));
  rec.active = 0;
  rec.fingerId = -1;
}

int countActiveRecords() {
  int count = 0;
  for (int i = 0; i < MAX_STUDENTS; i++) {
    if (gRecords[i].active) count++;
  }
  return count;
}

void saveMeta() {
  gHeader.magic = STORAGE_MAGIC;
  gHeader.count = countActiveRecords();
  prefs.putUInt("magic", gHeader.magic);
  prefs.putUInt("count", gHeader.count);
}

void initStorage() {
  prefs.begin(STORAGE_NAMESPACE, false);

  memset(&gHeader, 0, sizeof(gHeader));
  for (int i = 0; i < MAX_STUDENTS; i++) {
    clearLocalRecord(gRecords[i]);
  }

  uint32_t magic = prefs.getUInt("magic", 0);
  if (magic != STORAGE_MAGIC) {
    prefs.clear();
    gHeader.magic = STORAGE_MAGIC;
    gHeader.count = 0;
    saveMeta();
    return;
  }

  gHeader.magic = magic;
  gHeader.count = prefs.getUInt("count", 0);

  for (int i = 0; i < MAX_STUDENTS; i++) {
    bool valid = prefs.getUChar(slotValidKey(i).c_str(), 0) == 1;
    if (!valid) {
      clearLocalRecord(gRecords[i]);
      continue;
    }

    size_t got = prefs.getBytes(slotKey(i).c_str(), &gRecords[i], sizeof(StoredStudentRecord));
    if (got != sizeof(StoredStudentRecord)) {
      clearLocalRecord(gRecords[i]);
      prefs.remove(slotKey(i).c_str());
      prefs.remove(slotValidKey(i).c_str());
    }
  }

  gHeader.count = countActiveRecords();
  saveMeta();
  loadAttendanceLogsFromStorage();
}

StorageHeader readStorageHeader() {
  return gHeader;
}

void writeStorageHeader(const StorageHeader &h) {
  gHeader = h;
  saveMeta();
}

void readRecord(int index, StoredStudentRecord &rec) {
  if (index < 0 || index >= MAX_STUDENTS) {
    clearLocalRecord(rec);
    return;
  }
  rec = gRecords[index];
}

void writeRecord(int index, const StoredStudentRecord &rec) {
  if (index < 0 || index >= MAX_STUDENTS) return;

  gRecords[index] = rec;
  if (rec.active) {
    prefs.putBytes(slotKey(index).c_str(), &gRecords[index], sizeof(StoredStudentRecord));
    prefs.putUChar(slotValidKey(index).c_str(), 1);
  } else {
    prefs.remove(slotKey(index).c_str());
    prefs.remove(slotValidKey(index).c_str());
  }
}

void clearRecord(int index) {
  if (index < 0 || index >= MAX_STUDENTS) return;
  clearLocalRecord(gRecords[index]);
  prefs.remove(slotKey(index).c_str());
  prefs.remove(slotValidKey(index).c_str());
}

void refreshStoredCount() {
  saveMeta();
}

StudentInfo recordToStudent(const StoredStudentRecord &rec) {
  StudentInfo s;
  s.maSV = String(rec.maSV);
  s.ho = String(rec.ho);
  s.ten = String(rec.ten);
  s.lop = String(rec.lop);
  s.khoa = String(rec.khoa);
  s.khoaHoc = String(rec.khoaHoc);
  s.sdt = String(rec.sdt);
  s.ghiChu = String(rec.ghiChu);
  s.rfidUid = String(rec.rfidUid);
  s.fingerId = rec.fingerId;
  return s;
}

StoredStudentRecord studentToRecord(const StudentInfo &s) {
  StoredStudentRecord rec = {};
  rec.active = 1;
  safeCopy(rec.maSV, sizeof(rec.maSV), s.maSV);
  safeCopy(rec.ho, sizeof(rec.ho), s.ho);
  safeCopy(rec.ten, sizeof(rec.ten), s.ten);
  safeCopy(rec.lop, sizeof(rec.lop), s.lop);
  safeCopy(rec.khoa, sizeof(rec.khoa), s.khoa);
  safeCopy(rec.khoaHoc, sizeof(rec.khoaHoc), s.khoaHoc);
  safeCopy(rec.sdt, sizeof(rec.sdt), s.sdt);
  safeCopy(rec.ghiChu, sizeof(rec.ghiChu), s.ghiChu);
  safeCopy(rec.rfidUid, sizeof(rec.rfidUid), normalizeUid(s.rfidUid));
  rec.fingerId = s.fingerId;
  return rec;
}

int findFirstEmptySlot() {
  for (int i = 0; i < MAX_STUDENTS; i++) {
    if (!gRecords[i].active) return i;
  }
  return -1;
}

int findSlotByStudentId(const String &studentId) {
  if (studentId.length() == 0) return -1;
  for (int i = 0; i < MAX_STUDENTS; i++) {
    if (gRecords[i].active && String(gRecords[i].maSV).equalsIgnoreCase(studentId)) return i;
  }
  return -1;
}

int findSlotByUid(const String &uid) {
  String targetUid = normalizeUid(uid);
  if (targetUid.length() == 0) return -1;
  for (int i = 0; i < MAX_STUDENTS; i++) {
    if (!gRecords[i].active) continue;
    if (normalizeUid(String(gRecords[i].rfidUid)) == targetUid) return i;
  }
  return -1;
}

int findSlotByFingerId(int fingerId) {
  if (fingerId < 0) return -1;
  for (int i = 0; i < MAX_STUDENTS; i++) {
    if (gRecords[i].active && gRecords[i].fingerId == fingerId) return i;
  }
  return -1;
}
bool lookupStudent(const String &type, const String &value, StudentInfo &out, bool &hasCard, bool &hasFinger) {
  int slot = -1;
  if (type == "studentId") slot = findSlotByStudentId(value);
  else if (type == "rfid") slot = findSlotByUid(value);
  else if (type == "finger") slot = findSlotByFingerId(value.toInt());

  if (slot < 0) return false;

  out = recordToStudent(gRecords[slot]);
  hasCard = out.rfidUid.length() > 0;
  hasFinger = out.fingerId >= 0;
  return true;
}

int getStoredStudentCount() {
  return countActiveRecords();
}

bool getStoredStudentAt(int index, StudentInfo &out, bool &hasCard, bool &hasFinger) {
  if (index < 0) return false;

  int seen = 0;
  StoredStudentRecord rec;
  for (int slot = 0; slot < MAX_STUDENTS; slot++) {
    readRecord(slot, rec);
    if (!rec.active) continue;

    if (seen == index) {
      out = recordToStudent(rec);
      hasCard = out.rfidUid.length() > 0;
      hasFinger = out.fingerId >= 0;
      return true;
    }
    seen++;
  }

  return false;
}

bool deleteStudentByStudentId(const String &studentId, String &message) {
  int slot = findSlotByStudentId(studentId);
  if (slot < 0) {
    message = "Khong tim thay sinh vien.";
    return false;
  }

  StoredStudentRecord rec;
  readRecord(slot, rec);
  int fingerId = rec.fingerId;

  // Neu co van tay thi xoa trong AS608 truoc
  if (fingerId > 0) {
    if (!fingerSensorReady) {
      message = "Cam bien van tay chua san sang, chua the xoa.";
      return false;
    }

    uint8_t p = finger.deleteModel(fingerId);
    if (p != FINGERPRINT_OK) {
      message = "Xoa van tay trong cam bien that bai.";
      return false;
    }

    // Cap nhat lai so mau trong cam bien
    finger.getTemplateCount();
  }

  // Chi khi xoa van tay thanh cong moi xoa record trong bo nho
  clearRecord(slot);
  refreshStoredCount();

  if (currentStudent.maSV.equalsIgnoreCase(studentId)) {
    resetTempRegistration();
    if (webServerEnabled) {
      currentMode = MODE_WEBSERVER_OPEN;
    }
  }

  updateReadyToSave();
  renderCurrentScreen();

  if (fingerId > 0) {
    message = "Da xoa sinh vien va van tay thanh cong.";
  } else {
    message = "Da xoa sinh vien thanh cong.";
  }
  return true;
}

void recordAttendanceEvent(const StudentInfo &student) {
  syncAttendanceLogDay();

  if (gAttendanceLogCount >= MAX_ATTENDANCE_LOGS) {
    for (int i = 1; i < MAX_ATTENDANCE_LOGS; i++) {
      gAttendanceLogs[i - 1] = gAttendanceLogs[i];
      gAttendanceLogs[i - 1].stt = i;
    }
    gAttendanceLogCount = MAX_ATTENDANCE_LOGS - 1;
    gAttendanceSequence = gAttendanceLogCount;
  }

  AttendanceLogEntry entry = {};
  AttendanceDirection direction = getCurrentAttendanceDirection();
  struct tm timeinfo;
  if (getCurrentTimeInfo(timeinfo)) {
    safeCopy(entry.dateKey, sizeof(entry.dateKey), makeDateKey(timeinfo));
    safeCopy(entry.timeText, sizeof(entry.timeText), makeTimeLabel(timeinfo));
    int currentMinutes = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
    entry.diffMinutesFrom7am = currentMinutes - (7 * 60);
    entry.onTime = direction == ATTENDANCE_DIR_OUT ? 1 : (entry.diffMinutesFrom7am <= 0 ? 1 : 0);
  } else {
    safeCopy(entry.dateKey, sizeof(entry.dateKey), "N/A");
    safeCopy(entry.timeText, sizeof(entry.timeText), "N/A");
    entry.diffMinutesFrom7am = 0;
    entry.onTime = direction == ATTENDANCE_DIR_OUT ? 1 : 0;
  }

  entry.stt = ++gAttendanceSequence;
  safeCopy(entry.ho, sizeof(entry.ho), student.ho);
  safeCopy(entry.ten, sizeof(entry.ten), student.ten);
  safeCopy(entry.maSV, sizeof(entry.maSV), student.maSV);
  safeCopy(entry.lop, sizeof(entry.lop), student.lop);
  entry.direction = direction;

  gAttendanceLogs[gAttendanceLogCount++] = entry;
  saveAttendanceLogsToStorage();
}

bool isDuplicateAttendanceAttempt(const StudentInfo &student, AttendanceDirection direction) {
  unsigned long now = millis();

  if (student.maSV.equalsIgnoreCase(lastAcceptedAttendanceStudentId) &&
      direction == lastAcceptedAttendanceDirection) {
    if (now - lastAcceptedAttendanceMillis < ATTENDANCE_DUPLICATE_WINDOW_MS) {
      return true;
    }
  }

  struct tm timeinfo;
  if (getCurrentTimeInfo(timeinfo)) {
    int currentSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    for (int i = gAttendanceLogCount - 1; i >= 0; i--) {
      AttendanceLogEntry &entry = gAttendanceLogs[i];
      if (!String(entry.maSV).equalsIgnoreCase(student.maSV)) continue;
      if (entry.direction != direction) continue;

      int hh = 0, mm = 0, ss = 0;
      if (sscanf(entry.timeText, "%d:%d:%d", &hh, &mm, &ss) != 3) continue;

      int loggedSeconds = hh * 3600 + mm * 60 + ss;
      if (currentSeconds >= loggedSeconds &&
          (currentSeconds - loggedSeconds) < (int)(ATTENDANCE_DUPLICATE_WINDOW_MS / 1000UL)) {
        return true;
      }
      break;
    }
  }

  return false;
}

void markAttendanceAccepted(const StudentInfo &student, AttendanceDirection direction) {
  lastAcceptedAttendanceStudentId = student.maSV;
  lastAcceptedAttendanceDirection = direction;
  lastAcceptedAttendanceMillis = millis();
}

void restoreAttendanceIdleScreen() {
  if (currentMode != MODE_ATTENDANCE_IDLE) return;
  if (attendanceNameShowing) showAttendanceName(lastSavedDisplayName, lastAttendanceStudentId, lastAttendanceWasIn);
  else showAttendanceScreen();
}

String attendanceDirectionText(AttendanceDirection direction) {
  return direction == ATTENDANCE_DIR_OUT ? "OUT" : "IN";
}

bool speakDuplicateAttendanceWarning(bool fromFingerprint) {
  return speakTextVi(fromFingerprint ? "Vân tay bị trùng" : "Thẻ bị trùng");
}

void showDuplicateAttendanceWarning(bool fromFingerprint, const String &maSV) {
  showAttendanceError(fromFingerprint ? "VAN TAY BI TRUNG" : "THE BI TRUNG",
                      "VUI LONG DOI 1 PHUT",
                      maSV);
  duplicateWarningActive = true;
  duplicateWarningStartMillis = millis();
  duplicateWarningWaitingForSpeech = speakDuplicateAttendanceWarning(fromFingerprint);
}

bool handleAttendanceAccepted(const StudentInfo &student, bool fromFingerprint) {
  AttendanceDirection direction = getCurrentAttendanceDirection();
  if (isDuplicateAttendanceAttempt(student, direction)) {
    showDuplicateAttendanceWarning(fromFingerprint, student.maSV);
    return false;
  }

  recordAttendanceEvent(student);
  markAttendanceAccepted(student, direction);
  showAttendanceStudentName(student.ten, student.maSV);
  speakAttendance(student.ho, student.ten, student.maSV);
  enqueueToGoogleSheet(student);
  enqueueAttendanceSms(student);
  return true;
}

int getAttendanceLogCount() {
  syncAttendanceLogDay();
  return gAttendanceLogCount;
}

bool getAttendanceLogAt(int index, AttendanceLogEntry &out) {
  syncAttendanceLogDay();
  if (index < 0 || index >= gAttendanceLogCount) return false;
  out = gAttendanceLogs[index];
  return true;
}

String getAttendanceLogDateLabel() {
  syncAttendanceLogDay();
  if (strlen(gAttendanceDateKey) == 0) {
    struct tm timeinfo;
    if (getCurrentTimeInfo(timeinfo)) return makeDateLabel(timeinfo);
    return "N/A";
  }

  struct tm timeinfo;
  if (getCurrentTimeInfo(timeinfo) && strcmp(gAttendanceDateKey, makeDateKey(timeinfo).c_str()) == 0) {
    return makeDateLabel(timeinfo);
  }

  String key = String(gAttendanceDateKey);
  if (key.length() == 10) {
    return key.substring(8, 10) + "/" + key.substring(5, 7) + "/" + key.substring(0, 4);
  }
  return key;
}

String normalizeUid(String uid) {
  uid.trim();
  uid.toUpperCase();
  uid.replace(":", "");
  uid.replace(" ", "");
  uid.replace("-", "");
  return uid;
}

String uidToStringColon(MFRC522::Uid *uid) {
  String s = "";
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
    if (i < uid->size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}

void fillLcdPreview(String &l1, String &l2, String &l3, String &l4) {
  l1 = l2 = l3 = l4 = "";
  switch (currentMode) {
    case MODE_ATTENDANCE_IDLE:  
      l1 = "HE THONG DIEM DANH";
      l2 = isCurrentAttendanceIn() ? "DIEM DANH SV VAO LOP" : "DIEM DANH SV RA VE";
      l3 = attendanceNameShowing ? "Ten: " + vnToAsciiUpper(lastSavedDisplayName) : "Ten: --------";
      l4 = attendanceNameShowing ? attendanceBottomPreview(lastAttendanceStudentId)
                                 : attendanceBottomPreview("--------");
      break;

    case MODE_WEBSERVER_OPEN:
      l1 = "WEBSERVER DANG MO";
      l2 = "THEM SINH VIEN";
      l3 = "TRA CUU THONG TIN";
      l4 = "IP: " + localIP;
      break;

    case MODE_REGISTER_CARD_STEP:
      l1 = "BAT DAU DANG KY";
      l2 = "VUI LONG QUET THE";
      l3 = "ID: " + currentStudent.rfidUid;
      l4 = currentCardStatus;
      break;

    case MODE_REGISTER_FINGER_STEP:
      l1 = "DANG KY VAN TAY";
      l2 = "VUI LONG DAT TAY";
      l3 = currentFingerStatus;
      l4 = "IP: " + localIP;
      break;

    case MODE_REGISTER_SUMMARY:
      l1 = "THEM SINH VIEN";
      l2 = cardCaptured ? "Da quet the moi" : "Chua quet the moi";
      l3 = fingerCaptured ? "Da dang ky tay" : "Chua dang ky tay";
      l4 = readyToSave ? "San sang luu" : "Chua du dieu kien";
      break;

    case MODE_SAVE_SUCCESS:
      l1 = "HE THONG DIEM DANH";
      l2 = "Luu thanh cong";
      l3 = vnToAsciiUpper(lastSavedDisplayName);
      l4 = attendanceBottomPreview(lastAttendanceStudentId);
      break;
  }
}
void renderCurrentScreen() {
  updateReadyToSave();
  switch (currentMode) {
    case MODE_ATTENDANCE_IDLE:
      if (attendanceNameShowing) showAttendanceName(lastSavedDisplayName, lastAttendanceStudentId, lastAttendanceWasIn);
      else showAttendanceScreen();
      break;
    case MODE_WEBSERVER_OPEN:
      showWebServerOpenScreen();
      break;
    case MODE_REGISTER_CARD_STEP:
      showRegisterCardStep(currentStudent.rfidUid, currentCardStatus);
      break;
    case MODE_REGISTER_FINGER_STEP:
      showRegisterFingerStep(currentFingerStatus);
      break;
    case MODE_REGISTER_SUMMARY:
      showRegisterSummaryScreen();
      break;
    case MODE_SAVE_SUCCESS:
      showSaveSuccessScreen(lastSavedDisplayName, lastAttendanceStudentId);
      break;
  }
}

void resetFingerState() {
  fingerEnrollStage = FE_WAIT_FIRST;
  pendingFingerId = -1;
}

void openPortal() {
  portalSessionId++;
  webServerEnabled = true;
  resetTempRegistration();
  resetFingerState();
  currentMode = MODE_WEBSERVER_OPEN;
  startWebServer();
  renderCurrentScreen();
}

void closePortal() {
  webServerEnabled = false;
  stopWebServer();
  resetTempRegistration();
  resetFingerState();
  currentMode = MODE_ATTENDANCE_IDLE;
  attendanceNameShowing = false;
  renderCurrentScreen();
}

bool startRegistrationFromWeb(String &message) {
  if (!webServerEnabled) {
    message = "Webserver dang tat. Hay quet the master de mo.";
    return false;
  }

  updateReadyToSave();
  if (!formValid) {
    message = "Ban phai dien day du tat ca thong tin.";
    return false;
  }

  currentStudent.rfidUid = "";
  currentStudent.fingerId = -1;
  cardCaptured = false;
  fingerCaptured = false;
  readyToSave = false;

  currentCardStatus = "CHO QUET THE";
  currentFingerStatus = fingerSensorReady ? "CHO DAT TAY" : "CAM BIEN LOI";
  currentMode = MODE_REGISTER_CARD_STEP;
  stepStartMillis = millis();
  resetFingerState();
  renderCurrentScreen();

  message = "Da bat dau dang ky. Buoc 1: quet the moi trong 15 giay.";
  return true;
}

bool saveStudentFromWeb(String &message) {
  updateReadyToSave();
  if (!readyToSave) {
    message = "Chua du dieu kien luu.";
    return false;
  }
  if (findSlotByStudentId(currentStudent.maSV) >= 0) {
    message = "Ma sinh vien da ton tai.";
    return false;
  }
  if (cardCaptured && findSlotByUid(currentStudent.rfidUid) >= 0) {
    message = "The RFID bi trung.";
    return false;
  }
  if (fingerCaptured && findSlotByFingerId(currentStudent.fingerId) >= 0) {
    message = "Van tay bi trung.";
    return false;
  }

  int slot = findFirstEmptySlot();
  if (slot < 0) {
    message = "Bo nho da day.";
    return false;
  }

  StudentInfo toSave = currentStudent;
  if (!cardCaptured) toSave.rfidUid = "";
  if (!fingerCaptured) toSave.fingerId = -1;

  StoredStudentRecord rec = studentToRecord(toSave);
  writeRecord(slot, rec);
  refreshStoredCount();

  lastSavedDisplayName = currentStudent.ten;
  lastAttendanceStudentId = currentStudent.maSV;
  currentMode = MODE_SAVE_SUCCESS;
  saveSuccessMillis = millis();

  resetTempRegistration();
  resetFingerState();
  renderCurrentScreen();

  message = "Dang ky thanh cong.";
  return true;
}

void initFinger() {
  pinMode(FINGER_TCH_PIN, INPUT);
  pinMode(FINGER_VA_PIN, OUTPUT);
  digitalWrite(FINGER_VA_PIN, HIGH);
  fingerSerial.begin(FINGER_BAUD, SERIAL_8N1, FINGER_RX_PIN, FINGER_TX_PIN);
  delay(500);

  fingerSensorReady = false;
  for (int attempt = 0; attempt < 5; attempt++) {
    if (finger.verifyPassword()) {
      fingerSensorReady = true;
      break;
    }
    delay(200);
  }

  if (fingerSensorReady) {
    finger.getTemplateCount();
  }
}

bool isFingerIdUsed(int fingerId) {
  if (findSlotByFingerId(fingerId) >= 0) return true;
  if (!fingerSensorReady) return false;
  return finger.loadModel(fingerId) == FINGERPRINT_OK;
}

int nextAvailableFingerId() {
  for (int id = 1; id <= 127; id++) {
    if (!isFingerIdUsed(id)) return id;
  }
  return -1;
}

void goToFingerStep() {
  if (!fingerSensorReady) {
    currentFingerStatus = "CAM BIEN LOI";
    currentMode = MODE_REGISTER_SUMMARY;
    renderCurrentScreen();
    return;
  }
  currentMode = MODE_REGISTER_FINGER_STEP;
  currentFingerStatus = "CHO DAT TAY";
  stepStartMillis = millis();
  resetFingerState();
  renderCurrentScreen();
}

void processFingerRegisterStep() {
  if (!webServerEnabled) return;
  if (currentMode != MODE_REGISTER_FINGER_STEP || fingerCaptured) return;
  if (!fingerSensorReady) return;

  bool fingerTouched = (digitalRead(FINGER_TCH_PIN) == HIGH);

  if (millis() - stepStartMillis >= STEP_TIMEOUT_MS) {
    currentFingerStatus = "TIME OUT";
    renderCurrentScreen();
    delay(300);
    currentFingerStatus = "Chua dang ky";
    currentMode = MODE_REGISTER_SUMMARY;
    updateReadyToSave();
    resetFingerState();
    renderCurrentScreen();
    return;
  }

  uint8_t p;
  switch (fingerEnrollStage) {
    case FE_WAIT_FIRST:
      if (!fingerTouched) return;
      p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER) return;
      if (p != FINGERPRINT_OK) {
        logFingerResult("getImage first", p);
        currentFingerStatus = "LOI DOC TAY";
        renderCurrentScreen();
        return;
      }
      logFingerResult("getImage first", p);
      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
        logFingerResult("image2Tz first", p);
        currentFingerStatus = "LOI XU LY ANH";
        renderCurrentScreen();
        return;
      }
      logFingerResult("image2Tz first", p);
      p = finger.fingerFastSearch();
      if (p == FINGERPRINT_OK) {
        currentFingerStatus = "VAN TAY TRUNG";
        renderCurrentScreen();
        stepStartMillis = millis();
        return;
      }
      logFingerResult("fingerFastSearch first", p);
      pendingFingerId = nextAvailableFingerId();
      if (pendingFingerId < 0) {
        currentFingerStatus = "HET O NHO TAY";
        currentMode = MODE_REGISTER_SUMMARY;
        renderCurrentScreen();
        return;
      }
      currentFingerStatus = "NHA TAY RA";
      fingerEnrollStage = FE_WAIT_REMOVE;
      renderCurrentScreen();
      break;

    case FE_WAIT_REMOVE:
      if (!fingerTouched) {
        currentFingerStatus = "DAT LAI LAN 2";
        fingerEnrollStage = FE_WAIT_SECOND;
        renderCurrentScreen();
      }
      break;

    case FE_WAIT_SECOND:
      if (!fingerTouched) return;
      p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER) return;
      if (p != FINGERPRINT_OK) {
        logFingerResult("getImage second", p);
        currentFingerStatus = "LOI DOC TAY";
        renderCurrentScreen();
        return;
      }
      logFingerResult("getImage second", p);
      p = finger.image2Tz(2);
      if (p != FINGERPRINT_OK) {
        logFingerResult("image2Tz second", p);
        currentFingerStatus = "LOI LAN 2";
        fingerEnrollStage = FE_WAIT_FIRST;
        stepStartMillis = millis();
        renderCurrentScreen();
        return;
      }
      logFingerResult("image2Tz second", p);
      p = finger.createModel();
      if (p != FINGERPRINT_OK) {
        logFingerResult("createModel", p);
        currentFingerStatus = "2 LAN KHONG KHOP";
        fingerEnrollStage = FE_WAIT_FIRST;
        stepStartMillis = millis();
        renderCurrentScreen();
        return;
      }
      logFingerResult("createModel", p);
      p = finger.storeModel(pendingFingerId);
      if (p != FINGERPRINT_OK) {
        logFingerResult("storeModel", p);
        currentFingerStatus = "LOI LUU TAY";
        fingerEnrollStage = FE_WAIT_FIRST;
        stepStartMillis = millis();
        renderCurrentScreen();
        return;
      }
      logFingerResult("storeModel", p);
      currentStudent.fingerId = pendingFingerId;
      fingerCaptured = true;
      currentFingerStatus = "DA DANG KY TAY";
      currentMode = MODE_REGISTER_SUMMARY;
      updateReadyToSave();
      resetFingerState();
      renderCurrentScreen();
      break;
  }
}

void showAttendanceStudentName(const String &ten, const String &maSV) {
  lastSavedDisplayName = ten;
  lastAttendanceStudentId = maSV;
  lastAttendanceWasIn = isCurrentAttendanceIn();
  attendanceNameShowing = true;
  attendanceNameMillis = millis();
  showAttendanceName(ten, maSV, lastAttendanceWasIn);
}

void processFingerAttendance() {
  if (!fingerSensorReady) return;
  if (webServerEnabled || currentMode != MODE_ATTENDANCE_IDLE) return;
  if (attendanceNameShowing || duplicateWarningActive || speakerIsBusy()) return;

  static bool fingerTouchLatched = false;
  bool fingerTouched = (digitalRead(FINGER_TCH_PIN) == HIGH);

  if (!fingerTouched) {
    fingerTouchLatched = false;
    return;
  }

  if (fingerTouchLatched) return;
  if (millis() - lastFingerAttendanceCheck < 300) return;
  fingerTouchLatched = true;
  lastFingerAttendanceCheck = millis();

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    if (p != FINGERPRINT_NOFINGER) logFingerResult("attendance getImage", p);
    if (p == FINGERPRINT_NOFINGER) fingerTouchLatched = false;
    return;
  }
  logFingerResult("attendance getImage", p);

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    logFingerResult("attendance image2Tz", p);
    return;
  }
  logFingerResult("attendance image2Tz", p);

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    logFingerResult("attendance search", p);
    showAttendanceErrorTemp("VAN TAY KHONG DUNG", "VUI LONG THU LAI");
    return;
  }

  int slot = findSlotByFingerId(finger.fingerID);
  if (slot < 0) {
    showAttendanceErrorTemp("VAN TAY CHUA DK", "VUI LONG THU LAI");
    return;
  }

  StudentInfo s = recordToStudent(gRecords[slot]);
  handleAttendanceAccepted(s, true);
}

void initRFID() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  mfrc522.PCD_Init();
  delay(4);
  rfidReaderReady = true;
}

void finishCardRead() {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void handleRFID() {
  if (!rfidReaderReady) return;
  if (!webServerEnabled && currentMode == MODE_ATTENDANCE_IDLE &&
      (attendanceNameShowing || duplicateWarningActive || speakerIsBusy())) {
    return;
  }
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uidColon = uidToStringColon(&mfrc522.uid);
  String uidNorm = normalizeUid(uidColon);
  Serial.println("[RFID] UID raw: " + uidColon + " | norm: " + uidNorm +
                 " | mode: " + modeToText(currentMode) +
                 " | web: " + String(webServerEnabled ? "ON" : "OFF"));

  if (uidNorm == normalizeUid(String(MASTER_UID))) {
    if (webServerEnabled) closePortal();
    else openPortal();
    finishCardRead();
    delay(300);
    return;
  }

  if (webServerEnabled && currentMode == MODE_REGISTER_CARD_STEP) {
    currentStudent.rfidUid = uidColon;
    if (findSlotByUid(uidColon) >= 0) {
      currentCardStatus = "THE BI TRUNG";
      renderCurrentScreen();
      stepStartMillis = millis();
    } else {
      cardCaptured = true;
      currentCardStatus = "DA QUET THE MOI";
      goToFingerStep();
    }
    finishCardRead();
    delay(300);
    return;
  }

  if (!webServerEnabled && currentMode == MODE_ATTENDANCE_IDLE) {
    int slot = findSlotByUid(uidColon);
    if (slot >= 0) {
      Serial.println("[RFID] Matched stored card at slot " + String(slot));
      StudentInfo s = recordToStudent(gRecords[slot]);
      handleAttendanceAccepted(s, false);
    } else {
      Serial.println("[RFID] Card not found in storage");
      showAttendanceErrorTemp("THE KHONG HOP LE", "VUI LONG THU LAI");
    }
    finishCardRead();
    delay(300);
    return;
  }

  Serial.println("[RFID] Card ignored because current mode does not accept attendance scan");
  finishCardRead();
  delay(300);
}

void handleCardStepTimeout() {
  if (currentMode != MODE_REGISTER_CARD_STEP || cardCaptured) return;
  if (millis() - stepStartMillis < STEP_TIMEOUT_MS) return;

  currentCardStatus = "TIME OUT";
  renderCurrentScreen();
  delay(300);
  currentCardStatus = "Chua quet the moi";
  goToFingerStep();
}

void handleSaveSuccessTimeout() {
  if (currentMode != MODE_SAVE_SUCCESS) return;
  if (millis() - saveSuccessMillis < SAVE_SUCCESS_SHOW_MS) return;

  currentMode = webServerEnabled ? MODE_WEBSERVER_OPEN : MODE_ATTENDANCE_IDLE;
  attendanceNameShowing = false;
  renderCurrentScreen();
}

void handleAttendanceNameTimeout() {
  if (!attendanceNameShowing) return;
  if (speakerIsBusy()) return;
  attendanceNameShowing = false;
  if (currentMode == MODE_ATTENDANCE_IDLE) showAttendanceScreen();
}

void handleDuplicateWarningTimeout() {
  if (!duplicateWarningActive) return;

  if (duplicateWarningWaitingForSpeech) {
    if (speakerIsBusy()) return;
    duplicateWarningWaitingForSpeech = false;
    duplicateWarningActive = false;
    restoreAttendanceIdleScreen();
    return;
  }

  if (millis() - duplicateWarningStartMillis < 1500) return;
  duplicateWarningActive = false;
  restoreAttendanceIdleScreen();
}

void handleAttendanceSwitchChange() {
  int level = digitalRead(ATTENDANCE_SWITCH_PIN);
  if (level == lastAttendanceSwitchLevel) return;
  lastAttendanceSwitchLevel = level;

  if (currentMode == MODE_ATTENDANCE_IDLE && !attendanceNameShowing && !duplicateWarningActive) {
    showAttendanceScreen();
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  pinMode(ATTENDANCE_SWITCH_PIN, INPUT_PULLUP);
  lastAttendanceSwitchLevel = digitalRead(ATTENDANCE_SWITCH_PIN);
  initLCD();
  showBootWifiScreen("KHOI DONG HE THONG", "DANG TAI DU LIEU", "", "");
  clearAttendanceLogs();
  initStorage();
  initWiFi();
  initNTPTime();
  initWebServerRoutes();
  resetTempRegistration();
  if (!runHardwareChecks()) {
    return;
  }
  currentMode = MODE_ATTENDANCE_IDLE;
  renderCurrentScreen();
  initGoogleSheetSender();
}

void loop() {
  if (!systemHardwareReady) {
    delay(100);
    return;
  }
  syncAttendanceLogDay();
  handleWiFiReconnect();
  handleWebServerClients();
  handleAttendanceSwitchChange();
  handleRFID();
  handleCardStepTimeout();
  processFingerRegisterStep();
  processFingerAttendance();
  handleSaveSuccessTimeout();
  handleAttendanceNameTimeout();
  speakerLoop();
  handleDuplicateWarningTimeout();
}
