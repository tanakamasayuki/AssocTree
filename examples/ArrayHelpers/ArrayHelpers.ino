#include <Arduino.h>
#include <AssocTree.h>

// en: Shows append(), size(), clear(), contains() on arrays
// ja: append()/size()/clear()/contains() の配列向けサンプル
AssocTree<2048> arrayDoc;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  arrayDoc["nums"][0] = 1;
  arrayDoc["nums"][1] = 2;
  arrayDoc["nums"][2] = 3;

  Serial.print("initial size=");
  Serial.println(arrayDoc["nums"].size());

  NodeRef arr = arrayDoc["nums"];

  arr.append(42);
  arr.append(-5);

  Serial.print("after append size=");
  Serial.println(arr.size());

  for (auto entry : arr.children()) {
    Serial.print(entry.index());
    Serial.print(": ");
    Serial.println(entry.value().as<int>(0));
  }

  Serial.print("contains index 3? ");
  Serial.println(arr.contains(3) ? "yes" : "no");

  Serial.println("Clearing array...");
  arr.clear();
  Serial.print("size after clear=");
  Serial.println(arr.size());

  Serial.print("free bytes before gc=");
  Serial.println(arrayDoc.freeBytes());
  arrayDoc.gc();
  Serial.print("free bytes after gc=");
  Serial.println(arrayDoc.freeBytes());

  // en: After gc(), NodeRefs become invalid and must be reacquired.
  // ja: gc() 実行後は NodeRef が無効になるので再取得が必要。
}

void loop() {}
