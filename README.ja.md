# AssocTree

[English README](README.md) / [English Spec](assoc_tree_spec.md) / [日本語仕様](assoc_tree_spec.ja.md)

AssocTree は Arduino / ESP32 を含む組込み環境向けの静的メモリ連想ツリーです。PHP / Python の連想配列的な柔軟さを維持しつつ、ユーザーが用意した固定バッファ内だけで完結するように設計されています。

## 特徴

- **静的メモリのみ** – `AssocTree<容量>` でスタティックなプールを確保、もしくはテンプレート実引数を `0` にして外部バッファ（PSRAM 等）を渡せます。
- **遅延ノード生成** – `operator[]` のチェーンは LazyPath を構築し、`operator=` が呼ばれた瞬間だけノードを確保。読み取りは完全に副作用ゼロ。
- **混在階層に対応** – オブジェクト／配列を自由に組み合わせて JSON 的な構造を表現できます。
- **手動 GC** – `gc()` でマーク→圧縮→断片化解消までを一気に実行。Node 領域と文字列領域の両方を整理します。
- **マルチコア対応** – ESP32 ではデフォルトでクリティカルセクションを取り、`gc()` 実行中は他コアからのアクセスをブロックします（`ASSOCTREE_ENABLE_THREAD_SAFETY`）。
- **UTF-8 文字列対応** – 末尾側から確保し、GC 時に自動で再配置して断片化を解消。
- **JSON 出力** – デバッグ用に `toJson(std::string&)`、Arduino では `toJson(String&)` を用意。

## 導入方法

### Arduino IDE

1. このリポジトリを `Arduino/libraries/AssocTree` に配置します。
2. Arduino IDE を再起動します。
3. **ファイル → スケッチ例 → AssocTree → BasicUsage** を開き、書き込みます。

### PlatformIO / デスクトップ検証

```bash
git clone https://github.com/tanakamasayuki/AssocTree.git
cd AssocTree
g++ -std=c++17 -Wall -Wextra -I src examples/basic_usage.cpp src/AssocTree.cpp -o assoc_tree_sample
./assoc_tree_sample
```

## 基本的な使い方

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

完成したスケッチは `examples/BasicUsage/BasicUsage.ino` を参照してください。

## サンプル

- `examples/Simple/Simple.ino` – 最小限の「Hello AssocTree」。
- `examples/BasicUsage/BasicUsage.ino` – プロファイル情報とJSON出力。
- `examples/ConfigManager/ConfigManager.ino` – `unset()` / `gc()` を活用する設定ストア。
- `examples/ExternalBuffer/ExternalBuffer.ino` – `AssocTree<0>` と外部バッファ（PSRAM 等）の組み合わせ。
- `examples/IteratorDemo/IteratorDemo.ino` – オブジェクト/配列を走査するイテレータAPIの例。
- `examples/TypeChecks/TypeChecks.ino` – `exists()`, `type()`, `isXXX()`, `contains()` の使用例。
- `examples/ArrayHelpers/ArrayHelpers.ino` – `append()`, `size()`, `clear()`, `contains(index)`、GC の挙動確認。

## 実行時バッファ版

テンプレート引数を `0` にすると、外部バッファを渡して初期化できます。

```cpp
static uint8_t pool[4096];
AssocTree<0> doc(pool, sizeof(pool));
```

PSRAM や `heap_caps_malloc` を用いた独自アロケータと組み合わせたい場合に便利です。

## 主要 API

- `NodeRef operator[](const char* key)` / `NodeRef operator[](size_t index)`  
  オブジェクト/配列のパスを遅延構築。
- `NodeRef::operator=(value)`  
  `nullptr`, `bool`, `int32_t`, `double`, `const char*`, `String` などを格納。
- `template<typename T> T NodeRef::as(const T& defaultValue)`  
  ノードが無ければデフォルト値を返します。副作用なし。
- 型関連ヘルパー: `exists()`, `type()`, `isNull/isBool/isInt/isDouble/isString/isObject/isArray`
- コンテナヘルパー: `size()`, `contains(key/index)`, `append()`, `clear()`
- `size_t AssocTree::freeBytes() const`  
  Node 領域と String 領域の間に残っているバイト数を返します。
- `void AssocTree::gc()`  
  手動ガーベジコレクション。生きているノードのみ残して圧縮します。
- `bool AssocTree::toJson(std::string& out)` / `bool toJson(String& out)`  
  デバッグ用に JSON を生成。

詳細仕様は [`assoc_tree_spec.ja.md`](assoc_tree_spec.ja.md) にまとめています。

## 現状とコントリビュート

まだ試験段階です。以下のような貢献を歓迎します。

- 追加サンプル（GC 呼び出し例、外部バッファ例、PSRAM 連携など）
- ESP32 / Arduino 向けの軽量テストコード
- メモリ使用状況やパフォーマンスのフィードバック

Issue や Pull Request は日本語・英語どちらでも構いません。
