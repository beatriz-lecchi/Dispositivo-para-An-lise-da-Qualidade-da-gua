#include <WiFi.h>
#include <WebServer.h>
#define ALPHA 0.1  // Constante do filtro passa-baixa (0 < ALPHA <= 1)

// Configuração da rede local
const char* ssid = "ESP32-AP";
const char* password = "12345678";

IPAddress ip(192, 168, 2, 123);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

// PORTAS
const int ledPin = 2;
const int potPin = 35;
const int TurbPin = 34;

// Pinos do motor
int pwmPin = 25; // Pino DAC para controle de velocidade

// setting PWM properties
const int freq = 5000;
const int resolution = 8;

// Variáveis globais para sensores
float V_turb = 0;
float V_PH = 0;
float PH = 0;
float filteredValue = 0;  // Valor filtrado inicializado em 0

// Servidor Web
WebServer server(80);

// Função para página inicial
void handleRoot() {
  String html = "<html><body>"
                "<h1>Controle do Sistema</h1>"
                "<p><a href=\"/led/on\"><button>Acender LED</button></a></p>"
                "<p><a href=\"/led/off\"><button>Apagar LED</button></a></p>"
                "<p><a href=\"/var_PH\"><button>Obter Valor de PH</button></a></p>"
                "<p><a href=\"/var_turb\"><button>Obter Valor de Turbidez</button></a></p>"
                "<p><a href=\"/ligar_motor\"><button>Ligar Motor</button></a></p>"
                "<p><a href=\"/para_motor\"><button>Parar Motor</button></a></p>"
                "</body></html>";
  server.send(200, "text/html", html);
}

// Funções para controle do LED
void handleLedOn() {
  digitalWrite(ledPin, HIGH); // Acender o LED
  server.send(200, "text/html", "<html><body><h1>LED Ligado!</h1><a href=\"/\">Voltar</a></body></html>");
}

void handleLedOff() {
  digitalWrite(ledPin, LOW); // Apagar o LED
  server.send(200, "text/html", "<html><body><h1>LED Desligado!</h1><a href=\"/\">Voltar</a></body></html>");
}

// Função para ler turbidez
void lerTurbidez() {
  int x = analogRead(TurbPin);

  if (x <= 2600) {
    V_turb = 0.0008 * x + 0.1433; 
  } else {
    V_turb = 0.0006 * x + 0.7328; 
  }

  // Depuração no monitor serial
  Serial.println((String) "Turbidez - AN: " + x + "  V: " + V_turb);
}

// Função para responder com o valor de turbidez
void responderTurbidez() {
  String response_varTurb = "Valor Analógico de Turbidez: " + String(analogRead(TurbPin)) + 
                            "\nTensão no Turbidímetro: " + String(V_turb) + " V";
  server.send(200, "text/plain", response_varTurb);
}

// Função para ler valores do PH
void lerPH() {
  int convAD = analogRead(potPin);

  // Aplica o filtro passa-baixa
  filteredValue = ALPHA * convAD + (1 - ALPHA) * filteredValue;

  if (filteredValue <= 2600) {
    V_PH = (0.0008 * filteredValue + 0.1433);
  } else {
    V_PH = (0.0006 * filteredValue + 0.7328);
  }

//  PH = -25 * V_PH + 52;
//  PH = -23.323* V_PH + 48.972;
    PH = -18.22 * V_PH + 38.224;
//  PH = -19.639 * V_PH + 41.097;

  if (PH > 14){
    PH = 14;
    }
  else if (PH < 0){
    PH = 0;
  }
  
  // Depuração no monitor serial
  Serial.println((String) "PH - PH: " + PH + "  V: " + V_PH);
}

// Função para responder com o valor de PH
void responderPH() {
  String response_var_PH = "PH: " + String(PH) + "\nTensão PH: " + String(V_PH) + " V";
  server.send(200, "text/plain", response_var_PH);
}

// Funções para controlar o motor
void ligar_motor() {
  ledcWrite(pwmPin, 200);
  server.send(200, "text/html", "<html><body><h1>MOTOR Ligado!</h1><a href=\"/\">Voltar</a></body></html>");
}

void para_motor() {  
  ledcWrite(pwmPin, 0);
  server.send(200, "text/html", "<html><body><h1>MOTOR Desligado!</h1><a href=\"/\">Voltar</a></body></html>");
}

// Configuração inicial
void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  ledcAttach(pwmPin, freq, resolution);
  
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(ip, gateway, subnet);

  // Configuração de rotas
  server.on("/", handleRoot);           // Página inicial
  server.on("/led/on", handleLedOn);   // Acender LED
  server.on("/led/off", handleLedOff); // Apagar LED
  server.on("/var_PH", responderPH);   // Responder com valores de PH
  server.on("/var_turb", responderTurbidez); // Responder com valores de turbidez
  server.on("/ligar_motor", ligar_motor);   // Ligar motor
  server.on("/para_motor", para_motor);    // Parar motor

  server.begin(); // Inicia o servidor
  Serial.begin(115200);
  Serial.println("Servidor Web iniciado!");
}

// Loop principal
void loop() {
  server.handleClient(); // Trata requisições do cliente

  // Atualiza os valores dos sensores
  lerTurbidez();
  lerPH();
}
