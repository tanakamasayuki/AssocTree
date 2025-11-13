#include <Arduino.h>
#include <AssocTree.h>

// en: Document that we will traverse using iterators
// ja: イテレータで走査する対象ドキュメント

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  AssocTree<2048> iterDoc;

  // en: Populate nested structures (objects with arrays)
  // ja: オブジェクト＋配列の入れ子構造を準備
  iterDoc["users"][0]["name"] = "Taro";
  iterDoc["users"][0]["score"] = 95;
  iterDoc["users"][1]["name"] = "Hanako";
  iterDoc["users"][1]["score"] = 88;

  iterDoc["stats"]["max"] = 100;
  iterDoc["stats"]["min"] = 40;
  iterDoc["stats"]["avg"] = 72;

  Serial.println("== Object iteration ==");
  // en: Iterate over object entries (key access)
  // ja: オブジェクトの各要素をキー付きで列挙
  for (auto entry : iterDoc["stats"].children())
  {
    Serial.print(entry.key());
    Serial.print(" = ");
    Serial.println(entry.value().as<int>(0));
  }

  Serial.println("== Array iteration ==");
  // en: Iterate over array entries (index access)
  // ja: 配列要素をインデックス付きで列挙
  for (auto entry : iterDoc["users"].children())
  {
    Serial.print("[");
    Serial.print(entry.index());
    Serial.print("] ");
    Serial.print(entry.value()["name"].as<String>("(unknown)"));
    Serial.print(" score=");
    Serial.println(entry.value()["score"].as<int>(0));
  }

  delay(5000);
}
