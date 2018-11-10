#include <ESP8266WiFi.h>  // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubespClient
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "DHT.h"
//biblioteca responsável pela comunicação com o Cartão SD
#include <SD.h>

//defines de id mqtt e tópicos para publicação e subscribe

#define TOPICO_SUBSCRIBE_P1 "casa134/quarto/temperatura"      //tópico MQTT de escuta luz 1
#define TOPICO_SUBSCRIBE_P2 "casa134/quarto/umidade" //tópico MQTT de escuta luz 1

#define ID_MQTT "FelipeMelloFonseca" //id mqtt (para identificação de sessão)
                                     //IMPORTANTE: este deve ser único no broker (ou seja,
                                     //            se um espClient MQTT tentar entrar com o mesmo
                                  //            id de outro já conectado ao broker, o broker
                                     //            irá fechar a conexão de um deles).
#define USER_MQTT "felipe"   // usuario no MQTT
#define PASS_MQTT "12345678" // senha no MQTT

//DHT11
// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// DHT Sensor
const int DHTPin = 5;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

//defines - mapeamento de pinos do NodeMCU
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15 //ligação do CS_PIN
#define D9 3
#define D10 1
#define CS_PIN  D8

// WIFI
const char *myHostname = "Felipe_ESP";
const char *SSID = "C3T";            // SSID / nome da rede WI-FI que deseja se conectar
const char *PASSWORD = "caliel1234"; // Senha da rede WI-FI que deseja se conectar

// MQTT
const char *BROKER_MQTT = "10.0.2.10"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                      // Porta do Broker MQTT
long lastMsg = 0;                            //tempo da ultima mensagem publicada
char msg[50];
int value = 0;

//Variáveis e objetos globais
WiFiClient espClient;         // Cria o objeto espespClient
PubSubClient MQTT(espClient); // Instancia o espCliente MQTT passando o objeto
WiFiManager wifiManager;      //Instancia do WifiManage

// Variáveis climáticas
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
float temperatura = 0;
float umidade = 0;

// Web Server on port 80
WiFiServer server(80);

//Prototypes"
void escreverSD();
void verificarSD();
void initSerial();
void initDHT11();
void loopDHT11();
void init_WifiAp();
void initWiFi();
void initOTA();
void initMQTT();
void reconectWiFi();
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
void resetWifi();

/* 
 *  Implementações das funções
 */
void setup()
{
  //inicializações:

  initSerial();
  verificarSD();
  //initWiFi();// Versão com Wifi normal sem Wifimanage
  init_WifiAp(); //Versão usnado o Wifimanage
  initServer();
  //initOTA();
  initMQTT(); // Não estamos usando MQTT nesse módulo atpe o momento
  //InitOutput();
  initDHT11();
  loopDHT11();
}

//Inicia o DHT no SETUP do programa
void initDHT11()
{
  dht.begin();
}

// verifica se o cartão SD está presente e se pode ser inicializado
void verificarSD(){
  if (!SD.begin(CS_PIN)) {
    Serial.println("Falha, verifique se o cartão está presente.");
    //programa encerrrado
    return;
  }
}

//Inicia o server
void initServer()
{
  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
  delay(10000);
}
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial()
{
  Serial.begin(115200);
}

//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi()
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconectWiFi();
}

//Função: inicializa OTA - permite carga do novo programa via Wifi
//Parâmetros: nenhum
//Retorno: nenhum
void initOTA()
{
  Serial.println();
  Serial.println("Iniciando OTA....");
  ArduinoOTA.setHostname("pratica-4"); // Define o nome da porta

  // No authentication by default
  //ArduinoOTA.setPassword((const char *)"teste-ota"); // senha para carga via WiFi (OTA)
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

//Função: inicializa parâmetros de conexão MQTT(endereço do
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT()
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);          //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

//Função: Publica valores de temperatura no MQTT
//Parâmetros: nenhum
//Retorno: nenhum

void publMQTT()
{

  if (!MQTT.connected())
  {
    reconnectMQTT();
  }
  MQTT.loop();

  long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    ++value;
    snprintf(msg, 75, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(String(temperatura).c_str());
     MQTT.publish(TOPICO_SUBSCRIBE_P1, String(temperatura).c_str());
     Serial.println(String(umidade).c_str());
     MQTT.publish(TOPICO_SUBSCRIBE_P2, String(umidade).c_str());
  }
}

//Função: função de callback
//        esta função é chamada toda vez que uma informação de
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  String msg;

  //obtem a string do payload recebido
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.println("msg = " + msg);

  if (msg.equals("ON")) // liga led
  {
    digitalWrite(D1, HIGH);
    Serial.println("Ligado led");
  }

  if (msg.equals("OFF"))
  {
    digitalWrite(D1, LOW);
    Serial.println("Desligado led");
  }

  if (msg.equals("1")) // resetwifi
  {
    resetWifi();
    Serial.println("Reiniciando Wifi");
    init_WifiAp(); //Inicializar Wifimanager pq reiniciei
  }
}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT()
{
  while (!MQTT.connected())
  {
    escreverSD();//Escrever o log da temperatura no SD
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    // if (MQTT.connect(ID_MQTT, USER_MQTT,PASS_MQTT)) // parameros usados para broker proprietário
    // ID do MQTT, login do usuário, senha do usuário

    if (MQTT.connect(ID_MQTT))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.publish(TOPICO_SUBSCRIBE_P1, String(temperatura).c_str());
      MQTT.publish(TOPICO_SUBSCRIBE_P2, String(umidade).c_str());
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      delay(2000);
    }
  }
}

//Função: Resetar o wifimanager com as configurações guardadas
//Parâmetros: nenhum
//Retorno: nenhum
void resetWifi()
{
  wifiManager.resetSettings();
}

// Função de conexão com o WIFImanager
//Parâmetros: nenhum
//Retorno: nenhum
void init_WifiAp()
{
  WiFi.hostname(myHostname);
  //wifiManager.resetSettings(); //Usado para resetar sssid e senhas armazenadas
  wifiManager.setTimeout(60); //caso ninguém entre no wifimanager timeout
  wifiManager.autoConnect();  //Conectar através da parada

  Serial.print("Conectado com sucesso na rede via WifiManager na rede: ");
  Serial.println(WiFi.SSID());
  Serial.println();
  Serial.print("IP obtido: ");
  Serial.print(WiFi.localIP()); // mostra o endereço IP obtido via DHCP
  Serial.println();
  Serial.print("Endereço MAC: ");
  Serial.print(WiFi.macAddress()); // mostra o endereço MAC do esp8266
}

//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi()
{
  //se já está conectado a rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede: ");
  Serial.print(SSID);
  Serial.println();
  Serial.print("IP obtido: ");
  Serial.print(WiFi.localIP()); // mostra o endereço IP obtido via DHCP
  Serial.println();
  Serial.print("Endereço MAC: ");
  Serial.print(WiFi.macAddress()); // mostra o endereço MAC do esp8266
}

//Função: verifica o estado das conexões WiFI e ao broker MQTT.
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconectWiFi();
  reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

  reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

//Função: inicializa o output em nível lógico baixo
//Parâmetros: nenhum
//Retorno: nenhum
void InitOutput(void)
{

  pinMode(D1, OUTPUT); //enviar HIGH para o output faz o Led acender / enviar LOW faz o Led apagar)
  digitalWrite(D1, LOW);
}

void loopDHT11()
{
  // Listenning for new espClients
  espClient = server.available();

  if (espClient)
  {
    Serial.println("New espClient");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (espClient.connected())
    {
      if (espClient.available())
      {
        char c = espClient.read();

        if (c == '\n' && blank_line)
        {
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          float h = dht.readHumidity();
          // Read temperature as Celsius (the default)
          float t = dht.readTemperature();
          temperatura = t; //trasnferindo o valor para uma variável global para tentar usar
          // Read temperature as Fahrenheit (isFahrenheit = true)
          float f = dht.readTemperature(true);
          // Check if any reads failed and exit early (to try again).
          if (isnan(h) || isnan(t) || isnan(f))
          {
            Serial.println("Failed to read from DHT sensor!");
            strcpy(celsiusTemp, "Failed");
            strcpy(fahrenheitTemp, "Failed");
            strcpy(humidityTemp, "Failed");
          }
          else
          {
            // Computes temperature values in Celsius + Fahrenheit and Humidity
            float hic = dht.computeHeatIndex(t, h, false);
            dtostrf(hic, 6, 2, celsiusTemp);
            float hif = dht.computeHeatIndex(f, h);
            dtostrf(hif, 6, 2, fahrenheitTemp);
            dtostrf(h, 6, 2, humidityTemp);
            // You can delete the following Serial.print's, it's just for debugging purposes
            Serial.print("Humidity: ");
            Serial.print(h);
            Serial.print(" %\t Temperature: ");
            Serial.print(t);
            Serial.print(" *C ");
            Serial.print(f);
            Serial.print(" *F\t Heat index: ");
            Serial.print(hic);
            Serial.print(" *C ");
            Serial.print(hif);
            Serial.print(" *F");
            Serial.print("Humidity: ");
            Serial.print(h);
            Serial.print(" %\t Temperature: ");
            Serial.print(t);
            Serial.print(" *C ");
            Serial.print(f);
            Serial.print(" *F\t Heat index: ");
            Serial.print(hic);
            Serial.print(" *C ");
            Serial.print(hif);
            Serial.println(" *F");
          }
          espClient.println("HTTP/1.1 200 OK");
          espClient.println("Content-Type: text/html");
          espClient.println("Connection: close");
          espClient.println();
          // your actual web page that displays temperature and humidity
          espClient.println("<!DOCTYPE HTML>");
          espClient.println("<html>");
          espClient.println("<head></head><body><h1>ESP8266 - Temperature and Humidity</h1><h3>Temperature in Celsius: ");
          espClient.println(celsiusTemp);
          espClient.println("*C</h3><h3>Temperature in Fahrenheit: ");
          espClient.println(fahrenheitTemp);
          espClient.println("*F</h3><h3>Humidity: ");
          espClient.println(humidityTemp);
          espClient.println("%</h3><h3>");
          espClient.println("</body></html>");
          break;
        }
        if (c == '\n')
        {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r')
        {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }
    // closing the espClient connection
    delay(1);
    espClient.stop();
    Serial.println("espClient disconnected.");
  }
}

void loopDHT11Time()
{

  delay(2000);
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();

  //intervalo de espera para uma nova leitura dos dados.

  
}

void escreverSD(){
    File dataFile = SD.open("LOG.txt", FILE_WRITE);
  // se o arquivo foi aberto corretamente, escreve os dados nele
  if (dataFile) {
    Serial.println("O arquivo foi aberto com sucesso.");
      //formatação no arquivo: linha a linha >> UMIDADE | TEMPERATURA
      dataFile.print(umidade);
      dataFile.print(" | ");
      dataFile.println(temperatura);
      Serial.println("Escreveu no cartão");
      //fecha o arquivo após usá-lo
      dataFile.close();
  }
  // se o arquivo não pôde ser aberto os dados não serão gravados.
  else {
    Serial.println("Falha ao abrir o arquivo LOG.txt");
  }
 
}

//programa principal
void loop()
{
  ArduinoOTA.handle(); // keep-alive da comunicação OTA
  loopDHT11();         //loop para apresentar a analise do DHT11

  // VerificaConexoesWiFIEMQTT();//garante funcionamento das conexões WiFi e ao broker MQTT

  MQTT.loop(); //keep-alive da comunicação com broker MQTT
  publMQTT();  //Publicar o MQTT
  loopDHT11Time();
  escreverSD();
}
