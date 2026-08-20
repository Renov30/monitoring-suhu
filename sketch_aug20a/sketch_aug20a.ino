#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  dht.begin();

  Serial.println("Monitoring Suhu & Kelembapan");
  Serial.println("==============================");
}

void loop() {
  // Baca kelembapan
  float humidity = dht.readHumidity();

  // Baca suhu dalam Celsius
  float temperature = dht.readTemperature();

  // Cek apakah pembacaan berhasil
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca sensor DHT11!");
    delay(2000);
    return;
  }

  Serial.print("Suhu       : ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Kelembapan : ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.println("------------------------------");

  // DHT11 sebaiknya tidak dibaca terlalu cepat
  delay(2000);
}