#include <Arduino.h>
#include <AssocTree.h>

// en: Demonstrates type() / isXXX() / exists()
// ja: type() / isXXX() / exists() の使用例
AssocTree<1024> typeDoc;

void printType(const char* path, const NodeRef& ref) {
  Serial.print(path);
  Serial.print(" exists=");
  Serial.print(ref.exists() ? "yes" : "no");
  Serial.print(" type=");
  if (ref.isNull()) Serial.println("Null");
  else if (ref.isBool()) Serial.println("Bool");
  else if (ref.isInt()) Serial.println("Int");
  else if (ref.isDouble()) Serial.println("Double");
  else if (ref.isString()) Serial.println("String");
  else if (ref.isObject()) Serial.println("Object");
  else if (ref.isArray()) Serial.println("Array");
  else Serial.println("Unknown");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  typeDoc["flag"] = true;
  typeDoc["value"] = 123;
  typeDoc["pi"] = 3.14;
  typeDoc["label"] = "AssocTree";
  typeDoc["data"]["nested"] = 1;
  typeDoc["items"][0] = "A";

  printType("flag", typeDoc["flag"]);
  printType("value", typeDoc["value"]);
  printType("pi", typeDoc["pi"]);
  printType("label", typeDoc["label"]);
  printType("data", typeDoc["data"]);
  printType("items", typeDoc["items"]);
  printType("missing", typeDoc["missing"]);

  Serial.print("data contains 'nested'? ");
  Serial.println(typeDoc["data"].contains("nested") ? "yes" : "no");
  Serial.print("items contains index 0? ");
  Serial.println(typeDoc["items"].contains(static_cast<size_t>(0)) ? "yes" : "no");
  Serial.print("items contains index 5? ");
  Serial.println(typeDoc["items"].contains(static_cast<size_t>(5)) ? "yes" : "no");
}

void loop() {}
