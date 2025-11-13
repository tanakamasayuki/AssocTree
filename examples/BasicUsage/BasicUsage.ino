#include <Arduino.h>
#include <AssocTree.h>

// en: Fixed-size document that lives entirely in internal RAM
// ja: すべて内部RAM上で完結する固定サイズのドキュメント
AssocTree<2048> doc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // en: Populate a typical user profile using object and array mixes
  // ja: オブジェクトと配列を混在させた一般的なユーザープロファイルを格納
  doc["user"]["name"] = "Taro";
  doc["user"]["age"] = 20;
  doc["flags"]["debug"] = true;
  doc["values"][0] = 1;
  doc["values"][1] = 2;
  doc["values"][2] = 3;
}

void loop() {
  // en: Safe reads never allocate; missing keys simply return defaults
  // ja: 読み取りは一切アロケートせず、欠落キーはデフォルト値を返す
  const int age = doc["user"]["age"].as<int>(0);
  const bool debugEnabled = static_cast<bool>(doc["flags"]["debug"]);
  const bool debug2 = static_cast<bool>(doc["flags"]["debug2"]);

  Serial.print("age=");
  Serial.print(age);
  Serial.print(" debug=");
  Serial.print(debugEnabled ? "true" : "false");
  Serial.print(" debug2=");
  Serial.println(debug2 ? "true" : "false");

  Serial.print("Free bytes: ");
  Serial.println(doc.freeBytes());

  // en: Export to JSON for debugging or serial logging
  // ja: デバッグやシリアルロギング用途に JSON を生成
  String json;
  if (doc.toJson(json)) {
    Serial.print("JSON: ");
    Serial.println(json);
  } else {
    Serial.println("JSON: <failed>");
  }

  delay(3000);
}
