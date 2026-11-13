// Copyright 2026 Logihertz Systems (OPC) Private Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <cstddef>

namespace logihertz {
namespace json {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue {
public:
    JsonType type{JsonType::Null};
    bool bool_val{false};
    double num_val{0.0};
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool b);
    JsonValue(double n);
    JsonValue(int n);
    JsonValue(size_t n);
    JsonValue(const char *s);
    JsonValue(const std::string &s);
    JsonValue(const std::vector<JsonValue> &arr);
    JsonValue(const std::map<std::string, JsonValue> &obj);

    bool is_null() const { return type == JsonType::Null; }
    bool is_bool() const { return type == JsonType::Bool; }
    bool is_number() const { return type == JsonType::Number; }
    bool is_string() const { return type == JsonType::String; }
    bool is_array() const { return type == JsonType::Array; }
    bool is_object() const { return type == JsonType::Object; }

    bool as_bool(bool default_val = false) const;
    double as_double(double default_val = 0.0) const;
    int as_int(int default_val = 0) const;
    std::string as_string(const std::string &default_val = "") const;

    const std::vector<JsonValue>& as_array() const;
    std::vector<JsonValue>& as_array();

    const std::map<std::string, JsonValue>& as_object() const;
    std::map<std::string, JsonValue>& as_object();

    bool contains(const std::string &key) const;
    const JsonValue& get(const std::string &key) const;

    const JsonValue& operator[](const std::string &key) const;
    JsonValue& operator[](const std::string &key);

    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);
};

class JsonParser {
public:
    static JsonValue parse(const std::string &input);
    static std::string to_canonical_string(const JsonValue &val);
    static std::string to_pretty_string(const JsonValue &val, int indent_step = 2);

private:
    const std::string &src;
    size_t pos{0};

    explicit JsonParser(const std::string &input) : src(input), pos(0) {}

    void skip_whitespace();
    char peek() const;
    char get();
    bool match(char expected);

    JsonValue parse_value();
    JsonValue parse_null();
    JsonValue parse_bool();
    JsonValue parse_number();
    JsonValue parse_string();
    JsonValue parse_array();
    JsonValue parse_object();

    static void serialize_canonical(const JsonValue &val, std::string &out);
    static void serialize_pretty(const JsonValue &val, std::string &out, int current_indent, int indent_step);
};

} // namespace json
} // namespace logihertz

#endif // JSON_PARSER_HPP
