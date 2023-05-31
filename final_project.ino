#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "Font_Data.h"
#include <virtuabotixRTC.h>
#include <EEPROM.h>
#include <PS2Keyboard.h>
virtuabotixRTC myRTC(13, 12, 11);
const byte LDR_PIN = A0;       // LDR Sensor pin
const byte LM35_PIN = A1;      // LM35 Sensor pin
const byte buttonSet_PIN = 6;  // Button Setting
const byte buttonSel_PIN = 5;  // Button Sellect
const int DataPin = 2;         // D- Keyboard
const int IRQpin = 3;          // D+ Keyboard
const byte buzzer_PIN = 7;     // Buzzer
uint8_t dd = 21, mm = 12, yy = 2022;
uint8_t h, m, s;
float temp_C;
int setup_menu = 0;
int alarm_menu = 0, setting_menu = 0;
unsigned long time_pressed;
unsigned long previousMillis = 0;
int buzzState = LOW;
boolean secTime = false;
int cm, cs;
int hA1, mA1;
int hA2, mA2;
int hA3, mA3;
int hA4, mA4;
int hA5, mA5;
const int romhA1 = 0, rommA1 = 5, romsetA1 = 15;
const int romhA2 = 20, rommA2 = 25, romsetA2 = 30;
const int romhA3 = 35, rommA3 = 40, romsetA3 = 45;
const int romhA4 = 50, rommA4 = 55, romsetA4 = 60;
const int romhA5 = 65, rommA5 = 70, romsetA5 = 75;
boolean alarmSet1;
boolean alarmSet2;
boolean alarmSet3;
boolean alarmSet4;
boolean alarmSet5;
boolean change_alarm_menu = false;
boolean buttonSet_read;
boolean buttonSel_read;
#define MAX_DEVICES 4  // Set the number of devices
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define CLK_PIN 8
#define DATA_PIN 9
#define CS_PIN 10
// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_MAX72XX::fontType_t *pFont;
PS2Keyboard keyboard;
#define SPEED_TIME 75  // speed of the transition
#define PAUSE_TIME 0
static uint8_t display = 0;  // current display mode
#define MAX_MESG 20
// Global variables
char szTime[9];  // mm:ss\0
char szMesg[MAX_MESG + 1] = "";
char charh[3], charm[2];
static char c;
uint16_t indexC = 0;
char alarm5c[20];
uint16_t indexhOm = 0;
char hOmChar[2];
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 };  // Deg C
// Time Setup: Code for reading clock time
void alarm_menus(const char *Alarm, int hA, int mA, boolean alarmSet, int romhA, int rommA, int romsetA, bool f = true);
void getSec() {
  myRTC.updateTime();
  s = myRTC.seconds;
  m = myRTC.minutes;
}
void getTime(char *psz, bool f = true) {
  myRTC.updateTime();
  s = myRTC.seconds;
  m = myRTC.minutes;
  h = myRTC.hours;  //24hr Format
  if (secTime) {
    sprintf(psz, "%d%c%d", m, (f ? ':' : ' '), s);
  } else {
    sprintf(psz, "%d%c%d", h, (f ? ':' : ' '), m);
  }
}
void csTime(char *psz, bool f = true) {
  int temph, tempm;
  cs = 60 - s;
  if (cs == 60) { cs = 0; }
  if (hA4 == h) {
    cm = mA4 - m - 1;
  } else if (hA4 > h) {
    temph = (hA4 - h) * 60;
    cm = (mA4 - m - 1) + temph;
  } else if (hA4 < h) {
    temph = ((hA4 - h) + 24) * 60;
    cm = (mA4 - m - 1) + temph;
  }
  sprintf(psz, "%02d%c%02d", cm, (f ? ':' : ' '), cs);
}
float Get_Temp(int pin) {
  float temp_adc_val;
  float temp_val;
  float temp;
  unsigned long temptot = 0;
  for (int x = 0; x < 100; x++) {
    temptot += analogRead(pin);
  }
  temp_adc_val = temptot / 100;
  temp_val = temp_adc_val * (5000 / 1024);
  temp = temp_val * 0.1;
  return temp;
}
void setup(void) {
  Serial.begin(9600);
  myRTC.setDS1302Time(00, 13, 21, 3, 21, 12, 2022);
  myRTC.updateTime();
  Serial.print("Tanggal / Waktu: ");
  Serial.print(myRTC.dayofmonth);  //menampilkan tanggal
  Serial.print("/");
  Serial.print(myRTC.month);  //menampilkan bulan
  Serial.print("/");
  Serial.print(myRTC.year);  //menampilkan tahun
  Serial.print(" ");
  Serial.print(myRTC.hours);  //menampilkan jam
  Serial.print(":");
  Serial.print(myRTC.minutes);  //menampilkan menit
  Serial.print(":");
  Serial.println(myRTC.seconds);  //
  P.begin(2);
  P.setInvert(false);
  P.setZone(0, MAX_DEVICES - 4, MAX_DEVICES - 1);
  P.setZone(1, MAX_DEVICES - 4, MAX_DEVICES - 1);
  P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, 0, PA_PRINT, PA_NO_EFFECT);
  P.addChar('$', degC);
  pinMode(buttonSet_PIN, INPUT_PULLUP);
  pinMode(buttonSel_PIN, INPUT_PULLUP);
  pinMode(buzzer_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT_PULLUP);
  keyboard.begin(DataPin, IRQpin);
}
void loop(void) {
  byte ledIntensity = ledIntensitySelect(analogRead(LDR_PIN));
  P.setIntensity(ledIntensity);  // value between 0 and 15 for brightness
  static uint32_t lastTime = 0;  // millis() memory
  static bool flasher = false;   // seconds passing flasher
  buttonSet_read = digitalRead(buttonSet_PIN);
  buttonSel_read = digitalRead(buttonSel_PIN);
  hA1 = EEPROM.read(romhA1), mA1 = EEPROM.read(rommA1), alarmSet1 = EEPROM.read(romsetA1);
  hA2 = EEPROM.read(romhA2), mA2 = EEPROM.read(rommA2), alarmSet2 = EEPROM.read(romsetA2);
  hA3 = EEPROM.read(romhA3), mA3 = EEPROM.read(rommA3), alarmSet3 = EEPROM.read(romsetA3);
  hA4 = EEPROM.read(romhA4), mA4 = EEPROM.read(rommA4), alarmSet4 = EEPROM.read(romsetA4);
  hA5 = EEPROM.read(romhA5), mA5 = EEPROM.read(rommA5), alarmSet5 = EEPROM.read(romsetA5);
  if (keyboard.available()) { c = keyboard.read(); }
  buzzer_alarmOFF(romsetA1, alarmSet1, hA1, mA1);
  buzzer_alarmOFF(romsetA2, alarmSet2, hA2, mA2);
  buzzer_alarmOFF(romsetA3, alarmSet3, hA3, mA3);
  buzzer_alarmOFF(romsetA4, alarmSet4, hA4, mA4);
  buzzer_alarmOFF(romsetA5, alarmSet5, hA5, mA5);
  getSec();
  P.displayAnimate();
  if (P.getZoneStatus(0)) {
    switch (display) {
      case 0:  // Clock
        P.setFont(0, numeric7Seg);
        P.setTextEffect(0, PA_OPENING, PA_NO_EFFECT);
        P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
        P.setPause(0, 0);

        if (millis() - lastTime >= 1000) {
          lastTime = millis();
          getTime(szMesg, flasher);
          if (alarmSet4) { csTime(szMesg, flasher); }
          flasher = !flasher;
          setup_menu = 0;
          alarm_menu = 0;
          change_alarm_menu = false;
        }
        // Show Temp
        if (s == 10 || s == 40) {
          display = 1;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Alarm 1
        else if (h == hA1 && m == mA1 && alarmSet1 == true) {
          display = 2;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Alarm 2
        else if (h == hA2 && m == mA2 && alarmSet2 == true) {
          display = 3;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Alarm 3
        else if (h == hA3 && m == mA3 && alarmSet3 == true) {
          display = 4;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Alarm 4
        else if (h == hA4 && m == mA4 && alarmSet4 == true) {
          display = 5;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Alarm 5
        else if (h == hA5 && m == mA5 && alarmSet5 == true) {
          display = 6;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Button Set
        else if (buttonSet_read == LOW || c == PS2_ENTER) {
          c = 0;
          display = 7;
          P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
        }
        // Button Sellect
        else if (buttonSel_read == LOW || c == ' ') {
          c = 0;
          if (secTime == false) {
            secTime = true;
          } else {
            secTime = false;
          }
        }
        break;

      case 1:  // Temperature deg C
        P.setPause(0, 1000);
        temp_C = Get_Temp(LM35_PIN);
        P.setTextEffect(0, PA_OPENING, PA_CLOSING);
        display = 0;
        dtostrf(temp_C, 3, 1, szMesg);
        strcat(szMesg, "$");
        break;

      case 2:  // Alarm 1
        alarm("072211940000058");
        break;
      case 3:  // Alarm 2
        alarm("Ramadhan Sanyoto SW");
        break;
      case 4:  // Alarm 3
        alarm("07211940000058-Ramadhan Sanyoto SW");
        break;
      case 5:  // Alarm 4
        sprintf(szMesg, "%02d%c%02d", hA4, ':', mA4);
        alarm(szMesg);
        break;
      case 6:  // Alarm 5
        alarm(alarm5c);
        break;
      case 7:  // Setting
        if (millis() - lastTime >= 100) {
          lastTime = millis();
          flasher = !flasher;
        }
        switch (setup_menu) {
          case 0:
            alarm_menus("Setting", h, m, ' ', 100, 100, 100, flasher);
            break;

          case 1:
            alarm_menus("Alarm 1", hA1, mA1, alarmSet1, romhA1, rommA1, romsetA1, flasher);
            break;

          case 2:
            alarm_menus("Alarm 2", hA2, mA2, alarmSet2, romhA2, rommA2, romsetA2, flasher);
            break;
          case 3:
            alarm_menus("Alarm 3", hA3, mA3, alarmSet3, romhA3, rommA3, romsetA3, flasher);
            break;
          case 4:
            alarm_menus("Alarm 4", hA4, mA4, alarmSet4, romhA4, rommA4, romsetA4, flasher);
            break;
          case 5:
            alarm_menus("Alarm 5", hA5, mA5, alarmSet5, romhA5, rommA5, romsetA5, flasher);
            break;
        }
        break;
    }
    P.displayReset(0);
  }
}
void alarm_menus(const char *Alarm, int hA, int mA, boolean alarmSet, int romhA, int rommA, int romsetA, bool f = true) {
  if (!change_alarm_menu) {
    P.setFont(0, pFont);
    P.displayText(Alarm, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
    alarm_menu = 0;
    if (buttonSel_read == LOW) {
      setup_menu = setup_menu + 1;
      if (setup_menu == 6) {
        setup_menu = 0;
        display = 0;
      }
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
    } else if (c == PS2_RIGHTARROW || c == PS2_UPARROW) {
      setup_menu = setup_menu + 1;
      if (setup_menu == 6) { setup_menu = 0; }
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
    } else if (c == PS2_LEFTARROW || c == PS2_DOWNARROW) {
      setup_menu = setup_menu - 1;
      if (setup_menu < 0) { setup_menu = 5; }
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
    } else if (buttonSet_read == LOW || c == PS2_ENTER) {
      change_alarm_menu = true;
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
    } else if (c == PS2_ESC) {
      display = 0;
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
    }
    c = 0;
  } else if (change_alarm_menu) {
    switch (alarm_menu) {
      case 0:
        P.setFont(0, numeric7Seg);
        readKeyboardTime(hA);
        sprintf(charh, "%02d", hA);
        sprintf(szMesg, "%s%c%02d", (f ? charh : " "), ':', mA);
        P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
        time_choice(hA);
        menu_choice();
        break;

      case 1:
        P.setFont(0, numeric7Seg);
        readKeyboardTime(mA);
        sprintf(charm, "%02d", mA);
        sprintf(szMesg, "%02d%c%s", hA, ':', (f ? charm : " "));
        P.displayText(szMesg, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
        time_choice(mA);
        menu_choice();
        break;
      case 2:
        P.setFont(0, pFont);
        if (alarmSet) {
          P.displayText("ON", PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
          if (buttonSel_read == LOW || c == PS2_LEFTARROW || c == PS2_RIGHTARROW || c == PS2_UPARROW || c == PS2_DOWNARROW) {
            c = 0;
            alarmSet = false;
            P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
          }
        } else if (!alarmSet) {
          P.displayText("OFF", PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
          if (buttonSel_read == LOW || c == PS2_LEFTARROW || c == PS2_RIGHTARROW || c == PS2_UPARROW || c == PS2_DOWNARROW) {
            c = 0;
            alarmSet = true;
            P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
          }
        }
        menu_choice();
        break;
      case 3:
        readKeyboardA5();
        if (alarm5c[6] != 0) {
          P.displayText(alarm5c, PA_CENTER, P.getSpeed(), P.getPause(), PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        } else {
          P.displayText(alarm5c, PA_CENTER, P.getSpeed(), P.getPause(), PA_PRINT, PA_NO_EFFECT);
        }
        break;
    }
  }
  EEPROM.update(romhA, hA);
  EEPROM.update(rommA, mA);
  EEPROM.update(romsetA, alarmSet);
}
void menu_choice() {
  if (buttonSet_read == LOW || c == PS2_ENTER) {
    c = 0;
    if (alarm_menu == 2 && setup_menu != 5 || alarm_menu == 3) {
      change_alarm_menu = false;
    } else if (setup_menu == 5 && alarm_menu == 2) {
      alarm_menu = alarm_menu + 1;
    } else if (setup_menu == 0 && alarm_menu == 1) {
      change_alarm_menu = false;
    } else if (alarm_menu < 2) {
      alarm_menu = alarm_menu + 1;
    }
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
  } else if (c == PS2_DELETE) {
    c = 0;
    if (alarm_menu == 0) {
      change_alarm_menu = false;
    } else {
      alarm_menu = alarm_menu - 1;
    }
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
  } else if (c == PS2_ESC) {
    c = 0;
    display = 0;
    P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
  }
}
void time_choice(int &hOm) {
  if ((millis() - time_pressed) > 100 && buttonSel_read == LOW || c == PS2_RIGHTARROW || c == PS2_UPARROW) {
    time_pressed = millis();
    c = 0;
    if (setup_menu != 0) {
      hOm = hOm + 1;
      if (alarm_menu == 0 && hOm >= 24) {
        hOm = 0;
      } else if (hOm >= 60) {
        hOm = 0;
      }
    } else {
      if (alarm_menu == 0) {
        h = h + 1;
        if (h >= 24) { h = 0; }
      } else {
        m = m + 1;
        if (m >= 60) { m = 0; }
      }
      myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);
    }
  }
  if (c == PS2_LEFTARROW || c == PS2_DOWNARROW) {
    c = 0;
    if (setup_menu != 0) {
      hOm = hOm - 1;
      if (alarm_menu == 0 && hOm <= -1) {
        hOm = 23;
      } else if (hOm <= -1) {
        hOm = 59;
      }
    } else {
      if (alarm_menu == 0) {
        h = h - 1;
        if (h == -1) { h = 23; }
      } else {
        m = m - 1;
        if (m == -1) { m = 59; }
      }
      myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);
    }
  }
}
void readKeyboardTime(int &hOm) {
  if (c == PS2_ENTER) {
    c = 0;
    if (setup_menu == 0 && alarm_menu == 1) {
      change_alarm_menu = false;
    } else {
      alarm_menu = alarm_menu + 1;
    }
  } else if (c == PS2_DELETE) {
    c = 0;
    if (alarm_menu == 0) {
      change_alarm_menu = false;
    } else alarm_menu = alarm_menu - 1;
    hOmChar[indexhOm] = 0;
    hOmChar[indexhOm - 1] = 0;
    indexhOm = 0;
  } else if (c == PS2_ESC) {
    display = 0;
    c = 0;
  } else if (isDigit(c)) {
    hOmChar[indexhOm] = c;
    hOm = atoi(hOmChar);
    if (hOmChar[indexhOm] != ' ') {
      indexhOm = indexhOm + 1;
      c = 0;
      if (alarm_menu == 0 && hOm >= 24) {
        hOm = 0;
      } else if (hOm >= 60) {
        hOm = 0;
      }
    }
    if (setup_menu == 0) {
      if (alarm_menu == 0) {
        h = atoi(hOmChar);
        if (h >= 24) { h = 0; }
        myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);
      } else {
        m = atoi(hOmChar);
        if (m >= 60) { m = 0; }
        myRTC.setDS1302Time(s, m, h, 7, dd, mm, yy);
      }
    }
  } else if (indexhOm == 2) {
    hOmChar[indexhOm] = 0;
    hOmChar[indexhOm - 1] = 0;
    indexhOm = 0;
  }
}
void readKeyboardA5() {
  alarm5c[indexC] = c;
  if (buttonSet_read == LOW || c == PS2_ENTER) {
    alarm5c[indexC] = 0;
    c = 0;
    change_alarm_menu = false;
  } else if (c == PS2_ESC) {
    alarm5c[indexC] = 0;
    c = 0;
    display = 0;
  } else if (c == PS2_DELETE) {
    alarm5c[indexC] = 0;
    indexC = indexC - 1;
    c = 0;
  } else if (alarm5c[indexC] != 0) {
    indexC = indexC + 1;
    c = 0;
  }
}
void alarm(const char *Alarm) {
  P.setFont(0, pFont);
  P.displayText(Alarm, PA_CENTER, P.getSpeed(), 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}
void buzzer_alarmOFF(int romsetA, int alarmSet, int hA, int mA) {
  static uint32_t lastTime = 0;
  unsigned long currentMillis = millis();
  if (h == hA && m == mA && alarmSet == true && display != 0 && display != 1 && display != 7) {
    // buzzer
    if (currentMillis - previousMillis >= 100) {
      previousMillis = currentMillis;
      if (buzzState == LOW) {
        buzzState = HIGH;
      } else {
        buzzState = LOW;
      }
      digitalWrite(buzzer_PIN, buzzState);
    }
    // alarm OFF
    if (buttonSet_read == LOW || buttonSel_read == LOW || s >= 59 || c != 0) {
      c = 0;
      lastTime = millis();
      alarmSet = false;
      display = 0;
      P.setTextEffect(0, PA_PRINT, PA_SCROLL_UP);
      buzzState = LOW;
      digitalWrite(buzzer_PIN, buzzState);
    }
  }
  EEPROM.update(romsetA, alarmSet);
}
byte ledIntensitySelect(int light) {
  byte _value = 0;
  if (light >= 0 && light <= 300) {
    _value = 0;
  } else if (light >= 301 && light <= 750) {
    _value = 6;
  } else if (light >= 751) {
    _value = 15;
  }
  return _value;
};