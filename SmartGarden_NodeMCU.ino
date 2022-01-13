#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SoftwareSerial.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

////////////////////  Wifi, API, RTB URL, AUTH  ////////////////////
#define WIFI_SSID "DISINI_SSID_WIFI"
#define WIFI_PASSWORD "DISINI_PASSWORD_WIFI"
#define API_KEY "AIzaSyAwDN-6BkK77zar11XQZ0QySf-yFdiBPw4"
#define DATABASE_URL "hidroponik-5f127-default-rtdb.asia-southeast1.firebasedatabase.app"
#define DATABASE_SECRET "7PI189U37zaPi0MZu79E0yCupArGTa9LF9NSJ9PW"

////////////////////   FIREBASE DATA OBJECT    ////////////////////
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

////////////////////        SERIAL COMM        ////////////////////
SoftwareSerial megaSerial(12, 13); // RX(D6/GPIO12/MISO), TX(D7/MOSI/GPIO13)
String datasend;

////////////////////       PARSING DATA        ////////////////////
unsigned long previousMillis = 0;
String minta = "ya";
String arrData[7];
String sptds;

////////////////////       VAR DATA        ////////////////////
String ppm_value, ph_value, suhu_value, pom_ab, pom_air, pom_phup, pom_phd;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  megaSerial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setMaxRetry(fbdo, 3);
  Firebase.setMaxErrorQueue(fbdo, 30);
  fbdo.setBSSLBufferSize(512, 2048);
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  if (Firebase.ready() && (millis() - previousMillis > 2000 || previousMillis == 0)) {
    previousMillis = millis();
    String data = "";
    while (megaSerial.available() > 0) {
      data += char (megaSerial.read());
    }
    data.trim();
    if (data != "") {
      int index = 0;
      for (int i = 0 ; i <= data.length(); i++) {
        char delimiter = '#' ;
        if (data[i] != delimiter)
          arrData [index] += data[i];
        else
          index++;
      }
      if (index == 6) {
        ppm_value   = arrData[0];
        ph_value    = arrData[1];
        suhu_value  = arrData[2];
        pom_ab      = arrData[3];
        pom_air     = arrData[4];
        pom_phup    = arrData[5];
        pom_phd     = arrData[6];
        Firebase.setStringAsync(fbdo, "/ppm_value", ppm_value);
        Firebase.setStringAsync(fbdo, "/ph_value", ph_value);
        Firebase.setStringAsync(fbdo, "/suhu_value", suhu_value);
        Firebase.setStringAsync(fbdo, "/pom_abmix", pom_ab);
        Firebase.setStringAsync(fbdo, "/pom_air", pom_air);
        Firebase.setStringAsync(fbdo, "/pom_phup", pom_phup);
        Firebase.setStringAsync(fbdo, "/pom_phd", pom_phd);
      }
      arrData[0] = "";
      arrData[1] = "";
      arrData[2] = "";
      arrData[3] = "";
      arrData[4] = "";
      arrData[5] = "";
      arrData[6] = "";
    }
    sptds = Firebase.getString(fbdo, "/setpointTDS") ? fbdo.to<const char *>() : fbdo.errorReason().c_str();
    String datasend = String(minta) + '#' + String(sptds);
    megaSerial.println(datasend);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
