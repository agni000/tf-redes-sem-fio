// 8266 COM O ADESIVO

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const String SENSOR_ID = "S2";

const char* ssid_r1 = "REDE_R1";
const char* pass_r1 = "senha_r1";
const char* ip_r1 = "10.0.1.1"; 

const char* ssid_r2 = "REDE_R2";
const char* pass_r2 = "senha_r2";
const char* ip_r2 = "10.0.2.1"; 

int currentRouter = 2; 
int consecutiveFailures = 0; 
const int MAX_FAILURES = 30; 

WiFiUDP udp;
unsigned long lastSendTime = 0;
uint32_t seqNum = 1;
unsigned long lastRTT = 0;

void conectaWiFi() {
  WiFi.disconnect();
  delay(100);
  
  if (currentRouter == 1) {
    Serial.println("\nConectando em R1...");
    WiFi.begin(ssid_r1, pass_r1);
  } else {
    Serial.println("\nConectando em R2...");
    WiFi.begin(ssid_r2, pass_r2);
  }
  
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 6000) {
    delay(500); Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Conectado!");
    consecutiveFailures = 0; 
    udp.begin(8080); // Inicia a porta UDP local do sensor para ouvir as respostas
  } else {
    Serial.println("\nRoteador ausente no ar!");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  conectaWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    consecutiveFailures++;
    delay(1000); 
  }

  if (consecutiveFailures >= MAX_FAILURES) {
    Serial.println("\n[FALHA] Roteador atual indisponivel. Executando Roaming!");
    currentRouter = (currentRouter == 1) ? 2 : 1; 
    conectaWiFi();
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= 1000) { 
    lastSendTime = currentMillis;

    const char* target_ip = (currentRouter == 1) ? ip_r1 : ip_r2;
    long rssi_salto = WiFi.RSSI();
    String parent = (currentRouter == 1) ? "R1" : "R2";
    
    String msg = SENSOR_ID + "|SEQ:" + String(seqNum) + "|RSSI_" + SENSOR_ID + "_" + parent + ":" + String(rssi_salto) + "|RTT_ANT:" + String(lastRTT) + "\n";

    unsigned long start_rtt = millis();
    
    // Disparo UDP
    udp.beginPacket(target_ip, 8080);
    udp.print(msg);
    udp.endPacket();

    bool ack_received = false;
    unsigned long timeout = millis();
    
    // Aguarda o ACK voltar
    while (millis() - timeout < 2000) {
      if (udp.parsePacket()) {
        char incomingPacket[255];
        udp.read(incomingPacket, 255); // Limpa o buffer
        
        lastRTT = millis() - start_rtt; 
        consecutiveFailures = 0; 
        ack_received = true;
        break;
      }
      delay(1);
    }

    if (!ack_received) {
      lastRTT = 0; 
      consecutiveFailures++; // Conta como pacote perdido e Strike
    }
    
    seqNum++; 
  }
}
