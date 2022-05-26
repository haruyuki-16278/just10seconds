#define _red 255, 0, 0
#define _green 0, 255, 0
#define _blue 0, 0, 255
#define _black 0, 0, 0
#define _white 255, 255 ,255
#define _yellow 255 ,255, 0
#define _cyan 0, 255, 255
#define _magenta 255, 0, 255

// 状態管理用
#define _accepting 0
#define _playing 1
#define _result 2

// ユーザー管理用
#define _user_red 0
#define _user_green 1
#define _user_blue 2
#define _user_yellow 3
String username[4] = {"red", "green", "blue", "yellow"};
uint8_t userCount = 0;
bool isUserJoined[4] = {false, false, false, false};
unsigned long userReactioned[4] = {0, 0, 0, 0};
unsigned long startedAt = 0;

#include <M5StickC.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t state = 0;
uint8_t tmpstate = 0;

/* 現在のモードを保持する */
int mode = 0;

/****************
 * LEDまわりの記述
 ****************/
/* LED点滅関数 */
void blink(byte times, bool faster) {
  for(int i = 0; i < times; i++) {
    digitalWrite(10, HIGH);
    delay(faster ? 100 : 500);
    digitalWrite(10, LOW);
    delay(faster ? 100 : 500);
  }
}

/*********
 * esp_nowまわりの記述
 *********/
/* esp_nowで通信するアドレス */
esp_now_peer_info_t address;
/* データ送信時コールバック */
void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println("send data");
  return;
}
/* データ受信時コールバック */
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  switch (mode) {
    case _accepting:
      if (userCount < 4) {
        esp_now_send(address.peer_addr, &userCount, sizeof(userCount));
        userCount++;
      }
      break;
    case _playing:
      if (userReactioned[*data] == 0) {
        userReactioned[*data] = millis();
      }
      break;
  }
  return;
}

/*********************
 * ライフサイクル処理
 *********************/
// セットアップ(1回だけ実行)
void setup() {
  // PCでシリアルを見ながらデバッグしたりするために使う
  Serial.begin(115200);

  blink(3, false);

  // M5の初期化
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 1, 4);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("accepting...");

  // esp_nowの初期化(WiFiに依存するようなので先にWiFiを起動しておく)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("esp_now initialize Success.");
    blink(3, true);
  } else {
    Serial.println("esp_now initialize Failed.");
    blink(3, false);
    ESP.restart();
  }

  // broadcastしか扱わないのでアドレスをハードコーディングで指定
  const uint8_t broadcastaddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  memcpy(address.peer_addr, broadcastaddr, 6);

  if (esp_now_add_peer(&address) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // コールバック登録
  esp_now_register_send_cb(onDataSend);
  esp_now_register_recv_cb(onDataRecv);
}

// ループ(永遠に実行)
void loop() {
  M5.update();
  if (M5.BtnA.wasPressed() && mode == _accepting) {
    for (int i = 3; i > 0; i--) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(2, 1, 4);
      M5.Lcd.setTextSize(3);
      M5.Lcd.println(i);
      delay(1000);
    }
    mode = _playing;
    startedAt = millis();
    for (int i = 0; i < 5; i++) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(2, 1, 4);
      M5.Lcd.setTextSize(3);
      M5.Lcd.println(i == 0 ? "start" : (String)i);
      delay(1000);
    }
    M5.Lcd.fillScreen(BLACK);
    delay(8000);
    M5.Lcd.setCursor(2, 1, 4);
    M5.Lcd.println("stop");
  }
  else if (M5.BtnA.wasPressed() && mode == _playing) {
    mode = _result;
    if (userReactioned[0] == 0 &&
        userReactioned[1] == 0 &&
        userReactioned[2] == 0 &&
        userReactioned[3] == 0) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 1, 4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("no contest");
      return;
    }

    const unsigned long diff[4] = {
      (unsigned long)abs(startedAt - userReactioned[0]),
      (unsigned long)abs(startedAt - userReactioned[1]),
      (unsigned long)abs(startedAt - userReactioned[2]),
      (unsigned long)abs(startedAt - userReactioned[3])
    };
    int winner = 0;
    for (int i = 1; i < 4; i++) {
      if (diff[winner] > diff[i]) {
        winner = i;
      }
    }

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 1, 4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Winner");
    M5.Lcd.println(username[winner]);
  }
  else if (M5.BtnA.wasPressed() && mode == _result) {
    M5.Lcd.fillScreen(BLACK);
    esp_now_send(address.peer_addr, (uint8_t*)100, sizeof((uint8_t*)100));
    mode = _accepting;
    M5.Lcd.setCursor(0, 1, 4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("accepting...");
  }
}
