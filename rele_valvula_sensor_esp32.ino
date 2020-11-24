//===============DEFININDO LIVRARIAS UTILIZADAS NO CÓDIGO===============
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

//===============RELAY+VÁLVULA SOLENÓIDE===============
// RELAY NORMALMENTE FECHADO
#define RELAY_NO false

// QUANTIDADE DE RELAYS NO CIRCUITO
#define NUM_RELAYS  1

//PINO 33 AO RELAY DA VÁLVULA SOLENÓIDE
int relayGPIOs[NUM_RELAYS] = {33};

//DEFINIÇÃO DE PARÂMETROS PARA O ESTADO DO RELAY QUE CONTROLA A VÁLVULA
const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "estado";

//===============SENSOR DE VAZÃO===============
//DEFININDO PORTA DE SINAL PARA O SENSOR DE VAZÃO
const int portaVazao = GPIO_NUM_35;

//VARIÁVEIS PARA CÁLCULO DE VAZÃO 
static void atualizaVazao();
volatile int pulsosVazao = 0;
double calculoDaVazao;

// INTERRUPÇÃO NA CONTAGEM DE UM PULSO
void IRAM_ATTR gpio_isr_handler_up(void* arg)
{
  pulsosVazao++;
  portYIELD_FROM_ISR();
}

void iniciaVazao(gpio_num_t Port){
  gpio_set_direction(Port, GPIO_MODE_INPUT);
  gpio_set_intr_type(Port, GPIO_INTR_NEGEDGE);
  gpio_set_pull_mode(Port, GPIO_PULLUP_ONLY);
  gpio_intr_enable(Port);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(Port, gpio_isr_handler_up, (void*) Port);
}

//REGISTRANDO OS ENDEREÇOS DE CONEXÃO A INTERNET
const char* ssid = "Nycholas";
const char* password = "Kfi070380";

//CRIANDO UM OBJETO NO SERVER QUE RECEBE A SOLICITAÇÃO HTTP NA PORTA 80
AsyncWebServer server(80);

//DISPOSIÇÃO GRÁFICA DO WEB SERVER (TÍTULO E SWITCH)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>

//CONFIGURAÇÃO DOS ELEMENTOS NO WEB SERVER (TÍTULO E SWITCH)
  <h2>Projeto Integrador II</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&estado=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&estado=0", true); }
  xhr.send();
}</script>
</body>
</html>
)rawliteral";


String Switch(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String botoes ="";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayEstadoValor = relayEstado(i);
      botoes+= "<h4>Valvula solenoide #" + String(i) + " - GPIO " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayEstadoValor +"><span class=\"slider\"></span></label>";
    }
    return botoes;
  }
  return String();
}

String relayEstado(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "";
    }
    else {
      return "Verificado";
    }
  }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "Verificado";
    }
    else {
      return "";
    }
  }
  return "";
}

void setup(){
  
  //INICIAR PORTA SERIAL
  Serial.begin(115200);

  //INICIAR PINO DO SENSOR DE VAZAO
  iniciaVazao((gpio_num_t) portaVazao);

  // INICIAR RELAY DESLIGADO
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], HIGH);
    }
    else{
      digitalWrite(relayGPIOs[i-1], LOW);
    }
  }
  
  //INICIAR CONEXÃO WI-FI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando-se..");
  }

  //IMPRESSÃO DO IP LOCAL DO ESP32
  Serial.println(WiFi.localIP());

  // ROTA DE ENVIO DA SOLICITAÇÃO GET
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, Switch);
  });

  // ENVIO DA SOLICITAÇÃO GET PARA <ESP_IP>/update?relay=<inputMessage>&estado=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    
  // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        Serial.print("Normalmente aberto ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
      }
      else{
        Serial.print("Normalmente fechado ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
      }
    }
    else {
      inputMessage = "Mensagem não enviada";
      inputParam = "Vazio";
    }
    Serial.println(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  // INICIAR SERVIDOR
  server.begin();
}
  
void loop() {

  //TRANSFORMAÇÃO DOS PULSOS PARA CORRESPONDENTE VAZÃO
  calculoDaVazao = pulsosVazao * 2.25;
  
  //VAZÃO POR MINUTO
  calculoDaVazao = calculoDaVazao * 60;
  calculoDaVazao = calculoDaVazao / 1000;

  pulsosVazao = 0;

  //REALIZAR O PRINT DA LEITURA NO SERIAL
  Serial.println("Litros por minutos: ");
  Serial.println(calculoDaVazao);

  //REALIZAR UM DELAY E INICIALIZAR LEITURA DAQUI A 1 SEGUNDO
  delay(1000);
}
