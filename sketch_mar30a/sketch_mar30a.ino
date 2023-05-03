#include <Wire.h>
#include <SimpleDHT.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp LCDO;  //一號屏幕
hd44780_I2Cexp LCDT;  //二號屏幕
SimpleDHT11 dht11;    //DHT11初始化
//輔助函數
byte temp = 23, rh = 50, l_rh = 0, l_temp = 0;
byte degree[8] = {
  //設置溫度符號
  B01100,
  B10010,
  B10010,
  B01100,
  B00000,
  B00000,
  B00000,
  B00000,
};
const char *week[] = {
  //設置星期字串
  "_Sunday__",
  "_Monday__",
  "_Tuesday_",
  "Wednesday",
  "Thursday_",
  "_Friday__",
  "Saturday_",
};
const char *dayloop[] = {
  //設置晝夜字串
  "AM",
  "PM",
};
unsigned long timer = 0;
int l_yyyy = 0, l_mm = 0, l_dd = 0, l_hr = 0, l_tmm = 0, l_week = 0, c_week,
    stepp = 1, settup = 0, state = 0,
    datechanged = 1, infochanged = 1;
bool al_pause, al_relay, relay1, relay2;
int yyyy = 2023,  //年份設定
  mm = 3,         //月份設定
  dd = 22,        //日期設定
  t_hr = 5,       //小時設定
  t_mm = 0;       //分鐘設定
bool dloop = 1;   //AM,PM設定
int al_hr = 12,   //鬧鐘小時設定
  al_mm = 0;      //鬧鐘分鐘設定
bool al_dl = 1;   //鬧鐘AM,PM設定
//各功能鍵位設定
int pinDHT11 = 8,          //溫溼度感測器腳位
  SetupKey = 2,            //時間編輯模式鍵位
  AlarmSetKey = 3,         //鬧鐘編輯模式鍵位
  EditModeKey = 4,         //編輯位數鍵位
  AddValueKey = 5,         //增加數值鍵位
  SubValueKey = 6,         //減少數值鍵位
  AlarmControlKey = 7,     //鬧鐘暫停開關
  AlarmSpeakerOutput = 9;  //蜂鳴器輸出腳位

void setup() {
  char Signed[7] = { SetupKey, EditModeKey, AddValueKey, SubValueKey, AlarmSetKey, AlarmControlKey, 0 };
  //  Serial.begin(9600); //Debug
  for (int i = 0; Signed[i] != 0; i++) {
    pinMode(Signed[i], INPUT_PULLUP);
  }
  pinMode(AlarmSpeakerOutput, OUTPUT);
  DateCalculat();
  LCDO.begin(16, 2);  //一號屏幕初始化和設置背光狀態
  LCDO.setBacklight(1);
  LCDT.begin(16, 2);  //二號屏幕初始化和設置背光狀態
  LCDT.setBacklight(1);
}

void loop() {
  if (digitalRead(EditModeKey)) relay1 = 0;
  if (digitalRead(AddValueKey) && digitalRead(SubValueKey)) relay2 = 0;

  if (l_yyyy != yyyy || l_mm != mm || l_dd != dd || l_week != c_week) datechanged = 1;  //同步檢測
  if (l_rh != rh || l_temp != temp || l_hr != t_hr || l_tmm != t_mm) infochanged = 1;   //檢測完成

  if (!digitalRead(SetupKey)) {                      //時間編輯模式
    if (!digitalRead(EditModeKey) && relay1 == 0) {  //編輯位數
      relay1 = 1, stepp++;
    }
    if (!digitalRead(AddValueKey) && relay2 == 0) {  //增加數值
      switch (stepp) {
        case 1:
          yyyy++;
          break;
        case 2:
          if (mm != 12) mm++;
          break;
        case 3:
          if (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) {  //大月
            if (dd != 31) dd++;
          } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11) {  //小月
            if (dd != 30) dd++;
          } else if (mm == 2) {
            if ((yyyy % 4 == 0 && yyyy % 100 != 0) || yyyy % 400 == 0) {  //當年閏年判斷
              if (dd != 29) dd++;
            } else {
              if (dd != 28) dd++;
            }
          }
          break;
        case 4:
          if (digitalRead(AlarmSetKey)) {
            if (t_hr != 12) t_hr++;
          } else {
            if (al_hr != 12) al_hr++;
          }
          break;
        case 5:
          if (digitalRead(AlarmSetKey)) {
            if (t_mm != 60) t_mm++;
          } else {
            if (al_mm != 60) al_mm++;
          }
          break;
        case 6:
          stepp = 1;
      }
      relay2 = 1;
      DateCalculat();
    }
    if (!digitalRead(SubValueKey) && relay2 == 0) {  //減少數值
      switch (stepp) {
        case 1:
          if (yyyy > 1) yyyy--;
          break;
        case 2:
          if (mm > 1) mm--;
          break;
        case 3:
          if (dd > 1) dd--;
          break;
        case 4:
          if (digitalRead(AlarmSetKey)) {
            if (t_hr > 1) t_hr--;
          } else {
            if (al_hr > 1) al_hr--;
          }
          break;
        case 5:
          if (digitalRead(AlarmSetKey)) {
            if (t_mm > 0) t_mm--;
          } else {
            if (al_mm > 0) al_mm--;
          }
          break;
        case 6:
          stepp = 1;
      }
      relay2 = 1;
      DateCalculat();
    }
  }
  if (!digitalRead(SetupKey) || settup == 1) {
    if (timer % 1000 == 0) {  //編輯模式自動以1Hz頻率刷新
      if (stepp == 1 || stepp == 2 || stepp == 3 || stepp == 4 || stepp == 5) LCDDisplay(1);
      if (stepp == 4 || stepp == 5) LCDDisplay(2);
    }
  } else {
    if (datechanged != 0) {  //非同步檢測
      LCDDisplay(1);
    } else if (infochanged != 0) {  //非同步檢測
      LCDDisplay(2);
    }
  }
  if (timer % 200 == 0 && timer != 0) {                                                                                 //鬧鐘每隔兩百毫秒一個循環
    if (digitalRead(SetupKey) && al_hr == t_hr && al_mm == t_mm && al_dl == dloop && al_pause == 0 && al_relay == 0) {  //當不在編輯模式時且到達設定時間後觸發蜂鳴器
      al_relay = 1;
      tone(AlarmSpeakerOutput, 440, 200);
    } else if (digitalRead(SetupKey) && al_hr == t_hr && al_mm == t_mm && al_dl == dloop && al_pause == 0 && al_relay == 1) {  //當不在編輯模式時且到達設定時間後觸發蜂鳴器
      al_relay = 0;
    }
    if ((al_hr != t_hr || al_mm != t_mm || dloop != al_dl) && al_pause == 1) {  //重置暫停鬧鐘
      al_pause = 0;
    } else if (al_hr == t_hr && al_mm == t_mm && dloop == al_dl && al_pause == 0 && !digitalRead(AlarmControlKey)) {  //暫停鬧鐘
      al_pause = 1;
    }
  }
  if (timer % 60000 == 0 && timer != 0) {  //增加一分鐘
    timer = 0;
    t_mm++;
  }
  if (timer % 5000 == 0) {  //每五秒獲取一次溫度濕度
    GetDHT11info();
  }
  if (t_hr >= 13 || t_mm >= 60 || al_hr >= 13 || al_mm >= 60) {  //更新日期
    TimeDisplay();
  }
  /*
  //Debug
  Serial.println(timer);
  */
  timer = timer + 100;
  delay(100);
}

void LCDDisplay(int state) {
  switch (state) {
    case 1:
      //年月日顯示
      LCDO.setCursor(0, 0);
      if (!digitalRead(SetupKey) && digitalRead(AlarmSetKey)) {  //編輯模式時年份前會顯示 S表編輯模式 A表鬧鐘設定
        settup = 1;
        LCDO.print("S  ");
      } else if (!digitalRead(SetupKey) && !digitalRead(AlarmSetKey)) {
        settup = 1;
        LCDO.print("SA ");
      } else {
        settup = 0;
        LCDO.print("   ");
      }
      if (yyyy >= 1000) { //未達四位時補零
        LCDO.print(yyyy);
      } else if (yyyy < 1000 && yyyy >= 100) { //未達三位時補零
        LCDO.print(" ");
        LCDO.print(yyyy);
      } else if (yyyy < 100 && yyyy >= 10) {  //未達雙位時補零
        LCDO.print("  ");
        LCDO.print(yyyy);
      } else if (yyyy < 10 && yyyy >= 1) {    //輸入無效則輸出ERRO
        LCDO.print("   ");
        LCDO.print(yyyy);
      } else {
        LCDO.print("ERRO");
      }
      LCDO.print("/");
      if (mm >= 10) {  //未達雙位時補零
        LCDO.print(mm);
      } else {
        LCDO.print("0");
        LCDO.print(mm);
      }
      LCDO.print("/");
      if (dd >= 10) {  //未達雙位時補零
        LCDO.print(dd);
      } else {
        LCDO.print("0");
        LCDO.print(dd);
      }
      //星期顯示
      LCDO.setCursor(4, 1);
      LCDO.print(week[c_week]);
      l_yyyy = yyyy, l_mm = mm, l_dd = dd, l_week = c_week, datechanged = 0;  //同步數據
      break;
    case 2:
      LCDT.createChar(1, degree);  //註冊溫度符號
      //時間顯示
      LCDT.setCursor(0, 0);
      if (!digitalRead(SetupKey) && !digitalRead(AlarmSetKey)) {  //當處於設定模式且於鬧鐘設定時將時間切換至鬧鐘時間
        if (t_hr >= 10) {                                         //時間未達雙位時補零
          LCDT.print(al_hr);
        } else {
          LCDT.print("0");
          LCDT.print(al_hr);
        }
        LCDT.print(":");
        if (al_mm >= 10) {  //時間未達雙位時補零
          LCDT.print(al_mm);
        } else {
          LCDT.print("0");
          LCDT.print(al_mm);
        }
        LCDT.print(dayloop[al_dl]);
      } else {
        if (t_hr >= 10) {  //時間未達雙位時補零
          LCDT.print(t_hr);
        } else {
          LCDT.print("0");
          LCDT.print(t_hr);
        }
        LCDT.print(":");
        if (t_mm >= 10) {  //時間未達雙位時補零
          LCDT.print(t_mm);
        } else {
          LCDT.print("0");
          LCDT.print(t_mm);
        }
        LCDT.print(dayloop[dloop]);
      }
      LCDT.print(" TMP:");                                                                        //體感溫度
      LCDT.print(int(1.04 * temp + (rh * 0.01) / 6.105 * exp((17.27 * temp) / (237.7 + temp))));  //體感溫度計算
      LCDT.write(1);
      LCDT.print("C");
      //溫度濕度顯示
      LCDT.setCursor(0, 1);
      LCDT.print("RM:");
      LCDT.print(rh);
      LCDT.print("% ");
      LCDT.print("RTMP:");
      LCDT.print(temp);  //真實溫度
      LCDT.write(1);
      LCDT.print("C");
      l_rh = rh, l_temp = temp, l_hr = t_hr, l_tmm = t_mm, infochanged = 0;  //同步數據
      break;
  }
}

void GetDHT11info() {
  int info = SimpleDHTErrSuccess;
  if ((info = dht11.read(pinDHT11, &temp, &rh, NULL)) != SimpleDHTErrSuccess) {
    temp = 99, rh = 99;
  }
}

void TimeDisplay() {
  if (t_mm >= 60) t_mm = 0, t_hr++;  //小時計算
  if (t_hr >= 13 && dloop == 0) {    //早晚計算
    t_hr = 1, dloop = 1;
  } else if (t_hr >= 13 && dloop == 1) {
    t_hr = 1, dd++, dloop = 0;
  }
  if (al_mm >= 60) al_mm = 0;       //鬧鐘小時計算
  if (al_hr >= 13 && al_dl == 0) {  //鬧鐘早晚計算
    al_hr = 1, al_dl = 1;
  } else if (al_hr >= 13 && al_dl == 1) {
    al_hr = 1, al_dl = 0;
  }
  if (mm == 1 || mm == 3 || mm == 5 || mm == 7 || mm == 8 || mm == 10 || mm == 12) {  //大月  //跨月計算
    if (dd >= 31) dd = 1, mm++;
  } else if (mm == 4 || mm == 6 || mm == 9 || mm == 11) {  //小月
    if (dd >= 30) dd = 1, mm++;
  } else if (mm == 2) {
    if ((yyyy % 4 == 0 && yyyy % 100 != 0) || yyyy % 400 == 0) {  //當年閏年判斷
      if (dd >= 29) dd = 1, mm++;
    } else {
      if (dd >= 28) dd = 1, mm++;
    }
  }
  if (mm >= 13) mm = 1, yyyy++;    //跨年計算
  if (l_dd != dd) DateCalculat();  //重新計算星期
}

void DateCalculat() {
  int c_y1 = 0, c_m1 = 0;
  c_y1 = ((yyyy / 4) + (yyyy / 400) - (yyyy / 100));
  /*
  //Debug
  Serial.print("c_y1:");
  Serial.println(c_y1);
  */
  /*
  1.公元年分非4的倍數，為平年。
  2.公元年分為4的倍數但非100的倍數，為閏年。
  3.公元年分為100的倍數但非400的倍數，為平年。
  4.公元年分為400的倍數為閏年。
  */
  for (int i = 1; i != mm; i++) {                                                //月份天數計算
    if (i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10 || i == 12) {  //大月
      c_m1 = c_m1 + 31;
    } else if (i == 4 || i == 6 || i == 9 || i == 11) {  //小月
      c_m1 = c_m1 + 30;
    } else if (i == 2) {
      if ((yyyy % 4 == 0 && yyyy % 100 != 0) || yyyy % 400 == 0) {  //當年閏年判斷
        c_m1 = c_m1 + 29;
      } else {
        c_m1 = c_m1 + 28;
      }
    }
    /*
    //Debug
    Serial.print("i:");
    Serial.println(i);
    Serial.print("c_m1:");
    Serial.println(c_m1);
   */
  }
  c_week = (((((yyyy)-c_y1) * 365) + (c_y1 * 366) + c_m1 + dd)) % 7;  //星期計算
  /*
  //Debug
  Serial.print("c_week:");
  Serial.println(c_week);
  */
}