#include <Arduino.h>
#include <AssocTree.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  AssocTree<2048> doc;

  doc["user"]["name"] = "Taro";
  doc["user"]["age"] = 20;
  doc["flags"]["debug"] = true;
  doc["values"][0] = 1;
  doc["values"][1] = 2;
  doc["values"][2] = 3;

  Serial.print("age=");
  Serial.print(doc["user"]["age"].as<int>(0));
  Serial.print(" debug=");
  Serial.print(doc["flags"]["debug"] ? "true" : "false");
  Serial.print(" debug2=");
  Serial.println(doc["flags"]["debug2"] ? "true" : "false");

  Serial.print("Free bytes: ");
  Serial.println(doc.freeBytes());
  Serial.print("Free memoty: ");
  Serial.println(ESP.getFreeHeap());

  String json;
  if (doc.toJson(json))
  {
    Serial.print("JSON: ");
    Serial.println(json);
  }
  else
  {
    Serial.println("JSON: <failed>");
  }
  delay(5000);
}
