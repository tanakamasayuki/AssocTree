#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <iterator>

#ifndef ASSOCTREE_MAX_LAZY_SEGMENTS
#define ASSOCTREE_MAX_LAZY_SEGMENTS 16
#endif

#ifndef ASSOCTREE_LAZY_KEY_BYTES
#define ASSOCTREE_LAZY_KEY_BYTES 256
#endif

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace assoc_tree {

class AssocTreeBase;
class NodeRef;
class NodeIterator;
class NodeRange;
class NodeEntry;

namespace detail {

constexpr uint16_t kInvalidIndex = 0xFFFF;

enum class NodeType : uint8_t {
  Null = 0,
  Bool,
  Int,
  Double,
  String,
  Object,
  Array,
};

struct StringSlot {
  uint16_t offset = 0;
  uint16_t length = 0xFFFF;

  bool valid() const { return length != 0xFFFF; }
  void invalidate() {
    offset = 0;
    length = 0xFFFF;
  }
};

struct Node {
  NodeType type = NodeType::Null;
  uint16_t parent = kInvalidIndex;
  uint16_t firstChild = kInvalidIndex;
  uint16_t nextSibling = kInvalidIndex;
  StringSlot key{};

  union Value {
    bool asBool;
    int32_t asInt;
    double asDouble;
    StringSlot asString;

    constexpr Value() : asInt(0) {}
  } value;

  uint8_t used : 1;
  uint8_t mark : 1;
  uint8_t reserved : 6;

  Node() : value(), used(0), mark(0), reserved(0) {}
};

struct LazySegment {
  enum class Kind : uint8_t { Key, Index };

  Kind kind = Kind::Key;
  uint16_t keyOffset = 0;
  uint16_t keyLength = 0;
  size_t index = 0;
};

struct LazyPathRef {
  const LazySegment* segments = nullptr;
  size_t count = 0;
  const uint8_t* keyStorage = nullptr;

  const char* keyData(const LazySegment& seg) const {
    return reinterpret_cast<const char*>(keyStorage + seg.keyOffset);
  }
};

}  // namespace detail

class NodeRef {
 public:
  NodeRef() = default;
  NodeRef(const NodeRef&) = default;
  NodeRef& operator=(const NodeRef&) = default;

  NodeRef operator[](const char* key) const;
  NodeRef operator[](size_t index) const;
  NodeRef operator[](int index) const;

  NodeRef& operator=(std::nullptr_t);
  NodeRef& operator=(bool value);
  NodeRef& operator=(int32_t value);
  NodeRef& operator=(double value);
  NodeRef& operator=(const char* value);
  NodeRef& operator=(const std::string& value);

  template <typename T>
  std::enable_if_t<
      std::is_integral<T>::value && !std::is_same<T, bool>::value &&
          !std::is_same<T, int32_t>::value,
      NodeRef&>
  operator=(T value) {
    return (*this = static_cast<int32_t>(value));
  }

  template <typename T>
  std::enable_if_t<
      std::is_floating_point<T>::value && !std::is_same<T, double>::value,
      NodeRef&>
  operator=(T value) {
    return (*this = static_cast<double>(value));
  }

  template <typename T>
  T as(const T& defaultValue) const;

  const char* asCString(const char* defaultValue = nullptr) const;
  explicit operator bool() const;

  void unset();

  bool isAttached() const;
  NodeRange children() const;

 private:
  friend class AssocTreeBase;
  friend class NodeEntry;
  NodeRef(AssocTreeBase* tree, uint16_t baseIndex, uint16_t attachedIndex);

  AssocTreeBase* tree_ = nullptr;
  uint16_t baseIndex_ = detail::kInvalidIndex;
  uint16_t attachedIndex_ = detail::kInvalidIndex;
  uint32_t revision_ = 0;
  uint8_t pendingCount_ = 0;
  uint16_t keyBytesUsed_ = 0;
  detail::LazySegment pending_[ASSOCTREE_MAX_LAZY_SEGMENTS];
  uint8_t keyStorage_[ASSOCTREE_LAZY_KEY_BYTES]{};
  bool overflow_ = false;

  uint16_t ensureAttached();
  uint16_t resolveExisting() const;
  void touchRevision();
  NodeRef withKeySegment(const char* key, size_t len) const;
  NodeRef withIndexSegment(size_t index) const;
  bool prepareForSegment(NodeRef& ref) const;
  bool appendKey(NodeRef& ref, const char* key, size_t len) const;
  detail::LazyPathRef pendingPath() const;
};

class NodeEntry {
 public:
  const char* key() const;
  size_t index() const { return isArray_ ? arrayIndex_ : 0; }
  bool isArrayEntry() const { return isArray_; }
  NodeRef value() const;

 private:
  friend class NodeIterator;
  NodeEntry(AssocTreeBase* tree, uint16_t nodeIndex, bool isArray, size_t arrayIndex);

  AssocTreeBase* tree_ = nullptr;
  uint16_t nodeIndex_ = detail::kInvalidIndex;
  bool isArray_ = false;
  size_t arrayIndex_ = 0;
};

class NodeIterator {
 public:
  using value_type = NodeEntry;
  using reference = NodeEntry;
  using pointer = void;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  NodeIterator() = default;

  NodeEntry operator*() const;
  NodeIterator& operator++();
  NodeIterator operator++(int) {
    NodeIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const NodeIterator& other) const {
    return tree_ == other.tree_ && current_ == other.current_;
  }
  bool operator!=(const NodeIterator& other) const { return !(*this == other); }

 private:
  friend class NodeRange;
  NodeIterator(AssocTreeBase* tree, uint16_t start, bool isArray, uint32_t revision, size_t arrayIndex);
  void advanceToValid();

  AssocTreeBase* tree_ = nullptr;
  uint16_t current_ = detail::kInvalidIndex;
  bool isArray_ = false;
  uint32_t revision_ = 0;
  size_t arrayIndex_ = 0;
};

class NodeRange {
 public:
  NodeRange() = default;

  NodeIterator begin() const;
  NodeIterator end() const;
  bool empty() const { return begin() == end(); }

 private:
  friend class NodeRef;
  NodeRange(AssocTreeBase* tree, uint16_t firstChild, bool isArray, uint32_t revision);

  AssocTreeBase* tree_ = nullptr;
  uint16_t firstChild_ = detail::kInvalidIndex;
  bool isArray_ = false;
  uint32_t revision_ = 0;
};

class AssocTreeBase {
 public:
  AssocTreeBase(uint8_t* buffer, size_t totalBytes);

  NodeRef operator[](const char* key);
  NodeRef operator[](size_t index);

  size_t freeBytes() const;
  void gc();
  bool toJson(std::string& out) const;
#ifdef ARDUINO
  bool toJson(String& out) const;
#endif

 protected:
  friend class NodeRef;
  friend class NodeEntry;
  friend class NodeIterator;
  friend class NodeRange;
  using Node = detail::Node;
  using NodeType = detail::NodeType;
  using StringSlot = detail::StringSlot;

  NodeRef makeRootRef();
  uint16_t rootIndex() const { return 0; }

  Node* nodeAt(uint16_t index);
  const Node* nodeAt(uint16_t index) const;
  const char* stringAt(const StringSlot& slot) const;

  void setNodeNull(Node& node);
  void setNodeBool(Node& node, bool value);
  void setNodeInt(Node& node, int32_t value);
  void setNodeDouble(Node& node, double value);
  void setNodeString(Node& node, const char* data, size_t len);

  uint16_t ensurePath(uint16_t baseIndex, detail::LazyPathRef path);
  uint16_t findExisting(uint16_t baseIndex, detail::LazyPathRef path) const;

  void detachNode(uint16_t nodeIndex);

 private:
  uint16_t appendChild(uint16_t parentIndex);
  uint16_t createNode();
  StringSlot storeString(const char* data, size_t len);
  uint16_t findChildByKey(uint16_t parentIndex, const char* key, size_t len) const;
  uint16_t findChildByIndex(uint16_t parentIndex, size_t targetIndex) const;
  size_t countChildren(uint16_t parentIndex) const;
  bool writeJsonNode(std::string& out, uint16_t nodeIndex) const;
  void appendEscapedString(std::string& out, const char* data, size_t len) const;
  void markReachable(uint16_t index);
  void updateReferences(uint16_t limit, uint16_t from, uint16_t to);
  void compactNodes();
  void compactStrings();

  uint8_t* buffer_;
  size_t totalBytes_;
  size_t nodeTop_;
  size_t strTop_;
  uint16_t nodeCount_;
  uint32_t revision_;
};

template <size_t TOTAL_BYTES>
class AssocTree : public AssocTreeBase {
 public:
 AssocTree();

 private:
  alignas(std::max_align_t) uint8_t storage_[TOTAL_BYTES];
};

template <>
class AssocTree<0> : public AssocTreeBase {
 public:
  AssocTree(uint8_t* buffer, size_t bytes);
};

}  // namespace assoc_tree

using assoc_tree::AssocTree;
using assoc_tree::NodeRef;

#include "AssocTree.tpp"
