#include <WiFi.h>
#include <WiFiUdp.h>

const String ROUTER_ID = "R1"; 

// Sub-rede isolada (R1) - NOME ÚNICO E IMUTÁVEL
const char* ssid_ap = "REDE_R1";
const char* password_ap = "senha_r1";
const uint16_t port_ap = 8080;
WiFiUDP udpServer; 

// Possíveis Pais
const char* ssid_c = "coletor";
const char* pass_c = "coletor123";
const char* ip_c = "10.42.0.1";
const uint16_t port_c = 9090;

const char* ssid_r2 = "REDE_R2";
const char* pass_r2 = "senha_r2";
const char* ip_r2 = "10.0.2.1"; 
const uint16_t port_r2 = 8080;  

int currentParent = 1; 
WiFiUDP udpClient; 

int parentFailures = 0; 
const int MAX_PARENT_FAILURES = 3; 

void conectaPai() {
  Serial.println("\n[AVISO] Iniciando troca/reconexao de Pai...");
  WiFi.disconnect();
  delay(100);
  
  if (currentParent == 1) {
    Serial.println("-> Tentando conectar ao Coletor...");
    WiFi.begin(ssid_c, pass_c);
  } else {
    Serial.println("-> Tentando Encadeamento com R2...");
    // Sem scanNetworks, sem softAP novo. Apenas conecta direto!
    WiFi.begin(ssid_r2, pass_r2);
  }

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
    delay(500); 
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERRO] Nao conseguiu conectar no alvo.");
    currentParent = (currentParent == 1) ? 2 : 1; 
  } else {
    Serial.println("\n[SUCESSO] Conectado!");
    parentFailures = 0; 
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);

  IPAddress local_IP(10, 0, 1, 1);
  IPAddress gateway(10, 0, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  // Esta antena liga aqui e NUNCA MAIS é alterada!
  WiFi.softAP(ssid_ap, password_ap, 6); 

  udpServer.begin(port_ap); 
  
  // FIX CRÍTICO: Fixa a porta do cliente UDP para não perder os ACKs do Linux
  udpClient.begin(9091); 
}

void loop() {
  if (WiFi.status() != WL_CONNECTED || parentFailures >= MAX_PARENT_FAILURES) {
    if (parentFailures >= MAX_PARENT_FAILURES) {
      Serial.println("\n[FALHA] Sem ACKs do Pai! Forcando troca...");
      currentParent = (currentParent == 1) ? 2 : 1; 
      parentFailures = 0;
    }
    conectaPai();
  }

  int packetSize = udpServer.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    int len = udpServer.read(incomingPacket, 255);
    if (len > 0) incomingPacket[len] = 0;
    
    String msg = String(incomingPacket);
    msg.trim();

    // ==========================================
    // REGRA ANTI-LAÇO (Vetor de Caminho)
    // Se o pacote já passou por mim, quebra o laço agora mesmo!
    // ==========================================
    if (msg.indexOf("P:" + ROUTER_ID) != -1) {
      Serial.println("[ANTI-LACO] Pacote em loop detectado e destruido.");
      return; 
    }

    IPAddress sensorIP = udpServer.remoteIP();
    uint16_t sensorPort = udpServer.remotePort();

    long rssi_pai = WiFi.RSSI(); 
    String parentName = (currentParent == 1) ? "C" : "R2";
    String payload = msg + "|P:" + ROUTER_ID + "|RSSI_" + ROUTER_ID + "_" + parentName + ":" + String(rssi_pai) + "\n";

    const char* target_ip = (currentParent == 1) ? ip_c : ip_r2;
    const uint16_t target_port = (currentParent == 1) ? port_c : port_r2;

    udpClient.beginPacket(target_ip, target_port);
    udpClient.print(payload);
    udpClient.endPacket();

    bool ack_received = false;
    unsigned long timeout = millis();
    while (millis() - timeout < 2000) {
      if (udpClient.parsePacket()) {
        char ackPacket[255];
        int ackLen = udpClient.read(ackPacket, 255);
        if (ackLen > 0) ackPacket[ackLen] = 0;

        udpServer.beginPacket(sensorIP, sensorPort);
        udpServer.print(ackPacket);
        udpServer.endPacket();
        
        ack_received = true;
        parentFailures = 0; 
        break; 
      }
      delay(1);
    }
    
    if (!ack_received) {
      parentFailures++; 
      Serial.println("Pai não devolveu o ACK. Strike " + String(parentFailures) + "/" + String(MAX_PARENT_FAILURES));
    }
  }
}
