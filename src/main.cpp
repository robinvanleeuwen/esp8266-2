#include <stdio.h>
#include <unistd.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>


String ReadIniFile(String Key);
String getDateTime(const RtcDateTime& dt);

const uint8_t YELLOW_D2_TO_RTC_RST = 4;
const uint8_t ORANGE_D4_TO_RTC_DAT = 2;
const uint8_t GREEN__D5_TO_RTC_CLK = 14;

ThreeWire myWire(ORANGE_D4_TO_RTC_DAT, GREEN__D5_TO_RTC_CLK, YELLOW_D2_TO_RTC_RST);
RtcDS1302<ThreeWire> Rtc(myWire);

using namespace std;

WiFiClient client;
String server_ip;
String server_port;

void blink(int number_of_blinks, int delay_in_ms) {

    for (int i=0; i< number_of_blinks; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delay_in_ms);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("Blinked");
    }
}

String getDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring,
               sizeof(datestring),
               PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
               dt.Month(),
               dt.Day(),
               dt.Year(),
               dt.Hour(),
               dt.Minute(),
               dt.Second() );
    return (String) datestring;
}


void SendToServer(String line)
{
    if (client.connect(server_ip.c_str(), server_port.toInt()))
    {
        client.println(line);
    }

}

String IPAddressToString(IPAddress ip){
    char buffer[15];
    sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return String(buffer);
}

String ReadIniFile(String key)
{
    SPIFFS.begin();
    fs::File ini_file = SPIFFS.open("/settings.ini", "r");
        while (ini_file.available())
        {
            String line = ini_file.readStringUntil('=');
            if (line == key)
            {
                String value = ini_file.readStringUntil('\n');
                SPIFFS.end();
                Serial.println("Found Right Key: "+key);
                Serial.println("Value: "+ value);
                return value;
            }
            else {
                // Read until end of line but throw away value
                String value = ini_file.readStringUntil('\n');
            }
        }
    SPIFFS.end();
}

void connect_to_wifi(void)
{
  Serial.println("Connecting to WiFi");
  String ssid = ReadIniFile("WIFI_SSID");
  String pass = ReadIniFile("WIFI_PASS");
  bool network_found = false;

  Serial.println("** Scanning Networks **");
  int numOfSsids = WiFi.scanNetworks();
  for (int c=0;c<numOfSsids;c++){
      Serial.println("- "+WiFi.SSID(c));
      if (WiFi.SSID(c) == ssid) {
          network_found = true;
          WiFi.begin(ssid, pass);
          while (WiFi.status() != WL_CONNECTED){
              Serial.println("* Connecting to " + ssid);
              delay(500);
          }
      }
  }
  blink(3, 100);
  if (WiFi.status() == WL_CONNECTED) {
      SendToServer("Connected to WiFi!");
  }
  else {
      Serial.println("Failed to setup WiFi connection");
      if (network_found != true) {
          Serial.println("Could not find network: "+ssid);
      }
  }

}

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(YELLOW_D2_TO_RTC_RST, OUTPUT);
    pinMode(ORANGE_D4_TO_RTC_DAT, INPUT);
    pinMode(GREEN__D5_TO_RTC_CLK, INPUT);

    blink(10, 100);
    digitalWrite(LED_BUILTIN, LOW);

    digitalWrite(GREEN__D5_TO_RTC_CLK, HIGH);
    delay(500);
    digitalWrite(GREEN__D5_TO_RTC_CLK, LOW);


    server_ip = ReadIniFile("SERVER_IP");
    server_port = ReadIniFile("SERVER_PORT");
    connect_to_wifi();

    Serial.println(__DATE__);
    Serial.println(__TIME__);
    RtcDateTime compiled_datetime = RtcDateTime(__DATE__, __TIME__);
    Rtc.Begin();

}

void loop() {

    blink(5, 500);

    if (WiFi.status() != WL_CONNECTED)
    {
        connect_to_wifi();
        digitalWrite(LED_BUILTIN, LOW);
    }
    else {
        SendToServer("WL_CONNECTED with IP: " + IPAddressToString(WiFi.localIP()));
        RtcDateTime read_date = Rtc.GetDateTime();
        SendToServer((String) getDateTime(read_date));
        Serial.println(WiFi.localIP());
    }
    Serial.println();
    delay(5000);
}

