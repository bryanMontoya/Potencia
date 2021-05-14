#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include "mqtt/MQTTConnector.h"
#include "Credentials.h"
#include "max6675.h"

//Pines de termocupla.
int thermoDO = 14;
int thermoCS = 12;
int thermoCLK = 13;
int temperatura = 0;
//Pines MQ2.
const int sensorPin = A0;
int MQ2 = 0;
bool humo = false;
bool alerta = false;
//Pines OLED:Constantes.
#define ANCHO_PANTALLA 128 //Ancho pantalla OLED.
#define ALTO_PANTALLA 32   //Alto pantalla OLED.
//Pines de Salida.
int LTemperatura = 0;
int LHumo = 15;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Objeto de la clase Adafruit_SSD1306
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

void leerTermocupla(){
  //Termocupla..................
  Serial.print("Temperatura = "); 
  temperatura = thermocouple.readCelsius();
  Serial.println(temperatura);     

  //Publicar en Mqtt.
  String output = String( temperatura ) + " grados Celsius.";  
  MQTTPublish(TOPICTEMPERATURE, (char *)output.c_str());                

  if (temperatura > 100)
    digitalWrite(LTemperatura,LOW);  
  else        
    digitalWrite(LTemperatura,HIGH); 
}

void alertaHumo(){  
  if (!alerta){
    digitalWrite(LHumo,HIGH);  
    alerta = true;
  }
  else
  {
    digitalWrite(LHumo,LOW);  
    alerta = false;
  }  
}

void leerMQ2(){
  //MQ2.........................
  MQ2 = analogRead(sensorPin);
  Serial.print("MQ2 = ");
  Serial.println(MQ2);  
  
  if (MQ2 > 500){    //Para alcohol 600, para humo 700.
    humo = true;
    display.clearDisplay();   
    display.setTextSize(3);  
    display.setTextColor(SSD1306_WHITE);  
    display.setCursor(0,0);   
    display.println("Humo!!!");              
    display.display();

    //Publicar en Mqtt.    
    MQTTPublish(TOPICESTADO, "Alerta!!! Humo.");                
    delay(1000);     
  } 
  else    
    humo = false;  
}

void writeOled(){
  //OLED.........................
  display.clearDisplay();   
  display.setTextSize(2);  
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);   
  display.println("Temp:    C");        
  display.setCursor(58,0);  
  display.println(temperatura);     
  display.setCursor(95,0);
  display.cp437(true);  
  display.write(166); //Imprimir caracter especial.         
  display.display();          
  delay(50); 
}

void WiFiBegin(const char* ssid, const char* pass)
{
  WiFi.begin(ssid, pass);
  Serial.printf("Waiting for AP connection ...\n");
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.printf(".");
  }
  IPAddress ip = WiFi.localIP();
  Serial.printf("IP : %d.%d.%d.%d\n", ip[0],ip[1],ip[2],ip[3]);
}

void setup() 
{
  Serial.begin(115200);    
  WiFiBegin(STA_SSID, STA_PASS);    
  MQTTBegin();
  MQTTPublish(TOPICESTADO,"Im alive");
    
  pinMode(LTemperatura, OUTPUT);   
  pinMode(LHumo, OUTPUT);  
  digitalWrite(LHumo,HIGH);
  digitalWrite(LTemperatura, HIGH); 
     
  //OLED...................................................  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {    
    while (true);
  }   
  display.clearDisplay();   
  display.setTextSize(3);  
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0, 4);      // Posición del texto(Columna,fila):128x32
  display.println("Welcome.");   
  display.display();            // Enviar a pantalla
  delay(2000);     
  digitalWrite(LHumo,LOW);
}

void loop() 
{       
  MQTTLoop();     
  leerTermocupla();
  leerMQ2();
  writeOled();  
  if(humo)
    alertaHumo();
  else
    digitalWrite(LHumo,LOW);
    
  delay(500);        
}
  
  