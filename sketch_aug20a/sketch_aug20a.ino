#include <ETH.h>
#include <WebServer.h>
#include <DHT.h>

// =====================================================
// KONFIGURASI DHT11
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


// =====================================================
// KONFIGURASI STATIC IP
// =====================================================
// GANTI SESUAI JARINGAN ANDA
// =====================================================

IPAddress local_IP(192, 168, 2, 24);
IPAddress gateway(192, 168, 2, 254);
IPAddress subnet(255, 255, 255, 0);

IPAddress primaryDNS(192, 168, 2, 254);
IPAddress secondaryDNS(8, 8, 8, 8);


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
// HALAMAN WEB
// =====================================================

void handleRoot() {

  String html = "";

  html += "<!DOCTYPE html>";
  html += "<html lang='id'>";

  html += "<head>";

  html += "<meta charset='UTF-8'>";

  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";

  html += "<meta http-equiv='refresh' content='2'>";

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


  html += "</style>";

  html += "</head>";


  // =================================================
  // BODY
  // =================================================

  html += "<body>";

  html += "<div class='container'>";


  html += "<h1>Monitoring Ruangan</h1>";

  html += "<div class='subtitle'>WT32-ETH01 + DHT11</div>";


  // =================================================
  // STATUS ETHERNET
  // =================================================

  html += "<div class='status'>";

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

    html += "<div class='value'>--</div>";

  } else {

    html += "<div class='value'>";

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

    html += "<div class='value'>--</div>";

  } else {

    html += "<div class='value'>";

    html += String(humidity, 1);

    html += " <span class='unit'>%</span>";

    html += "</div>";

  }

  html += "</div>";


  html += "</div>";


  // =================================================
  // FOOTER
  // =================================================

  html += "<div class='footer'>";

  html += "Data diperbarui setiap 2 detik";

  html += "</div>";


  html += "</div>";

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

}