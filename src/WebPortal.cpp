#include "WebPortal.h"

WebPortal Portal;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP-SSTV Satellite Station</title>
<style>
:root {
  --bg: #0b0f19;
  --card: #151d30;
  --card-border: #24324f;
  --text: #f1f5f9;
  --muted: #94a3b8;
  --accent: #06b6d4;
  --accent-hover: #0891b2;
  --danger: #ef4444;
  --success: #10b981;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background-color: var(--bg);
  color: var(--text);
  padding: 16px;
  max-width: 600px;
  margin: 0 auto;
}
header {
  text-align: center;
  margin-bottom: 20px;
}
h1 { font-size: 1.5rem; color: var(--accent); margin-bottom: 4px; }
.subhead { font-size: 0.85rem; color: var(--muted); }
.badge {
  display: inline-block;
  padding: 3px 8px;
  border-radius: 12px;
  font-size: 0.75rem;
  font-weight: bold;
  margin-top: 6px;
}
.badge-idle { background: #334155; color: #cbd5e1; }
.badge-rx { background: #065f46; color: #6ee7b7; }
.badge-rec { background: #991b1b; color: #fca5a5; animation: pulse 1.5s infinite; }
@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }

.card {
  background: var(--card);
  border: 1px solid var(--card-border);
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3);
}
.card-title {
  font-size: 1rem;
  font-weight: 600;
  margin-bottom: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  color: var(--text);
}
.grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 12px; }
.stat-box {
  background: rgba(0,0,0,0.25);
  padding: 8px 12px;
  border-radius: 8px;
  border-left: 3px solid var(--accent);
}
.stat-label { font-size: 0.7rem; color: var(--muted); text-transform: uppercase; }
.stat-val { font-size: 1.05rem; font-weight: bold; margin-top: 2px; }

button {
  background: var(--accent);
  color: #fff;
  border: none;
  padding: 10px 14px;
  border-radius: 8px;
  font-size: 0.9rem;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.2s, transform 0.1s;
  width: 100%;
}
button:active { transform: scale(0.98); }
button.btn-danger { background: var(--danger); }
button.btn-sec { background: #334155; color: #f1f5f9; }
button.btn-sec:hover { background: #475569; }
button.btn-rec {
  padding: 16px;
  font-size: 1.1rem;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.btn-group { display: flex; gap: 8px; margin-bottom: 8px; }
.btn-group button { flex: 1; }

.file-item {
  display: flex;
  flex-direction: column;
  background: rgba(0,0,0,0.25);
  border-radius: 8px;
  padding: 10px;
  margin-bottom: 8px;
}
.file-info { display: flex; justify-content: space-between; font-size: 0.85rem; margin-bottom: 6px; }
audio { width: 100%; height: 32px; margin: 6px 0; border-radius: 4px; }
.file-actions { display: flex; gap: 8px; }
.file-actions a, .file-actions button { flex: 1; text-align: center; text-decoration: none; padding: 6px 10px; font-size: 0.8rem; border-radius: 6px; }

input, select {
  width: 100%;
  padding: 9px 12px;
  border-radius: 8px;
  border: 1px solid var(--card-border);
  background: #0f172a;
  color: #f1f5f9;
  font-size: 0.9rem;
  margin-bottom: 8px;
}
</style>
</head>
<body>

<header>
  <h1>ESP-SSTV Ground Station</h1>
  <div class="subhead">ARISS ISS 437.550 MHz & Robot 36 Lab | <a href="http://esp-sstv.local" style="color:var(--accent);">esp-sstv.local</a></div>
  <div id="statusBadge" class="badge badge-idle">STANDBY</div>
</header>

<!-- 1-Click Satellite Pass Recording Card -->
<div class="card">
  <button id="recordBtn" class="btn-rec" onclick="togglePassRecord()">START PASS RECORDING</button>
  <div id="recordStatus" style="font-size:0.8rem; color:var(--muted); text-align:center; margin-top:8px;">
    1-Click: Tunes 437.550 MHz + Auto-Doppler + Records .WAV to SD Card
  </div>
</div>

<!-- Live Telemetry Card -->
<div class="card">
  <div class="card-title">Live Radio Telemetry</div>
  <div class="grid">
    <div class="stat-box">
      <div class="stat-label">Frequency</div>
      <div id="statFreq" class="stat-val">437.550 MHz</div>
    </div>
    <div class="stat-box">
      <div class="stat-label">Doppler Offset</div>
      <div id="statDoppler" class="stat-val">+0.00 kHz</div>
    </div>
    <div class="stat-box">
      <div class="stat-label">Signal RSSI</div>
      <div id="statRssi" class="stat-val">- dBm</div>
    </div>
    <div class="stat-box">
      <div class="stat-label">Audio Tone</div>
      <div id="statTone" class="stat-val">-- Hz</div>
    </div>
  </div>

  <div class="card-title" style="font-size:0.9rem; margin-top:8px;">Doppler Shift Stepping</div>
  <div class="btn-group">
    <button class="btn-sec" onclick="sendCmd('2')">+2.5 kHz</button>
    <button class="btn-sec" onclick="sendCmd('3')">-2.5 kHz</button>
    <button class="btn-sec" onclick="sendCmd('4')">Auto Pass</button>
  </div>
</div>

<!-- Transmitter & Lab Testing -->
<div class="card">
  <div class="card-title">Test Signal Generator</div>
  <div class="btn-group">
    <button class="btn-sec" onclick="sendCmd('5')">TX CC1101 (2-FSK)</button>
    <button class="btn-sec" onclick="sendCmd('7')">TX FS1000A (ASK)</button>
  </div>
  <button class="btn-sec" onclick="sendCmd('6')">Send SSTV Test Tones (1200-2300 Hz)</button>
</div>

<!-- SD Card Recorded Passes -->
<div class="card">
  <div class="card-title">
    <span>Recorded Satellite Passes (.WAV)</span>
    <button style="width:auto; padding:4px 8px; font-size:0.75rem;" onclick="loadFiles()">Refresh</button>
  </div>
  <div id="fileList">Loading recordings...</div>
</div>

<!-- WiFi Settings & Scanner -->
<div class="card">
  <div class="card-title">WiFi Network Settings</div>
  <button class="btn-sec" style="margin-bottom:8px;" onclick="scanWiFi()">Scan WiFi Networks</button>
  <select id="wifiSsid"><option value="">Select or type network SSID...</option></select>
  <input type="text" id="customSsid" placeholder="Or enter SSID manually">
  <input type="password" id="wifiPass" placeholder="WiFi Password">
  <button onclick="connectWiFi()">Connect to WiFi</button>
</div>

<script>
let isRecording = false;

function updateStatus() {
  fetch('/api/status')
    .then(r => r.json())
    .then(d => {
      document.getElementById('statFreq').innerText = d.freq.toFixed(3) + ' MHz';
      document.getElementById('statDoppler').innerText = (d.doppler >= 0 ? '+' : '') + d.doppler.toFixed(2) + ' kHz';
      document.getElementById('statRssi').innerText = d.rssi + ' dBm';
      document.getElementById('statTone').innerText = d.toneHz > 0 ? Math.round(d.toneHz) + ' Hz' : '-- Hz';

      const badge = document.getElementById('statusBadge');
      const recBtn = document.getElementById('recordBtn');

      isRecording = d.isRecording;
      if (isRecording) {
        badge.className = 'badge badge-rec';
        badge.innerText = 'RECORDING PASS';
        recBtn.className = 'btn-rec btn-danger';
        recBtn.innerText = 'STOP & SAVE RECORDING';
        document.getElementById('recordStatus').innerText = 'Active file: ' + d.recFile + ' (' + Math.round(d.recBytes/1024) + ' KB)';
      } else if (d.mode === 'RX') {
        badge.className = 'badge badge-rx';
        badge.innerText = 'RECEIVING (NO RECORD)';
        recBtn.className = 'btn-rec';
        recBtn.innerText = 'START PASS RECORDING';
        document.getElementById('recordStatus').innerText = 'Receiver listening. Click to start recording to SD.';
      } else {
        badge.className = 'badge badge-idle';
        badge.innerText = 'IDLE';
        recBtn.className = 'btn-rec';
        recBtn.innerText = 'START PASS RECORDING';
        document.getElementById('recordStatus').innerText = '1-Click: Tunes 437.550 MHz + Auto-Doppler + Records .WAV to SD Card';
      }
    })
    .catch(e => console.log(e));
}

function sendCmd(c) {
  fetch('/api/action?cmd=' + c, {method: 'POST'})
    .then(() => setTimeout(updateStatus, 300));
}

function togglePassRecord() {
  if (isRecording) {
    sendCmd('0'); // Stop
  } else {
    // 1-click start: Start RX ('1') + Auto Doppler ('4')
    fetch('/api/action?cmd=1', {method: 'POST'})
      .then(() => fetch('/api/action?cmd=4', {method: 'POST'}))
      .then(() => {
        setTimeout(updateStatus, 300);
        setTimeout(loadFiles, 1000);
      });
  }
}

function loadFiles() {
  fetch('/api/files')
    .then(r => r.json())
    .then(files => {
      const container = document.getElementById('fileList');
      if (!files || files.length === 0) {
        container.innerHTML = '<div style="color:var(--muted); font-size:0.85rem;">No recordings on SD card yet.</div>';
        return;
      }
      let html = '';
      files.forEach(f => {
        const sizeKb = Math.round(f.size / 1024);
        const dur = Math.round(f.duration);
        html += `
          <div class="file-item">
            <div class="file-info">
              <strong>${f.name}</strong>
              <span>${sizeKb} KB (${dur}s)</span>
            </div>
            <audio controls preload="none" src="/download?file=${encodeURIComponent(f.name)}"></audio>
            <div class="file-actions">
              <a href="/download?file=${encodeURIComponent(f.name)}" download class="btn-sec" style="background:#0284c7; color:#fff;">Download WAV</a>
              <button class="btn-danger" style="padding:6px 10px; font-size:0.8rem;" onclick="deleteFile('${f.name}')">Delete</button>
            </div>
          </div>
        `;
      });
      container.innerHTML = html;
    })
    .catch(() => {
      document.getElementById('fileList').innerHTML = '<div style="color:var(--danger); font-size:0.85rem;">SD card not detected or read error.</div>';
    });
}

function deleteFile(name) {
  if (!confirm('Delete ' + name + '?')) return;
  fetch('/api/delete?file=' + encodeURIComponent(name), {method: 'POST'})
    .then(() => loadFiles());
}

function scanWiFi() {
  const sel = document.getElementById('wifiSsid');
  sel.innerHTML = '<option>Scanning...</option>';
  fetch('/api/scan')
    .then(r => r.json())
    .then(list => {
      sel.innerHTML = '<option value="">Select network...</option>';
      list.forEach(n => {
        sel.innerHTML += `<option value="${n.ssid}">${n.ssid} (${n.rssi} dBm)</option>`;
      });
    });
}

function connectWiFi() {
  const selSsid = document.getElementById('wifiSsid').value;
  const customSsid = document.getElementById('customSsid').value;
  const ssid = customSsid ? customSsid : selSsid;
  const pass = document.getElementById('wifiPass').value;

  if (!ssid) { alert('Please enter or select an SSID'); return; }

  fetch('/api/connect', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass)
  }).then(r => r.text()).then(t => alert(t));
}

setInterval(updateStatus, 1200);
updateStatus();
loadFiles();
</script>
</body>
</html>
)rawliteral";

WebPortal::WebPortal()
    : server(80), onCommand(nullptr), stationConnected(false),
      stationIP(""), apIP(""), lastDnsUpdate(0) {}

void WebPortal::setupWiFi() {
    prefs.begin("sstv-wifi", false);
    String savedSSID = prefs.getString("ssid", "");
    String savedPass = prefs.getString("pass", "");

    WiFi.mode(WIFI_AP_STA);

    // Start Access Point with Captive Portal
    WiFi.softAP("ESP-SSTV-Station");
    apIP = WiFi.softAPIP().toString();

    // Start DNS server on port 53 to redirect all queries to AP IP (Captive Portal)
    dnsServer.start(53, "*", WiFi.softAPIP());

    // Connect to Station if credentials exist
    if (savedSSID.length() > 0) {
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        uint32_t startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 6000) {
            delay(200);
        }
        if (WiFi.status() == WL_CONNECTED) {
            stationConnected = true;
            stationIP = WiFi.localIP().toString();
        }
    }

    // Start mDNS responder: esp-sstv.local
    if (MDNS.begin("esp-sstv")) {
        MDNS.addService("http", "tcp", 80);
    }
}

void WebPortal::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void WebPortal::handleCaptiveRedirect() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

void WebPortal::handleStatus() {
    String json = "{";
    json += "\"freq\":" + String(Doppler.getCurrentFreqMHz(), 3) + ",";
    json += "\"doppler\":" + String(Doppler.getCurrentOffsetKHz(), 2) + ",";
    json += "\"rssi\":" + String(Radio.getRSSI()) + ",";
    json += "\"toneHz\":" + String(Demodulator.getInstantaneousFrequency(), 1) + ",";
    json += "\"isRecording\":" + String(Recorder.isRecordingActive() ? "true" : "false") + ",";
    json += "\"recFile\":\"" + Recorder.getCurrentFilename() + "\",";
    json += "\"recBytes\":" + String(Recorder.getBytesWritten()) + ",";
    json += "\"mode\":\"" + String(Recorder.isRecordingActive() ? "REC" : (Demodulator.isReceivingTone() ? "RX" : "IDLE")) + "\"";
    json += "}";

    server.send(200, "application/json", json);
}

void WebPortal::handleAction() {
    if (server.hasArg("cmd") && onCommand) {
        char cmd = server.arg("cmd")[0];
        onCommand(cmd);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing cmd");
    }
}

void WebPortal::handleFiles() {
    std::vector<WavFileInfo> files = Recorder.listFiles();
    String json = "[";
    for (size_t i = 0; i < files.size(); i++) {
        json += "{";
        json += "\"name\":\"" + files[i].filename + "\",";
        json += "\"size\":" + String(files[i].sizeBytes) + ",";
        json += "\"duration\":" + String(files[i].durationSec, 1);
        json += "}";
        if (i < files.size() - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void WebPortal::handleDownload() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "Missing file parameter");
        return;
    }

    String path = server.arg("file");
    File f = Recorder.openFile(path, "r");
    if (!f || f.isDirectory()) {
        server.send(404, "text/plain", "File not found");
        return;
    }

    server.streamFile(f, "audio/wav");
    f.close();
}

void WebPortal::handleDelete() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "Missing file parameter");
        return;
    }

    String path = server.arg("file");
    if (Recorder.deleteFile(path)) {
        server.send(200, "text/plain", "Deleted");
    } else {
        server.send(500, "text/plain", "Delete failed");
    }
}

void WebPortal::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i));
        json += "}";
        if (i < n - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void WebPortal::handleConnect() {
    if (!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Missing SSID");
        return;
    }

    String ssid = server.arg("ssid");
    String pass = server.hasArg("pass") ? server.arg("pass") : "";

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);

    server.send(200, "text/plain", "Credentials saved! Connecting...");

    WiFi.begin(ssid.c_str(), pass.c_str());
}

void WebPortal::setupRouting() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/api/status", [this]() { handleStatus(); });
    server.on("/api/action", [this]() { handleAction(); });
    server.on("/api/files", [this]() { handleFiles(); });
    server.on("/download", [this]() { handleDownload(); });
    server.on("/api/delete", [this]() { handleDelete(); });
    server.on("/api/scan", [this]() { handleScan(); });
    server.on("/api/connect", [this]() { handleConnect(); });

    // Captive portal probes
    server.on("/generate_204", [this]() { handleCaptiveRedirect(); });
    server.on("/gen_204", [this]() { handleCaptiveRedirect(); });
    server.on("/hotspot-detect.html", [this]() { handleCaptiveRedirect(); });
    server.on("/canonical.html", [this]() { handleCaptiveRedirect(); });
    server.on("/ncsi.txt", [this]() { handleCaptiveRedirect(); });
    server.on("/connecttest.txt", [this]() { handleCaptiveRedirect(); });

    server.onNotFound([this]() { handleCaptiveRedirect(); });
}

void WebPortal::begin(CommandHandlerFunc cmdHandler) {
    onCommand = cmdHandler;
    setupWiFi();
    setupRouting();
    server.begin();
}

void WebPortal::update() {
    dnsServer.processNextRequest();
    server.handleClient();
}
