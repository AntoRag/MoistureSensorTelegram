#include <Wire.h>;
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string>
// Wifi network station credentials
#define WIFI_SSID "YOUR_SSD"
#define WIFI_PASSWORD "YOUR_PASS"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "YOUR_TOKEN"
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define A0 0
#define DryValue  685   
#define WetValue  445 

int sda_pin = D2;  // pin to connect to SDA pin on LCD (can be any data pin)
int scl_pin = D1;  // pin to connect to SCL pin on LCD (can be any data pin)
int moisture_analog_pin = A0; // Soil Moisture Sensor input at Analog PIN A0
int button_pin = D5;
int moisture_value= 0;
int moisture_value_prev = 0;
int moisture_value_perc = 0;
int buzz_pin = D7;
const unsigned long BOT_MTBS = 500; // mean time between scan messages
bool forced_stop = false;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

LiquidCrystal_I2C lcd(0x27, 20, 4);  // start up LCD as 20 characters X 4 lines

void UpdateValueMoisturePerc(){
  moisture_value = analogRead(moisture_analog_pin);
  moisture_value_perc = map(moisture_value,WetValue,DryValue,100,0);
}

void CheckNewMessages(){
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
    Serial.println(bot_lasttime);
  }
}

void handleNewMessages(int numNewMessages)
{

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
 
    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/level")
    {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Updating Telegram");
      char message[30];
      sprintf(message, "Moisture level: %d%%", moisture_value_perc);
      bot.sendMessage(chat_id=chat_id, text=message);
      delay(3000);
    }
 
    if (text == "/start")
    {
      String welcome = "Welcome to moisture sensor bot, " + from_name + ".\n";
      welcome += "This is a bot to track the moisture level in your plant(s).\n\n";
      welcome += "/level : to get the reading of the sensor\n";
      welcome += "/stopalarm : to stop the buzzer\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }

    if (text == "/stopalarm"){
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Stopping the alarm");
      forced_stop = true;
      digitalWrite(buzz_pin, LOW);
      delay(3000);
    }
    lcd.clear();
  }
}

void setup() {
  Wire.begin(sda_pin, scl_pin);
  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to Wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    lcd.print(".");
    delay(500);
  }
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("WiFi connected.");
  delay(2000);
  lcd.clear();
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  lcd.backlight();
  pinMode(buzz_pin, OUTPUT);
  pinMode(button_pin,INPUT);
}

void loop() {

  
  CheckNewMessages();

  UpdateValueMoisturePerc();
  
  if (moisture_value_perc!=moisture_value_prev){
    moisture_value_prev = moisture_value_perc;
    lcd.clear();
    lcd.home();
    lcd.setCursor(3, 0);
    lcd.print("Moisture sensor");
    lcd.setCursor(0, 1);  // set cursor to 2nd position on line 2
    lcd.print("Moisture level:");
    lcd.setCursor(16, 1);
    lcd.print((int)moisture_value_perc);
     lcd.print("%");
    if (moisture_value_perc < 20 && forced_stop!= true){
      digitalWrite(buzz_pin, HIGH);
    }
    else if (moisture_value_perc >= 20) {
      digitalWrite(buzz_pin, LOW);
      forced_stop = false;
    }
  }

}
