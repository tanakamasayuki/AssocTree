#include <Arduino.h>
#include <AssocTree.h>

// en: Minimal document for tiny boards (fits on stack)
// ja: 小型ボード向けの最小構成（スタックにそのまま置けるサイズ）
AssocTree<512> simpleDoc;

void setup()
{
  Serial.begin(115200);

  // en: Store a single string and integer pair
  // ja: 文字列と整数のシンプルなセットを保存
  simpleDoc["msg"] = "Hello";
  simpleDoc["count"] = 1;

  // en: Demonstrate safe read with fallback values
  // ja: 既存しないキーでも安全に読み出せることを確認
  Serial.println(simpleDoc["msg"].as<String>("(empty)"));
  Serial.println(simpleDoc["missing"].as<int>(42));
}

void loop()
{
  // en: Increment the counter without allocating new nodes
  // ja: 新たなノードを増やさずにカウンタを更新
  int current = simpleDoc["count"].as<int>(0);
  simpleDoc["count"] = current + 1;

  Serial.print(F("count="));
  Serial.println(simpleDoc["count"].as<int>(0));
  delay(1000);
}
