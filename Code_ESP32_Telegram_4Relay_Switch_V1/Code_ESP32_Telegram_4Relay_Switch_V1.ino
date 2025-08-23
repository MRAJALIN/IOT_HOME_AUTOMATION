 /**********************************************************************************
 *  TITLE: Telegram + Manual (Switch) + Local server control 4 Relays using ESP32 with Real time feedback
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
#include <WebServer.h>  // <-- New
#include <UniversalTelegramBot.h>
#include <AceButton.h>

#define LED_BUILTIN 2

using namespace ace_button;

bool lastSwitchStates[4] = {HIGH, HIGH, HIGH, HIGH};
String relayNames[4] = {"Tank Light", "House Back Light", "Room Light", "Bathroom Light"};

// Wi-Fi Credentials (Same Router, No Internet Required for local control)
const char* ssid = "RAJALINGAM";
const char* password = "KJRkjr1994";
// unsigned long wifiReconnectInterval = 10000;  // try reconnect every 10 seconds
// unsigned long lastWifiReconnectAttempt = 0;

// Telegram Bot
const char* botToken = "8302149504:AAGer7qxSCQ96eapPkHGx1rYL171XKo7ngM";
const uint64_t allowedChatIDs[] = {1874156167, 991581343, 867468063, 8337473690, 5628705880, 8435774388};  // Add IDs here
const int totalUsers = sizeof(allowedChatIDs) / sizeof(allowedChatIDs[0]);

bool isUserAllowed(uint64_t id) {
  for (int i = 0; i < totalUsers; i++) {
    if (allowedChatIDs[i] == id) {
      return true;
    }
  }
  return false;
}

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// Local Web Server on port 80
WebServer server(80); // <-- New

// Relay and Switch Pins
const int relayPins[4] = {23, 22, 21, 19};
const int switchPins[4] = {13, 12, 14, 27};

bool relayStates[4] = {false, false, false, false};

// AceButton setup
ButtonConfig config1, config2, config3, config4;
AceButton button1(&config1), button2(&config2), button3(&config3), button4(&config4);

void button1Handler(AceButton*, uint8_t, uint8_t);
void button2Handler(AceButton*, uint8_t, uint8_t);
void button3Handler(AceButton*, uint8_t, uint8_t);
void button4Handler(AceButton*, uint8_t, uint8_t);

void handleNewMessages(int numNewMessages);

// -------------------- LOCAL WEB UI --------------------
String htmlPage() {
  // String relayNames[4] = {"Tank Light", "House Back Light", "Room Light", "Bathroom Light"};

  String html = "<!DOCTYPE html><html><head><title>ESP32 Relay Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; text-align: center; background: linear-gradient(135deg, #f9f9f9, #d7e1ec); }";
  html += "h1 { background-color: #333; color: white; padding: 15px; margin: 0; font-size: 22px; }";
  html += ".container { display: flex; flex-direction: column; align-items: center; padding: 10px; }";
  html += ".card { background: white; box-shadow: 0 4px 8px rgba(0,0,0,0.1); border-radius: 10px; padding: 15px; margin: 10px; width: 90%; max-width: 350px; }";
  html += ".status { font-size: 18px; margin-bottom: 10px; font-weight: bold; }";
  html += "button { padding: 10px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; width: 100%; }";
  html += ".on { background-color: #4CAF50; color: white; }"; // Green ON
  html += ".off { background-color: #f44336; color: white; }"; // Red OFF
  html += ".btn-group { display: flex; gap: 10px; }"; // space between buttons
  html += ".btn-group a { flex: 1; }"; // make links equally wide
  html += "@media (max-width: 500px) { button { font-size: 14px; padding: 8px; } }";
  html += "</style></head><body>";

  html += "<h1>VASANTHAMANI HOME AUTOMATION</h1>";
  html += "<div class='container'>";
  html += "<div class='card'>";
  html += "<div class='status' style='color:blue;'>Control All Lights</div>";
  html += "<div class='btn-group'>";
  html += "<a href='/All_ON'><button class='on'>ALL ON</button></a>";
  html += "<a href='/All_OFF'><button class='off'>ALL OFF</button></a>";
  html += "</div></div>";

  for (int i = 0; i < 4; i++) {
    String urlName = relayNames[i];
    urlName.replace(" ", "_"); // modify in place

    html += "<div class='card'>";
    html += "<div class='status' style='color:" + String(relayStates[i] ? "green" : "red") + ";'>" +
            relayNames[i] + ": " + (relayStates[i] ? "ON" : "OFF") + "</div>";
    html += "<div class='btn-group'>";
    html += "<a href='/" + urlName + "_ON'><button class='on'>ON</button></a>";
    html += "<a href='/" + urlName + "_OFF'><button class='off'>OFF</button></a>";
    html += "</div></div>";
  }

  html += "</div></body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void relayControl(int relayNum, bool state) {
  relayStates[relayNum] = state;
  digitalWrite(relayPins[relayNum], state ? LOW : HIGH);
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);

  // Relay setup
  for (int i = 0; i < 4; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);   // ON/OFF switch
    pinMode(relayPins[i], OUTPUT);

    // Step 1: Force relays OFF immediately at boot (no flicker)
    digitalWrite(relayPins[i], HIGH);   // HIGH = OFF for active LOW relay modules
    relayStates[i] = false;
  }

  // Small delay to stabilize pins
  delay(50);

  // Step 2: Now read physical switch state and update relays properly
  for (int i = 0; i < 4; i++) {
    bool switchState = (digitalRead(switchPins[i]) == LOW); // LOW = switch pressed (ON)
    relayStates[i] = switchState;
    digitalWrite(relayPins[i], switchState ? LOW : HIGH);   // apply real state
  }

  pinMode(LED_BUILTIN, OUTPUT);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected to WiFi: " + WiFi.localIP().toString());
  digitalWrite(LED_BUILTIN, HIGH);
  client.setInsecure();
  String ipMsg = "Running in IP Address: " + WiFi.localIP().toString();
for (int i = 0; i < totalUsers; i++) {
    char chatIdStr[21]; // Enough for 64-bit integer + null terminator
    sprintf(chatIdStr, "%llu", (unsigned long long)allowedChatIDs[i]);
    bot.sendMessage(chatIdStr, ipMsg);
    // Menu option
    String menuMsg = "🏠 *Home Automation Menu*\n";
    menuMsg += "📶 IP Address: " + WiFi.localIP().toString() + "\n\n";
    menuMsg += "/Tank_Light_ON  | /Tank_Light_OFF\n";
    menuMsg += "/House_Back_Light_ON  | /House_Back_Light_OFF\n";
    menuMsg += "/Room_Light_ON  | /Room_Light_OFF\n";
    menuMsg += "/Bathroom_Light_ON  | /Bathroom_Light_OFF\n\n";
    menuMsg += "/All_ON  | /All_OFF\n";
    menuMsg += "/Show_All_Light_Status\n";
    bot.sendMessage(chatIdStr, menuMsg);
}


  // Button handlers
  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  config3.setEventHandler(button3Handler);
  config4.setEventHandler(button4Handler);

  button1.init(switchPins[0]);

  button2.init(switchPins[1]);
  button3.init(switchPins[2]);
  button4.init(switchPins[3]);

  // Local Web Server routes
server.on("/", handleRoot);

// String relayNames[4] = {"Tank Light", "House Back Light", "Room Light", "Bathroom Light"};
for (int i = 0; i < 4; i++) {
  String urlName = relayNames[i];
  urlName.replace(" ", "_"); // Same formatting as in HTML

  server.on(("/" + urlName + "_ON").c_str(), [i]() {
    relayControl(i, true);
    server.send(200, "text/html", htmlPage());
  });
  server.on(("/" + urlName + "_OFF").c_str(), [i]() {
    relayControl(i, false);
    server.send(200, "text/html", htmlPage());
  });
  // Route for All Lights ON
  server.on("/All_ON", []() {
    for (int i = 0; i < 4; i++) {
      relayControl(i, true);
      delay(500); // delay between each relay ON
    }
    server.send(200, "text/html", htmlPage());
  });

  // Route for All Lights OFF
  server.on("/All_OFF", []() {
    for (int i = 0; i < 4; i++) {
      relayControl(i, false);
      delay(500); // delay between each relay OFF
    }
    server.send(200, "text/html", htmlPage());
  });
}

  server.begin();
}


// void ensureWiFiConnected() {
//   if (WiFi.status() == WL_CONNECTED) {
//     // Already connected, nothing to do
//     return;
//   }

//   // Check if it's time to try reconnecting again
//   if (millis() - lastWifiReconnectAttempt >= wifiReconnectInterval) {
//     lastWifiReconnectAttempt = millis();
//     Serial.println("WiFi disconnected! Attempting reconnect...");

//     WiFi.disconnect(true);    // fully reset WiFi
//     WiFi.mode(WIFI_STA);      // station mode
//     WiFi.begin(ssid, password);
//   }
// }

// -------------------- LOOP --------------------
void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) { // poll Telegram when online
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) handleNewMessages(numNewMessages);
  }

  // Check switch state changes
  for (int i = 0; i < 4; i++) {
    bool currentState = digitalRead(switchPins[i]);

    if (currentState != lastSwitchStates[i]) {
      lastSwitchStates[i] = currentState;

      // Only act when switch changes
      if (currentState == LOW) {  
        relayControl(i, !relayStates[i]); // toggle relay state
      }
    }
  }
}
// -------------------- TELEGRAM --------------------
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    uint64_t chat_id_num = strtoull(chat_id.c_str(), NULL, 10);

    Serial.println("Message from Chat ID: " + chat_id);

    if (!isUserAllowed(chat_id_num)) {
      bot.sendMessage(chat_id, "❌ You are not authorized to control this bot.");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println("Received command: " + text);

    if (text == "/Tank_Light_ON") {
      relayControl(0, true);
      bot.sendMessage(chat_id, "Tank Light turned ON\n\n/Show_Menu", "");
    } else if (text == "/Tank_Light_OFF") {
      relayControl(0, false);
      bot.sendMessage(chat_id, "Tank Light turned OFF\n\n/Show_Menu", "");
    } else if (text == "/House_Back_Light_ON") {
      relayControl(1, true);
      bot.sendMessage(chat_id, "House Back Light Turned ON\n\n/Show_Menu", "");
    } else if (text == "/House_Back_Light_OFF") {
      relayControl(1, false);
      bot.sendMessage(chat_id, "House Back Light Turned OFF\n\n/Show_Menu", "");
    } else if (text == "/Room_Light_ON") {
      relayControl(2, true);
      bot.sendMessage(chat_id, "Room Light Turned ON\n\n/Show_Menu", "");
    } else if (text == "/Room_Light_OFF") {
      relayControl(2, false);
      bot.sendMessage(chat_id, "Room Light Turned OFF\n\n/Show_Menu", "");
    } else if (text == "/Bathroom_Light_ON") {
      relayControl(3, true);
      bot.sendMessage(chat_id, "Bathroom Light Turned ON\n\n/Show_Menu", "");
    } else if (text == "/Bathroom_Light_OFF") {
      relayControl(3, false);
      bot.sendMessage(chat_id, "Bathroom Light Turned OFF\n\n/Show_Menu", "");
    } else if (text == "/Show_All_Light_Status") {
      String statusMsg = "Relay Status:\n";
      for (int j = 0; j < 4; j++) {
        statusMsg += String(relayNames[j]) + ": " + (relayStates[j] ? "ON\n" : "OFF\n");
      }
      bot.sendMessage(chat_id, statusMsg, "");
    } else if (text == "/All_ON") {
      for (int j = 0; j < 4; j++) {
        relayControl(j, true);
        delay(300);
      }
      bot.sendMessage(chat_id, "✅ All Lights Turned ON\n\n/Show_Menu", "");
    } else if (text == "/All_OFF") {
      for (int j = 0; j < 4; j++) {
        relayControl(j, false);
        delay(300);
      }
      bot.sendMessage(chat_id, "✅ All Lights Turned OFF\n\n/Show_Menu", "");
    } else if (text == "SUJA") {
      bot.sendMessage(chat_id, "♻️ Restarting Device...");
      delay(1000);
      bot.getUpdates(bot.last_message_received + 1); 
      ESP.restart();
    } else {
      // Default menu
      String menuMsg = "🏠 *Home Automation Menu*\n";
      menuMsg += "📶 IP Address: " + WiFi.localIP().toString() + "\n\n";
      menuMsg += "/Tank_Light_ON  | /Tank_Light_OFF\n";
      menuMsg += "/House_Back_Light_ON  | /House_Back_Light_OFF\n";
      menuMsg += "/Room_Light_ON  | /Room_Light_OFF\n";
      menuMsg += "/Bathroom_Light_ON  | /Bathroom_Light_OFF\n\n";
      menuMsg += "/All_ON  | /All_OFF\n";
      menuMsg += "/Show_All_Light_Status\n";
      bot.sendMessage(chat_id, menuMsg);
    }
  }
}

// -------------------- BUTTON EVENTS --------------------
void button1Handler(AceButton*, uint8_t eventType, uint8_t) {
  if (eventType == AceButton::kEventPressed) relayControl(0, !relayStates[0]);
}
void button2Handler(AceButton*, uint8_t eventType, uint8_t) {
  if (eventType == AceButton::kEventPressed) relayControl(1, !relayStates[1]);
}
void button3Handler(AceButton*, uint8_t eventType, uint8_t) {
  if (eventType == AceButton::kEventPressed) relayControl(2, !relayStates[2]);
}
void button4Handler(AceButton*, uint8_t eventType, uint8_t) {
  if (eventType == AceButton::kEventPressed) relayControl(3, !relayStates[3]);
}
