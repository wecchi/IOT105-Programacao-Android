/*
 *  Aulas Online - LSI-TEC: IOT105 Aplicativos para dispositivos móveis
 *  Programa/hardware: 015 - Aula 09 - Escrevendo em uma porta Arduino
 *  https://codeiot.org.br/courses/course-v1:LSI-TEC+IOT105+2021_OC/courseware/a4e9877e66da4498b33c070fb8a2b7e8/1cfee213ca1449ed93f5ca4ced3bc955/?child=first
 *  
 *  Descrição: usar uma Arduino conectada a um módulo ESP-01S para permitir controlar o LED usando um 
 *  browser (navegador). A ideia é ligar, desligar e piscar o LED, permitir:
 *     - controlar um atuador (liga ou desliga LED);
 *     - exibir uma mensagem no browser ao usuário.
 * 
 *  O Circuito:
 *     RX is digital rxPin (connect to TX of other device)
 *     TX is digital txPin (connect to RX of other device)
 * 
 *  Autor: Marcelo Wecchi
 *  Revisão: 17Abr2022
*/

// =============================================================================================================
// --- Bibliotecas ---
#include "SoftwareSerial.h"

// --- Portas utilizadas da Arduino UNO ---
#define rxPin 6   // Porta para recebimento dos dados da ESP8266
#define txPin 7   // Porta para envio dos dados da ESP8266

// --- Constantes --- 
#define TIMEOUT 5000                    // miliSegundos para timeout
const String strSSID = "@@@@@@@@@@@@";   // Nome da rede WiFi
const String strPSW = "*************";  // Senha da rede WiFi
const String strHTML = "<!DOCTYPE html><html lang=\"pt-br\"><head><title>LSI-TEC: IOT105</title><meta charset=\"UTF-8\"></head><body>#c</body></html>";
//const String strHTML = "#c";

// --- Variáveis --- 
String Header;
int blinkFreq = 0;

// --- Criação de objetos --- 
SoftwareSerial esp01(rxPin, txPin); // RX, TX


// =============================================================================================================
// --- Assinatura das Funções Auxiliares ---
void init_ESP8266();
boolean SendCommand(String cmd, String ack);
void piscaLED(int LED);
boolean sendHttpResponse(int connectionId, String content);


// =============================================================================================================
// --- Configurações Iniciais ---
void setup()
{
 
  Serial.begin(9600);
  esp01.begin(19200); // Geralmente iniciar com 19200 funciona!
  // LED para sinalização das operações
  pinMode(LED_BUILTIN, OUTPUT);

  if(esp01.isListening()){
    init_ESP8266();    
  } else {
    Serial.println("Setup has error and NOT complete!!");
  }
  
}

void loop() {
  String IncomingString = "";
  boolean StringReady = false;
  int connectionId = 0;

  while (esp01.available()) {
    if (esp01.find("+IPD,")){
      delay(300);

      // Obtêm o ID de conexão com o Cliente
      connectionId = esp01.read() - 48;

      // Lê caracteres do buffer serial e os move para uma String. A função termina se ocorre time-out 
      IncomingString = esp01.readString();
      StringReady = true;
      delay(50);
    }
  }

  if (StringReady) {
    String cipClose = "AT+CIPCLOSE=";
    cipClose += connectionId;

    // processa URL -> http://IP_ARDUINO/FREQ=xxxx
    if (IncomingString.indexOf("GET /FREQ") != -1 && IncomingString.indexOf("HTTP/1.1") != -1) {
      // busca frequência para delay usado no blink
      int inicio = IncomingString.indexOf("GET /FREQ") + 10;
      int fim = IncomingString.indexOf("HTTP/1.1") - 1;

      String valueTxt = IncomingString.substring(inicio, fim);
      blinkFreq = valueTxt.toInt();
      valueTxt = "<h1>LED Piscando: " + valueTxt + "</h1>";
      sendHttpResponse(connectionId, valueTxt);
      // Fecha conexão com o cliente
      SendCommand(cipClose, "CLOSED");
    } 
    
    // processa URL -> http://IP_ARDUINO/LED=ON
    if (IncomingString.indexOf("/LED=ON HTTP") != -1) {
      blinkFreq = 0;
      digitalWrite(LED_BUILTIN, HIGH);
      sendHttpResponse(connectionId, "<h1>LED Aceso</h1>");
      // Fecha conexão com o cliente
      SendCommand(cipClose, "CLOSED");
    }
    
    // processa URL -> http://IP_ARDUINO/LED=OFF
    if (IncomingString.indexOf("/LED=OFF HTTP") != -1) {
      blinkFreq = 0;
      digitalWrite(LED_BUILTIN, LOW);
      sendHttpResponse(connectionId, "<h1>LED Apagado</h1>");
      // Fecha conexão com o cliente
      SendCommand(cipClose, "CLOSED");      
    }
    
  // piscar LED caso tenha sido definido blinkFreq
  } else if (blinkFreq > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(blinkFreq);
    digitalWrite(LED_BUILTIN, LOW);
    delay(blinkFreq);
  }
}


// =============================================================================================================
// --- Funções Auxiliares ---

/* Inicializar a ESP01 */
void init_ESP8266(){   
  Serial.println("---------------------\r\n> Iniciando ESP8266 <\r\n---------------------");
    
    //Reseta as configurações anteriores - se houver
    SendCommand("AT+RST", "OK");
 
    // Define a velocidade de comunicacao do modulo ESP8266 (9600, 19200, 38400, 57600, etc)
    SendCommand("AT+CIOBAUD=19200", "OK");    

    // Modo de coneção da ESP - Station mode (client)
    SendCommand("AT+CWMODE=1", "OK");
     
    // Conecta na rede wireless **OBRIGATÓRIO**
    String cwjap = "AT+CWJAP=\"#SSID#\",\"#PSW#\"";
    cwjap.replace("#SSID#", strSSID);
    cwjap.replace("#PSW#", strPSW);
    SendCommand(cwjap, "OK");
    delay(3000);
    
     // Obtêm o endereco IP atribuído pelo roteador
    SendCommand("AT+CIFSR", "OK");
     
    // Configura para multiplas conexoes
    SendCommand("AT+CIPMUX=1", "OK");
     
    // Inicia o web server na porta 80
    SendCommand("AT+CIPSERVER=1,80", "OK");
    delay(500);
    
    // Sets the TCP Server Timeout
    SendCommand("AT+CIPSTO=100", "OK");
    delay(500);

    Serial.println("---------------------\r\n>  ESP8266 ativado  <\r\n---------------------");
    piscaLED(3);
}

/* Envia comandos para a placa ESP */
boolean SendCommand(String cmd, String ack) {
  esp01.println(cmd); // Send "AT+" command to module
  if (!echoFind(ack)) // timed out waiting for ack string
    return true; // ack blank or ack found
}

boolean echoFind(String keyword) {
  byte current_char = 0;
  byte keyword_length = keyword.length();
  long deadline = millis() + TIMEOUT;
  while (millis() < deadline) {
    if (esp01.available()) {
      char ch = esp01.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length) {
          Serial.println();
          return true;
        }
    }
  }
  return false; // Timed out
}

/* Sinaliza configuração concluída */
void piscaLED(int LED){
  for (int i = 1; i<=LED; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
  }
}

/* função para respostas HTTP */
boolean sendHttpResponse(int connectionId, String content) {
  // formatação da página HTML
  String valueStr = strHTML;
  valueStr.replace("#c", content);
  
  // cabeçalho HTTP para GET
  Header =  "HTTP/1.1 200 OK\r\n";
  Header += "Content-Type: text/html; charset=utf-8\r\n";
  Header += "Connection: close\r\n";
  Header += "Content-Length: ";
  Header += (int)(valueStr.length());
  Header += "\r\n\r\n";    //blank line

  // Tamanho do pacote a ser enviado ao Cliente
  int sizeString = Header.length() + valueStr.length();

  // envia comandos AT para modulo ESP
  String cipSend = "AT+CIPSEND=";    
  cipSend += connectionId;       
  cipSend += ",";
  cipSend += sizeString;
  esp01.println(cipSend);
  delay(10);

  // continua comunicacao somente se receber o caractere de prompt - Cliente aguardando ...
  if (esp01.find(">")) {
    esp01.print(Header);
    //delay(10);
    esp01.println(valueStr);
    delay(100);
  }
}
  
