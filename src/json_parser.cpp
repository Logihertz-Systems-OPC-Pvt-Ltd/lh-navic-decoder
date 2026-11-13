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

#include "json_parser.hpp"
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <cmath>

namespace logihertz {
namespace json {

static const JsonValue STATIC_NULL_VALUE;

JsonValue::JsonValue() : type(JsonType::Null) {}
JsonValue::JsonValue(std::nullptr_t) : type(JsonType::Null) {}
JsonValue::JsonValue(bool b) : type(JsonType::Bool), bool_val(b) {}
JsonValue::JsonValue(double n) : type(JsonType::Number), num_val(n) {}
JsonValue::JsonValue(int n) : type(JsonType::Number), num_val(static_cast<double>(n)) {}
JsonValue::JsonValue(size_t n) : type(JsonType::Number), num_val(static_cast<double>(n)) {}
JsonValue::JsonValue(const char *s) : type(JsonType::String), str_val(s ? s : "") {}
JsonValue::JsonValue(const std::string &s) : type(JsonType::String), str_val(s) {}
JsonValue::JsonValue(const std::vector<JsonValue> &arr) : type(JsonType::Array), arr_val(arr) {}
JsonValue::JsonValue(const std::map<std::string, JsonValue> &obj) : type(JsonType::Object), obj_val(obj) {}

bool JsonValue::as_bool(bool default_val) const {
    if (type == JsonType::Bool) return bool_val;
    return default_val;
}

double JsonValue::as_double(double default_val) const {
    if (type == JsonType::Number) return num_val;
    return default_val;
}

int JsonValue::as_int(int default_val) const {
    if (type == JsonType::Number) return static_cast<int>(num_val);
    return default_val;
}

std::string JsonValue::as_string(const std::string &default_val) const {
    if (type == JsonType::String) return str_val;
    return default_val;
}

const std::vector<JsonValue>& JsonValue::as_array() const {
    return arr_val;
}

std::vector<JsonValue>& JsonValue::as_array() {
    return arr_val;
}

const std::map<std::string, JsonValue>& JsonValue::as_object() const {
    return obj_val;
}

std::map<std::string, JsonValue>& JsonValue::as_object() {
    return obj_val;
}

bool JsonValue::contains(const std::string &key) const {
    if (type != JsonType::Object) return false;
    return obj_val.find(key) != obj_val.end();
}

const JsonValue& JsonValue::get(const std::string &key) const {
    if (type != JsonType::Object) return STATIC_NULL_VALUE;
    auto it = obj_val.find(key);
    if (it != obj_val.end()) {
        return it->second;
    }
    return STATIC_NULL_VALUE;
}

const JsonValue& JsonValue::operator[](const std::string &key) const {
    return get(key);
}

JsonValue& JsonValue::operator[](const std::string &key) {
    if (type != JsonType::Object) {
        type = JsonType::Object;
        obj_val.clear();
    }
    return obj_val[key];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (type == JsonType::Array && index < arr_val.size()) {
        return arr_val[index];
    }
    return STATIC_NULL_VALUE;
}

JsonValue& JsonValue::operator[](size_t index) {
    if (type != JsonType::Array) {
        type = JsonType::Array;
        arr_val.clear();
    }
    if (index >= arr_val.size()) {
        arr_val.resize(index + 1);
    }
    return arr_val[index];
}

void JsonParser::skip_whitespace() {
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) {
        pos++;
    }
}

char JsonParser::peek() const {
    if (pos < src.size()) return src[pos];
    return '\0';
}

char JsonParser::get() {
    if (pos < src.size()) return src[pos++];
    return '\0';
}

bool JsonParser::match(char expected) {
    if (peek() == expected) {
        pos++;
        return true;
    }
    return false;
}

JsonValue JsonParser::parse(const std::string &input) {
    JsonParser parser(input);
    parser.skip_whitespace();
    if (parser.pos >= input.size()) {
        throw std::runtime_error("Empty JSON input");
    }
    JsonValue val = parser.parse_value();
    parser.skip_whitespace();
    if (parser.pos < input.size()) {
        throw std::runtime_error("Unexpected trailing character after JSON root");
    }
    return val;
}

JsonValue JsonParser::parse_value() {
    skip_whitespace();
    char c = peek();
    if (c == 'n') return parse_null();
    if (c == 't' || c == 'f') return parse_bool();
    if (c == '"') return parse_string();
    if (c == '[') return parse_array();
    if (c == '{') return parse_object();
    if (c == '-' || (c >= '0' && c <= '9')) return parse_number();

    throw std::runtime_error(std::string("Unexpected token in JSON: ") + c);
}

JsonValue JsonParser::parse_null() {
    if (src.compare(pos, 4, "null") == 0) {
        pos += 4;
        return JsonValue(nullptr);
    }
    throw std::runtime_error("Expected 'null'");
}

JsonValue JsonParser::parse_bool() {
    if (src.compare(pos, 4, "true") == 0) {
        pos += 4;
        return JsonValue(true);
    }
    if (src.compare(pos, 5, "false") == 0) {
        pos += 5;
        return JsonValue(false);
    }
    throw std::runtime_error("Expected 'true' or 'false'");
}

JsonValue JsonParser::parse_number() {
    size_t start = pos;
    if (peek() == '-') pos++;

    if (!std::isdigit(static_cast<unsigned char>(peek()))) {
        throw std::runtime_error("Invalid number format in JSON");
    }

    while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
        pos++;
    }

    if (peek() == '.') {
        pos++;
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw std::runtime_error("Invalid number: missing digits after decimal point");
        }
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
            pos++;
        }
    }

    if (peek() == 'e' || peek() == 'E') {
        pos++;
        if (peek() == '+' || peek() == '-') pos++;
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            throw std::runtime_error("Invalid number: missing exponent digits");
        }
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
            pos++;
        }
    }

    std::string num_str = src.substr(start, pos - start);
    double val = std::stod(num_str);
    return JsonValue(val);
}

JsonValue JsonParser::parse_string() {
    if (get() != '"') {
        throw std::runtime_error("Expected opening quote");
    }

    std::string out;
    while (pos < src.size()) {
        char c = get();
        if (c == '"') {
            return JsonValue(out);
        }
        if (c == '\\') {
            if (pos >= src.size()) throw std::runtime_error("Unterminated escape sequence in string");
            char esc = get();
            switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    if (pos + 4 > src.size()) throw std::runtime_error("Incomplete unicode escape");
                    std::string u_hex = src.substr(pos, 4);
                    pos += 4;
                    unsigned long cp = std::stoul(u_hex, nullptr, 16);
                    if (cp < 0x80) {
                        out.push_back(static_cast<char>(cp));
                    } else {
                        out.push_back('?');
                    }
                    break;
                }
                default:
                    out.push_back(esc);
                    break;
            }
        } else {
            out.push_back(c);
        }
    }
    throw std::runtime_error("Unclosed string in JSON");
}

JsonValue JsonParser::parse_array() {
    if (get() != '[') {
        throw std::runtime_error("Expected '['");
    }
    std::vector<JsonValue> arr;
    skip_whitespace();
    if (match(']')) {
        return JsonValue(arr);
    }

    while (true) {
        arr.push_back(parse_value());
        skip_whitespace();
        if (match(']')) {
            break;
        }
        if (!match(',')) {
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        skip_whitespace();
        if (peek() == ']') {
            throw std::runtime_error("Trailing comma in JSON array");
        }
    }
    return JsonValue(arr);
}

JsonValue JsonParser::parse_object() {
    if (get() != '{') {
        throw std::runtime_error("Expected '{'");
    }
    std::map<std::string, JsonValue> obj;
    skip_whitespace();
    if (match('}')) {
        return JsonValue(obj);
    }

    while (true) {
        skip_whitespace();
        if (peek() != '"') {
            throw std::runtime_error("Expected string key in object");
        }
        JsonValue key_val = parse_string();
        std::string key = key_val.str_val;

        skip_whitespace();
        if (!match(':')) {
            throw std::runtime_error("Expected ':' after key in object");
        }

        JsonValue val = parse_value();
        obj[key] = val;

        skip_whitespace();
        if (match('}')) {
            break;
        }
        if (!match(',')) {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        skip_whitespace();
        if (peek() == '}') {
            throw std::runtime_error("Trailing comma in JSON object");
        }
    }
    return JsonValue(obj);
}

static void escape_string(const std::string &s, std::string &out) {
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"': out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\n': out.append("\\n"); break;
            case '\r': out.append("\\r"); break;
            case '\t': out.append("\\t"); break;
            case '\b': out.append("\\b"); break;
            case '\f': out.append("\\f"); break;
            default:
                out.push_back(c);
                break;
        }
    }
    out.push_back('"');
}

void JsonParser::serialize_canonical(const JsonValue &val, std::string &out) {
    switch (val.type) {
        case JsonType::Null: out.append("null"); break;
        case JsonType::Bool: out.append(val.bool_val ? "true" : "false"); break;
        case JsonType::Number: {
            if (val.num_val == std::floor(val.num_val) && std::abs(val.num_val) < 1e14) {
                out.append(std::to_string(static_cast<long long>(val.num_val)));
            } else {
                std::ostringstream oss;
                oss << val.num_val;
                out.append(oss.str());
            }
            break;
        }
        case JsonType::String: escape_string(val.str_val, out); break;
        case JsonType::Array: {
            out.push_back('[');
            for (size_t i = 0; i < val.arr_val.size(); ++i) {
                if (i > 0) out.append(",");
                serialize_canonical(val.arr_val[i], out);
            }
            out.push_back(']');
            break;
        }
        case JsonType::Object: {
            out.push_back('{');
            size_t idx = 0;
            for (const auto &pair : val.obj_val) {
                if (idx > 0) out.append(",");
                escape_string(pair.first, out);
                out.push_back(':');
                serialize_canonical(pair.second, out);
                idx++;
            }
            out.push_back('}');
            break;
        }
    }
}

void JsonParser::serialize_pretty(const JsonValue &val, std::string &out, int current_indent, int indent_step) {
    std::string indent_str(current_indent, ' ');
    std::string next_indent_str(current_indent + indent_step, ' ');

    switch (val.type) {
        case JsonType::Null: out.append("null"); break;
        case JsonType::Bool: out.append(val.bool_val ? "true" : "false"); break;
        case JsonType::Number: {
            if (val.num_val == std::floor(val.num_val) && std::abs(val.num_val) < 1e14) {
                out.append(std::to_string(static_cast<long long>(val.num_val)));
            } else {
                std::ostringstream oss;
                oss << val.num_val;
                out.append(oss.str());
            }
            break;
        }
        case JsonType::String: escape_string(val.str_val, out); break;
        case JsonType::Array: {
            if (val.arr_val.empty()) {
                out.append("[]");
                return;
            }
            out.append("[\n");
            for (size_t i = 0; i < val.arr_val.size(); ++i) {
                out.append(next_indent_str);
                serialize_pretty(val.arr_val[i], out, current_indent + indent_step, indent_step);
                if (i + 1 < val.arr_val.size()) {
                    out.push_back(',');
                }
                out.push_back('\n');
            }
            out.append(indent_str);
            out.push_back(']');
            break;
        }
        case JsonType::Object: {
            if (val.obj_val.empty()) {
                out.append("{}");
                return;
            }
            out.append("{\n");
            size_t idx = 0;
            for (const auto &pair : val.obj_val) {
                out.append(next_indent_str);
                escape_string(pair.first, out);
                out.append(": ");
                serialize_pretty(pair.second, out, current_indent + indent_step, indent_step);
                if (idx + 1 < val.obj_val.size()) {
                    out.push_back(',');
                }
                out.push_back('\n');
                idx++;
            }
            out.append(indent_str);
            out.push_back('}');
            break;
        }
    }
}

std::string JsonParser::to_canonical_string(const JsonValue &val) {
    std::string out;
    serialize_canonical(val, out);
    return out;
}

std::string JsonParser::to_pretty_string(const JsonValue &val, int indent_step) {
    std::string out;
    serialize_pretty(val, out, 0, indent_step);
    return out;
}

} // namespace json
} // namespace logihertz
