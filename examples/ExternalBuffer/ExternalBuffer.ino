#include <Arduino.h>
#include <AssocTree.h>

#if defined(ESP32)
#include <esp_heap_caps.h>
#endif

// en: Custom buffer pointer for the runtime-sized AssocTree
// ja: 実行時にサイズを決める AssocTree 用のカスタムバッファ
uint8_t* psramBuffer = nullptr;
size_t psramBytes = 0;

// en: Runtime variant that consumes external memory
// ja: 外部メモリを利用するランタイム版
AssocTree<0>* runtimeDoc = nullptr;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

#if defined(ESP32)
  // en: Ask for 8 KB from PSRAM; fallback to heap if PSRAM is unavailable
  // ja: PSRAM から 8KB を要求し、確保できない場合は通常ヒープを利用
  psramBytes = 8 * 1024;
  psramBuffer = static_cast<uint8_t*>(heap_caps_malloc(psramBytes, MALLOC_CAP_SPIRAM));
#endif

  if (!psramBuffer) {
    psramBytes = 4096;
    psramBuffer = static_cast<uint8_t*>(malloc(psramBytes));
  }

  if (!psramBuffer) {
    Serial.println(F("Failed to allocate buffer"));
    while (true) {
      delay(1000);
    }
  }

  runtimeDoc = new AssocTree<0>(psramBuffer, psramBytes);

  // en: Mirror OTA metadata or other large payloads without touching internal RAM
  // ja: 内部RAMを消費せず OTA メタデータなど大きな情報を保持
  (*runtimeDoc)["ota"]["version"] = "1.2.3";
  (*runtimeDoc)["ota"]["checksum"] = "deadbeef";
  (*runtimeDoc)["logs"][0] = "Boot ok";
}

void loop() {
  if (!runtimeDoc) {
    return;
  }

  // en: Append timestamped message
  // ja: タイムスタンプ付きメッセージを追加
  static uint32_t tick = 0;
  String line = String("tick=") + tick++;
  (*runtimeDoc)["logs"][tick % 10] = line.c_str();

  String json;
  if (runtimeDoc->toJson(json)) {
    Serial.println(json);
  }

  delay(1000);
}
