#include <ETH.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <time.h>

#include "credentials.h"

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
// HALAMAN WEB
// =====================================================

void handleRoot() {

  String html = "";

  html += "<!DOCTYPE html>";
  html += "<html lang='id'>";

  html += "<head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  html += "<title>Monitoring Ruangan</title>";


  // =================================================
  // CSS
  // =================================================

  html += "<style>";

  html += "* { box-sizing: border-box; }";

  html += "body {";
  html += "font-family: Arial, sans-serif;";
  html += "background: #f2f2f2;";
  html += "margin: 0;";
  html += "padding: 20px;";
  html += "text-align: center;";
  html += "}";


  html += ".container {";
  html += "max-width: 800px;";
  html += "margin: auto;";
  html += "}";


  html += "h1 {";
  html += "color: #333;";
  html += "margin-bottom: 5px;";
  html += "}";


  html += ".subtitle {";
  html += "color: #777;";
  html += "margin-bottom: 25px;";
  html += "}";


  html += ".status {";
  html += "background: #ffffff;";
  html += "padding: 15px;";
  html += "border-radius: 12px;";
  html += "margin-bottom: 20px;";
  html += "box-shadow: 0 3px 10px rgba(0,0,0,0.08);";
  html += "}";


  html += ".status-online {";
  html += "color: green;";
  html += "font-weight: bold;";
  html += "}";


  html += ".status-offline {";
  html += "color: red;";
  html += "font-weight: bold;";
  html += "}";


  html += ".cards {";
  html += "display: flex;";
  html += "gap: 20px;";
  html += "justify-content: center;";
  html += "flex-wrap: wrap;";
  html += "}";


  html += ".card {";
  html += "background: white;";
  html += "width: 300px;";
  html += "padding: 30px;";
  html += "border-radius: 15px;";
  html += "box-shadow: 0 4px 12px rgba(0,0,0,0.1);";
  html += "}";


  html += ".card h2 {";
  html += "color: #555;";
  html += "}";


  html += ".value {";
  html += "font-size: 50px;";
  html += "font-weight: bold;";
  html += "color: #222;";
  html += "}";


  html += ".unit {";
  html += "font-size: 22px;";
  html += "color: #777;";
  html += "}";


  html += ".footer {";
  html += "margin-top: 25px;";
  html += "font-size: 14px;";
  html += "color: #888;";
  html += "}";


  html += ".panel {";
  html += "background: white;";
  html += "padding: 25px;";
  html += "border-radius: 15px;";
  html += "margin-top: 20px;";
  html += "box-shadow: 0 4px 12px rgba(0,0,0,0.1);";
  html += "text-align: left;";
  html += "}";


  html += ".panel h2 {";
  html += "color: #555;";
  html += "text-align: center;";
  html += "font-size: 20px;";
  html += "margin-top: 0;";
  html += "}";


  html += ".chart-wrap {";
  html += "position: relative;";
  html += "height: 300px;";
  html += "}";


  html += "table {";
  html += "width: 100%;";
  html += "border-collapse: collapse;";
  html += "font-size: 14px;";
  html += "}";


  html += "th, td {";
  html += "padding: 8px 10px;";
  html += "border-bottom: 1px solid #eee;";
  html += "text-align: center;";
  html += "}";


  html += "th {";
  html += "background: #f7f7f7;";
  html += "color: #555;";
  html += "}";


  html += ".hist-nav {";
  html += "display: flex;";
  html += "justify-content: center;";
  html += "align-items: center;";
  html += "gap: 10px;";
  html += "margin-bottom: 15px;";
  html += "flex-wrap: wrap;";
  html += "}";


  html += ".hist-nav button {";
  html += "border: none;";
  html += "background: #eef2f7;";
  html += "color: #333;";
  html += "padding: 8px 14px;";
  html += "border-radius: 8px;";
  html += "font-size: 14px;";
  html += "font-weight: bold;";
  html += "cursor: pointer;";
  html += "}";


  html += ".hist-nav button:hover:not(:disabled) {";
  html += "background: #dbe4ef;";
  html += "}";


  html += ".hist-nav button:disabled {";
  html += "opacity: 0.4;";
  html += "cursor: default;";
  html += "}";


  html += "#hist-label {";
  html += "font-weight: bold;";
  html += "color: #444;";
  html += "}";


  html += ".hist-nav input[type='date'] {";
  html += "border: 1px solid #ccc;";
  html += "padding: 7px 10px;";
  html += "border-radius: 8px;";
  html += "font-size: 14px;";
  html += "color: #333;";
  html += "}";


  html += "</style>";

  html += "</head>";


  // =================================================
  // BODY
  // =================================================

  html += "<body>";

  html += "<div class='container'>";


  html += "<h1>Monitoring Ruangan</h1>";

  html += "<div class='subtitle'>" + String(DEVICE_NAME) + " - WT32-ETH01 + DHT11</div>";


  // =================================================
  // STATUS ETHERNET
  // =================================================

  html += "<div class='status' id='status-box'>";

  if (eth_connected) {

    html += "<div class='status-online'>";
    html += "● Ethernet Connected";
    html += "</div>";

    html += "<div>";
    html += "IP: ";
    html += ETH.localIP().toString();
    html += "</div>";

  } else {

    html += "<div class='status-offline'>";
    html += "● Ethernet Disconnected";
    html += "</div>";

  }

  html += "</div>";


  // =================================================
  // SENSOR CARDS
  // =================================================

  html += "<div class='cards'>";


  // -------------------------------------------------
  // SUHU
  // -------------------------------------------------

  html += "<div class='card'>";

  html += "<h2>🌡 Suhu</h2>";

  if (isnan(temperature)) {

    html += "<div class='value' id='temp-value'>--</div>";

  } else {

    html += "<div class='value' id='temp-value'>";

    html += String(temperature, 1);

    html += " <span class='unit'>°C</span>";

    html += "</div>";

  }

  html += "</div>";


  // -------------------------------------------------
  // KELEMBAPAN
  // -------------------------------------------------

  html += "<div class='card'>";

  html += "<h2>💧 Kelembapan</h2>";

  if (isnan(humidity)) {

    html += "<div class='value' id='hum-value'>--</div>";

  } else {

    html += "<div class='value' id='hum-value'>";

    html += String(humidity, 1);

    html += " <span class='unit'>%</span>";

    html += "</div>";

  }

  html += "</div>";


  html += "</div>";


  // =================================================
  // GRAFIK RIWAYAT
  // =================================================

  html += "<div class='panel'>";

  html += "<h2>Grafik Riwayat</h2>";

  html += "<div class='chart-wrap'>";

  html += "<canvas id='chart'></canvas>";

  html += "</div>";

  html += "</div>";


  // =================================================
  // TABEL RIWAYAT
  // =================================================

  html += "<div class='panel'>";

  html += "<h2>Riwayat Data</h2>";

  html += "<div class='hist-nav'>";

  html += "<button onclick='prevDay()'>&#10094;</button>";

  html += "<input type='date' id='hist-date' onchange='onDatePick()'>";

  html += "<button onclick='nextDay()'>&#10095;</button>";

  html += "<span id='hist-label'>Memuat...</span>";

  html += "<button id='btn-today' onclick='goToday()'>Hari Ini</button>";

  html += "</div>";

  html += "<table>";

  html += "<thead>";

  html += "<tr><th>Waktu</th><th>Suhu (°C)</th><th>Kelembapan (%)</th></tr>";

  html += "</thead>";

  html += "<tbody id='history-body'>";

  html += "<tr><td colspan='3'>Memuat...</td></tr>";

  html += "</tbody>";

  html += "</table>";

  html += "</div>";


  // =================================================
  // FOOTER
  // =================================================

  html += "<div class='footer'>";

  html += "Data diperbarui setiap 2 detik";

  html += "</div>";


  html += "</div>";


  // =================================================
  // JAVASCRIPT (CHART.JS + RIWAYAT)
  // =================================================

  html += "<script src='" + String(apiHost) + "/chart.umd.min.js'></script>";

  html += "<script>";

  html += "var chart = null;";

  html += "if (typeof Chart !== 'undefined') {";

  html += "chart = new Chart(document.getElementById('chart'), {";

  html += "type: 'line',";

  html += "data: { labels: [], datasets: [";

  html += "{ label: 'Suhu (°C)', data: [], borderColor: '#e74c3c', backgroundColor: 'rgba(231,76,60,0.08)', fill: true, tension: 0.3, pointRadius: 2, yAxisID: 'y' },";

  html += "{ label: 'Kelembapan (%)', data: [], borderColor: '#3498db', backgroundColor: 'rgba(52,152,219,0.08)', fill: true, tension: 0.3, pointRadius: 2, yAxisID: 'y1' }";

  html += "] },";

  html += "options: {";

  html += "responsive: true,";

  html += "maintainAspectRatio: false,";

  html += "interaction: { mode: 'index', intersect: false },";

  html += "scales: {";

  html += "x: { ticks: { maxTicksLimit: 10 } },";

  html += "y: { position: 'left', title: { display: true, text: '°C' } },";

  html += "y1: { position: 'right', min: 0, max: 100, title: { display: true, text: '%' }, grid: { drawOnChartArea: false } }";

  html += "}";

  html += "}";

  html += "});";

  html += "}";


  html += "var selectedDate = todayStr();";

  html += "";

  html += "function pad(n) { return n < 10 ? '0' + n : '' + n; }";

  html += "function todayStr() { var d = new Date(); return d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate()); }";

  html += "function dateLabel(ds) {";

  html += "var days = ['Minggu', 'Senin', 'Selasa', 'Rabu', 'Kamis', 'Jumat', 'Sabtu'];";

  html += "var months = ['Januari', 'Februari', 'Maret', 'April', 'Mei', 'Juni', 'Juli', 'Agustus', 'September', 'Oktober', 'November', 'Desember'];";

  html += "var p = ds.split('-'); var d = new Date(p[0], p[1] - 1, p[2]);";

  html += "return days[d.getDay()] + ', ' + d.getDate() + ' ' + months[d.getMonth()] + ' ' + d.getFullYear();";

  html += "}";

  html += "";

  html += "function updateNav() {";

  html += "document.getElementById('hist-date').value = selectedDate;";

  html += "document.getElementById('hist-label').textContent = dateLabel(selectedDate);";

  html += "document.getElementById('btn-today').disabled = (selectedDate === todayStr());";

  html += "}";

  html += "";

  html += "function renderRows(rows) {";

  html += "if (chart) {";

  html += "chart.data.labels = rows.map(function(d) { return d.recorded_at.substring(11, 16); });";

  html += "chart.data.datasets[0].data = rows.map(function(d) { return parseFloat(d.temperature); });";

  html += "chart.data.datasets[1].data = rows.map(function(d) { return parseFloat(d.humidity); });";

  html += "chart.update('none');";

  html += "}";

  html += "var body = document.getElementById('history-body');";

  html += "if (rows.length === 0) { body.innerHTML = '<tr><td colspan=\"3\">Belum ada data pada tanggal ini</td></tr>'; return; }";

  html += "body.innerHTML = rows.map(function(d) {";

  html += "return '<tr><td>' + d.recorded_at.substring(11) + '</td><td>' + parseFloat(d.temperature).toFixed(1) + '</td><td>' + parseFloat(d.humidity).toFixed(1) + '</td></tr>';";

  html += "}).join('');";

  html += "}";

  html += "";

  html += "function loadHistory() {";

  html += "updateNav();";

  html += "fetch('" + String(apiHost) + "/api.php?action=history&device=" + String(DEVICE_NAME) + "&date=' + selectedDate)";

  html += ".then(function(r) { return r.json(); })";

  html += ".then(function(json) {";

  html += "if (!json.ok) { return; }";

  html += "renderRows(json.data);";

  html += "})";

  html += ".catch(function() {";

  html += "document.getElementById('history-body').innerHTML = '<tr><td colspan=\"3\">Server database tidak dapat dihubungi</td></tr>';";

  html += "});";

  html += "}";

  html += "";

  html += "function prevDay() {";

  html += "var d = new Date(selectedDate + 'T00:00:00');";

  html += "d.setDate(d.getDate() - 1);";

  html += "selectedDate = d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate());";

  html += "loadHistory();";

  html += "}";

  html += "";

  html += "function nextDay() {";

  html += "var d = new Date(selectedDate + 'T00:00:00');";

  html += "d.setDate(d.getDate() + 1);";

  html += "selectedDate = d.getFullYear() + '-' + pad(d.getMonth() + 1) + '-' + pad(d.getDate());";

  html += "loadHistory();";

  html += "}";

  html += "";

  html += "function goToday() {";

  html += "selectedDate = todayStr();";

  html += "loadHistory();";

  html += "}";

  html += "";

  html += "function onDatePick() {";

  html += "var v = document.getElementById('hist-date').value;";

  html += "if (v) { selectedDate = v; loadHistory(); }";

  html += "}";

  html += "";

  html += "setInterval(function() { if (selectedDate === todayStr()) loadHistory(); }, 30000);";

  html += "";

  html += "loadHistory();";


  html += "function updateRealtime() {";

  html += "fetch('/data')";

  html += ".then(function(r) { return r.json(); })";

  html += ".then(function(d) {";

  html += "var st = document.getElementById('status-box');";

  html += "if (d.eth_connected) {";

  html += "st.innerHTML = '<div class=\"status-online\">● Ethernet Connected</div><div>IP: ' + d.ip + '</div>';";

  html += "} else {";

  html += "st.innerHTML = '<div class=\"status-offline\">● Ethernet Disconnected</div>';";

  html += "}";

  html += "document.getElementById('temp-value').innerHTML = d.temperature === null ? '--' : d.temperature.toFixed(1) + ' <span class=\"unit\">°C</span>';";

  html += "document.getElementById('hum-value').innerHTML = d.humidity === null ? '--' : d.humidity.toFixed(1) + ' <span class=\"unit\">%</span>';";

  html += "})";

  html += ".catch(function() {});";

  html += "}";

  html += "setInterval(updateRealtime, 2000);";

  html += "updateRealtime();";

  html += "</script>";

  html += "</body>";

  html += "</html>";


  server.send(200, "text/html", html);
}


// =====================================================
// SETUP
// =====================================================

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
      Serial.println(" °C");

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