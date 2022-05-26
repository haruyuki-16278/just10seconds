#define _red 255, 0, 0
#define _green 0, 255, 0
#define _blue 0, 0, 255
#define _black 0, 0, 0
#define _white 255, 255 ,255
#define _yellow 255 ,255, 0
#define _cyan 0, 255, 255
#define _magenta 255, 0, 255

#include <M5Atom.h>
#include <WiFi.h>
#include <esp_now.h>

String username[4] = {"red", "green", "blue", "yellow"};
uint8_t myId = 100;

/****************
 * LEDまわりの記述
 ****************/
/* CRGB生成関数 */
CRGB dispColor(byte r, byte g, byte b) {
  return (CRGB)((r << 16) | (g << 8) | b);
}
/* LED出力関数 */
void led(CRGB color) {
  M5.dis.drawpix(0, color);
  return;
}
/* LED点滅関数 */
void blink(byte r, byte g, byte b, byte times, bool faster) {
  for(int i = 0; i < times; i++) {
    led(dispColor(r, g, b));
    delay(faster ? 100 : 500);
    led(dispColor(_black));
    delay(faster ? 100 : 500);
  }
}
CRGB myColor[] = {dispColor(_red), dispColor(_green), dispColor(_blue), dispColor(_yellow)};

/*********
 * esp_nowまわりの記述
 *********/
/* esp_nowで通信するアドレス */
esp_now_peer_info_t address;
/* データ送信時コールバック */
void onDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
  return;
}
/* データ受信時コールバック */
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (myId == 100) {
    myId = *data;
  } else if (myId != 100) {
    myId = 100;
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

  // M5の機能（ボタンとか）を使うため
  M5.begin(true, false, true);
  M5.dis.drawpix(0, dispColor(127, 127, 127));

  // esp_nowの初期化(WiFiに依存するようなので先にWiFiを起動しておく)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("esp_now initialize Success.");
    blink(_blue, 3, true);
  } else {
    Serial.println("esp_now initialize Failed.");
    blink(_red, 3, false);
    ESP.restart();
  }

  // broadcastしか扱わないのでアドレスをハードコーディングで指定
  const uint8_t broadcastaddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  memcpy(address.peer_addr, broadcastaddr, 6);

  if (esp_now_add_peer(&address) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_send_cb(onDataSend);
  esp_now_register_recv_cb(onDataRecv);
}

// ループ(永遠に実行)
void loop() {
  M5.update();
  if (M5.Btn.wasPressed()) {
    Serial.println("Btn Pressed");
    esp_now_send(address.peer_addr, &myId, sizeof(myId));
  }
  if (myId != 100) {
    led(myColor[myId]);
  } else {
    led(dispColor(127, 127, 127));
  }
  delay(50);
}
