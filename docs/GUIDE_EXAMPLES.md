# AssocTree — Example workflow

> 日本語版: [GUIDE_EXAMPLES.ja.md](GUIDE_EXAMPLES.ja.md)

Store typed tree data in a bounded memory pool.

Check installation and platform requirements in the [library README](../README.md), then choose a sketch from the [example index](../examples/README.md). Version/support tables and internal capacity defaults stay in their original reference rather than being duplicated here.

## From setup to a verified result

1. Choose the smallest relevant sketch and check its board, hardware, dependencies and bundled files.
2. Match pins, peers and input files to your actual setup. Each README identifies important placeholders.
3. Open the .ino, select board/port and upload. Match Serial baud where used.
4. Locate the last completed stage: initialization, input, processing or output. Compilation does not test hardware I/O.
5. Reproduce the supplied example before changing one setting at a time.

## API and data handling

Choose an internal `AssocTree<bytes>` pool or supply storage to `AssocTree<0>`. Keep external storage alive for the entire tree lifetime; PSRAM availability and fallback allocation must be checked. Chained indexing describes a path, while assignment creates/stores data; `.as<T>(fallback)` provides a fallback rather than proving the stored type was correct.

Test type predicates when format matters. `unset()` / `clear()` remove logical content; `gc()` compacts storage and invalidates attached references, so reacquire NodeRef/iterators/string views afterwards. Check serialization results and free capacity when adapting to larger data.

Serializing to Arduino String or allocating external storage still uses memory outside the fixed pool.

## When adapting it

- Distinguish bool status, processed counts and handles; do not use stale output after failure.
- Registered callbacks run later from their documented context. Decide synchronization and retained-data lifetime.
- Keep bytes, samples, frames and pixels distinct; capacity is different from valid length.
- Retain views/pointers/buffers for the required period. Copying a pointer does not copy its payload.
- Add application-specific failure handling, retry and cleanup that a minimal example may omit.

## Read the individual README

Use Prepare and run for hardware and steps, Follow the sketch for the actual APIs and precautions for feature-specific constraints. If output is missing, check initialization first, continued service calls next and input/peer readiness after that.
