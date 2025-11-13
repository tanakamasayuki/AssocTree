#pragma once

namespace assoc_tree {

template <typename T>
T NodeRef::as(const T& defaultValue) const {
  uint16_t idx = resolveExisting();
  const AssocTreeBase* tree = tree_;
  if (!tree || idx == detail::kInvalidIndex) {
    return defaultValue;
  }
  const detail::Node* node = tree->nodeAt(idx);
  if (!node) {
    return defaultValue;
  }

  if constexpr (std::is_same<T, bool>::value) {
    switch (node->type) {
      case detail::NodeType::Bool:
        return node->value.asBool;
      case detail::NodeType::Int:
        return node->value.asInt != 0;
      case detail::NodeType::Double:
        return node->value.asDouble != 0.0;
      case detail::NodeType::String:
        return node->value.asString.valid() && node->value.asString.length > 0;
      default:
        return defaultValue;
    }
  } else if constexpr (
      std::is_integral<T>::value && !std::is_same<T, bool>::value) {
    switch (node->type) {
      case detail::NodeType::Int:
        return static_cast<T>(node->value.asInt);
      case detail::NodeType::Bool:
        return static_cast<T>(node->value.asBool ? 1 : 0);
      case detail::NodeType::Double:
        return static_cast<T>(node->value.asDouble);
      default:
        return defaultValue;
    }
  } else if constexpr (std::is_floating_point<T>::value) {
    switch (node->type) {
      case detail::NodeType::Double:
        return static_cast<T>(node->value.asDouble);
      case detail::NodeType::Int:
        return static_cast<T>(node->value.asInt);
      case detail::NodeType::Bool:
        return static_cast<T>(node->value.asBool ? 1 : 0);
      default:
        return defaultValue;
    }
  } else if constexpr (std::is_same<T, std::string>::value) {
    if (node->type == detail::NodeType::String &&
        node->value.asString.valid()) {
      const char* data = tree->stringAt(node->value.asString);
      return std::string(data, node->value.asString.length);
    }
    return defaultValue;
  } else {
    static_assert(
        !std::is_same<T, T>::value,
        "NodeRef::as<T>() is not implemented for this type.");
  }
}

template <size_t TOTAL_BYTES>
AssocTree<TOTAL_BYTES>::AssocTree() : AssocTreeBase(storage_, TOTAL_BYTES) {}

inline AssocTree<0>::AssocTree(uint8_t* buffer, size_t bytes)
    : AssocTreeBase(buffer, bytes) {}

}  // namespace assoc_tree
