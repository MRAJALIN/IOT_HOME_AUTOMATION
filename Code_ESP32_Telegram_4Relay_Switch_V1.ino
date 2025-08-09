 /**********************************************************************************
 *  TITLE: Telegram + Manual (Switch) control 4 Relays using ESP32 with Real time feedback
 *  Click on the following links to learn more. 
 *  YouTube Video: https://youtu.be/e4mtwrwoLnU
 *  Related Blog : https://iotcircuithub.com/esp32-projects/
 *  
 *  This code is provided free for project purpose and fair use only.
 *  Please do mail us to techstudycell@gmail.com if you want to use it commercially.
 *  Copyrighted © by Tech StudyCell
 *  
 *  Preferences--> Aditional boards Manager URLs : 
 *  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *  
 *  Download Board ESP32 (3.2.0) : https://github.com/espressif/arduino-esp32
 *
 *  Download the libraries 
 *  UniversalTelegramBot Library (1.3.0):  https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
 *  AceButton Library (1.10.1): hhttps://github.com/bxparks/AceButton
 **********************************************************************************/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <AceButton.h>

#define LED_BUILTIN 2

using namespace ace_button;

// Wi-Fi and Telegram Credentials
const char* ssid = "Airtel_raja_2060";  //wifi name
const char* password = "air30349";  //Wifi password
const char* botToken = "8302149504:AAGer7qxSCQ96eapPkHGx1rYL171XKo7ngM"; // Replace with your Token ID
const long chatID = 1874156167;  // Replace with your chat ID

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// Relay and Switch Pins
const int relayPins[4] = {23, 22, 21, 19};   // Active LOW relay pins
const int switchPins[4] = {13, 12, 14, 27};  // Latching switches pins

bool relayStates[4] = {false, false, false, false}; // Stores relay states

// AceButton setup
ButtonConfig config1;
AceButton button1(&config1);
ButtonConfig config2;
AceButton button2(&config2);
ButtonConfig config3;
AceButton button3(&config3);
ButtonConfig config4;
AceButton button4(&config4);

void handleEvent1(AceButton*, uint8_t, uint8_t);
void handleEvent2(AceButton*, uint8_t, uint8_t);
void handleEvent3(AceButton*, uint8_t, uint8_t);
void handleEvent4(AceButton*, uint8_t, uint8_t);

void handleNewMessages(int);

void setup() {
    Serial.begin(115200);
    
    // Initialize Relays (Active LOW)
    for (int i = 0; i < 4; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], HIGH); // Ensure relays are off at startup
        pinMode(switchPins[i], INPUT_PULLUP);
    }
    
    pinMode(LED_BUILTIN, OUTPUT); // LED to indicate Wi-Fi connection

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on LED when connected

    client.setInsecure();  // Skip SSL verification for Telegram

    // Initialize AceButton
    config1.setEventHandler(button1Handler);
    config2.setEventHandler(button2Handler);
    config3.setEventHandler(button3Handler);
    config4.setEventHandler(button4Handler);

    button1.init(switchPins[0]);
    button2.init(switchPins[1]);
    button3.init(switchPins[2]);
    button4.init(switchPins[3]);

    delay(200);
}

// Handle Telegram Messages
void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        String chat_id = bot.messages[i].chat_id;
        if (chat_id.toInt() != chatID) continue; // Ignore messages from others

        String text = bot.messages[i].text;
        Serial.println("Received command: " + text);

        if (text == "/relay1_on") {
            relayStates[0] = true;
            digitalWrite(relayPins[0], LOW);
            bot.sendMessage(chat_id, "Relay 1 is ON", "");
        } else if (text == "/relay1_off") {
            relayStates[0] = false;
            digitalWrite(relayPins[0], HIGH);
            bot.sendMessage(chat_id, "Relay 1 is OFF", "");
        } else if (text == "/relay2_on") {
            relayStates[1] = true;
            digitalWrite(relayPins[1], LOW);
            bot.sendMessage(chat_id, "Relay 2 is ON", "");
        } else if (text == "/relay2_off") {
            relayStates[1] = false;
            digitalWrite(relayPins[1], HIGH);
            bot.sendMessage(chat_id, "Relay 2 is OFF", "");
        } else if (text == "/relay3_on") {
            relayStates[2] = true;
            digitalWrite(relayPins[2], LOW);
            bot.sendMessage(chat_id, "Relay 3 is ON", "");
        } else if (text == "/relay3_off") {
            relayStates[2] = false;
            digitalWrite(relayPins[2], HIGH);
            bot.sendMessage(chat_id, "Relay 3 is OFF", "");
        } else if (text == "/relay4_on") {
            relayStates[3] = true;
            digitalWrite(relayPins[3], LOW);
            bot.sendMessage(chat_id, "Relay 4 is ON", "");
        } else if (text == "/relay4_off") {
            relayStates[3] = false;
            digitalWrite(relayPins[3], HIGH);
            bot.sendMessage(chat_id, "Relay 4 is OFF", "");
        } else if (text == "/relay_state") {
            String statusMsg = "Relay Status:\n";
            for (int j = 0; j < 4; j++) {
                statusMsg += "Relay " + String(j + 1) + ": " + (relayStates[j] ? "ON\n" : "OFF\n");
            }
            bot.sendMessage(chat_id, statusMsg, "");
        } else {
            bot.sendMessage(chat_id, "Valid commands:\n/relay1_on, /relay1_off\n/relay2_on, /relay2_off\n/relay3_on, /relay3_off\n/relay4_on, /relay4_off\n/relay_state", "");
        }
    }
}

void loop() {
    //Control Switches Manualy
    button1.check();
    button2.check();
    button3.check();
    button4.check();

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
        handleNewMessages(numNewMessages);
    }
    delay(100);
}

// Handle AceButton Events
void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT1");
  switch (eventType) {
    case AceButton::kEventPressed:
      relayStates[0] = true;
      digitalWrite(relayPins[0], LOW);
      break;
    case AceButton::kEventReleased:
      relayStates[0] = false;
      digitalWrite(relayPins[0], HIGH);
      break;
  }
}

void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT2");
  switch (eventType) {
    case AceButton::kEventPressed:
      relayStates[1] = true;
      digitalWrite(relayPins[1], LOW);
      break;
    case AceButton::kEventReleased:
      relayStates[1] = false;
      digitalWrite(relayPins[1], HIGH);
      break;
  }
}

void button3Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT3");
  switch (eventType) {
    case AceButton::kEventPressed:
      relayStates[2] = true;
      digitalWrite(relayPins[2], LOW);
      break;
    case AceButton::kEventReleased:
      relayStates[2] = false;
      digitalWrite(relayPins[2], HIGH);
      break;
  }
}

void button4Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT4");
  switch (eventType) {
    case AceButton::kEventPressed:
      relayStates[3] = true;
      digitalWrite(relayPins[3], LOW);
      break;
    case AceButton::kEventReleased:
      relayStates[3] = false;
      digitalWrite(relayPins[3], HIGH);
      break;
  }
}
