# AssocTree

[日本語はこちら](README.ja.md) / [Japanese Spec](assoc_tree_spec.ja.md) / [English Spec](assoc_tree_spec.md)

AssocTree is a static-memory associative tree designed for Arduino/ESP32 and other embedded targets. It offers PHP/Python-like flexible key/value structures while keeping the entire data set inside a user-provided buffer, enabling deterministic RAM usage even on MCUs without heap allocation.

## Features

- **Static buffer only** – either fix the pool size via `AssocTree<bytes>` or pass an external buffer (PSRAM, heap, static array) when the template size is `0`.
- **Lazy node creation** – chained `operator[]` builds a path; nodes are allocated only when assigning values, so read operations cause zero side effects.
- **Mixed hierarchy** – seamlessly combine objects and arrays to model JSON-like data.
- **Manual GC** – `gc()` executes mark/compact/defragment cycles for both node and string areas.
- **UTF-8 strings** – stored tail-first inside the pool, with automatic compaction during GC.
- **Optional JSON dump** – `toJson(std::string&)` (and `toJson(String&)` on Arduino) makes debugging easy without pulling extra libraries.

## Getting Started

### Installation (Arduino IDE)

1. Clone or copy this repository into your Arduino `libraries/` folder (folder name should be `AssocTree`).
2. Restart the Arduino IDE.
3. Open **File → Examples → AssocTree → BasicUsage**.

### Installation (PlatformIO or desktop)

```bash
git clone https://github.com/tanakamasayuki/AssocTree.git
cd AssocTree
g++ -std=c++17 -Wall -Wextra -I src examples/basic_usage.cpp src/AssocTree.cpp -o assoc_tree_sample
./assoc_tree_sample
```

## Basic Example

```cpp
#include <AssocTree.h>

AssocTree<2048> doc;

void setup() {
  Serial.begin(115200);

  doc["user"]["name"] = "Taro";
  doc["user"]["age"] = 20;
  doc["flags"]["debug"] = true;
  doc["values"][0] = 1;
  doc["values"][1] = 2;
  doc["values"][2] = 3;

  const int age = doc["user"]["age"].as<int>(0);
  const bool debug = doc["flags"]["debug"];

  Serial.print("age=");
  Serial.print(age);
  Serial.print(" debug=");
  Serial.println(debug ? "true" : "false");

  String json;
  if (doc.toJson(json)) {
    Serial.println(json);
  }
}

void loop() {}
```

See `examples/BasicUsage/BasicUsage.ino` for the complete sketch.

## Examples

- `examples/Simple/Simple.ino` – minimal “hello AssocTree” using only a few bytes.
- `examples/BasicUsage/BasicUsage.ino` – common profile data plus JSON dump.
- `examples/ConfigManager/ConfigManager.ino` – runtime configuration store with `unset()` and `gc()`.
- `examples/ExternalBuffer/ExternalBuffer.ino` – template `AssocTree<0>` fed by PSRAM or custom buffers.
- `examples/IteratorDemo/IteratorDemo.ino` – demonstrates the child iterator API for objects/arrays.
- `examples/TypeChecks/TypeChecks.ino` – highlights `exists()`, `type()`, `isXXX()`, `contains()` helpers.
- `examples/ArrayHelpers/ArrayHelpers.ino` – shows `append()`, `size()`, `clear()`, `contains(index)`, and GC impact.

## Runtime Buffer Variant

When the template parameter is `0`, supply a buffer manually:

```cpp
static uint8_t pool[4096];
AssocTree<0> doc(pool, sizeof(pool));
```

This is ideal when PSRAM or a custom allocator is involved (e.g., `heap_caps_malloc` on ESP32). You can wrap that in a factory helper tailored to your board.

## API Highlights

- `NodeRef operator[](const char* key)` / `NodeRef operator[](size_t index)`  
  Build lazy paths for objects or arrays.
- `NodeRef::operator=(value)`  
  Assign `nullptr`, `bool`, `int32_t`, `double`, `const char*`, or `String`.
- `template<typename T> T NodeRef::as(const T& defaultValue)`  
  Read without side effects. Supports `bool`, integral, floating, `std::string`.
- Type helpers: `exists()`, `type()`, `isNull/isBool/isInt/isDouble/isString/isObject/isArray`.
- Container helpers: `size()`, `contains(key/index)`, `append()`, `clear()`.
- `size_t AssocTree::freeBytes() const`  
  Observe remaining space between node and string regions.
- `void AssocTree::gc()`  
  Manually compact nodes and strings (invalidates attached references).
- `bool AssocTree::toJson(std::string& out)` / `bool toJson(String& out)`  
  Emit JSON for inspection/logging.

More design details are documented in [`assoc_tree_spec.md`](assoc_tree_spec.md).

## Status & Contributions

The project is still experimental. Issues and pull requests are welcome, especially for:

- Additional examples (GC usage, external buffers, PSRAM integration).
- Lightweight tests targeting ESP32/Arduino.
- Performance profiling and memory usage insights.

Please file issues in Japanese or English as you prefer.
