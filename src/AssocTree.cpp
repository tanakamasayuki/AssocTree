#include "AssocTree.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>

namespace assoc_tree {
namespace {

constexpr size_t kNodeSize = sizeof(detail::Node);

uint16_t remapIndex(
    uint16_t index,
    const std::vector<uint16_t>& map) {
  if (index == detail::kInvalidIndex) {
    return detail::kInvalidIndex;
  }
  if (index >= map.size()) {
    return detail::kInvalidIndex;
  }
  return map[index];
}

}  // namespace

NodeRef::NodeRef(AssocTreeBase* tree, uint16_t baseIndex, uint16_t attachedIndex)
    : tree_(tree),
      baseIndex_(baseIndex),
      attachedIndex_(attachedIndex),
      revision_(tree ? tree->revision_ : 0) {}

NodeRef NodeRef::operator[](const char* key) const {
  std::string safeKey = key ? key : "";
  return withAddedSegment(detail::LazySegment::forKey(std::move(safeKey)));
}

NodeRef NodeRef::operator[](size_t index) const {
  return withAddedSegment(detail::LazySegment::forIndex(index));
}

NodeRef NodeRef::operator[](int index) const {
  if (index < 0) {
    return NodeRef();
  }
  return (*this)[static_cast<size_t>(index)];
}

NodeRef& NodeRef::operator=(std::nullptr_t) {
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeNull(*node);
  }
  return *this;
}

NodeRef& NodeRef::operator=(bool value) {
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeBool(*node, value);
  }
  return *this;
}

NodeRef& NodeRef::operator=(int32_t value) {
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeInt(*node, value);
  }
  return *this;
}

NodeRef& NodeRef::operator=(double value) {
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeDouble(*node, value);
  }
  return *this;
}

NodeRef& NodeRef::operator=(const char* value) {
  if (!value) {
    return (*this = nullptr);
  }
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeString(*node, value, std::strlen(value));
  }
  return *this;
}

NodeRef& NodeRef::operator=(const std::string& value) {
  uint16_t idx = ensureAttached();
  if (idx == detail::kInvalidIndex) {
    return *this;
  }
  detail::Node* node = tree_->nodeAt(idx);
  if (node) {
    tree_->setNodeString(*node, value.c_str(), value.size());
  }
  return *this;
}

const char* NodeRef::asCString(const char* defaultValue) const {
  uint16_t idx = resolveExisting();
  if (idx == detail::kInvalidIndex) {
    return defaultValue;
  }
  const detail::Node* node = tree_->nodeAt(idx);
  if (!node || node->type != detail::NodeType::String ||
      !node->value.asString.valid()) {
    return defaultValue;
  }
  return tree_->stringAt(node->value.asString);
}

NodeRef::operator bool() const {
  uint16_t idx = resolveExisting();
  if (idx == detail::kInvalidIndex) {
    return false;
  }
  const AssocTreeBase* tree = tree_;
  const detail::Node* node = tree ? tree->nodeAt(idx) : nullptr;
  if (!node) {
    return false;
  }
  switch (node->type) {
    case detail::NodeType::Null:
      return false;
    case detail::NodeType::Bool:
      return node->value.asBool;
    case detail::NodeType::Int:
      return node->value.asInt != 0;
    case detail::NodeType::Double:
      return node->value.asDouble != 0.0;
    case detail::NodeType::String:
      return node->value.asString.valid() && node->value.asString.length > 0;
    case detail::NodeType::Object:
    case detail::NodeType::Array: {
      const detail::Node* child = node->firstChild == detail::kInvalidIndex
                                      ? nullptr
                                      : tree->nodeAt(node->firstChild);
      while (child) {
        if (child->used) {
          return true;
        }
        if (child->nextSibling == detail::kInvalidIndex) {
          break;
        }
        child = tree->nodeAt(child->nextSibling);
      }
      return false;
    }
    default:
      return false;
  }
}

void NodeRef::unset() {
  uint16_t idx = resolveExisting();
  if (idx == detail::kInvalidIndex) {
    return;
  }
  tree_->detachNode(idx);
  attachedIndex_ = detail::kInvalidIndex;
  pending_.clear();
  baseIndex_ = tree_->rootIndex();
}

bool NodeRef::isAttached() const {
  if (!tree_) {
    return false;
  }
  if (!pending_.empty()) {
    return false;
  }
  if (attachedIndex_ == detail::kInvalidIndex) {
    return false;
  }
  return revision_ == tree_->revision_;
}

uint16_t NodeRef::ensureAttached() {
  if (!tree_) {
    return detail::kInvalidIndex;
  }
  if (revision_ != tree_->revision_) {
    attachedIndex_ = detail::kInvalidIndex;
  }
  if (pending_.empty()) {
    if (attachedIndex_ != detail::kInvalidIndex) {
      touchRevision();
    }
    return attachedIndex_;
  }
  if (baseIndex_ == detail::kInvalidIndex) {
    baseIndex_ = tree_->rootIndex();
  }
  uint16_t idx = tree_->ensurePath(baseIndex_, pending_);
  if (idx != detail::kInvalidIndex) {
    attachedIndex_ = idx;
    baseIndex_ = idx;
    pending_.clear();
    touchRevision();
  }
  return idx;
}

uint16_t NodeRef::resolveExisting() const {
  const AssocTreeBase* tree = tree_;
  if (!tree) {
    return detail::kInvalidIndex;
  }
  if (pending_.empty()) {
    if (attachedIndex_ != detail::kInvalidIndex &&
        revision_ == tree->revision_) {
      return attachedIndex_;
    }
    return detail::kInvalidIndex;
  }
  uint16_t anchor = baseIndex_;
  if (anchor == detail::kInvalidIndex) {
    anchor = tree->rootIndex();
  }
  return tree->findExisting(anchor, pending_);
}

void NodeRef::touchRevision() {
  revision_ = tree_ ? tree_->revision_ : 0;
}

NodeRef NodeRef::withAddedSegment(detail::LazySegment seg) const {
  if (!tree_) {
    return *this;
  }
  NodeRef next = *this;
  if (next.pending_.empty()) {
    if (next.attachedIndex_ != detail::kInvalidIndex) {
      next.baseIndex_ = next.attachedIndex_;
      next.attachedIndex_ = detail::kInvalidIndex;
    } else if (next.baseIndex_ == detail::kInvalidIndex) {
      next.baseIndex_ = tree_->rootIndex();
    }
  } else if (next.baseIndex_ == detail::kInvalidIndex) {
    next.baseIndex_ = tree_->rootIndex();
  }
  next.pending_.push_back(std::move(seg));
  return next;
}

AssocTreeBase::AssocTreeBase(uint8_t* buffer, size_t totalBytes)
    : buffer_(buffer),
      totalBytes_(std::min(
          totalBytes,
          static_cast<size_t>(std::numeric_limits<uint16_t>::max()))),
      nodeTop_(0),
      strTop_(0),
      nodeCount_(0),
      revision_(1) {
  auto invalidate = [this]() {
    buffer_ = nullptr;
    totalBytes_ = 0;
    nodeTop_ = 0;
    strTop_ = 0;
    nodeCount_ = 0;
    revision_ = 0;
  };
  if (!buffer_ || totalBytes_ == 0) {
    invalidate();
    return;
  }
  size_t alignment = alignof(Node);
  size_t misalign = reinterpret_cast<uintptr_t>(buffer_) % alignment;
  if (misalign != 0) {
    size_t adjust = alignment - misalign;
    if (totalBytes_ <= adjust) {
      invalidate();
      return;
    }
    buffer_ += adjust;
    totalBytes_ -= adjust;
  }
  strTop_ = totalBytes_;
  if (totalBytes_ < kNodeSize) {
    invalidate();
    return;
  }
  createNode();  // root
  Node* root = nodeAt(rootIndex());
  if (root) {
    root->type = NodeType::Object;
    root->used = 1;
  }
}

NodeRef AssocTreeBase::operator[](const char* key) {
  return makeRootRef()[key];
}

NodeRef AssocTreeBase::operator[](size_t index) {
  return makeRootRef()[index];
}

size_t AssocTreeBase::freeBytes() const {
  if (strTop_ <= nodeTop_) {
    return 0;
  }
  return strTop_ - nodeTop_;
}

void AssocTreeBase::gc() {
  if (!buffer_) {
    return;
  }
  for (uint16_t i = 0; i < nodeCount_; ++i) {
    Node* node = nodeAt(i);
    if (node) {
      node->mark = 0;
    }
  }
  markReachable(rootIndex());
  compactNodes();
  compactStrings();
  ++revision_;
}

bool AssocTreeBase::toJson(std::string& out) const {
  if (!buffer_) {
    out.clear();
    return false;
  }
  const Node* root = nodeAt(rootIndex());
  if (!root || !root->used) {
    out.clear();
    return false;
  }
  out.clear();
  return writeJsonNode(out, rootIndex());
}

#ifdef ARDUINO
bool AssocTreeBase::toJson(String& out) const {
  std::string buffer;
  if (!toJson(buffer)) {
    out = String();
    return false;
  }
  out = buffer.c_str();
  return true;
}
#endif

NodeRef AssocTreeBase::makeRootRef() {
  return NodeRef(this, rootIndex(), rootIndex());
}

AssocTreeBase::Node* AssocTreeBase::nodeAt(uint16_t index) {
  if (!buffer_ || index == detail::kInvalidIndex) {
    return nullptr;
  }
  size_t offset = static_cast<size_t>(index) * kNodeSize;
  if (offset + kNodeSize > nodeTop_) {
    return nullptr;
  }
  return reinterpret_cast<Node*>(buffer_ + offset);
}

const AssocTreeBase::Node* AssocTreeBase::nodeAt(uint16_t index) const {
  if (!buffer_ || index == detail::kInvalidIndex) {
    return nullptr;
  }
  size_t offset = static_cast<size_t>(index) * kNodeSize;
  if (offset + kNodeSize > nodeTop_) {
    return nullptr;
  }
  return reinterpret_cast<const Node*>(buffer_ + offset);
}

const char* AssocTreeBase::stringAt(const StringSlot& slot) const {
  if (!buffer_ || !slot.valid()) {
    return "";
  }
  size_t offset = slot.offset;
  size_t len = slot.length;
  if (offset + len + 1 > totalBytes_) {
    return "";
  }
  return reinterpret_cast<const char*>(buffer_ + offset);
}

void AssocTreeBase::setNodeNull(Node& node) {
  node.type = NodeType::Null;
  node.value.asInt = 0;
}

void AssocTreeBase::setNodeBool(Node& node, bool value) {
  node.type = NodeType::Bool;
  node.value.asBool = value;
}

void AssocTreeBase::setNodeInt(Node& node, int32_t value) {
  node.type = NodeType::Int;
  node.value.asInt = value;
}

void AssocTreeBase::setNodeDouble(Node& node, double value) {
  node.type = NodeType::Double;
  node.value.asDouble = value;
}

void AssocTreeBase::setNodeString(Node& node, const char* data, size_t len) {
  if (!data) {
    setNodeNull(node);
    return;
  }
  StringSlot slot = storeString(data, len);
  if (!slot.valid()) {
    return;
  }
  node.type = NodeType::String;
  node.value.asString = slot;
}

uint16_t AssocTreeBase::ensurePath(
    uint16_t baseIndex,
    const std::vector<detail::LazySegment>& segments) {
  if (segments.empty()) {
    return baseIndex;
  }
  uint16_t current = baseIndex;
  for (const auto& segment : segments) {
    Node* parent = nodeAt(current);
    if (!parent) {
      return detail::kInvalidIndex;
    }
    if (segment.kind == detail::LazySegment::Kind::Key) {
      if (parent->type == NodeType::Null) {
        parent->type = NodeType::Object;
      }
      if (parent->type != NodeType::Object) {
        return detail::kInvalidIndex;
      }
      uint16_t child = findChildByKey(current, segment.key);
      if (child == detail::kInvalidIndex) {
        child = appendChild(current);
        if (child == detail::kInvalidIndex) {
          return detail::kInvalidIndex;
        }
        Node* node = nodeAt(child);
        if (!node) {
          return detail::kInvalidIndex;
        }
        node->type = NodeType::Null;
        node->key = storeString(segment.key.c_str(), segment.key.size());
        if (!node->key.valid()) {
          detachNode(child);
          return detail::kInvalidIndex;
        }
      }
      current = child;
      continue;
    }

    if (parent->type == NodeType::Null) {
      parent->type = NodeType::Array;
    }
    if (parent->type != NodeType::Array) {
      return detail::kInvalidIndex;
    }
    uint16_t child = findChildByIndex(current, segment.index);
    if (child == detail::kInvalidIndex) {
      size_t count = countChildren(current);
      while (count <= segment.index) {
        uint16_t newChild = appendChild(current);
        if (newChild == detail::kInvalidIndex) {
          return detail::kInvalidIndex;
        }
        Node* node = nodeAt(newChild);
        if (!node) {
          return detail::kInvalidIndex;
        }
        node->type = NodeType::Null;
        if (count == segment.index) {
          child = newChild;
        }
        ++count;
      }
    }
    current = child;
  }
  return current;
}

uint16_t AssocTreeBase::findExisting(
    uint16_t baseIndex,
    const std::vector<detail::LazySegment>& segments) const {
  if (segments.empty()) {
    return baseIndex;
  }
  uint16_t current = baseIndex;
  for (const auto& segment : segments) {
    current = (segment.kind == detail::LazySegment::Kind::Key)
                  ? findChildByKey(current, segment.key)
                  : findChildByIndex(current, segment.index);
    if (current == detail::kInvalidIndex) {
      return detail::kInvalidIndex;
    }
  }
  return current;
}

void AssocTreeBase::detachNode(uint16_t nodeIndex) {
  Node* node = nodeAt(nodeIndex);
  if (!node || node->parent == detail::kInvalidIndex) {
    return;
  }
  Node* parent = nodeAt(node->parent);
  if (!parent) {
    return;
  }
  uint16_t* link = &parent->firstChild;
  while (*link != detail::kInvalidIndex) {
    if (*link == nodeIndex) {
      *link = node->nextSibling;
      break;
    }
    Node* current = nodeAt(*link);
    if (!current) {
      break;
    }
    link = &current->nextSibling;
  }
  node->parent = detail::kInvalidIndex;
  node->nextSibling = detail::kInvalidIndex;
  node->used = 0;
  node->type = NodeType::Null;
}

uint16_t AssocTreeBase::appendChild(uint16_t parentIndex) {
  Node* parent = nodeAt(parentIndex);
  if (!parent) {
    return detail::kInvalidIndex;
  }
  uint16_t childIndex = createNode();
  if (childIndex == detail::kInvalidIndex) {
    return detail::kInvalidIndex;
  }
  Node* child = nodeAt(childIndex);
  if (!child) {
    return detail::kInvalidIndex;
  }
  child->parent = parentIndex;
  child->nextSibling = detail::kInvalidIndex;
  child->firstChild = detail::kInvalidIndex;
  if (parent->firstChild == detail::kInvalidIndex) {
    parent->firstChild = childIndex;
  } else {
    uint16_t cursor = parent->firstChild;
    Node* prev = nodeAt(cursor);
    while (prev && prev->nextSibling != detail::kInvalidIndex) {
      cursor = prev->nextSibling;
      prev = nodeAt(cursor);
    }
    if (prev) {
      prev->nextSibling = childIndex;
    }
  }
  child->used = 1;
  return childIndex;
}

uint16_t AssocTreeBase::createNode() {
  if (!buffer_) {
    return detail::kInvalidIndex;
  }
  if (nodeCount_ == detail::kInvalidIndex) {
    return detail::kInvalidIndex;
  }
  size_t newTop = nodeTop_ + kNodeSize;
  if (newTop > strTop_) {
    return detail::kInvalidIndex;
  }
  uint16_t index = nodeCount_;
  Node* node = reinterpret_cast<Node*>(buffer_ + nodeTop_);
  *node = Node();
  node->used = 1;
  nodeTop_ = newTop;
  ++nodeCount_;
  return index;
}

AssocTreeBase::StringSlot AssocTreeBase::storeString(const char* data, size_t len) {
  StringSlot slot;
  if (!buffer_) {
    slot.invalidate();
    return slot;
  }
  if (len > std::numeric_limits<uint16_t>::max()) {
    slot.invalidate();
    return slot;
  }
  size_t bytes = len + 1;
  if (bytes > freeBytes()) {
    slot.invalidate();
    return slot;
  }
  strTop_ -= bytes;
  std::memcpy(buffer_ + strTop_, data, len);
  buffer_[strTop_ + len] = '\0';
  slot.offset = static_cast<uint16_t>(strTop_);
  slot.length = static_cast<uint16_t>(len);
  return slot;
}

uint16_t AssocTreeBase::findChildByKey(
    uint16_t parentIndex,
    const std::string& key) const {
  const Node* parent = nodeAt(parentIndex);
  if (!parent || !parent->used || parent->type != NodeType::Object) {
    return detail::kInvalidIndex;
  }
  uint16_t child = parent->firstChild;
  while (child != detail::kInvalidIndex) {
    const Node* node = nodeAt(child);
    if (!node) {
      break;
    }
    if (node->used && node->key.valid() &&
        node->key.length == key.size()) {
      const char* stored = stringAt(node->key);
      if (std::memcmp(stored, key.data(), key.size()) == 0) {
        return child;
      }
    }
    child = node->nextSibling;
  }
  return detail::kInvalidIndex;
}

uint16_t AssocTreeBase::findChildByIndex(
    uint16_t parentIndex,
    size_t targetIndex) const {
  const Node* parent = nodeAt(parentIndex);
  if (!parent || !parent->used || parent->type != NodeType::Array) {
    return detail::kInvalidIndex;
  }
  uint16_t child = parent->firstChild;
  size_t index = 0;
  while (child != detail::kInvalidIndex) {
    const Node* node = nodeAt(child);
    if (!node) {
      break;
    }
    if (node->used) {
      if (index == targetIndex) {
        return child;
      }
      ++index;
    }
    child = node->nextSibling;
  }
  return detail::kInvalidIndex;
}

size_t AssocTreeBase::countChildren(uint16_t parentIndex) const {
  const Node* parent = nodeAt(parentIndex);
  if (!parent || !parent->used || parent->firstChild == detail::kInvalidIndex) {
    return 0;
  }
  size_t count = 0;
  uint16_t child = parent->firstChild;
  while (child != detail::kInvalidIndex) {
    const Node* node = nodeAt(child);
    if (!node) {
      break;
    }
    if (node->used) {
      ++count;
    }
    child = node->nextSibling;
  }
  return count;
}

bool AssocTreeBase::writeJsonNode(std::string& out, uint16_t nodeIndex) const {
  const Node* node = nodeAt(nodeIndex);
  if (!node) {
    return false;
  }
  switch (node->type) {
    case NodeType::Null:
      out += "null";
      return true;
    case NodeType::Bool:
      out += node->value.asBool ? "true" : "false";
      return true;
    case NodeType::Int:
      out += std::to_string(node->value.asInt);
      return true;
    case NodeType::Double: {
      char buffer[32];
      int len = std::snprintf(buffer, sizeof(buffer), "%.6g", node->value.asDouble);
      if (len <= 0) {
        return false;
      }
      out.append(buffer, static_cast<size_t>(len));
      return true;
    }
    case NodeType::String:
      if (node->value.asString.valid()) {
        appendEscapedString(
            out,
            stringAt(node->value.asString),
            node->value.asString.length);
      } else {
        out += "\"\"";
      }
      return true;
    case NodeType::Object: {
      out.push_back('{');
      bool first = true;
      uint16_t child = node->firstChild;
      while (child != detail::kInvalidIndex) {
        const Node* entry = nodeAt(child);
        if (!entry) {
          break;
        }
        if (entry->used && entry->key.valid()) {
          if (!first) {
            out.push_back(',');
          }
          first = false;
          appendEscapedString(
              out, stringAt(entry->key), entry->key.length);
          out.push_back(':');
          if (!writeJsonNode(out, child)) {
            return false;
          }
        }
        child = entry->nextSibling;
      }
      out.push_back('}');
      return true;
    }
    case NodeType::Array: {
      out.push_back('[');
      bool first = true;
      uint16_t child = node->firstChild;
      while (child != detail::kInvalidIndex) {
        const Node* entry = nodeAt(child);
        if (!entry) {
          break;
        }
        if (entry->used) {
          if (!first) {
            out.push_back(',');
          }
          first = false;
          if (!writeJsonNode(out, child)) {
            return false;
          }
        }
        child = entry->nextSibling;
      }
      out.push_back(']');
      return true;
    }
    default:
      return false;
  }
}

void AssocTreeBase::appendEscapedString(
    std::string& out,
    const char* data,
    size_t len) const {
  out.push_back('\"');
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = static_cast<unsigned char>(data[i]);
    switch (c) {
      case '\"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out.append(buf);
        } else {
          out.push_back(static_cast<char>(c));
        }
        break;
    }
  }
  out.push_back('\"');
}

void AssocTreeBase::markReachable(uint16_t index) {
  if (index == detail::kInvalidIndex) {
    return;
  }
  std::vector<uint16_t> stack;
  stack.push_back(index);
  while (!stack.empty()) {
    uint16_t current = stack.back();
    stack.pop_back();
    Node* node = nodeAt(current);
    if (!node || !node->used || node->mark) {
      continue;
    }
    node->mark = 1;
    uint16_t child = node->firstChild;
    while (child != detail::kInvalidIndex) {
      Node* c = nodeAt(child);
      if (c && c->used) {
        stack.push_back(child);
      }
      child = c ? c->nextSibling : detail::kInvalidIndex;
    }
  }
}

void AssocTreeBase::compactNodes() {
  if (!buffer_) {
    return;
  }
  std::vector<uint16_t> remap(nodeCount_, detail::kInvalidIndex);
  uint16_t write = 0;
  for (uint16_t i = 0; i < nodeCount_; ++i) {
    Node* node = nodeAt(i);
    if (!node || !node->used || !node->mark) {
      continue;
    }
    remap[i] = write;
    if (write != i) {
      Node* target = nodeAt(write);
      if (target) {
        *target = *node;
      }
    }
    Node* updated = nodeAt(write);
    if (updated) {
      updated->mark = 0;
    }
    ++write;
  }
  nodeCount_ = write;
  nodeTop_ = static_cast<size_t>(nodeCount_) * kNodeSize;
  for (uint16_t i = 0; i < nodeCount_; ++i) {
    Node* node = nodeAt(i);
    if (!node) {
      continue;
    }
    node->parent = remapIndex(node->parent, remap);
    node->firstChild = remapIndex(node->firstChild, remap);
    node->nextSibling = remapIndex(node->nextSibling, remap);
  }
}

void AssocTreeBase::compactStrings() {
  if (!buffer_) {
    return;
  }
  std::vector<StringSlot*> slots;
  slots.reserve(nodeCount_ * 2);
  for (uint16_t i = 0; i < nodeCount_; ++i) {
    Node* node = nodeAt(i);
    if (!node) {
      continue;
    }
    if (node->key.valid()) {
      slots.push_back(&node->key);
    }
    if (node->type == NodeType::String && node->value.asString.valid()) {
      slots.push_back(&node->value.asString);
    }
  }
  std::sort(
      slots.begin(),
      slots.end(),
      [](const StringSlot* a, const StringSlot* b) { return a->offset > b->offset; });
  size_t newStrTop = totalBytes_;
  for (StringSlot* slot : slots) {
    size_t len = slot->length;
    size_t bytes = len + 1;
    if (newStrTop < bytes || slot->offset + bytes > totalBytes_) {
      slot->invalidate();
      continue;
    }
    newStrTop -= bytes;
    std::memmove(buffer_ + newStrTop, buffer_ + slot->offset, bytes);
    slot->offset = static_cast<uint16_t>(newStrTop);
  }
  strTop_ = std::max(newStrTop, nodeTop_);
}

}  // namespace assoc_tree
