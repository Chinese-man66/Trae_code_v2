#pragma once

#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <iomanip>
#include <optional>

namespace mydb {

// ==================== 数据类型定义 ====================

using Cell = std::variant<
    std::monostate,  // NULL
    int64_t,
    double,
    std::string,
    bool
>;

// 列定义
struct Column {
    std::string name;
    enum class Type { INTEGER, DOUBLE, TEXT, BOOLEAN } type;
    bool not_null = false;
    std::optional<Cell> default_value;
};

// 记录（一行数据）
class Record {
public:
    explicit Record(const std::vector<Column>& schema);

    // 设置字段值
    void set(const std::string& name, const Cell& value);

    // 获取字段值
    Cell get(const std::string& name) const;

    // 序列化到字节流
    std::vector<uint8_t> serialize() const;

    // 从字节流反序列化
    static Record deserialize(const std::vector<uint8_t>& data,
                              const std::vector<Column>& schema);

    // 转换为字符串显示
    std::string to_string() const;

    // 获取内部值向量
    const std::vector<Cell>& values() const { return values_; }

private:
    std::vector<Cell> values_;
    const std::vector<Column>* schema_;
};

// 表定义
struct TableMeta {
    std::string name;
    std::vector<Column> columns;
    size_t row_id_counter = 0;

    // 获取列索引
    int column_index(const std::string& name) const;
};

// 记录序列化实现
inline Record::Record(const std::vector<Column>& schema) : schema_(&schema) {
    values_.resize(schema.size());
    for (size_t i = 0; i < schema.size(); ++i) {
        if (schema[i].default_value.has_value()) {
            values_[i] = schema[i].default_value.value();
        } else {
            values_[i] = Cell{};
        }
    }
}

inline void Record::set(const std::string& name, const Cell& value) {
    int idx = schema_->column_index(name);
    if (idx >= 0) {
        values_[idx] = value;
    }
}

inline Cell Record::get(const std::string& name) const {
    int idx = schema_->column_index(name);
    if (idx >= 0) {
        return values_[idx];
    }
    return Cell{};
}

inline std::string CellToString(const Cell& cell) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "NULL";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << arg;
            return oss.str();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        }
        return "???";
    }, cell);
}

inline std::string Record::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) oss << " | ";
        oss << CellToString(values_[i]);
    }
    return oss.str();
}

inline int TableMeta::column_index(const std::string& name) const {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

// 序列化辅助函数
template<typename T>
void write_value(std::vector<uint8_t>& data, const T& value) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
    data.insert(data.end(), bytes, bytes + sizeof(T));
}

template<typename T>
T read_value(const std::vector<uint8_t>& data, size_t& offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

inline void write_string(std::vector<uint8_t>& data, const std::string& str) {
    write_value(data, static_cast<uint32_t>(str.size()));
    data.insert(data.end(), str.begin(), str.end());
}

inline std::string read_string(const std::vector<uint8_t>& data, size_t& offset) {
    auto size = read_value<uint32_t>(data, offset);
    std::string str(data.begin() + offset, data.begin() + offset + size);
    offset += size;
    return str;
}

// Cell 序列化
inline void serialize_cell(std::vector<uint8_t>& data, const Cell& cell) {
    uint8_t type_tag = std::visit([](auto&& arg) -> uint8_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) return 0;
        else if constexpr (std::is_same_v<T, int64_t>) return 1;
        else if constexpr (std::is_same_v<T, double>) return 2;
        else if constexpr (std::is_same_v<T, std::string>) return 3;
        else if constexpr (std::is_same_v<T, bool>) return 4;
        return 0;
    }, cell);

    data.push_back(type_tag);

    std::visit([&data](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {}
        else if constexpr (std::is_same_v<T, int64_t>) {
            write_value(data, arg);
        }
        else if constexpr (std::is_same_v<T, double>) {
            write_value(data, arg);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            write_string(data, arg);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            data.push_back(arg ? 1 : 0);
        }
    }, cell);
}

inline Cell deserialize_cell(const std::vector<uint8_t>& data, size_t& offset) {
    uint8_t type_tag = data[offset++];

    switch (type_tag) {
        case 0: return Cell{};
        case 1: return Cell{read_value<int64_t>(data, offset)};
        case 2: return Cell{read_value<double>(data, offset)};
        case 3: return Cell{read_string(data, offset)};
        case 4: return Cell{data[offset++] != 0};
        default: return Cell{};
    }
}

inline std::vector<uint8_t> Record::serialize() const {
    std::vector<uint8_t> data;
    for (const auto& cell : values_) {
        serialize_cell(data, cell);
    }
    return data;
}

inline Record Record::deserialize(const std::vector<uint8_t>& data,
                                   const std::vector<Column>& schema) {
    Record record(schema);
    size_t offset = 0;
    for (size_t i = 0; i < schema.size() && offset < data.size(); ++i) {
        record.values_[i] = deserialize_cell(data, offset);
    }
    return record;
}

} // namespace mydb
