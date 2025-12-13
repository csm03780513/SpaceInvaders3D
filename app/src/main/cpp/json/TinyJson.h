#pragma once

#include <cctype>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace TinyJson {

struct Value;
using Object = std::unordered_map<std::string, Value>;
using Array = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
    Storage storage{};

    Value() : storage(nullptr) {}
    explicit Value(std::nullptr_t) : storage(nullptr) {}
    explicit Value(bool v) : storage(v) {}
    explicit Value(double v) : storage(v) {}
    explicit Value(std::string v) : storage(std::move(v)) {}
    explicit Value(Array v) : storage(std::move(v)) {}
    explicit Value(Object v) : storage(std::move(v)) {}

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::nullptr_t>(storage); }
    [[nodiscard]] bool isBool() const { return std::holds_alternative<bool>(storage); }
    [[nodiscard]] bool isNumber() const { return std::holds_alternative<double>(storage); }
    [[nodiscard]] bool isString() const { return std::holds_alternative<std::string>(storage); }
    [[nodiscard]] bool isArray() const { return std::holds_alternative<Array>(storage); }
    [[nodiscard]] bool isObject() const { return std::holds_alternative<Object>(storage); }

    [[nodiscard]] const Object &asObject() const { return std::get<Object>(storage); }
    [[nodiscard]] const Array &asArray() const { return std::get<Array>(storage); }
    [[nodiscard]] const std::string &asString() const { return std::get<std::string>(storage); }
    [[nodiscard]] double asNumber() const { return std::get<double>(storage); }
    [[nodiscard]] bool asBool() const { return std::get<bool>(storage); }
};

class Parser {
public:
    explicit Parser(std::string_view input) : input_(input) {}

    Value parse() {
        skipWs();
        Value v = parseValue();
        skipWs();
        if (pos_ != input_.size()) {
            throw std::runtime_error("TinyJson: trailing characters");
        }
        return v;
    }

private:
    std::string_view input_;
    size_t pos_ = 0;

    void skipWs() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            pos_++;
        }
    }

    [[nodiscard]] char peek() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    char get() {
        return pos_ < input_.size() ? input_[pos_++] : '\0';
    }

    void expect(char c) {
        if (get() != c) {
            throw std::runtime_error("TinyJson: unexpected character");
        }
    }

    static bool isNumChar(char c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
    }

    Value parseValue() {
        skipWs();
        const char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return Value(parseString());
        if (c == 't') return parseTrue();
        if (c == 'f') return parseFalse();
        if (c == 'n') return parseNull();
        if (isNumChar(c)) return Value(parseNumber());
        throw std::runtime_error("TinyJson: invalid value");
    }

    Value parseObject() {
        expect('{');
        skipWs();
        Object obj;
        if (peek() == '}') {
            get();
            return Value(std::move(obj));
        }
        while (true) {
            skipWs();
            if (peek() != '"') throw std::runtime_error("TinyJson: expected string key");
            std::string key = parseString();
            skipWs();
            expect(':');
            skipWs();
            obj.emplace(std::move(key), parseValue());
            skipWs();
            const char sep = get();
            if (sep == '}') break;
            if (sep != ',') throw std::runtime_error("TinyJson: expected ',' or '}'");
        }
        return Value(std::move(obj));
    }

    Value parseArray() {
        expect('[');
        skipWs();
        Array arr;
        if (peek() == ']') {
            get();
            return Value(std::move(arr));
        }
        while (true) {
            arr.push_back(parseValue());
            skipWs();
            const char sep = get();
            if (sep == ']') break;
            if (sep != ',') throw std::runtime_error("TinyJson: expected ',' or ']'");
            skipWs();
        }
        return Value(std::move(arr));
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (true) {
            if (pos_ >= input_.size()) throw std::runtime_error("TinyJson: unterminated string");
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                if (pos_ >= input_.size()) throw std::runtime_error("TinyJson: bad escape");
                char e = get();
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: throw std::runtime_error("TinyJson: unsupported escape");
                }
                continue;
            }
            out.push_back(c);
        }
        return out;
    }

    double parseNumber() {
        const size_t start = pos_;
        while (pos_ < input_.size() && isNumChar(input_[pos_])) pos_++;
        const std::string numStr(input_.substr(start, pos_ - start));
        try {
            return std::stod(numStr);
        } catch (...) {
            throw std::runtime_error("TinyJson: invalid number");
        }
    }

    Value parseTrue() {
        if (input_.substr(pos_, 4) != "true") throw std::runtime_error("TinyJson: expected true");
        pos_ += 4;
        return Value(true);
    }

    Value parseFalse() {
        if (input_.substr(pos_, 5) != "false") throw std::runtime_error("TinyJson: expected false");
        pos_ += 5;
        return Value(false);
    }

    Value parseNull() {
        if (input_.substr(pos_, 4) != "null") throw std::runtime_error("TinyJson: expected null");
        pos_ += 4;
        return Value(nullptr);
    }
};

inline Value parse(std::string_view input) {
    return Parser(input).parse();
}

inline const Value *get(const Object &obj, const char *key) {
    auto it = obj.find(key);
    return it == obj.end() ? nullptr : &it->second;
}

inline std::optional<std::string> getString(const Object &obj, const char *key) {
    if (const Value *v = get(obj, key)) {
        if (v->isString()) return v->asString();
    }
    return std::nullopt;
}

inline std::optional<double> getNumber(const Object &obj, const char *key) {
    if (const Value *v = get(obj, key)) {
        if (v->isNumber()) return v->asNumber();
    }
    return std::nullopt;
}

} // namespace TinyJson

