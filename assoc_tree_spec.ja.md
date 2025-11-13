# AssocTree 仕様書（完全版・日本語）

## 1. 概要

**AssocTree** は、マイクロコントローラや組込み環境向けに設計された、  
柔軟な「連想配列（PHP / Python の dict 的）」メモリストレージです。

特徴:

- 総メモリ量（TOTAL_BYTES）だけを指定すれば使用可能
- メモリは Node と String 領域に動的に分配される（境界可変）
- 階層構造（オブジェクト、配列、混在構造）に対応
- `operator[]` は遅延パス（LazyPath）を返し、書き込み時だけノードを確保
- 読み取りはノードを生成しない（安全・副作用なし）
- PHP の `unset()` 的なキー削除に対応
- 必要なときのみ `gc()` による手動ガーベジコレクション
- 任意で JSON 出力も可能（デバッグ用途）

AssocTree は JSON 専用データ構造ではなく、  
**「PHP の配列」「Python の dict」** を静的メモリ上で安全に扱うための軽量ツリーです。

---

## 2. メモリ構造

### 2.1 ユーザーが指定するのは総メモリ量のみ

```cpp
AssocTree<2048> doc;
```

内部では以下のように扱われます：

```
rawPool[0]                                  rawPool[TOTAL_BYTES-1]
+-----------------------------------------------+
| Node[0], Node[1], ... Node[n]   ...  Strings |
+-----------------------------------------------+
          ↑                          ↑
       nodeTop                    strTop
```

- Node は先頭側から連番で配置  
- 文字列は末尾側から逆向きに配置  
- `nodeTop > strTop` になった時点でメモリ不足

### 2.2 実行時バッファの受け取り

テンプレート引数を `0` にすると、コンストラクタでユーザーが用意したバッファとサイズを渡して初期化できます。

```cpp
static uint8_t pool[4096];
AssocTree<0> doc(pool, sizeof(pool));
```

ESP32 の PSRAM など、特殊なメモリ確保方法を利用したい場合に有効です。対応するバッファサイズは 16 ビット（最大 65535 バイト）までを推奨します。

---

## 3. Node モデル

各 Node は以下の情報を持ちます：

```
Node {
    NodeType  type;        // Null / Bool / Int / Double / String / Object / Array
    uint16_t  parent;
    uint16_t  firstChild;
    uint16_t  nextSibling;

    StringSlot key;        // Object のキー（offset + length）

    union {
        bool      b;
        int32_t   i;
        double    d;
        StringSlot str;
    } value;

    uint8_t used : 1;      // スロットが使用中か
    uint8_t mark : 1;      // GC 用
};
```

ツリー構造は  
**parent / firstChild / nextSibling** の 3 ポインタで実現されます。

---

## 4. 文字列モデル

文字列は以下の構造で表されます：

```
StringSlot {
    uint16_t offset;
    uint16_t length;
}
```

末尾側から確保され、`strTop` を前に押し出して詰めていきます。

---

## 5. NodeRef（参照）と遅延パス（LazyPath）

### 5.1 `operator[]` はノードを生成しない

```cpp
auto r = doc["user"]["name"];
```

この時点ではノードは生成されず、  
**親ノードの情報＋キー情報のみ**を持つ LazyPath が構築されます。

### 5.2 LazyPath の固定バッファ制限

静的メモリのみで完結させるため、LazyPath は内部固定バッファを利用し以下の制限を持ちます。

- セグメント数（`["key"]` や `[idx]` のチェーン）は `ASSOCTREE_MAX_LAZY_SEGMENTS` 個まで（デフォルト 16）
- 文字列キーは合計 `ASSOCTREE_LAZY_KEY_BYTES` バイトまで（デフォルト 256 バイト）

具体的には、以下のようなケースでエラー（NodeRef が無効化され、`operator=` 等も失敗）となります。

- `doc["a"]["b"]["c"]...` のように 17 個以上の `operator[]` をチェーンした場合  
- `doc["very_long_key_..."]` のように、LazyPath 内で保持するキー文字列の総バイト数が 257 バイト以上に達した場合  
- 上記制限を超えた後にさらに `operator=` や `as<T>()` などを呼び出した場合

必要に応じて `ASSOCTREE_MAX_LAZY_SEGMENTS` / `ASSOCTREE_LAZY_KEY_BYTES` マクロを増やすことで対応できますが、値を増やすほど NodeRef のコピーサイズも大きくなる点に注意してください。

### 5.3 イテレータ的な参照

NodeRef からオブジェクト／配列の子要素を列挙できる軽量イテレータ機能を提供します。

- `NodeRange` … `NodeRef::children()` が返すビュー。`begin()/end()` を備える。
- `NodeIterator` … `uint16_t` インデックスを保持する前進イテレータ。読み取り専用。
- `NodeEntry` … イテレータの `operator*` が返す値で、以下を提供：
  - オブジェクトの場合: `const char* key()` でキー文字列を参照
  - 配列の場合: `size_t index()` で 0 始まりインデックスを取得
  - `NodeRef value()` で該当ノードを参照（`as<T>()`, `children()` など利用可）

利用例:

```cpp
for (auto entry : doc["settings"].children()) {
    Serial.println(entry.key());
    Serial.println(entry.value().as<int>(0));
}

for (auto entry : doc["values"].children()) {
    Serial.println(entry.index());
    Serial.println(entry.value().as<double>(0.0));
}
```

注意点:

- 対象がオブジェクト／配列以外の場合 `children()` は空 Range を返す。
- GC や書き込みが走ると従来の NodeRef 同様にイテレータも無効化される。`revision` を比較しつつ安全に扱う必要あり。
- 動的確保は行わない設計とし、`NodeIterator` は `AssocTreeBase*` とノードインデックスのみを持つ。

---

## 6. 遅延確保（operator=）

```cpp
doc["a"]["b"][0]["c"] = 100;
```

`operator=` が呼ばれたタイミングで：

1. LazyPath を root から順にたどる  
2. 必要ノードが存在しなければ作成（Object / Array 含む）  
3. 最終ノードに値を書き込む  
4. NodeRef を Attached 状態に更新

---

## 7. 読み取り（副作用なし）

### 7.1 as<T>()

```cpp
int v = doc["config"]["threshold"].as<int>(0);
```

- ノードが存在する場合 → 値を返す  
- 存在しない場合 → default 値を返す  
- ノード生成は行わない  

---

### 7.2 operator bool()

```cpp
if (doc["flags"]["debug"]) { ... }
```

仕様：

- ノードを探索し、存在しなければ false  
- 存在する場合は型に応じて真偽値変換  
- ノードの自動生成は行わない  
- 暗黙変換は bool のみ許可（int や double には変換しない）

---

## 8. 削除（unset）

AssocTree におけるキー削除は：

```cpp
doc["user"]["name"].unset();
```

挙動：

- 親の child/sibling リンクから除外  
- ノードは `used=0` になり、空きスロットとして即座に再利用可能  
- 実メモリの片付けは GC が実行されたタイミングでのみ行われる  

---

## 9. ガーベジコレクション（gc）

`doc.gc()` により、以下が行われます：

### 9.1 マークフェーズ  
root からすべての到達可能ノードに mark=1 を付与。

### 9.2 ノード圧縮  
- mark=1 のノードのみ前詰め  
- oldIndex → newIndex の変換マップを作成  
- すべてのリンクを更新  
- nodeCount を実ノード数に縮小

### 9.3 文字列圧縮  
- 生きている文字列を移動  
- 各 offset を更新  
- strTop を再設定

### 9.4 結果  
- メモリの断片化が完全解消  
- `freeBytes()` の戻り値が最大化  
- NodeRef（Attached）は失効（再取得が必要）

---

## 10. ユーティリティ

### freeBytes()

```cpp
size_t freeBytes() const {
    return (strTop > nodeTop) ? (strTop - nodeTop) : 0;
}
```

残りメモリはこの関数のみで管理すればよい。

---

## 11. API 使用例

### 書き込み
```cpp
doc["user"]["name"] = "Taro";
doc["user"]["age"] = 20;
```

### 読み取り
```cpp
int age = doc["user"]["age"].as<int>(0);

if (doc["flags"]["debug"]) {
    ...
}
```

### 削除（unset）
```cpp
doc["user"]["name"].unset();
```

### GC
```cpp
doc.gc();
```

---

## 12. 設計方針まとめ

- PHP/Python の連想配列に近い柔軟な構造
- 静的メモリのみ、高速・安全
- 読み取りは副作用ゼロ
- 書き込みは遅延確保
- ノードと文字列の動的境界管理
- 手動 GC による完全圧縮
- JSON 出力はデバッグ用のオプション

---

## 13. 一文でまとめると

**AssocTree は、静的メモリ上で動作する柔軟な連想配列ツリー。  
operator[] は遅延パスを返し、書き込み時にだけノードを生成。  
読み取りは副作用ゼロ。GC により断片化を完全解消する。**
