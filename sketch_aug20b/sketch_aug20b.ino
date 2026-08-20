#include <ETH.h>
#include <WebServer.h>
#include "DHT.h"

// Definisi Pin & Tipe Sensor
#define DHTPIN 4     // Pin data DHT11 terhubung ke GPIO 4
#define DHTTYPE DHT11
#define LED_PIN 2    // Pin LED indikator terhubung ke GPIO 2

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// Konfigurasi IP Statis (Sesuaikan dengan subnet jaringan Anda)
IPAddress local_IP(192, 168, 2, 23); // Alamat IP untuk WT32-ETH01
IPAddress gateway(192, 168, 2, 254);   // Alamat IP Gateway / Router
IPAddress subnet(255, 255, 255, 0);  // Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);    // DNS Primer (Google DNS)

void handleRoot() {
  // Nyalakan LED saat halaman web diakses/dibaca
  digitalWrite(LED_PIN, HIGH);
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // Membuat halaman HTML sederhana dengan fitur auto-refresh setiap 5 detik
  String html = "<html lang='en'>";
  html += "<head><meta http-equiv='refresh' content='5'><title>Monitoring Suhu LAN</title></head>";
  html += "<body style='font-family: Arial; text-align: center; margin-top: 50px;'>";
  html += "<h1>Monitoring Suhu & Kelembapan (WT32-ETH01)</h1>";
  html += "<p style='font-size: 24px;'>Suhu: <b>" + String(t) + " &deg;C</b></p>";
  html += "<p style='font-size: 24px;'>Kelembapan: <b>" + String(h) + " %</b></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Matikan LED setelah proses selesai
  delay(200);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  
  // Konfigurasi Pin LED sebagai Output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Inisialisasi Sensor DHT
  dht.begin();

  // Terapkan konfigurasi IP Statis (HARUS dipanggil sebelum ETH.begin())
  if (!ETH.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("Gagal mengatur konfigurasi IP statis!");
  }

  // Mulai Ethernet
  ETH.begin();

  // Daftarkan fungsi handler untuk web server
  server.on("/", handleRoot);
  server.begin();
  
  Serial.println("Web Server LAN Berjalan!");
  Serial.print("Akses melalui IP Statis: http://");
  Serial.println(local_IP);
}

void loop() {
  // Menangani permintaan akses dari browser secara terus-menerus
  server.handleClient();
}