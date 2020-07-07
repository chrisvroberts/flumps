/*
   Chris Roberts, 12 February 2020
*/

#ifndef FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_CALLBACK_VISITOR_IMPL_H_
#define FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_CALLBACK_VISITOR_IMPL_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <flumps/json_parser.h>
#include <flumps/json_value_path.h>

namespace flumps {

using JsonValueCallback =
  std::function<void(
    JsonValueType type,
    const std::string& path,
    const char* data,
    std::size_t length)>;

namespace detail {

// TODO: Add default callback for unhandled items to allow JSON
//       rejection and error handling. This should be called for the
//       any outer most value that has not been reported via a call-
//       back (either itself or within a larger callback).
// TODO: Extend call back registration to allow filter on
//       JsonValueType?
class JsonCallbackVisitorImpl {
 private:
  struct Node {
   public:
    enum class Type {
      Object, Array, Key
    } type;
    static std::unique_ptr<Node> root_node() {
      return std::make_unique<Node>(nullptr, Type::Key);
    }
    static std::unique_ptr<Node> key_node(std::string key, Node* parent) {
      return std::make_unique<Node>(parent, std::move(key));
    }
    static std::unique_ptr<Node> object_node(Node* parent) {
      return std::make_unique<Node>(parent, Type::Object);
    }
    static std::unique_ptr<Node> array_node(Node* parent) {
      return std::make_unique<Node>(parent, Type::Array);
    }
    Node(Node* p, Type t) : type(t), callbacks(), parent(p), children(), key() {
    }
    Node(Node* p, std::string k) :
      type(Type::Key), callbacks(), parent(p), children(), key(std::move(k)) {
    }
    // these may need to be more efficiently organised
    std::vector<JsonValueCallback> callbacks;
    Node* parent;
    std::vector<std::unique_ptr<Node>> children;
    std::string key;  // empty for array, object and root key
  };
  enum class OverFlowType : char {
    Object = 'o',
    Array = 'a',
    Key = 'k'
  };
  struct JsonCollectionTraits {
    const Node::Type node_type;
    const OverFlowType overflow_type;
    const JsonValueType value_type;
    const std::string node_repr;
  };
  static const JsonCollectionTraits json_object_traits;
  static const JsonCollectionTraits json_array_traits;

 public:
  JsonCallbackVisitorImpl() :
    root_(Node::root_node()),
    pos_(root_.get()),
    overflow_(),
    path_() {
  }
  void register_callback(
    const JsonValuePath& path,
    JsonValueCallback callback) {

    Node* insert_pos = root_.get();
    for (const auto& segment : path.segments()) {
      if (segment.is_object()) {
        auto match = std::find_if(
          insert_pos->children.begin(),
          insert_pos->children.end(),
          [&](const std::unique_ptr<Node>& c) {
            return c->type == Node::Type::Object;
          });
        if (match == insert_pos->children.end()) {
          insert_pos->children.push_back(Node::object_node(insert_pos));
          match = insert_pos->children.end() - 1u;
        }
        insert_pos = match->get();
        match = std::find_if(
          insert_pos->children.begin(),
          insert_pos->children.end(),
          [&](const std::unique_ptr<Node>& c) {
            return c->type == Node::Type::Key && c->key == segment.key;
          });
        if (match == insert_pos->children.end()) {
          insert_pos->children.push_back(
            Node::key_node(segment.key, insert_pos));
          match = insert_pos->children.end() - 1u;
        }
        insert_pos = match->get();
      } else {
        auto match = std::find_if(
          insert_pos->children.begin(),
          insert_pos->children.end(),
          [&](const std::unique_ptr<Node>& c) {
            return c->type == Node::Type::Array;
          });
        if (match == insert_pos->children.end()) {
          insert_pos->children.push_back(Node::array_node(insert_pos));
          match = insert_pos->children.end() - 1u;
        }
        insert_pos = match->get();
      }
    }
    insert_pos->callbacks.emplace_back(std::move(callback));
  }
  void on_object_start(const char* /*start*/) {
    do_collection_start(json_object_traits);
  }
  void on_object_key(const char* start, std::size_t length) {
    assert(
      overflow_.empty() ?
      pos_->type == Node::Type::Object :
      overflow_.back() == static_cast<char>(OverFlowType::Object));
    if (overflow_.empty()) {
      auto match = std::find_if(
        pos_->children.begin(),
        pos_->children.end(),
        [&](const std::unique_ptr<Node>& c) {
          return c->type == Node::Type::Key &&
            0 == c->key.compare(0, std::string::npos, start, length);
        });
      if (match != pos_->children.end()) {
        pos_ = match->get();
        path_.append(pos_->key);
      } else {
        overflow_.push_back(static_cast<char>(OverFlowType::Key));
      }
    } else {
      overflow_.push_back(static_cast<char>(OverFlowType::Key));
    }
  }
  void on_object_end(const char* start, std::size_t length) {
    do_collection_end(json_object_traits, start, length);
  }
  void on_array_start(const char* /*start*/) {
    do_collection_start(json_array_traits);
  }
  void on_array_end(const char* start, std::size_t length) {
    do_collection_end(json_array_traits, start, length);
  }
  void on_primitive_value(
    JsonValueType type, const char* start, std::size_t length) {
    assert(
      overflow_.empty() ?
      pos_->type != Node::Type::Object :
      overflow_.back() != static_cast<char>(OverFlowType::Object));
    if (overflow_.empty()) {
      exec_callbacks(type, start, length);
      // If this primitive value was associated with an object key
      // then step back up again to reach the object root
      if (pos_->parent != nullptr &&
            pos_->parent->type == Node::Type::Object) {
        path_.resize(path_.size() - pos_->key.size());
        pos_ = pos_->parent;
      }
    } else {
      if (overflow_.back() == static_cast<char>(OverFlowType::Key)) {
        overflow_.pop_back();
      }
    }
  }

 private:
  void exec_callbacks(JsonValueType type, const char* data, std::size_t length) {
    for (auto& callback : pos_->callbacks) {
      // Prefer '.' repr for empty path over ''
      const auto& path = path_.empty() ? std::string(".") : path_;
      callback(type, path, data, length);
    }
  }
  void do_collection_start(JsonCollectionTraits traits) {
    assert(
      overflow_.empty() ?
      pos_->type != Node::Type::Object :
      overflow_.back() != static_cast<char>(OverFlowType::Object));
    if (overflow_.empty()) {
      auto match = std::find_if(
        pos_->children.begin(),
        pos_->children.end(),
        [&](const std::unique_ptr<Node>& c) {
          return c->type == traits.node_type;
        });
      if (match != pos_->children.end()) {
        pos_ = match->get();
        // Arrays path repr should lead with '.' only when at the start so
        // add one in this case.
        if (path_.empty() && traits.node_type == Node::Type::Array) {
          path_.push_back('.');
        }
        path_.append(traits.node_repr);
      } else {
        overflow_.push_back(static_cast<char>(traits.overflow_type));
      }
    } else {
      overflow_.push_back(static_cast<char>(traits.overflow_type));
    }
  }
  void do_collection_end(
    JsonCollectionTraits traits, const char* start, std::size_t length) {
    assert(
      overflow_.empty() ?
      pos_->type == traits.node_type && pos_->parent != nullptr :
      overflow_.back() == static_cast<char>(traits.overflow_type));
    if (overflow_.empty()) {
      // Close off this collection and run any callbacks
      path_.resize(path_.size() - traits.node_repr.size());
      // Arrays path repr should lead with '.' only when at the start so
      // remove it in this case.
      if (traits.node_type == Node::Type::Array && path_.size() == 1) {
        path_.clear();
      }
      pos_ = pos_->parent;
      exec_callbacks(traits.value_type, start, length);
      // If this collection was the value associated with an object key
      // then step back up again to reach the object root
      if (pos_->parent != nullptr &&
            pos_->parent->type == Node::Type::Object) {
        path_.resize(path_.size() - pos_->key.size());
        pos_ = pos_->parent;
      }
    } else {
      // Simulate closing off this collection
      overflow_.pop_back();
      if (overflow_.empty()) {
        exec_callbacks(traits.value_type, start, length);
        // If this collection was the value associated with an object key
        // then step back up again to reach the object root
        if (pos_->parent != nullptr &&
              pos_->parent->type == Node::Type::Object) {
          path_.resize(path_.size() - pos_->key.size());
          pos_ = pos_->parent;
        }
      } else {
        if (overflow_.back() == static_cast<char>(OverFlowType::Key)) {
          overflow_.pop_back();
        }
      }
    }
  }

  std::unique_ptr<Node> root_;
  Node* pos_;
  std::string overflow_;
  std::string path_;
};

}  // end namespace detail

}  // end namespace flumps

#endif  // FLUMPS_INCLUDE_FLUMPS_DETAIL_JSON_CALLBACK_VISITOR_IMPL_H_
