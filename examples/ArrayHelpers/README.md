# ArrayHelpers

> 日本語版: [README.ja.md](README.ja.md)

Append, inspect and clear an array.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [ArrayHelpers.ino](ArrayHelpers.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<Arduino.h>`, `<AssocTree.h>`

3. Upload and open the Serial monitor at 115200 baud. Check startup messages and input conditions in the source.

## Follow the sketch

These APIs are listed in their first source order. Registering a callback does not execute its body at that point.

1. `arr.append(...)` — Add one array entry within the available pool capacity.
2. `arr.children(...)` — Iterate direct children and distinguish object keys from array indices.
3. `arr.clear(...)` — Discard or reset the selected content intentionally; pending data may be lost.
4. `arrayDoc.freeBytes(...)` — Inspect remaining pool space rather than assuming updates never consume storage.
5. `arrayDoc.gc(...)` — Compact the tree pool and reacquire all attached references afterwards.

## API usage and precautions

Choose an internal `AssocTree<bytes>` pool or supply storage to `AssocTree<0>`. Keep external storage alive for the entire tree lifetime; PSRAM availability and fallback allocation must be checked. Chained indexing describes a path, while assignment creates/stores data; `.as<T>(fallback)` provides a fallback rather than proving the stored type was correct.

Test type predicates when format matters. `unset()` / `clear()` remove logical content; `gc()` compacts storage and invalidates attached references, so reacquire NodeRef/iterators/string views afterwards. Check serialization results and free capacity when adapting to larger data.

Serializing to Arduino String or allocating external storage still uses memory outside the fixed pool.

## Inputs and observable output

These are selected output formats from the source. printf specifiers are replaced by runtime values; inspect the surrounding conditions rather than assuming these lines always occur in this order.

```text
initial size=
after append size=
:
```

## Verify and troubleshoot

Append, inspect and clear an array and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.
