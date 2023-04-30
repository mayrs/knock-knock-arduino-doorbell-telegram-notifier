/**
   # Knock Knock – An Arduino doorbell Telegram notifier

   Please find an in-depth setup in the projects README.md.


   # Setup credentials

   1. Either rename `passwords_template.h` to `passwords.h` or copy its content to a new file named `passwords.h` in the same folder.
   2. Enter your WiFi and Telegram credentials in `password.h`.


   # Adjust settings

   The system can be fine tuned according to your needs by adjusting the constants in `settings.h`.


   # Flashing the code

   1. Use the board `LOLIN(WEMOS) D1 R2 & mini` for flashing.
   2. Connect to the board via USB (not to the battery shield USB that is routed to the outside of the enclosure!).
   3. Hit `Upload` in the Arduino IDE.


   # Read more


   ## HTTP over TLS (HTTPS)

   https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/HTTPSRequest/HTTPSRequest.ino


   ## Telegram HTTPS API

   https://core.telegram.org/mtproto/transports#https
   https://sarafian.github.io/low-code/2020/03/24/create-private-telegram-chatbot.html


   ## Deep sleep

   https://randomnerdtutorials.com/esp8266-deep-sleep-with-arduino-ide/
*/
#include "passwords.h"
#include "settings.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define SOUND_SENSOR_PIN A0

const char* TELEGRAM_API_HOST = "api.telegram.org";

// https://apps.timwhitlock.info/emoji/tables/unicode
const char EAR_EMOJI[] PROGMEM = "\xF0\x9F\x91\x82";
/**
  How to create an array of strings in PROGMEM
  https://stackoverflow.com/a/14325519
  https://www.arduino.cc/reference/en/language/variables/utilities/progmem/#_example_code
  https://www.nongnu.org/avr-libc/user-manual/pgmspace.html
*/
const char KNOCK_KNOCK[] PROGMEM = "Knock Knock";
char messagesBuffer[10]; // make sure this is large enough for the largest string it must hold
const char WEARY_FACE_EMOJI[] PROGMEM = "\xF0\x9F\x98\xA9";
const char SLEEPING_SYMBOL_EMOJI[] PROGMEM = "\xF0\x9F\x92\xA4";

const char* const MESSAGES[] PROGMEM = {
  EAR_EMOJI,
  KNOCK_KNOCK,
  WEARY_FACE_EMOJI,
  SLEEPING_SYMBOL_EMOJI,
};

const int HTTPS_PORT = 443;

bool isDeepSleepAnnounced = false;
unsigned long lastMillisecondsSinceStartOfProgrammDeepSleepAnnouncement = 0;
unsigned long lastMillisecondsSinceStartOfProgrammDeepSleep = 0;
unsigned long millisecondsSinceStartOfProgrammOfLastNotification = 0;

bool isAllowedToNotify = true;
int notificationImportanceLevel = 0;

void setup() {
  setupSerialConnection();
  serialPrintln("[i] Waking up Knock Knock");
  setupWemosD1MiniOnboardLed();
  setupSoundSensor();
  notifyProjectIsRunning();
}

void loop() {
  unsigned long millisecondsSinceStartOfProgramm = millis();

  bool isDeepSleepAnnouncementRequired = NOTIFY_UPCOMING_DEEP_SLEEP == true &&
                                         (millisecondsSinceStartOfProgramm - lastMillisecondsSinceStartOfProgrammDeepSleepAnnouncement >= RUNTIME_BEFORE_UPCOMING_DEEP_SLEEP_NOTIFICATION_IN_MILLISECONDS);
  if (isDeepSleepAnnounced == false && isDeepSleepAnnouncementRequired == true) {
    lastMillisecondsSinceStartOfProgrammDeepSleepAnnouncement = millisecondsSinceStartOfProgramm;
    loadWearyFaceEmojiIntoBuffer();
    setupWiFi();
    bool success = notifyViaTelegramBot(messagesBuffer, SILENTLY_NOTIFY_UPCOMING_DEEP_SLEEP);
    if (success == true) {
      isDeepSleepAnnounced = true;
    }
    teardownWiFi();
  }

  bool isDeepSleepRequired = millisecondsSinceStartOfProgramm - lastMillisecondsSinceStartOfProgrammDeepSleep >= RUNTIME_BEFORE_DEEP_SLEEP_IN_MILLISECONDS;
  if (isDeepSleepRequired == true) {
    if (NOTIFY_DEEP_SLEEP == true) {
      loadSleepingSymbolEmojiIntoBuffer();
      setupWiFi();
      notifyViaTelegramBot(messagesBuffer, SILENTLY_NOTIFY_DEEP_SLEEP);
      teardownWiFi();
    }
    enterDeepSleep();
  }

  isAllowedToNotify = millisecondsSinceStartOfProgrammOfLastNotification == 0
                      || millisecondsSinceStartOfProgramm - millisecondsSinceStartOfProgrammOfLastNotification > NOTIFICATION_LOCK_IN_MILLISECONDS;

  bool isNotificationRequired = isDoorbellRinging();
  if (isNotificationRequired == true) {
    serialPrintln("[i] Doorbell is ringing");
  }

  if (isNotificationRequired == true && isAllowedToNotify == false) {
    notificationImportanceLevel = min(notificationImportanceLevel + 1, 3);
    return;
  }

  if (isNotificationRequired == true && isAllowedToNotify == true) {
    loadKnockKnockIntoBuffer();
    const byte characterLength = strlen(messagesBuffer) + notificationImportanceLevel + 1;
    char knockKnockBuffer[characterLength];
    strcpy(knockKnockBuffer, messagesBuffer);
    for (byte i = 0; i < notificationImportanceLevel; i++) {
      strcat(knockKnockBuffer, "!");
    }
    knockKnockBuffer[characterLength - 1] = '\0'; // zero-terminated string
    setupWiFi();
    bool success = notifyViaTelegramBot(knockKnockBuffer, SILENTLY_NOTIFY_DOORBELL_RINGING);
    teardownWiFi();
    if (success == true) {
      millisecondsSinceStartOfProgrammOfLastNotification = millisecondsSinceStartOfProgramm;
      isAllowedToNotify = false;
      notificationImportanceLevel = 0;
    }
  }
}

void serialPrint(String message) {
  if (DEBUG == true) {
    Serial.print(message);
  }
}

void serialPrintln(String message) {
  if (DEBUG == true) {
    Serial.println(message);
  }
}

void setupSerialConnection() {
  if (DEBUG == true) {
    Serial.begin(115200);
    while (!Serial);
    Serial.flush();
    serialPrintln("[✓] Setting up serial connection");
  }
}

void turnWemosD1MiniOnboardLedOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void setupWemosD1MiniOnboardLed() {
  if (DEBUG == false) {
    return;
  }
  turnWemosD1MiniOnboardLedOff();
  serialPrintln("[✓] Setting up D1 Mini NodeMcu onboard LED");
}

void setupSoundSensor() {
  pinMode(SOUND_SENSOR_PIN, INPUT);
  serialPrintln("[✓] Setting up sound sensor");
}

void notifyProjectIsRunning() {
  loadEarEmojiIntoBuffer();
  setupWiFi();
  notifyViaTelegramBot(messagesBuffer, SILENTLY_NOTIFY_PROJECT_STARTUP);
  teardownWiFi();
}

void setupWiFi() {
  serialPrint(String("[i] Connecting to SSID \"") + WIFI_SSID + "\"");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    serialPrint(".");
    delay(500);
  }
  serialPrintln("");
  serialPrintln(String("[i] MAC: ") + WiFi.macAddress());
  serialPrintln(String("[i] RSSI: ") + WiFi.RSSI());
}

void teardownWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  serialPrintln("[✓] Disconnecting WiFi");
  delay(1000);
}

void enterDeepSleep() {
  serialPrintln("[i] Entering deep sleep");
  ESP.deepSleep(0);
}

/**
  Necessary casts and dereferencing, just copy.
  https://www.arduino.cc/reference/en/language/variables/utilities/progmem/#_example_code

  ESP8266 need to use pgm_read_dword!
  https://www.esp8266.com/viewtopic.php?p=32410#p32410
*/
void loadEarEmojiIntoBuffer() {
  strcpy_P(messagesBuffer, (char*)pgm_read_dword(&MESSAGES[0]));
}

void loadKnockKnockIntoBuffer() {
  strcpy_P(messagesBuffer, (char*)pgm_read_dword(&MESSAGES[1]));
}

void loadWearyFaceEmojiIntoBuffer() {
  strcpy_P(messagesBuffer, (char*)pgm_read_dword(&MESSAGES[2]));
}

void loadSleepingSymbolEmojiIntoBuffer() {
  strcpy_P(messagesBuffer, (char*)pgm_read_dword(&MESSAGES[3]));
}

bool notifyViaTelegramBot(char* message, bool isSilentNotification) {
  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  bool isSecureClientConnected = false;
  int connectionRetries = 0;
  serialPrint(String("[i] Connecting to \"https://") + TELEGRAM_API_HOST + "\"");
  do {
    isSecureClientConnected = secureClient.connect(TELEGRAM_API_HOST, HTTPS_PORT);
    if (isSecureClientConnected == false) {
      serialPrint(".");
    }
    connectionRetries++;
    delay(500);
  } while (isSecureClientConnected == false && connectionRetries < NOTIFICATION_RETRIES);
  serialPrintln("");
  if (connectionRetries == NOTIFICATION_RETRIES) {
    serialPrintln(String("[✗] Connecting to \"https://") + TELEGRAM_API_HOST + "\"");
    return false;
  }
  serialPrintln(String("[✓] Connecting to \"https://") + TELEGRAM_API_HOST + "\"");
  const String TELEGRAM_BOT_ENDPOINT = String("/bot") + TELEGRAM_BOT_TOKEN + "/sendMessage";
  const String QUERY_PARAMETERS = String("?chat_id=") + TELEGRAM_CHAT_ID + "&text=" + urlencode(message) + "&disable_notification=" + isSilentNotification;
  const String REQUEST = String("GET ") + TELEGRAM_BOT_ENDPOINT + QUERY_PARAMETERS + " HTTP/1.1\r\n" + "Host: " + TELEGRAM_API_HOST + "\r\n" + "Connection: close\r\n\r\n";
  secureClient.print(REQUEST);
  secureClient.stop();
  serialPrintln(String("[✓] Sending message \"") + message + "\"");
  return true;
}

double sampleSoundPeak() {
  double startMillis = millis();
  int signalMax = 0;
  int signalMin = 1024;
  int sample;

  while ((millis() - startMillis) < SAMPLE_WINDOW_WIDTH_IN_MILLISECONDS) {
    sample = analogRead(SOUND_SENSOR_PIN);
    if (sample < 1024) {
      if (sample > signalMax) {
        signalMax = sample;
      } else if (sample < signalMin) {
        signalMin = sample;
      }
    }
  }
  int peakDifference = signalMax - signalMin;
  return peakDifference;
}

bool isDoorbellRinging() {
  double soundSensed = sampleSoundPeak();
  return soundSensed > SOUND_THRESHOLD;
}

// https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino
String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}
