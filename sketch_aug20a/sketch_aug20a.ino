#include <ETH.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <time.h>

#include "credentials.h"
#include "images.h"

// =====================================================
// KONFIGURASI DHT11
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


// =====================================================
// KONFIGURASI STATIC IP
// =====================================================
// Nilai asli ada di credentials.h (tidak ikut di-commit).
// Salin credentials.h.example untuk membuatnya.
// =====================================================

IPAddress local_IP(NET_LOCAL_IP);
IPAddress gateway(NET_GATEWAY);
IPAddress subnet(NET_SUBNET);

IPAddress primaryDNS(NET_PRIMARY_DNS);
IPAddress secondaryDNS(NET_SECONDARY_DNS);


// =====================================================
// KONFIGURASI SERVER DATABASE (API)
// =====================================================

const char* apiHost = API_HOST;


// =====================================================
// KONFIGURASI NTP
// =====================================================
// Dipakai agar pengiriman ke database tepat di detik 0
// setiap menit (waktu nyata, bukan sejak perangkat nyala).
// Butuh koneksi internet melalui jaringan.
// =====================================================

const char* ntpServer1 = "pool.ntp.org";

const char* ntpServer2 = "time.google.com";

const long gmtOffsetSec = 7 * 3600;  // WIB = UTC+7 (WITA ganti 8, WIT ganti 9)

const int dstOffsetSec = 0;


// =====================================================
// KONFIGURASI ETHERNET WT32-ETH01
// =====================================================

#define ETH_PHY_TYPE  ETH_PHY_LAN8720
#define ETH_PHY_ADDR  1
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER 16
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN


// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);


// =====================================================
// VARIABEL SENSOR
// =====================================================

float temperature = NAN;
float humidity = NAN;


// =====================================================
// STATUS ETHERNET
// =====================================================

bool eth_connected = false;


// =====================================================
// WAKTU PEMBACAAN SENSOR
// =====================================================

unsigned long lastSensorRead = 0;

const unsigned long sensorInterval = 2000;


// =====================================================
// WAKTU PENGIRIMAN KE DATABASE
// =====================================================

unsigned long lastDbSend = 0;

const unsigned long dbSendInterval = 60000;


// =====================================================
// STATUS WAKTU & PENGIRIMAN PER MENIT
// =====================================================

bool timeSynced = false;

int lastSentMinute = -1;


// =====================================================
// EVENT ETHERNET
// =====================================================

void onEvent(arduino_event_id_t event) {

  switch (event) {

    case ARDUINO_EVENT_ETH_START:

      Serial.println();
      Serial.println("[ETH] Ethernet Started");

      ETH.setHostname("WT32-DHT11");

      break;


    case ARDUINO_EVENT_ETH_CONNECTED:

      Serial.println("[ETH] Ethernet Connected");

      break;


    case ARDUINO_EVENT_ETH_GOT_IP:

      Serial.println("[ETH] Ethernet Got IP");

      eth_connected = true;

      Serial.print("[ETH] IP Address : ");
      Serial.println(ETH.localIP());

      Serial.print("[ETH] Gateway    : ");
      Serial.println(ETH.gatewayIP());

      Serial.print("[ETH] Subnet     : ");
      Serial.println(ETH.subnetMask());

      Serial.println("[WEB] http://" + ETH.localIP().toString());


      configTime(
        gmtOffsetSec,
        dstOffsetSec,
        ntpServer1,
        ntpServer2
      );

      Serial.println("[NTP] Sinkronisasi waktu dimulai");

      break;


    case ARDUINO_EVENT_ETH_DISCONNECTED:

      Serial.println("[ETH] Ethernet Disconnected");

      eth_connected = false;

      break;


    case ARDUINO_EVENT_ETH_STOP:

      Serial.println("[ETH] Ethernet Stopped");

      eth_connected = false;

      break;


    default:

      break;
  }
}


// =====================================================
// KIRIM DATA KE DATABASE SERVER
// =====================================================

void sendToDatabase() {

  if (!eth_connected) {

    Serial.println("[DB] SKIP: Ethernet tidak terhubung");

    return;
  }


  if (isnan(temperature) || isnan(humidity)) {

    Serial.println("[DB] SKIP: Belum ada data sensor valid");

    return;
  }


  HTTPClient http;

  http.begin(String(apiHost) + "/api.php");

  http.addHeader("Content-Type", "application/json");

  http.setTimeout(5000);


  String payload = "{";

  payload += "\"device\":\"";

  payload += DEVICE_NAME;

  payload += "\",";

  payload += "\"temperature\":";

  payload += String(temperature, 1);

  payload += ",\"humidity\":";

  payload += String(humidity, 1);

  payload += "}";


  int httpCode = http.POST(payload);


  if (httpCode > 0) {

    Serial.print("[DB] Terkirim, response: ");

    Serial.println(httpCode);

  } else {

    Serial.print("[DB] ERROR: ");

    Serial.println(http.errorToString(httpCode));

  }


  http.end();
}


// =====================================================
// ENDPOINT DATA REALTIME (JSON)
// =====================================================

void handleData() {

  String json = "{";

  json += "\"temperature\":";

  if (isnan(temperature)) {

    json += "null";

  } else {

    json += String(temperature, 1);

  }

  json += ",\"humidity\":";

  if (isnan(humidity)) {

    json += "null";

  } else {

    json += String(humidity, 1);

  }

  json += ",\"eth_connected\":";

  json += eth_connected ? "true" : "false";

  json += ",\"ip\":\"";

  json += ETH.localIP().toString();

  json += "\"}";

  server.send(200, "application/json", json);
}


// =====================================================
// BACAPROGMEM STRING (untuk data URI base64 gambar)
// =====================================================

String progmemToString(const char* src, size_t len) {

  String s;

  s.reserve(len + 1);

  for (size_t i = 0; i < len; i++) {

    s += (char)pgm_read_byte(&src[i]);

  }

  return s;
}


// =====================================================
// HALAMAN WEB
// =====================================================

void handleRoot() {

  String html = "";
  html.reserve(30000);

  String iconB64 = progmemToString(IMG_ICON_B64, IMG_ICON_B64_LEN);

  String tabB64  = progmemToString(IMG_TAB_B64, IMG_TAB_B64_LEN);

  html += "<!DOCTYPE html>";
  html += "<html lang='id'>";

  html += "<head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  html += "<link rel='icon' type='image/png' href='data:image/png;base64," + tabB64 + "'>";

  html += "<title>Monitoring Suhu Ruangan</title>";

  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: 'Segoe UI', system-ui, -apple-system, Arial, sans-serif; background:#f1f5f9; color:#1e293b; min-height:100vh; padding:0 0 40px; }";
  html += ".container { max-width:1000px; margin:0 auto; padding:0 20px; }";
  html += ".topbar { background:#187ca2; color:#fff; padding:28px 20px; margin-bottom:28px; border-radius:0; box-shadow:0 10px 25px -5px rgba(15,23,42,0.08); }";
  html += ".topbar-inner { max-width:1000px; margin:0 auto; display:flex; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:14px; }";
  html += ".brand { display:flex; align-items:center; gap:14px; }";
  html += ".brand-icon { width:48px; height:48px; display:flex; align-items:center; justify-content:center; overflow:hidden; }";
  html += ".brand-icon img { width:100%; height:100%; object-fit:cover; }";
  html += ".brand h1 { font-size:22px; font-weight:700; }";
  html += ".brand .device { font-size:13px; opacity:0.85; }";
  html += ".status-pill { display:inline-flex; align-items:center; gap:8px; background:rgba(255,255,255,0.16); padding:9px 16px; border-radius:999px; font-weight:600; font-size:13.5px; }";
  html += ".status-dot { width:9px; height:9px; border-radius:50%; background:#10b981; box-shadow:0 0 0 3px rgba(16,185,129,0.35); animation:pulse 1.6s infinite; }";
  html += ".status-dot.offline { background:#ef4444; box-shadow:0 0 0 3px rgba(239,68,68,0.35); animation:none; }";
  html += "@keyframes pulse { 0%,100% { transform:scale(1); } 50% { transform:scale(1.25); } }";
  html += ".ip-text { font-size:12px; opacity:0.8; font-weight:400; }";
  html += ".live-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(220px,1fr)); gap:20px; margin-bottom:28px; }";
  html += ".live-card { background:#fff; border-radius:18px; padding:24px; box-shadow:0 10px 25px -5px rgba(15,23,42,0.08); border:1px solid #e2e8f0; position:relative; overflow:hidden; }";
  html += ".live-card::before { content:''; position:absolute; left:0; top:0; bottom:0; width:5px; }";
  html += ".live-card.temp::before { background:#ef4444; }";
  html += ".live-card.hum::before { background:#2563eb; }";
  html += ".live-card .label { display:flex; align-items:center; gap:8px; color:#64748b; font-size:13px; font-weight:600; text-transform:uppercase; letter-spacing:0.5px; }";
  html += ".live-card .label .ico { font-size:18px; }";
  html += ".live-value { font-size:46px; font-weight:800; color:#1e293b; line-height:1.2; margin-top:8px; }";
  html += ".live-value .unit { font-size:20px; font-weight:600; color:#64748b; margin-left:3px; }";
  html += ".live-card .caption { margin-top:6px; font-size:12px; color:#64748b; }";
  html += ".stats-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(180px,1fr)); gap:16px; }";
  html += ".stat-card { background:#fff; border-radius:16px; padding:16px 20px; box-shadow:0 10px 25px -5px rgba(15,23,42,0.08); border:1px solid #e2e8f0; }";
  html += ".stat-card .stat-title { font-size:12px; color:#64748b; text-transform:uppercase; letter-spacing:0.4px; font-weight:600; }";
  html += ".stat-card .stat-num { font-size:24px; font-weight:800; margin-top:6px; color:#1e293b; }";
  html += ".stat-card .stat-num small { font-size:13px; font-weight:600; color:#64748b; }";
  html += ".stat-card .stat-badge { display:inline-block; font-size:11px; font-weight:700; padding:2px 8px; border-radius:999px; margin-bottom:4px; }";
  html += ".badge-min { background:#e0f2fe; color:#0369a1; }";
  html += ".badge-max { background:#fee2e2; color:#b91c1c; }";
  html += ".badge-avg { background:#dcfce7; color:#15803d; }";
  html += ".panel { background:#fff; border-radius:18px; padding:24px; margin-bottom:24px; box-shadow:0 10px 25px -5px rgba(15,23,42,0.08); border:1px solid #e2e8f0; }";
  html += ".panel-header { display:flex; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:12px; margin-bottom:18px; }";
  html += ".panel-header h2 { font-size:16px; font-weight:700; color:#1e293b; }";
  html += ".chart-wrap { position:relative; height:320px; }";
  html += ".hist-nav { display:flex; align-items:center; gap:8px; flex-wrap:wrap; }";
  html += ".hist-nav button { border:1px solid #e2e8f0; background:#f8fafc; color:#1e293b; padding:8px 13px; border-radius:10px; font-size:14px; font-weight:600; cursor:pointer; transition:all 0.15s ease; line-height:1; }";
  html += ".hist-nav button:hover:not(:disabled) { background:#187ca2; color:#fff; border-color:#187ca2; }";
  html += ".hist-nav button:disabled { opacity:0.4; cursor:default; }";
  html += ".hist-nav input[type='date'] { border:1px solid #e2e8f0; background:#f8fafc; padding:8px 12px; border-radius:10px; font-size:14px; color:#1e293b; font-family:inherit; }";
  html += "#hist-label { font-weight:700; color:#3730a3; background:#eef2ff; padding:8px 14px; border-radius:10px; font-size:14px; }";
  html += ".range-nav { display:flex; gap:6px; }";
  html += ".range-btn { border:1px solid #e2e8f0; background:#f8fafc; color:#1e293b; padding:7px 14px; border-radius:10px; font-size:13px; font-weight:600; cursor:pointer; transition:all 0.15s ease; }";
  html += ".range-btn.active { background:#187ca2; color:#fff; border-color:#187ca2; }";
  html += ".table-scroll { max-height:420px; overflow-y:auto; border:1px solid #e2e8f0; border-radius:12px; }";
  html += "table { width:100%; border-collapse:collapse; font-size:13.5px; }";
  html += "thead { position:sticky; top:0; background:#f8fafc; z-index:1; }";
  html += "th, td { padding:11px 14px; text-align:center; border-bottom:1px solid #e2e8f0; }";
  html += "th { color:#64748b; font-weight:600; text-transform:uppercase; letter-spacing:0.4px; font-size:12px; }";
  html += "tbody tr:hover { background:#f8fafc; }";
  html += "tbody tr:last-child td { border-bottom:none; }";
  html += ".temp-badge, .hum-badge { display:inline-block; min-width:64px; padding:3px 10px; border-radius:999px; font-weight:600; font-size:13px; }";
  html += ".temp-badge { background:#fee2e2; color:#b91c1c; }";
  html += ".hum-badge { background:#dbeafe; color:#1d4ed8; }";
  html += ".time-cell { font-variant-numeric:tabular-nums; color:#1e293b; font-weight:600; }";
  html += ".footer { text-align:center; font-size:12.5px; color:#64748b; margin-top:10px; }";
  html += ".empty-row { padding:24px; color:#64748b !important; background:#f8fafc; }";
  html += "</style>";

  html += "</head>";

  html += "<body>";

  html += "<div class='topbar'>";
  html += "<div class='topbar-inner'>";
  html += "<div class='brand'>";
  html += "<div class='brand-icon'><img src='data:image/png;base64," + iconB64 + "' alt='logo'></div>";
  html += "<div>";
  html += "<h1>Monitoring Suhu Ruangan</h1>";
  html += "<div class='device'>" + String(DEVICE_NAME) + " &middot; WT32-ETH01 + DHT11</div>";
  html += "</div>";
  html += "</div>";
  html += "<div class='status-pill' id='status-pill'>";
  html += "<span class='status-dot' id='status-dot'></span>";
  html += "<span id='status-text'>Menyambung&hellip;</span>";
  html += "<span class='ip-text' id='ip-text'></span>";
  html += "</div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='container'>";

  html += "<div class='live-grid'>";
  html += "<div class='live-card temp'>";
  html += "<div class='label'><span class='ico'>&#127777;</span> Suhu</div>";
  html += "<div class='live-value' id='temp-value'>--<span class='unit'>&deg;C</span></div>";
  html += "<div class='caption'>Pembacaan realtime</div>";
  html += "</div>";
  html += "<div class='live-card hum'>";
  html += "<div class='label'><span class='ico'>&#128167;</span> Kelembapan</div>";
  html += "<div class='live-value' id='hum-value'>--<span class='unit'>%</span></div>";
  html += "<div class='caption'>Pembacaan realtime</div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'>";
  html += "<h2>Statistik Hari Ini</h2>";
  html += "<span id='hist-label'>Memuat&hellip;</span>";
  html += "</div>";
  html += "<div class='stats-grid'>";
  html += "<div class='stat-card'><span class='stat-badge badge-min'>RENDAH</span><div class='stat-title'>Suhu</div><div class='stat-num' id='st-min-temp'>--<small>&deg;C</small></div></div>";
  html += "<div class='stat-card'><span class='stat-badge badge-max'>TERTINGGI</span><div class='stat-title'>Suhu</div><div class='stat-num' id='st-max-temp'>--<small>&deg;C</small></div></div>";
  html += "<div class='stat-card'><span class='stat-badge badge-avg'>RATA-RATA</span><div class='stat-title'>Suhu</div><div class='stat-num' id='st-avg-temp'>--<small>&deg;C</small></div></div>";
  html += "<div class='stat-card'><span class='stat-badge badge-avg'>RATA-RATA</span><div class='stat-title'>Kelembapan</div><div class='stat-num' id='st-avg-hum'>--<small>%</small></div></div>";
  html += "</div>";
  html += "</div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'>";
  html += "<h2>Grafik Riwayat Suhu &amp; Kelembapan</h2>";
  html += "<div class='range-nav'>";
  html += "<button id='btn-1h' class='range-btn active' onclick='setRange(1)'>&#127783; 1 Jam</button>";
  html += "<button id='btn-24h' class='range-btn' onclick='setRange(24)'>&#128337; 24 Jam</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class='chart-wrap'>";
  html += "<canvas id='chart'></canvas>";
  html += "</div>";
  html += "</div>";

  html += "<div class='panel'>";
  html += "<div class='panel-header'>";
  html += "<h2>Riwayat Data</h2>";
  html += "<div class='hist-nav'>";
  html += "<button onclick='prevDay()'>&#10094;</button>";
  html += "<input type='date' id='hist-date' onchange='onDatePick()'>";
  html += "<button onclick='nextDay()'>&#10095;</button>";
  html += "<button id='btn-today' onclick='goToday()'>Hari Ini</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class='table-scroll'>";
  html += "<table>";
  html += "<thead><tr><th>Waktu</th><th>Suhu</th><th>Kelembapan</th></tr></thead>";
  html += "<tbody id='history-body'><tr><td colspan='3' class='empty-row'>Memuat&hellip;</td></tr></tbody>";
  html += "</table>";
  html += "</div>";
  html += "</div>";

  html += "<div class='footer'>Data diperbarui otomatis setiap 30 detik (hanya jika tanggal aktif = hari ini)</div>";

  html += "</div>";

  html += "<script src='" + String(apiHost) + "/chart.umd.min.js'></script>";

  html += "<script>";

  html += "var apiHost = '" + String(apiHost) + "';";
  html += "var DEVICE_NAME = '" + String(DEVICE_NAME) + "';";
  html += "var selectedDate = todayStr();";
  html += "var rangeMode = 1;";
  html += "function pad(n) { return n < 10 ? '0' + n : '' + n; }";
  html += "function todayStr() { var d = new Date(); return d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()); }";
  html += "function dateLabel(ds) {";
  html += "var days = ['Minggu','Senin','Selasa','Rabu','Kamis','Jumat','Sabtu'];";
  html += "var months = ['Januari','Februari','Maret','April','Mei','Juni','Juli','Agustus','September','Oktober','November','Desember'];";
  html += "var p = ds.split('-'); var d = new Date(p[0], p[1] - 1, p[2]);";
  html += "return days[d.getDay()] + ', ' + d.getDate() + ' ' + months[d.getMonth()] + ' ' + d.getFullYear();";
  html += "}";
  html += "function updateNav() {";
  html += "var dl = document.getElementById('hist-label'); if (dl) dl.textContent = dateLabel(selectedDate);";
  html += "var dd = document.getElementById('hist-date'); if (dd) dd.value = selectedDate;";
  html += "var bt = document.getElementById('btn-today'); if (bt) bt.disabled = (selectedDate === todayStr());";
  html += "}";

  html += "var chart = null;";
  html += "if (typeof Chart !== 'undefined') {";
  html += "Chart.defaults.font.family = 'Segoe UI, system-ui, sans-serif';";
  html += "Chart.defaults.color = '#64748b';";
  html += "chart = new Chart(document.getElementById('chart'), {";
  html += "type: 'line',";
  html += "data: { labels: [], datasets: [";
  html += "{ label: 'Suhu (\\u00b0C)', data: [], borderColor: '#ef4444', backgroundColor: 'rgba(239,68,68,0.08)', fill: true, tension: 0.35, pointRadius: 3, pointBackgroundColor: '#ef4444', borderWidth: 2, yAxisID: 'y' },";
  html += "{ label: 'Kelembapan (%)', data: [], borderColor: '#2563eb', backgroundColor: 'rgba(37,99,235,0.08)', fill: true, tension: 0.35, pointRadius: 3, pointBackgroundColor: '#2563eb', borderWidth: 2, yAxisID: 'y1' }";
  html += "] },";
  html += "options: {";
  html += "responsive: true, maintainAspectRatio: false,";
  html += "interaction: { mode: 'index', intersect: false },";
  html += "plugins: {";
  html += "tooltip: { mode: 'index', intersect: false, backgroundColor: '#1e293b', titleColor: '#fff', bodyColor: '#e2e8f0', padding: 10, cornerRadius: 10 },";
  html += "legend: { position: 'top', labels: { usePointStyle: true, padding: 18 } }";
  html += "},";
  html += "scales: {";
  html += "x: { grid: { display: false }, ticks: { maxTicksLimit: 12, maxRotation: 0 } },";
  html += "y: { position: 'left', title: { display: true, text: 'Suhu (\\u00b0C)' }, grid: { color: '#f1f5f9' } },";
  html += "y1: { position: 'right', min: 0, max: 100, title: { display: true, text: 'Kelembapan (%)' }, grid: { drawOnChartArea: false } }";
  html += "}";
  html += "}";
  html += "});";
  html += "}";

  html += "function computeStats(rows) {";
  html += "if (!rows.length) { ['st-min-temp','st-max-temp','st-avg-temp','st-avg-hum'].forEach(function(id){ var el = document.getElementById(id); if (el) el.innerHTML = '--'; }); return; }";
  html += "var temps = rows.map(function(d){ return parseFloat(d.temperature); });";
  html += "var hums = rows.map(function(d){ return parseFloat(d.humidity); });";
  html += "var minT = Math.min.apply(null, temps);";
  html += "var maxT = Math.max.apply(null, temps);";
  html += "var avgT = temps.reduce(function(a,b){ return a+b; },0) / temps.length;";
  html += "var avgH = hums.reduce(function(a,b){ return a+b; },0) / hums.length;";
  html += "function set(id, v, unit) { var el = document.getElementById(id); if (el) el.innerHTML = v.toFixed(1) + '<small>' + unit + '</small>'; }";
  html += "set('st-min-temp', minT, '&deg;C'); set('st-max-temp', maxT, '&deg;C'); set('st-avg-temp', avgT, '&deg;C'); set('st-avg-hum', avgH, '%');";
  html += "}";

  html += "function filterChartRows(rows) {";
  html += "if (rangeMode === 24) {";
  html += "var byHour = {};";
  html += "rows.forEach(function(d){ var h = d.recorded_at.substring(0, 13); if (!byHour[h]) { byHour[h] = { t: d.temperature, h: d.humidity, n: 1 }; } else { byHour[h].t += d.temperature; byHour[h].h += d.humidity; byHour[h].n++; } });";
  html += "return Object.keys(byHour).map(function(k){ return { recorded_at: k + ':00', temperature: byHour[k].t / byHour[k].n, humidity: byHour[k].h / byHour[k].n }; });";
  html += "}";
  html += "var now = new Date();";
  html += "var cutoff = new Date(now.getTime() - rangeMode * 60 * 60 * 1000);";
  html += "var cutoffStr = now.getFullYear() + '-' + pad(now.getMonth() + 1) + '-' + pad(now.getDate()) + ' ' + pad(cutoff.getHours()) + ':' + pad(cutoff.getMinutes()) + ':' + pad(cutoff.getSeconds());";
  html += "return rows.filter(function(d){ return d.recorded_at >= cutoffStr; });";
  html += "}";

  html += "function setRange(mode) {";
  html += "rangeMode = mode;";
  html += "var a = document.getElementById('btn-1h'); var b = document.getElementById('btn-24h');";
  html += "if (mode === 1) { a.className = 'range-btn active'; b.className = 'range-btn'; } else { a.className = 'range-btn'; b.className = 'range-btn active'; }";
  html += "loadHistory();";
  html += "}";

  html += "function renderRows(rows) {";
  html += "computeStats(rows);";
  html += "if (chart) {";
  html += "var chartRows = filterChartRows(rows);";
  html += "chart.data.labels = chartRows.map(function(d){ return d.recorded_at.substring(11,16); });";
  html += "chart.data.datasets[0].data = chartRows.map(function(d){ return parseFloat(d.temperature); });";
  html += "chart.data.datasets[1].data = chartRows.map(function(d){ return parseFloat(d.humidity); });";
  html += "chart.data.datasets[0].label = rangeMode === 1 ? 'Suhu (\\u00b0C)' : 'Suhu (\\u00b0C, rata-rata/jam)';";
  html += "chart.data.datasets[1].label = rangeMode === 1 ? 'Kelembapan (%)' : 'Kelembapan (%, rata-rata/jam)';";
  html += "chart.update('none');";
  html += "}";
  html += "var body = document.getElementById('history-body');";
  html += "if (rows.length === 0) { body.innerHTML = '<tr><td colspan=\"3\" class=\"empty-row\">Belum ada data pada tanggal ini</td></tr>'; return; }";
  html += "body.innerHTML = rows.map(function(d){ return '<tr><td class=\"time-cell\">' + d.recorded_at.substring(11) + '</td><td><span class=\"temp-badge\">' + parseFloat(d.temperature).toFixed(1) + ' &deg;C</span></td><td><span class=\"hum-badge\">' + parseFloat(d.humidity).toFixed(1) + ' %</span></td></tr>'; }).join('');";
  html += "}";

  html += "function loadHistory() {";
  html += "updateNav();";
  html += "fetch(apiHost + '/api.php?action=history&device=' + DEVICE_NAME + '&date=' + selectedDate)";
  html += ".then(function(r){ return r.json(); })";
  html += ".then(function(json){ if (json.ok) renderRows(json.data || []); })";
  html += ".catch(function(){ document.getElementById('history-body').innerHTML = '<tr><td colspan=\"3\" class=\"empty-row\">Server database tidak dapat dihubungi</td></tr>'; });";
  html += "}";

  html += "function prevDay() { var d = new Date(selectedDate + 'T00:00:00'); d.setDate(d.getDate() - 1); selectedDate = d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()); loadHistory(); }";
  html += "function nextDay() { var d = new Date(selectedDate + 'T00:00:00'); d.setDate(d.getDate() + 1); selectedDate = d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()); loadHistory(); }";
  html += "function goToday() { selectedDate = todayStr(); loadHistory(); }";
  html += "function onDatePick() { var v = document.getElementById('hist-date').value; if (v) { selectedDate = v; loadHistory(); } }";

  html += "setInterval(function(){ if (selectedDate === todayStr()) loadHistory(); }, 30000);";
  html += "loadHistory();";

  html += "function updateRealtime() {";
  html += "fetch('/data').then(function(r){ return r.json(); }).then(function(d){";
  html += "var pill = document.getElementById('status-pill');";
  html += "var dot = document.getElementById('status-dot');";
  html += "var txt = document.getElementById('status-text');";
  html += "var ip = document.getElementById('ip-text');";
  html += "if (d.eth_connected) { dot.className = 'status-dot'; txt.textContent = 'Online'; ip.textContent = d.ip; }";
  html += "else { dot.className = 'status-dot offline'; txt.textContent = 'Offline'; ip.textContent = ''; }";
  html += "if (d.temperature !== null) document.getElementById('temp-value').innerHTML = d.temperature.toFixed(1) + '<span class=\"unit\">&deg;C</span>';";
  html += "if (d.humidity !== null) document.getElementById('hum-value').innerHTML = d.humidity.toFixed(1) + '<span class=\"unit\">%</span>';";
  html += "}).catch(function(){});";
  html += "}";
  html += "setInterval(updateRealtime, 2000);";
  html += "updateRealtime();";

  html += "</script>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}
void setup() {

  // =================================================
  // SERIAL
  // =================================================

  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("======================================");
  Serial.println(" WT32-ETH01 DHT11 MONITORING");
  Serial.println("======================================");


  // =================================================
  // DHT11
  // =================================================

  dht.begin();

  Serial.println("[DHT] DHT11 Started");


  // =================================================
  // ETHERNET EVENT
  // =================================================

  Network.onEvent(onEvent);


  // =================================================
  // STATIC IP
  // =================================================

  Serial.println("[ETH] Configuring Static IP...");

  if (!ETH.config(
        local_IP,
        gateway,
        subnet,
        primaryDNS,
        secondaryDNS
      )) {

    Serial.println("[ETH] ERROR: Static IP configuration failed!");

  } else {

    Serial.println("[ETH] Static IP configured");

  }


  // =================================================
  // START ETHERNET
  // =================================================

  Serial.println("[ETH] Starting Ethernet...");


  ETH.begin(
    ETH_PHY_TYPE,
    ETH_PHY_ADDR,
    ETH_PHY_MDC,
    ETH_PHY_MDIO,
    ETH_PHY_POWER,
    ETH_CLK_MODE
  );


  // =====================================================
  // CONFIGURE STATIC IP
  // =====================================================

  delay(1000);

  Serial.println("[ETH] Configuring Static IP...");

  if (ETH.config(
        local_IP,
        gateway,
        subnet,
        primaryDNS,
        secondaryDNS
      )) {

    Serial.println("[ETH] Static IP configuration OK");

  } else {

    Serial.println("[ETH] ERROR: Static IP configuration FAILED!");

  }


  // =================================================
  // WEB SERVER
  // =================================================

  server.on("/", handleRoot);

  server.on("/data", handleData);

  server.begin();

  Serial.println("[WEB] Web Server Started");

  Serial.println();
  Serial.println("System ready.");

}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // =================================================
  // HANDLE WEB REQUEST
  // =================================================

  server.handleClient();


  // =================================================
  // BACA SENSOR SETIAP 2 DETIK
  // =================================================

  if (millis() - lastSensorRead >= sensorInterval) {

    lastSensorRead = millis();


    // -----------------------------------------------
    // BACA DHT11
    // -----------------------------------------------

    float newTemperature = dht.readTemperature();

    float newHumidity = dht.readHumidity();


    // -----------------------------------------------
    // VALIDASI
    // -----------------------------------------------

    if (isnan(newTemperature) || isnan(newHumidity)) {

      Serial.println("[DHT] ERROR: Gagal membaca sensor!");

    } else {

      temperature = newTemperature;

      humidity = newHumidity;


      // ---------------------------------------------
      // SERIAL MONITOR
      // ---------------------------------------------

      Serial.println();

      Serial.println("[SENSOR] Data terbaru:");

      Serial.print("  Suhu       : ");
      Serial.print(temperature, 1);
      Serial.println(" C");

      Serial.print("  Kelembapan : ");
      Serial.print(humidity, 1);
      Serial.println(" %");

      Serial.println("--------------------------------------");

    }

  }


  // =================================================
  // KIRIM DATA KE DATABASE TIAP MENIT (DETIK 0)
  // =================================================

  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 0)) {

    // -----------------------------------------------
    // NTP SINKRON: kirim saat menit berganti
    // -----------------------------------------------

    if (!timeSynced) {

      timeSynced = true;

      lastSentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;

      Serial.println("[NTP] Waktu tersinkronisasi, kirim per menit aktif");

    }


    int nowMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    if (nowMinute != lastSentMinute) {

      lastSentMinute = nowMinute;

      sendToDatabase();

    }

  } else {

    // -----------------------------------------------
    // FALLBACK: NTP BELUM SINKRON
    // Kirim per 1 menit sejak nyala agar data tetap jalan
    // -----------------------------------------------

    if (millis() - lastDbSend >= dbSendInterval) {

      lastDbSend = millis();

      Serial.println("[DB] NTP belum sinkron, kirim berbasis uptime");

      sendToDatabase();

    }

  }

}
