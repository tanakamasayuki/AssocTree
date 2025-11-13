# AssocTree Specification (English)

## 1. Overview

AssocTree is a flexible associative-memory storage designed for microcontrollers and embedded systems. It mimics PHP/Python-like arrays on a static memory pool with safe read/write semantics, lazy node creation, manual garbage collection, and optional JSON output.

Key traits:

- Only the total memory size (`TOTAL_BYTES`) is required to start.
- Memory is split dynamically between Node and String regions.
- Supports mixed hierarchies (objects, arrays).
- `operator[]` returns a lazy path; nodes are materialized only on assignment.
- Read operations have zero side effects.
- Supports `unset()`-style deletions with optional GC to reclaim memory.
- Optional JSON serialization for debugging.

---

## 2. Memory Layout

### 2.1 Static template size

```cpp
AssocTree<2048> doc;
```

```
rawPool[0]                                  rawPool[TOTAL_BYTES-1]
+-----------------------------------------------+
| Node[0], Node[1], ... Node[n]   ...  Strings |
+-----------------------------------------------+
          ↑                          ↑
       nodeTop                    strTop
```

- Nodes are allocated from the head; strings grow from the tail.
- Memory runs out when `nodeTop > strTop`.

### 2.2 Runtime buffer injection

Set the template parameter to `0` to provide your own buffer and size:

```cpp
uint8_t pool[4096];
AssocTree<0> doc(pool, sizeof(pool));
```

This is useful for PSRAM or custom allocators on ESP32. Buffers up to 16-bit length (~65535 bytes) are recommended.

---

## 3. Node Model

Each node stores:

```
Node {
    NodeType  type;        // Null / Bool / Int / Double / String / Object / Array
    uint16_t  parent;
    uint16_t  firstChild;
    uint16_t  nextSibling;

    StringSlot key;        // object key (offset + length)

    union {
        bool      b;
        int32_t   i;
        double    d;
        StringSlot str;
    } value;

    uint8_t used : 1;      // slot in use
    uint8_t mark : 1;      // GC mark
};
```

The tree is navigated via `parent/firstChild/nextSibling`.

---

## 4. String Slot

```
StringSlot {
    uint16_t offset;
    uint16_t length;
}
```

Strings are allocated from the tail (`strTop` backwards). GC compacts them on demand.

---

## 5. NodeRef and LazyPath

### 5.1 `operator[]` does not allocate immediately

```cpp
auto r = doc["user"]["name"];
```

At this point no node is created; only a lightweight LazyPath is stored.

### 5.2 Fixed buffer constraints

To avoid dynamic allocation, LazyPath stores segments and key bytes in fixed buffers:

- Maximum segments per path: `ASSOCTREE_MAX_LAZY_SEGMENTS` (default 16)
- Total key bytes stored across the path: `ASSOCTREE_LAZY_KEY_BYTES` (default 256)

Errors occur when:

- You chain more than 16 `operator[]` calls (`doc["a"]["b"]...`)
- Total key size exceeds 256 bytes
- After hitting the limit, you attempt `operator=` or `as<T>()`

Increase the macros if deeper paths or longer keys are required (note: NodeRef size increases).

---

## 6. Lazy allocation (`operator=`)

```cpp
doc["a"]["b"][0]["c"] = 100;
```

On assignment:

1. Traverse LazyPath from root.
2. Create intermediate nodes (objects/arrays) only if missing.
3. Write the value to the final node.
4. Mark the NodeRef as attached.

---

## 7. Reads (`as<T>`, `operator bool`)

```cpp
int v = doc["config"]["threshold"].as<int>(0);
```

- Returns stored value, or default if missing.
- No nodes are created.

```cpp
if (doc["flags"]["debug"]) { ... }
```

`operator bool()` simply checks existence and truthiness; it's marked `explicit` so no implicit conversions to other numeric types happen.

---

## 8. Deletion (`unset`)

```cpp
doc["user"]["name"].unset();
```

Behavior:

- Remove from parent’s child/sibling chain.
- Mark node as unused (`used=0`), making the slot immediately reusable.
- Actual memory cleanup occurs only during `gc()`.

---

## 9. Garbage Collection (`gc`)

1. **Mark**: traverse from root, marking reachable nodes.
2. **Node compaction**: shift live nodes toward the head; update links.
3. **String compaction**: reinsert surviving strings tail-first to remove fragmentation.
4. **Result**: maximum `freeBytes()`; previously attached NodeRefs become invalid.

---

## 10. Utility

`size_t freeBytes() const` returns remaining bytes between `nodeTop` and `strTop`.

---

## 11. Example API usage

```cpp
doc["user"]["name"] = "Taro";
doc["user"]["age"] = 20;

int age = doc["user"]["age"].as<int>(0);
if (doc["flags"]["debug"]) {
    ...
}

doc["user"]["name"].unset();
doc.gc();
```

---

## 12. Design Summary

- PHP/Python-like associative arrays on static memory
- Lazy writes, side-effect-free reads
- Adjustable boundary between node/string regions
- Manual GC to eliminate fragmentation
- Optional JSON output for debugging

---
