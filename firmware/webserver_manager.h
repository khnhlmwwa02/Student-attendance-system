#pragma once
#include <WebServer.h>
#include "globals.h"

static WebServer server(80);
static bool webRoutesRegistered = false;

inline String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

inline String htmlPage() {
  return R"HTML(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8" />
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Hệ thống điểm danh sinh viên</title>
  <style>
    :root {
      --bg: #f4f7fb;
      --card: #ffffff;
      --text: #1f2937;
      --muted: #6b7280;
      --line: #dbe3ef;
      --blue: #1d4ed8;
      --blue-soft: #dbeafe;
      --green: #15803d;
      --green-soft: #dcfce7;
      --red: #b91c1c;
      --red-soft: #fee2e2;
      --yellow: #b45309;
      --yellow-soft: #fef3c7;
      --shadow: 0 10px 28px rgba(15, 23, 42, 0.08);
      --radius: 18px;
    }
    * { box-sizing: border-box; }
    body { margin: 0; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif; background: var(--bg); color: var(--text); }
    .wrap { max-width: 1240px; margin: 0 auto; padding: 20px; }
    .header { background: linear-gradient(135deg, #1d4ed8, #2563eb); color: white; padding: 22px; border-radius: 22px; box-shadow: var(--shadow); }
    .header h1 { margin: 0 0 8px; font-size: 28px; }
    .header p { margin: 0; opacity: 0.95; }
    .menu { display: flex; gap: 12px; margin: 18px 0; flex-wrap: wrap; }
    .menu button { border: none; border-radius: 14px; padding: 12px 18px; font-size: 15px; font-weight: 700; cursor: pointer; background: white; color: var(--text); box-shadow: var(--shadow); transition: transform .15s ease, background .15s ease; }
    .menu button.active { background: var(--blue); color: white; }
    .menu button:hover { transform: translateY(-1px); }
    .panel { display: none; animation: fade .2s ease; }
    .panel.active { display: block; }
    @keyframes fade { from { opacity: 0; transform: translateY(4px);} to { opacity: 1; transform: translateY(0);} }
    .grid { display: grid; grid-template-columns: 1.05fr 0.95fr; gap: 18px; align-items: start; }
    .card { background: var(--card); border-radius: var(--radius); padding: 20px; box-shadow: var(--shadow); }
    .card h2 { margin: 0 0 16px; font-size: 22px; }
    .card h3 { margin: 0 0 14px; font-size: 18px; }
    .muted { color: var(--muted); font-size: 14px; }
    form { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .full { grid-column: 1 / -1; }
    label { display: block; font-weight: 700; margin-bottom: 8px; font-size: 14px; }
    input, textarea, select { width: 100%; padding: 12px 13px; border: 1px solid var(--line); border-radius: 12px; font-size: 14px; background: white; outline: none; }
    input:focus, textarea:focus, select:focus { border-color: var(--blue); box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.12); }
    textarea { min-height: 86px; resize: vertical; }
    .actions { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 2px; }
    .btn { border: none; border-radius: 12px; padding: 12px 16px; font-size: 14px; font-weight: 700; cursor: pointer; }
    .btn-primary { background: var(--blue); color: white; }
    .btn-secondary { background: #e5e7eb; color: #111827; }
    .btn-green { background: var(--green); color: white; }
    .btn-red { background: var(--red); color: white; }
    .btn:disabled { cursor: not-allowed; }
    .btn-green:disabled { background: #cfead7; color: #4b6b56; box-shadow: none; opacity: 1; }
    .btn.active-filter { background: var(--blue); color: white; }
    .status-grid { display: grid; gap: 12px; }
    .status-box { border: 1px solid var(--line); border-radius: 14px; padding: 14px; background: #fafcff; }
    .badge { display: inline-block; font-size: 13px; font-weight: 700; border-radius: 999px; padding: 7px 11px; }
    .badge-yellow { background: var(--yellow-soft); color: var(--yellow); }
    .badge-green { background: var(--green-soft); color: var(--green); }
    .badge-blue { background: var(--blue-soft); color: var(--blue); }
    .badge-red { background: var(--red-soft); color: var(--red); }
    .lcd { background: #0f3f1f; color: #9dfb8b; border-radius: 16px; padding: 16px; font-family: Consolas, monospace; box-shadow: inset 0 0 0 2px rgba(255,255,255,0.05); }
    .lcd-row { min-height: 24px; white-space: pre; letter-spacing: .3px; }
    .toolbar { display: flex; gap: 10px; flex-wrap: wrap; margin-bottom: 14px; }
    .toolbar input { min-width: 220px; flex: 1; }
    .table-wrap { overflow: auto; border: 1px solid var(--line); border-radius: 16px; background: white; }
    table { width: 100%; border-collapse: collapse; min-width: 640px; }
    thead th { position: sticky; top: 0; background: #eff6ff; color: #1e3a8a; font-size: 14px; text-align: left; padding: 12px; border-bottom: 1px solid var(--line); }
    tbody td { padding: 12px; border-bottom: 1px solid #edf2f7; font-size: 14px; }
    tbody tr { cursor: pointer; }
    tbody tr:hover { background: #f8fbff; }
    .danger-cell button { background: var(--red); color: white; border: none; border-radius: 10px; padding: 8px 12px; font-weight: 700; cursor: pointer; }
    .detail-box { border: 1px solid var(--line); border-radius: 16px; padding: 18px; background: #fbfdff; }
    .msg { min-height: 22px; margin-top: 10px; font-weight: 700; white-space: pre-line; }
    .hidden { display: none; }
    .status-pill { display: inline-flex; align-items: center; gap: 8px; font-weight: 700; }
    .status-icon { width: 24px; height: 24px; border-radius: 999px; display: inline-flex; align-items: center; justify-content: center; font-size: 14px; color: white; }
    .status-icon.ok { background: var(--green); }
    .status-icon.late { background: var(--red); }
    @media (max-width: 960px) { .grid, form { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="header">
      <h1>Hệ thống điểm danh sinh viên</h1>
      <p>Phát triển từ giao diện bạn thiết kế, nối dữ liệu thật từ ESP32.</p>
    </div>

    <div class="menu">
      <button id="btnRegister" class="active" onclick="showSection('register')">Đăng ký thông tin</button>
      <button id="btnLookup" onclick="showSection('lookup')">Tra cứu sinh viên</button>
      <button id="btnAttendance" onclick="showSection('attendance')">Quản lý điểm danh</button>
    </div>

    <div class="panel active" id="registerPanel">
      <div class="grid">
        <div class="card">
          <h2>Đăng ký thông tin sinh viên</h2>
          <form id="registerForm">
            <div><label for="maSV">Mã sinh viên</label><input id="maSV" name="maSV" /></div>
            <div><label for="lop">Lớp</label><input id="lop" name="lop" /></div>
            <div><label for="ho">Họ</label><input id="ho" name="ho" /></div>
            <div><label for="ten">Tên</label><input id="ten" name="ten" /></div>
            <div><label for="khoa">Khoa</label><input id="khoa" name="khoa" /></div>
            <div><label for="khoaHoc">Khóa học</label><input id="khoaHoc" name="khoaHoc" /></div>
            <div><label for="sdt">Số điện thoại</label><input id="sdt" name="sdt" /></div>
            <div><label for="ghiChu">Ghi chú</label><textarea id="ghiChu" name="ghiChu"></textarea></div>
            <div class="full actions">
              <button type="button" class="btn btn-primary" onclick="startRegister()">Bắt đầu đăng ký</button>
              <button type="button" class="btn btn-green" id="saveBtn" onclick="saveStudent()" disabled>Lưu thông tin</button>
              <button type="button" class="btn btn-secondary" onclick="clearRegisterForm()">Xóa form</button>
            </div>
          </form>
          <div class="msg" id="registerMsg"></div>
        </div>

        <div class="card">
          <h2>Trạng thái hệ thống</h2>
          <div class="status-grid">
            <div class="status-box"><strong>Chế độ</strong><div style="margin-top:8px"><span id="modeBadge" class="badge badge-blue">Đang tải...</span></div></div>
            <div class="status-box"><strong>RFID</strong><div style="margin-top:8px"><span id="cardBadge" class="badge badge-yellow">Chưa quẹt thẻ mới</span></div></div>
            <div class="status-box"><strong>Vân tay</strong><div style="margin-top:8px"><span id="fingerBadge" class="badge badge-yellow">Chưa đăng ký</span></div></div>
            <div class="status-box"><strong>Điều kiện lưu</strong><div style="margin-top:8px"><span id="saveBadge" class="badge badge-red">Chưa đủ điều kiện</span></div></div>
            <div class="status-box"><strong>Session</strong><div id="sessionText" style="margin-top:8px">0</div></div>
          </div>
          <div class="msg" id="statusMsg"></div>
        </div>
      </div>

      <div class="grid" style="margin-top:18px;">
        <div class="card">
          <h3>Mô phỏng LCD 20x4</h3>
          <div class="lcd">
            <div class="lcd-row" id="lcd1"></div>
            <div class="lcd-row" id="lcd2"></div>
            <div class="lcd-row" id="lcd3"></div>
            <div class="lcd-row" id="lcd4"></div>
          </div>
        </div>
        <div class="card">
          <h3>Luồng hoạt động</h3>
          <p class="muted">Điền đủ thông tin rồi bấm <b>Bắt đầu đăng ký</b>. Hệ thống sẽ lần lượt chờ quẹt thẻ mới và đăng ký vân tay. Khi đủ điều kiện, nút <b>Lưu thông tin</b> sẽ sáng lên.</p>
        </div>
      </div>
    </div>

    <div class="panel" id="lookupPanel">
      <div class="grid">
        <div class="card">
          <h2>Tra cứu sinh viên</h2>
          <div class="toolbar">
            <input id="searchInput" placeholder="Tìm theo MSSV, tên, lớp..." oninput="filterStudents()" />
            <button class="btn btn-secondary" onclick="loadStudents()">Làm mới</button>
          </div>
          <div class="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>MSSV</th>
                  <th>Tên SV</th>
                  <th>Lớp</th>
                  <th>Xóa</th>
                </tr>
              </thead>
              <tbody id="studentTableBody">
                <tr><td colspan="4">Đang tải dữ liệu...</td></tr>
              </tbody>
            </table>
          </div>
          <div class="msg" id="lookupMsg"></div>
        </div>

        <div class="card">
          <h2>Thông tin chi tiết sinh viên</h2>
          <div class="detail-box">
            <form id="detailForm">
              <div><label>Mã sinh viên</label><input id="d_maSV" disabled /></div>
              <div><label>Lớp</label><input id="d_lop" disabled /></div>
              <div><label>Họ</label><input id="d_ho" disabled /></div>
              <div><label>Tên</label><input id="d_ten" disabled /></div>
              <div><label>Khoa</label><input id="d_khoa" disabled /></div>
              <div><label>Khóa học</label><input id="d_khoaHoc" disabled /></div>
              <div><label>Số điện thoại</label><input id="d_sdt" disabled /></div>
              <div><label>UID thẻ RFID</label><input id="d_rfidUid" disabled /></div>
              <div class="full"><label>Ghi chú</label><textarea id="d_ghiChu" disabled></textarea></div>
              <div><label>ID vân tay</label><input id="d_fingerId" disabled /></div>
              <div><label>Trạng thái</label><input id="d_status" disabled /></div>
              <div class="full actions">
                <button type="button" class="btn btn-red" id="detailDeleteBtn" onclick="deleteCurrentStudent()" disabled>Xóa sinh viên</button>
              </div>
            </form>
          </div>
          <div class="msg" id="detailMsg">Chọn một sinh viên trong bảng để xem chi tiết.</div>
        </div>
      </div>
    </div>

    <div class="panel" id="attendancePanel">
      <div class="card">
        <h2>Quản lý điểm danh trong ngày</h2>
        <div class="toolbar">
          <input id="attendanceDateText" value="Đang tải..." disabled />
          <button class="btn btn-secondary active-filter" id="attendanceFilterInBtn" onclick="setAttendanceFilter('IN')">IN</button>
          <button class="btn btn-secondary" id="attendanceFilterOutBtn" onclick="setAttendanceFilter('OUT')">OUT</button>
          <button class="btn btn-secondary" onclick="loadAttendanceLogs()">Làm mới</button>
        </div>
        <div class="table-wrap">
          <table>
            <thead>
              <tr>
                <th>STT</th>
                <th>Họ và tên</th>
                <th>MSSV</th>
                <th>Lớp</th>
                <th>Thời gian điểm danh</th>
                <th>Trạng thái</th>
                <th>Ghi chú</th>
              </tr>
            </thead>
            <tbody id="attendanceTableBody">
              <tr><td colspan="7">Đang tải dữ liệu...</td></tr>
            </tbody>
          </table>
        </div>
        <div class="msg" id="attendanceMsg"></div>
      </div>
    </div>
  </div>

  <script>
    let lastSessionId = -1;
    let allStudents = [];
    let currentStudentId = "";
    let allAttendanceLogs = [];
    let attendanceFilter = 'IN';

    function setActiveMenu(section) {
      document.getElementById('btnRegister').classList.toggle('active', section === 'register');
      document.getElementById('btnLookup').classList.toggle('active', section === 'lookup');
      document.getElementById('btnAttendance').classList.toggle('active', section === 'attendance');
    }

    function showSection(section) {
      document.getElementById('registerPanel').classList.toggle('active', section === 'register');
      document.getElementById('lookupPanel').classList.toggle('active', section === 'lookup');
      document.getElementById('attendancePanel').classList.toggle('active', section === 'attendance');
      setActiveMenu(section);
      if (section === 'lookup') loadStudents();
      if (section === 'attendance') loadAttendanceLogs();
    }

    function escapeHtml(text) {
      return String(text || '').replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;').replaceAll('"', '&quot;').replaceAll("'", '&#39;');
    }

    function clearRegisterForm() {
      document.getElementById('registerForm').reset();
      document.getElementById('registerMsg').innerText = 'Đã xóa dữ liệu trên form.';
    }

    function clearDetailForm() {
      currentStudentId = "";
      ['maSV','lop','ho','ten','khoa','khoaHoc','sdt','rfidUid','ghiChu','fingerId','status'].forEach(k => {
        const el = document.getElementById('d_' + k);
        if (el) el.value = '';
      });
      document.getElementById('detailDeleteBtn').disabled = true;
    }

    function setBadge(id, text, cls) {
      const el = document.getElementById(id);
      el.textContent = text;
      el.className = 'badge ' + cls;
    }

    function setAttendanceFilter(type) {
      attendanceFilter = type;
      document.getElementById('attendanceFilterInBtn').classList.toggle('active-filter', type === 'IN');
      document.getElementById('attendanceFilterOutBtn').classList.toggle('active-filter', type === 'OUT');
      document.getElementById('attendanceMsg').innerText = 'Tổng số lượt ' + attendanceFilter + ' hôm nay: ' + allAttendanceLogs.filter(item => item.direction === attendanceFilter).length;
      renderAttendanceTable(allAttendanceLogs);
    }

    function renderAttendanceTable(logs) {
      const body = document.getElementById('attendanceTableBody');
      const filteredLogs = logs.filter(item => item.direction === attendanceFilter);
      if (!filteredLogs.length) {
        body.innerHTML = '<tr><td colspan="7">Chưa có dữ liệu điểm danh trong ngày.</td></tr>';
        return;
      }

      body.innerHTML = filteredLogs.map(item => `
        <tr>
          <td>${item.stt}</td>
          <td>${escapeHtml(item.fullName)}</td>
          <td>${escapeHtml(item.maSV)}</td>
          <td>${escapeHtml(item.lop)}</td>
          <td>${escapeHtml(item.timeText)}</td>
          <td>
            <span class="status-pill">
              <span class="status-icon ${item.onTime ? 'ok' : 'late'}">${item.onTime ? '&#10003;' : '&#10007;'}</span>
              <span>${escapeHtml(item.statusText)}</span>
            </span>
          </td>
          <td>${escapeHtml(item.noteText)}</td>
        </tr>
      `).join('');
    }

    async function loadAttendanceLogs() {
      const res = await fetch('/api/attendance-logs');
      const json = await res.json();
      allAttendanceLogs = json.logs || [];
      document.getElementById('attendanceDateText').value = json.dateLabel || 'N/A';
      const filteredCount = allAttendanceLogs.filter(item => item.direction === attendanceFilter).length;
      document.getElementById('attendanceMsg').innerText = 'Tổng số lượt ' + attendanceFilter + ' hôm nay: ' + filteredCount;
      renderAttendanceTable(allAttendanceLogs);
    }

    async function startRegister() {
      const data = new URLSearchParams(new FormData(document.getElementById('registerForm')));
      const res = await fetch('/api/start-register', { method: 'POST', body: data });
      const json = await res.json();
      document.getElementById('registerMsg').innerText = json.message || '';
      refreshStatus();
    }

    async function saveStudent() {
      const res = await fetch('/api/save', { method: 'POST' });
      const json = await res.json();
      document.getElementById('registerMsg').innerText = json.message || '';
      if (json.success) {
        document.getElementById('registerForm').reset();
        loadStudents();
      }
      refreshStatus();
    }

    async function loadStudents() {
      const res = await fetch('/api/students');
      const json = await res.json();
      allStudents = json.students || [];
      renderStudentTable(allStudents);
    }

    function renderStudentTable(students) {
      const body = document.getElementById('studentTableBody');
      if (!students.length) {
        body.innerHTML = '<tr><td colspan="4">Chưa có sinh viên nào được lưu.</td></tr>';
        return;
      }

      body.innerHTML = students.map(s => `
        <tr data-id="${escapeHtml(s.maSV)}" onclick="showStudentDetail('${encodeURIComponent(s.maSV)}')">
          <td>${escapeHtml(s.maSV)}</td>
          <td>${escapeHtml(s.ho + ' ' + s.ten)}</td>
          <td>${escapeHtml(s.lop)}</td>
          <td class="danger-cell"><button type="button" onclick="event.stopPropagation(); deleteStudent('${encodeURIComponent(s.maSV)}')">Xóa</button></td>
        </tr>
      `).join('');
    }

    function filterStudents() {
      const q = document.getElementById('searchInput').value.trim().toLowerCase();
      if (!q) {
        renderStudentTable(allStudents);
        return;
      }
      const filtered = allStudents.filter(s => (
        s.maSV.toLowerCase().includes(q) ||
        (s.ho + ' ' + s.ten).toLowerCase().includes(q) ||
        s.lop.toLowerCase().includes(q)
      ));
      renderStudentTable(filtered);
    }

    async function showStudentDetail(studentIdEncoded) {
      const studentId = decodeURIComponent(studentIdEncoded);
      const res = await fetch('/api/student?studentId=' + encodeURIComponent(studentId));
      const json = await res.json();
      if (!json.found) {
        document.getElementById('detailMsg').innerText = json.message || 'Không tìm thấy sinh viên.';
        clearDetailForm();
        return;
      }
      currentStudentId = json.maSV;
      document.getElementById('d_maSV').value = json.maSV || '';
      document.getElementById('d_lop').value = json.lop || '';
      document.getElementById('d_ho').value = json.ho || '';
      document.getElementById('d_ten').value = json.ten || '';
      document.getElementById('d_khoa').value = json.khoa || '';
      document.getElementById('d_khoaHoc').value = json.khoaHoc || '';
      document.getElementById('d_sdt').value = json.sdt || '';
      document.getElementById('d_rfidUid').value = json.rfidUid || '';
      document.getElementById('d_ghiChu').value = json.ghiChu || '';
      document.getElementById('d_fingerId').value = json.fingerId >= 0 ? json.fingerId : 'Chưa đăng ký';
      document.getElementById('d_status').value = json.hasCard || json.hasFinger ? 'Đã lưu' : 'Thiếu định danh';
      document.getElementById('detailDeleteBtn').disabled = false;
      document.getElementById('detailMsg').innerText = 'Đang xem thông tin chi tiết của sinh viên ' + json.maSV + '.';
    }

    async function deleteStudent(studentIdEncoded) {
      const studentId = decodeURIComponent(studentIdEncoded);
      if (!confirm('Bạn có chắc muốn xóa sinh viên ' + studentId + ' không?')) return;
      const body = new URLSearchParams();
      body.set('studentId', studentId);
      const res = await fetch('/api/delete-student', { method: 'POST', body });
      const json = await res.json();
      document.getElementById('lookupMsg').innerText = json.message || '';
      if (json.success && currentStudentId === studentId) {
        clearDetailForm();
        document.getElementById('detailMsg').innerText = 'Đã xóa sinh viên. Chọn dòng khác để xem chi tiết.';
      }
      await loadStudents();
      refreshStatus();
    }

    function deleteCurrentStudent() {
      if (!currentStudentId) return;
      deleteStudent(encodeURIComponent(currentStudentId));
    }

    async function refreshStatus() {
      const res = await fetch('/status');
      const s = await res.json();
      if (lastSessionId !== -1 && lastSessionId !== s.sessionId) {
        document.getElementById('registerForm').reset();
        document.getElementById('registerMsg').innerText = 'WebServer vừa được mở lại. Form đã được xóa trắng.';
        clearDetailForm();
        if (document.getElementById('lookupPanel').classList.contains('active')) loadStudents();
        if (document.getElementById('attendancePanel').classList.contains('active')) loadAttendanceLogs();
      }
      lastSessionId = s.sessionId;
      document.getElementById('sessionText').innerText = s.sessionId;
      setBadge('modeBadge', s.modeText, 'badge-blue');
      setBadge('cardBadge', s.cardText, s.cardOk ? 'badge-green' : 'badge-yellow');
      setBadge('fingerBadge', s.fingerText, s.fingerOk ? 'badge-green' : 'badge-yellow');
      setBadge('saveBadge', s.readyToSave ? 'Sẵn sàng lưu' : 'Chưa đủ điều kiện', s.readyToSave ? 'badge-green' : 'badge-red');
      document.getElementById('saveBtn').disabled = !s.readyToSave;
      document.getElementById('statusMsg').innerText = s.webEnabled ? 'WebServer đang mở' : 'WebServer đang tắt';
      document.getElementById('lcd1').innerText = s.lcd1;
      document.getElementById('lcd2').innerText = s.lcd2;
      document.getElementById('lcd3').innerText = s.lcd3;
      document.getElementById('lcd4').innerText = s.lcd4;
      if (document.getElementById('attendancePanel').classList.contains('active')) loadAttendanceLogs();
    }

    showSection('register');
    setInterval(refreshStatus, 1000);
    refreshStatus();
  </script>
</body>
</html>
)HTML";
}

inline void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", htmlPage());
}

inline void handleStatus() {
  updateReadyToSave();
  String l1, l2, l3, l4;
  fillLcdPreview(l1, l2, l3, l4);

  String json = "{";
  json += "\"webEnabled\":" + String(webServerEnabled ? "true" : "false") + ",";
  json += "\"sessionId\":" + String(portalSessionId) + ",";
  json += "\"modeText\":\"" + jsonEscape(modeToText(currentMode)) + "\",";
  json += "\"cardText\":\"" + jsonEscape(cardCaptured ? "Đã quẹt thẻ mới" : currentCardStatus) + "\",";
  json += "\"fingerText\":\"" + jsonEscape(fingerCaptured ? "Đã đăng ký tay" : currentFingerStatus) + "\",";
  json += "\"cardOk\":" + String(cardCaptured ? "true" : "false") + ",";
  json += "\"fingerOk\":" + String(fingerCaptured ? "true" : "false") + ",";
  json += "\"readyToSave\":" + String(readyToSave ? "true" : "false") + ",";
  json += "\"lcd1\":\"" + jsonEscape(l1) + "\",";
  json += "\"lcd2\":\"" + jsonEscape(l2) + "\",";
  json += "\"lcd3\":\"" + jsonEscape(l3) + "\",";
  json += "\"lcd4\":\"" + jsonEscape(l4) + "\"";
  json += "}";
  server.send(200, "application/json; charset=UTF-8", json);
}

inline void handleStartRegister() {
  currentStudent.maSV = server.arg("maSV");
  currentStudent.ho = server.arg("ho");
  currentStudent.ten = server.arg("ten");
  currentStudent.lop = server.arg("lop");
  currentStudent.khoa = server.arg("khoa");
  currentStudent.khoaHoc = server.arg("khoaHoc");
  currentStudent.sdt = server.arg("sdt");
  currentStudent.ghiChu = server.arg("ghiChu");

  String message;
  bool ok = startRegistrationFromWeb(message);
  server.send(200, "application/json; charset=UTF-8",
              String("{\"success\":") + (ok ? "true" : "false") +
              ",\"message\":\"" + jsonEscape(message) + "\"}");
}

inline void handleSave() {
  String message;
  bool ok = saveStudentFromWeb(message);
  server.send(200, "application/json; charset=UTF-8",
              String("{\"success\":") + (ok ? "true" : "false") +
              ",\"message\":\"" + jsonEscape(message) + "\"}");
}

inline void handleStudentList() {
  int count = getStoredStudentCount();
  String json = "{\"count\":" + String(count) + ",\"students\":[";

  bool first = true;
  for (int i = 0; i < count; i++) {
    StudentInfo out;
    bool hasCard = false;
    bool hasFinger = false;
    if (!getStoredStudentAt(i, out, hasCard, hasFinger)) continue;
    if (!first) json += ",";
    first = false;
    json += "{";
    json += "\"maSV\":\"" + jsonEscape(out.maSV) + "\",";
    json += "\"ho\":\"" + jsonEscape(out.ho) + "\",";
    json += "\"ten\":\"" + jsonEscape(out.ten) + "\",";
    json += "\"lop\":\"" + jsonEscape(out.lop) + "\",";
    json += "\"hasCard\":" + String(hasCard ? "true" : "false") + ",";
    json += "\"hasFinger\":" + String(hasFinger ? "true" : "false");
    json += "}";
  }

  json += "]}";
  server.send(200, "application/json; charset=UTF-8", json);
}

inline void handleStudentDetail() {
  String studentId = server.arg("studentId");
  StudentInfo out;
  bool hasCard = false;
  bool hasFinger = false;
  bool found = lookupStudent("studentId", studentId, out, hasCard, hasFinger);

  if (!found) {
    server.send(200, "application/json; charset=UTF-8", "{\"found\":false,\"message\":\"Không tìm thấy sinh viên.\"}");
    return;
  }

  String json = "{";
  json += "\"found\":true,";
  json += "\"maSV\":\"" + jsonEscape(out.maSV) + "\",";
  json += "\"ho\":\"" + jsonEscape(out.ho) + "\",";
  json += "\"ten\":\"" + jsonEscape(out.ten) + "\",";
  json += "\"lop\":\"" + jsonEscape(out.lop) + "\",";
  json += "\"khoa\":\"" + jsonEscape(out.khoa) + "\",";
  json += "\"khoaHoc\":\"" + jsonEscape(out.khoaHoc) + "\",";
  json += "\"sdt\":\"" + jsonEscape(out.sdt) + "\",";
  json += "\"ghiChu\":\"" + jsonEscape(out.ghiChu) + "\",";
  json += "\"rfidUid\":\"" + jsonEscape(out.rfidUid) + "\",";
  json += "\"fingerId\":" + String(out.fingerId) + ",";
  json += "\"hasCard\":" + String(hasCard ? "true" : "false") + ",";
  json += "\"hasFinger\":" + String(hasFinger ? "true" : "false");
  json += "}";
  server.send(200, "application/json; charset=UTF-8", json);
}

inline void handleDeleteStudent() {
  String studentId = server.arg("studentId");
  String message;
  bool ok = deleteStudentByStudentId(studentId, message);
  server.send(200, "application/json; charset=UTF-8",
              String("{\"success\":") + (ok ? "true" : "false") +
              ",\"message\":\"" + jsonEscape(message) + "\"}");
}

inline void handleAttendanceLogs() {
  int count = getAttendanceLogCount();
  String json = "{";
  json += "\"count\":" + String(count) + ",";
  json += "\"dateLabel\":\"" + jsonEscape(getAttendanceLogDateLabel()) + "\",";
  json += "\"logs\":[";

  bool first = true;
  for (int i = 0; i < count; i++) {
    AttendanceLogEntry entry;
    if (!getAttendanceLogAt(i, entry)) continue;

    if (!first) json += ",";
    first = false;
    json += "{";
    json += "\"stt\":" + String(entry.stt) + ",";
    json += "\"fullName\":\"" + jsonEscape(String(entry.ho) + " " + String(entry.ten)) + "\",";
    json += "\"maSV\":\"" + jsonEscape(String(entry.maSV)) + "\",";
    json += "\"lop\":\"" + jsonEscape(String(entry.lop)) + "\",";
    json += "\"timeText\":\"" + jsonEscape(String(entry.timeText)) + "\",";
    json += "\"direction\":\"" + String(entry.direction == ATTENDANCE_DIR_OUT ? "OUT" : "IN") + "\",";
    json += "\"onTime\":" + String(entry.onTime ? "true" : "false") + ",";
    json += "\"statusText\":\"" + jsonEscape(buildAttendanceStatusText(entry)) + "\",";
    json += "\"noteText\":\"" + jsonEscape(buildAttendanceNoteText(entry)) + "\"";
    json += "}";
  }

  json += "]}";
  server.send(200, "application/json; charset=UTF-8", json);
}

inline void initWebServerRoutes() {
  if (webRoutesRegistered) return;
  webRoutesRegistered = true;
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/api/start-register", HTTP_POST, handleStartRegister);
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/students", HTTP_GET, handleStudentList);
  server.on("/api/student", HTTP_GET, handleStudentDetail);
  server.on("/api/delete-student", HTTP_POST, handleDeleteStudent);
  server.on("/api/attendance-logs", HTTP_GET, handleAttendanceLogs);
}

inline void startWebServer() {
  initWebServerRoutes();
  server.begin();
}

inline void stopWebServer() {
  server.stop();
}

inline void handleWebServerClients() {
  if (webServerEnabled) server.handleClient();
}
