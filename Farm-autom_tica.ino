//importando bibliotecas
#include <WiFi.h>                         //importa biblioteca para conectar esp32 com wifi
#include <WiFiUdp.h> 
#include <IOXhop_FirebaseESP32.h>          //importa biblioteca para esp32 se comunicar com firebase
#include <ArduinoJson.h>                   //importa biblioteca para colocar informação no formato json, utilizado no firebase (intalar versão 5)
#include <TimeLib.h>
#include <NTPClient.h>


#define WIFI_SSID "NOME DA REDE"                  
#define WIFI_PASSWORD "SENHA DA REDE"             
#define FIREBASE_HOST "FIREBASE URL"
#define FIREBASE_AUTH "FIREBASE API KEY"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(5, OUTPUT);
  
  Serial.begin(115200);      //inicia comunicação serial
  Serial.println();          //imprime pulo de linha

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);     //inicia comunicação com wifi com rede definica anteriormente
  
  Serial.print("Conectando ao wifi");       //imprime "Conectando ao wifi"
  while (WiFi.status() != WL_CONNECTED)     //enquanto se conecta ao wifi fica colocando pontos
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();                        
  timeClient.begin();
  timeClient.setTimeOffset(-10800); // Fuso horário para o Brasil (GMT-3)
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //estabelecendo conexão com firebase
}

void loop() {
  bool regar=Firebase.getBool("/Regar");
  bool autoRega=Firebase.getBool("/AutoRega");
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  struct tm *ptm = gmtime((time_t *)&epochTime); 
  int day = ptm->tm_mday;
  int month = ptm->tm_mon+1;
  int year = ptm->tm_year+1900;
  int hour = ptm->tm_hour;
  int minute = ptm->tm_min;
  int second = ptm->tm_sec;

  char formattedTime[20];
  sprintf(formattedTime, "%02d-%02d-%d %02d:%02d:%02d", day, month, year, hour, minute, second);

  int humidNum=calcular_porcentagem(analogRead(35));
  String humid=String(humidNum) + "%";

  Firebase.setString("/UmidadeAtual", humid);
  String data = "{\"humidity\": " +  String(humid) + ", \"hour\": \""+ formattedTime + "\"}";
  Serial.println(data);
  Firebase.pushString("/Registros", data);

  if(humidNum<20 && autoRega){
    regar=true;
  }

  if(regar){
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(5, HIGH);
    delay(3000);
    Firebase.setBool("/Regar", false);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(5, LOW);
  }else{
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(5, LOW);
  }

  // delay pro proximo loop
  delay(3000);
}

int calcular_porcentagem(int x) {
    // Se x é menor ou igual a 1400(valor retornado com sensor na água), retorna 100%
    if (x <= 1400) {
        return 100;
    }
    // Se x é maior ou igual a 4095(valor retornado com sensor fora da água), retorna 0%
    else if (x >= 4095) {
        return 0;
    }
    // Caso contrário, calcula a porcentagem linearmente entre 1400 e 4095
    else {
        return 100 - ((x - 1400) * 100) / (4095 - 1400);
    }
}
