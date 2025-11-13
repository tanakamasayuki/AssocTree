#include <Arduino.h>
#include <AssocTree.h>

// en: Larger pool to keep runtime configuration and history
// ja: 実行時設定と履歴をまとめて保持するための大きめプール
AssocTree<4096> configDoc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // en: Boot-time defaults
  // ja: 起動時デフォルト設定
  configDoc["network"]["ssid"] = "ExampleSSID";
  configDoc["network"]["pass"] = "password123";
  configDoc["sensors"]["threshold"] = 75;
  configDoc["flags"]["maintenance"] = false;
}

void loop() {
  static uint32_t cycle = 0;
  ++cycle;

  // en: Pretend external command toggles maintenance flag
  // ja: 外部コマンドでメンテナンスフラグが切り替わる想定
  if (cycle % 10 == 0) {
    bool current = static_cast<bool>(configDoc["flags"]["maintenance"]);
    configDoc["flags"]["maintenance"] = !current;
  }

  // en: Push a reading into an array (auto-expands lazily)
  // ja: センサ読値を配列へ積み上げ（必要時のみノード生成）
  configDoc["readings"][cycle % 5] = analogRead(A0);

  // en: Remove the password once it has been shipped to the Wi-Fi stack
  // ja: Wi-Fi スタックへ渡した後はパスワードを即削除
  configDoc["network"]["pass"].unset();

  // en: Run GC occasionally to reclaim slots from removed keys
  // ja: 削除済みキーのスロットを取り戻すため定期的にGCを呼ぶ
  if (cycle % 20 == 0) {
    configDoc.gc();
  }

  String json;
  if (configDoc.toJson(json)) {
    Serial.println(json);
  }

  delay(500);
}
