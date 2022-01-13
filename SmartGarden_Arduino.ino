#include <Arduino.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <GravityTDS.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <SoftwareSerial.h>

////////////////////  LCD 20x04 I2C  ////////////////////
LiquidCrystal_I2C lcd(0x27, 20, 4);

////////////////////  DS18B20 Sensor  ////////////////////
#define ONE_WIRE_BUS A7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor_suhu(&oneWire);
float tempNow;
float suhuRaw = 0;
float suhuKal = 0;
float suhuRawTinggi = 99.6;
float suhuRawRendah = 0.5;
float refSuhuTinggi = 99.8;
float refSuhuRendah = 0;
float jarakRaw = suhuRawTinggi - suhuRawRendah;
float jarakRef = refSuhuTinggi - refSuhuRendah;

////////////////////  pH Sensor  ////////////////////
#define PHpin A0
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
float total = 0;
float offsetPH = 0.170568562;
float adc_resolution = 1023.00;
float phValue, voltage, phNow;

////////////////////  TDS METER  ////////////////////
#define TDSPin A1
GravityTDS gravityTds;
float temperature = 25, tdsNow, tdsValue = 0;

////////////////////  PUMP  ////////////////////
int ABMix = 24;
String stAbMix;
int phUp = 26;
String stphUp;
int phD = 28;
String stphD;
int Air = 30;
String stAir;
int kipas = 32;

////////////////////  DATA SEND  ////////////////////
SoftwareSerial nodeMCU(19, 18); // RX, TX
String data;
unsigned long previousMillis = 0;
const long interval = 500;
const int ledPin = LED_BUILTIN;
int ledState = LOW;
unsigned long currentMillis = millis();
String arrData[2];

////////////////////  TDS FUZZY  ////////////////////
float eTDS, dTDS, edeTDS, E, D, delayTDS;
float erN[] = { -100, -100, -50, -10};
float erZ[] = { -50, -10, 10, 50};
float erP[] = {10, 50, 100, 100};
float deN[] = { -50, -50, -25, -5};
float deZ[] = { -25, -5, 5, 25};
float deP[] = {5, 25, 50, 50};
float setpointTDS = 200;
float upLAMA = 1000;
float upSEDANG = 500;
float MATI = 0;
float dnSEDANG = -500;
float dnLAMA = -1000;
float minrTDS[10];
float RuleTDS[10];

////////////////////  PH FUZZY  ////////////////////
float ePH, P, H, delayPH;
float SA[] = {4, 4, 5, 6};
float N[] = {5, 6, 7, 8};
float SB[] = {7, 8, 9, 9};
float upHIDUP = 1000;
float dnHIDUP = -1000;
float minrPH[4];
float RulePH[4];

void lcd_setup() {
	lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(F("HIDROPONIK"));
  lcd.setCursor(2, 1);
  lcd.print(F("SMART GARDEN IoT"));
  delay(2000);
  lcd.clear();
}

void mon_setup() {
  Serial.println(F("HIDROPONIK"));
  Serial.print(F("SMART GARDEN IoT\n\n"));
  Serial.print(F("Nama\t: Reynaldo\n"));
  Serial.print(F("NIM\t: 171011450241\n\n"));
  delay(2000);
}

void pinInit() {
  pinMode(ledPin, OUTPUT);
  pinMode(kipas, OUTPUT);
  pinMode(ABMix, OUTPUT);
  pinMode(phUp, OUTPUT);
  pinMode(phD, OUTPUT);
  pinMode(Air, OUTPUT);
  pinMode(TDSPin, INPUT);
  pinMode(PHpin, INPUT);
  digitalWrite(kipas, HIGH);
  digitalWrite(ABMix, LOW);
  digitalWrite(phUp, LOW);
  digitalWrite(phD, LOW);
  digitalWrite(Air, LOW);
}

void tdsSetup() {
  gravityTds.setPin(TDSPin);
  gravityTds.setAref(5.0);
  gravityTds.setAdcRange(1024);
  gravityTds.begin();
}

void phRead() {
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

float phRaw() {
  int measurings = 0;
  for (int i = 0; i < 10; i++) {
    measurings += analogRead(PHpin);
    delay(10);
  }
  voltage = 5 / adc_resolution * measurings / 10;
  phValue = 7 + ((2.5 - voltage) / offsetPH);
  return phValue;
}

float getPh() {
  phRaw();
  total = total - readings[readIndex];
  readings[readIndex] = phValue;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  phNow = total / numReadings;
  delay(10);
}

float getSuhu() {
  sensor_suhu.requestTemperatures();
  suhuRaw = sensor_suhu.getTempCByIndex(0);
  suhuKal = (((suhuRaw - suhuRawRendah) * jarakRef) / jarakRaw) + refSuhuRendah;
  return suhuKal;
}

int getMedian(int bArray[], int iFtrLen) {
  int bTab[iFtrLen];
  for (byte m = 0; m < iFtrLen; m++)
    bTab[m] = bArray[m];
  int m, n, bTemp;
  for (n = 0; n < iFtrLen - 1; n++) {
    for (m = 0; m < iFtrLen - n - 1; m++) {
      if (bTab[m] > bTab[m + 1]) {
        bTemp = bTab[m];
        bTab[m] = bTab[m + 1];
        bTab[m + 1] = bTemp;
        delay(10);
      }
    }
  }
  if ((iFtrLen & 1) > 0)
    bTemp = bTab[(iFtrLen - 1) / 2];
  else
    bTemp = (bTab[iFtrLen / 2] + bTab[iFtrLen / 2 - 1]) / 2;
  return bTemp;
  delay(10);
}

float getTds() {
  temperature = (float)25.00f;
  gravityTds.setTemperature(temperature);
  gravityTds.update();
  tdsValue = gravityTds.getTdsValue();
  return tdsValue;
  delay(1000);
}

void phDump() {
  Serial.println("INIT PH");
  for (int i = 0; i < 50; i++) {
    getPh();
    delay(10);
  }
  return phValue;
}

void tdsDump() {
  Serial.println("INIT TDS");
  for (int i = 0; i < 100; i++) {
    getTds();
    delay(10);
  }
  return tdsValue;
}

void suhuDump() {
  Serial.println("INIT SUHU");
  for (int i = 0; i < 50; i++) {
    getSuhu();
    delay(10);
  }
  return suhuKal;
}

void setup() {
  lcd.clear();
  Serial.begin(9600);
  Serial1.begin(9600);
  pinInit();
  sensor_suhu.begin();
  tdsSetup();
  phRead();
  phDump();
  tdsDump();
  suhuDump();
  lcd_setup();
  mon_setup();
}

void loop() {
  String minta = "";
  while (Serial1.available() > 0) {
    minta += char(Serial1.read());
  }
  minta.trim();
  if (minta != "") {
    int index = 0;
    for (int l = 0; l <= minta.length(); l++) {
      char delimiter = '#';
      if (minta[l] != delimiter)
        arrData[index] += minta[l];
      else
        index++;
    }
    if (index == 1) {
      kirimdata();
      setpointTDS = arrData[1].toFloat();
    }
    arrData[0] = "";
    arrData[1] = "";
  }
  tampil();
  getPh();
  getTds();
  getSuhu();
  hitungTDS();
  pompaTDS();
  pompaPH();
  delay(1000);
}

void hitungTDS() {
  edeTDS = eTDS;
  eTDS = setpointTDS - tdsValue;
  dTDS = eTDS - edeTDS;
}

//////////////////////////////  FUZZY TDS START HERE  //////////////////////////////
float uerN() {
  if (eTDS < erN[2]) {
    return 1;
  } else if (eTDS >= erN[2] && eTDS <= erN[3]) {
    return (erN[3] - eTDS) / (erN[3] - erN[2]);
  } else if (eTDS > erN[3]) {
    return 0;
  }
}

float uerZ() {
  if (eTDS < erZ[0]) {
    return 0;
  } else if (eTDS >= erZ[0] && eTDS <= erZ[1]) {
    return (eTDS - erZ[0]) / (erZ[1] - erZ[0]);
  } else if (eTDS >= erZ[1] && eTDS <= erZ[2]) {
    return 1;
  } else if (eTDS >= erZ[2] && eTDS <= erZ[3]) {
    return (erZ[3] - eTDS) / (erZ[3] - erZ[2]);
  } else if (eTDS > erZ[3]) {
    return 0;
  }
}

float uerP() {
  if (eTDS < erP[0]) {
    return 0;
  } else if (eTDS >= erP[0] && eTDS <= erP[1]) {
    return (eTDS - erP[0]) / (erP[1] - erP[0]);
  } else if (eTDS > erP[1]) {
    return 1;
  }
}

float udeN() {
  if (dTDS < deN[2]) {
    return 1;
  } else if (dTDS >= deN[2] && dTDS <= deN[3]) {
    return (deN[3] - dTDS) / (deN[3] - deN[2]);
  } else if (dTDS > deN[3]) {
    return 0;
  }
}

float udeZ() {
  if (dTDS < deZ[0]) {
    return 0;
  } else if (dTDS >= deZ[0] && dTDS <= deZ[1]) {
    return (dTDS - deZ[0]) / (deZ[1] - deZ[0]);
  } else if (dTDS >= deZ[1] && dTDS <= deZ[2]) {
    return 1;
  } else if (dTDS >= deZ[2] && dTDS <= deZ[3]) {
    return (deZ[3] - dTDS) / (deZ[3] - deZ[2]);
  } else if (dTDS > deZ[3]) {
    return 0;
  }
}

float udeP() {
  if (dTDS <= deP[0]) {
    return 0;
  } else if (dTDS > deP[0] && dTDS < deP[1]) {
    return (dTDS - deP[0]) / (deP[1] - deP[0]);
  } else if (dTDS >= deP[1]) {
    return 1;
  }
}

float Min(float a, float b) {
  if (a < b) {
    return a;
  } else if (b < a) {
    return b;
  } else {
    return a;
  }
}

void rulesTDS() {
  minrTDS[1] = Min(uerN(), udeN());
  RuleTDS[1] = upLAMA;
  minrTDS[2] = Min(uerN(), udeZ());
  RuleTDS[2] = upSEDANG;
  minrTDS[3] = Min(uerN(), udeP());
  RuleTDS[3] = MATI;
  minrTDS[4] = Min(uerZ(), udeN());
  RuleTDS[4] = upSEDANG;
  minrTDS[5] = Min(uerZ(), udeZ());
  RuleTDS[5] = MATI;
  minrTDS[6] = Min(uerZ(), udeP());
  RuleTDS[6] = dnSEDANG;
  minrTDS[7] = Min(uerP(), udeN());
  RuleTDS[7] = MATI;
  minrTDS[8] = Min(uerP(), udeZ());
  RuleTDS[8] = dnSEDANG;
  minrTDS[9] = Min(uerP(), udeP());
  RuleTDS[9] = dnLAMA;
}

float defuzzTDS() {
  rulesTDS();
  E = 0;
  D = 0;
  for (int i = 1; i <= 9; i++) {
    E += RuleTDS[i] * minrTDS[i];
    D += minrTDS[i];
  }
  delayTDS = E / D;
  return delayTDS;
}
//////////////////////////////  FUZZY TDS END HERE  //////////////////////////////


//////////////////////////////  FUZZY PH START HERE  //////////////////////////////
float uSA() {
  if (phNow < SA[2]) {
    return 1;
  } else if (phNow >= SA[2] && phNow <= SA[3]) {
    return (SA[3] - phNow) / (SA[3] - SA[2]);
  } else if (phNow > SA[3]) {
    return 0;
  }
}

float uN() {
  if (phNow < N[0]) {
    return 0;
  } else if (phNow >= N[0] && phNow <= N[1]) {
    return (phNow - N[0]) / (N[1] - N[0]);
  } else if (phNow >= N[1] && phNow <= N[2]) {
    return 1;
  } else if (phNow >= N[2] && phNow <= N[3]) {
    return (N[3] - phNow) / (N[3] - N[2]);
  } else if (phNow > N[3]) {
    return 0;
  }
}

float uSB() {
  if (phNow < SB[0]) {
    return 0;
  } else if (phNow >= SB[0] && phNow <= SB[1]) {
    return (phNow - SB[0]) / (SB[1] - SB[0]);
  } else if (phNow >= SB[1] && phNow <= SB[2]) {
    return 1;
  }
}

void rulesPH() {
  minrPH[1] = uSA();
  RulePH[1] = upHIDUP;
  minrPH[2] = uN();
  RulePH[2] = MATI;
  minrPH[3] = uSB();
  RulePH[3] = dnHIDUP;
}

float defuzzPH() {
  rulesPH();
  P = 0;
  H = 0;
  for (int i = 1; i <= 3; i++) {
    P += RulePH[i] * minrPH[i];
    H += minrPH[i];
  }
  delayPH = P / H;
  return delayPH;
}
//////////////////////////////  FUZZY PH END HERE  //////////////////////////////

void pompaTDS() {
  defuzzTDS();
  if (tdsValue == setpointTDS || delayTDS == 0.00) {
    digitalWrite(ABMix, LOW);
    stAbMix = "L";
    digitalWrite(Air, LOW);
    stAir = "L";
  } else if (tdsValue < setpointTDS && delayTDS < -100) {
    digitalWrite(ABMix, HIGH);
    stAbMix = "H";
    digitalWrite(Air, LOW);
    stAir = "L";
    delay(fabsf(delayTDS));
  } else if (tdsValue > setpointTDS && delayTDS > 100) {
    digitalWrite(ABMix, LOW);
    stAbMix = "L";
    digitalWrite(Air, HIGH);
    stAir = "H";
    delay(fabsf(delayTDS));
  }
}

void pompaPH() {
  defuzzPH();
  if (phNow >= (6.00f) && phNow <= (7.00f)) {
    digitalWrite(phUp, LOW);
    stphUp = "L";
    digitalWrite(phD, LOW);
    stphD = "L";
  } else if (phNow < (6.00f) && delayPH > 100) {
    digitalWrite(phUp, HIGH);
    stphUp = "H";
    digitalWrite(phD, LOW);
    stphD = "L";
    delay(fabsf(delayPH));
  } else if (phNow > (7.00f) && delayPH < -100) {
    digitalWrite(phUp, LOW);
    stphUp = "L";
    digitalWrite(phD, HIGH);
    stphD = "H";
    delay(fabsf(delayPH));
  }
}

void tampil() {
  lcd.setCursor(0, 0);
  lcd.print("TDS:");
  if (tdsValue < 1000) {
    lcd.print(" ");
    lcd.print(tdsValue, 0);
  } else {
    lcd.print(tdsValue, 0);
  }
  lcd.setCursor(8, 0);
  lcd.print("ppm");

  lcd.setCursor(0, 1);
  lcd.print("pH :");
  lcd.print(phNow);
  clearLCDph(1);

  lcd.setCursor(0, 2);
  lcd.print("T  :");
  lcd.print(suhuKal, 0);
  lcd.setCursor(6, 2);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(13, 0);
  lcd.print("SP:");
  if (setpointTDS < 1000) {
    lcd.print(" ");
    lcd.print(setpointTDS, 0);
  } else {
    lcd.print(setpointTDS, 0);
  }

}

void clearLCDph(int line) {
  lcd.setCursor(8, line);
  for (int n = 0; n < 2; n++) {
    lcd.print(" ");
  }
}

void kirimdata() {
  Serial.print(F("TDS: "));
  Serial.print(tdsValue, 0);
  Serial.print(F("PPM _ "));
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    } digitalWrite(ledPin, ledState);
  }

  tempNow = getSuhu();
  Serial.print(F("TempAir: "));
  Serial.print(tempNow);
  Serial.print(F(" C _ "));
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    } digitalWrite(ledPin, ledState);
  }

  Serial.print("pH: ");
  Serial.println(phNow);
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    } digitalWrite(ledPin, ledState);
  }

  String datasend = String(tdsValue, 0) + "#" +
                    String(phNow) + "#" +
                    String(suhuKal, 0) + "#" +
                    String(stAbMix) + "#" +
                    String(stAir) + "#" +
                    String(stphUp) + "#" +
                    String(stphD);
  Serial1.println(datasend);
  Serial.println(datasend);
}
