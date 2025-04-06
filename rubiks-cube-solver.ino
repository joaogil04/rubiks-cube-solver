#include <Arduino.h>

// Incluir a biblioteca correta com base na plataforma
#ifdef ESP32
#include <WebServer.h>
#include <WiFi.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#endif
#include <LittleFS.h>

#include "Solver.hpp"

const char* ssid = "CubeSolver";
const char* password = "#cubeSolver";

Solver cube("054305225013310135223124124533035111423144040402550245");
#ifdef ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif


//ajustar depois
#define SERVOS_0 100
#define SERVOS_90 350
#define SERVOS_180 600
#define DELAY 1000 

char cube_state[] = "UFRBLD";
int valueServo1 = SERVOS_90;
int valueServo2 = SERVOS_180;

void rotate_base_clock(){
  valueServo1 -= SERVOS_90;
  delay(DELAY);
}

void rotate_base_counter_clock(){
  valueServo1 += SERVOS_90;
  delay(DELAY);
}

void rotate_cubo_up(){
  valueServo2 = SERVOS_180;
  delay(DELAY);
  char temp = cube_state[0];
  cube_state[0] = cube_state[3];
  cube_state[3] = cube_state[5];
  cube_state[5] = cube_state[1];
  cube_state[1] = temp;
  valueServo2 = SERVOS_90;
}

void upper_lock(){
  valueServo2 = SERVOS_90;
  delay(DELAY);
}

void lower_lock(){
  valueServo2 = SERVOS_0;
  delay(DELAY);
}

void rotate_cubo_counter_clock(){
  char temp = cube_state[1];
  upper_lock();
  rotate_base_counter_clock();
  cube_state[1] = cube_state[4];
  cube_state[4] = cube_state[3];
  cube_state[3] = cube_state[2];
  cube_state[2] = temp;
}

void rotate_cubo_clock(){
  char temp = cube_state[1];
  upper_lock();
  rotate_base_clock();
  cube_state[1] = cube_state[2];
  cube_state[2] = cube_state[3];
  cube_state[3] = cube_state[4];
  cube_state[4] = temp;
}

void counter_clock_move(){
  if(valueServo1 == SERVOS_180){//também pode ser 0º
    rotate_cubo_counter_clock();
  }
  lower_lock();
  rotate_base_clock();
  rotate_cubo_counter_clock();
}

void clock_move(){
  if(valueServo1 == SERVOS_0){//também pode ser 180º
    rotate_cubo_clock();
  }
  lower_lock();
  rotate_base_counter_clock();
  rotate_cubo_clock();
}

void move2(){
  if(valueServo1 != SERVOS_0 && valueServo1 != SERVOS_180){
    rotate_cubo_clock();
  }
  lower_lock();
  if(valueServo1 == SERVOS_0){
    rotate_base_clock();
  }else if(valueServo1 == SERVOS_180){
    rotate_base_counter_clock();
  }
}

void rotate_to_x(char x){
  for(int i = 0; i < sizeof(cube_state); i++){
    if(cube_state[i] == x){
      int y = i;
    }
  }

  switch (y)
  {
  case 0:
    for(int i = 0; i < 2; i++){
      rotate_cubo_up();
    }
    break;
  
  case 1:
    rotate_cubo_up();
    break;

  case 2:
    rotate_cubo_clock();
    rotate_cubo_up();
    rotate_cubo_counter_clock();
    break;

  case 3:
    for(int i = 0; i < 3; i++){
      rotate_cubo_up();
    }
    break;

  case 4:
    rotate_cubo_counter_clock();
    rotate_cubo_up();
    rotate_cubo_clock();
    break;
  }
}

// Envia ficheiro do LittleFS para o cliente
void serveFile(const String& path, const String& contentType) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
  } else {
    server.send(404, "text/plain", "Ficheiro não encontrado");
  }
}

// Inicia todas as rotas
void setupRoutes() {
  // Principal
  server.on("/", HTTP_GET, []() {
    serveFile("/index.html", "text/html");
  });

  // Envia CSS
  server.on("/css/style.css", HTTP_GET, []() {
    serveFile("/css/style.css", "text/css");
  });

  // Envia JS
  server.on("/js/app.js", HTTP_GET, []() {
    serveFile("/js/app.js", "application/javascript");
  });

  // Envia o estado do cubo (peças)
  server.on("/cubestate", HTTP_GET, []() {
    server.send(200, "application/json",
                getCubeStateString(cube.piece_state()));
  });

  // Recebe uma sequencia de movimentos e executa-a
  server.on("/sendmove", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String data = server.arg("plain");
      cube.move(string(data.c_str()));
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Erro: Nenhum dado recebido");
    }
  });

  // Resolve o cubo e envia a resolução
  server.on("/solve", HTTP_GET, []() {
    server.send(200, "text/plain", String(cube.solve().c_str()));
  });

  // Aplica um scramble e envia os movimentos
  server.on("/scramble", HTTP_GET, []() {
    server.send(200, "text/plain", String(cube.scramble(20).c_str()));
  });
}


// Transforma um array em uma string para ser lido pelo js
String getCubeStateString(const std::array<std::array<int, 3>, 26>& cubeState) {
  String result = "[";
  for (size_t j = 0; j < cubeState.size(); j++) {
    // Juntar trios de cores
    result += "[";
    for (int i = 0; i < 3; i++) {
      result += String(cubeState[j][i]);
      if (i < 2) result += ", ";
    }
    result += "]";
    // Adicionar a virgula para o próxima trio
    if (j < cubeState.size() - 1) result += ",";
  }
  result += "]";
  return result;
}

void setup() {
  // Iniciar o Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nIniciando ...");


  // Iniciar o LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Falha ao montar LittleFS! A formatar...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("Erro: Não foi possível montar LittleFS!");
      return;
    }
  }

  // Configurar Wi-Fi
  WiFi.softAP(ssid, password);

  // Informação sobre o Wi-Fi
  Serial.println("\nRede Wi-Fi criada!");
  Serial.print("Endereço IP do AP: ");
  Serial.println(WiFi.softAPIP());

  // Iniciar rotas
  setupRoutes();

  // Iniciar rotas
  server.begin();

  // Mensagem Final
  Serial.println("Servidor HTTP iniciado!");
}

void loop() {
  // Receber Clientes
  server.handleClient();
}
