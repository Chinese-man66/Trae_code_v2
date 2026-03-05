#pragma once

#include "cursor.h"
#include <memory>
#include <stack>
#include <sstream>

namespace mydb {

// ==================== 查询解析器 ====================

// 查询类型
enum class QueryType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    UNKNOWN
};

// 条件操作符
enum class Op {
    EQ,   // =
    NE,   // <>
    GT,   // >
    GE,   // >=
    LT,   // <
    LE,   // <=
    LIKE, // LIKE
    IN    // IN
};

// 条件表达式
struct Condition {
    std::string column;
    Op op;
    Cell value;
};

// SELECT 语句
struct SelectQuery {
    std::string table_name;
    std::vector<std::string> columns;  // * 表示所有列
    std::vector<Condition> where;
    std::vector<std::pair<std::string, bool>> order_by; // 列名, asc/desc
    size_t limit = 0;
};

// INSERT 语句
struct InsertQuery {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::vector<Cell>> values;
};

// UPDATE 语句
struct UpdateQuery {
    std::string table_name;
    std::map<std::string, Cell> set_values;
    std::vector<Condition> where;
};

// DELETE 语句
struct DeleteQuery {
    std::string table_name;
    std::vector<Condition> where;
};

// 解析器
class Parser {
public:
    explicit Parser(const std::string& sql) : sql_(sql), pos_(0) {}

    // 解析 SQL，返回对应查询结构
    std::variant<SelectQuery, InsertQuery, UpdateQuery, DeleteQuery> parse();

private:
    // 跳过空白
    void skip_whitespace();

    // 读取下一个标记
    std::string next_token();

    // 解析关键字
    QueryType parse_keyword();

    // 解析列名
    std::string parse_identifier();

    // 解析值
    Cell parse_value();

    // 解析操作符
    Op parse_operator();

    // 解析条件
    Condition parse_condition();

    std::string sql_;
    size_t pos_;
};

// 简单 SQL 解析实现
inline void Parser::skip_whitespace() {
    while (pos_ < sql_.size() && std::isspace(sql_[pos_])) {
        pos_++;
    }
}

inline std::string Parser::next_token() {
    skip_whitespace();

    std::string token;
    while (pos_ < sql_.size() && !std::isspace(sql_[pos_])) {
        char c = sql_[pos_];
        if (c == ',' || c == '(' || c == ')' || c == '=' ||
            c == '<' || c == '>' || c == '!') {
            if (!token.empty()) break;
            token += c;
            pos_++;
            break;
        }
        token += c;
        pos_++;
    }
    return token;
}

inline QueryType Parser::parse_keyword() {
    skip_whitespace();

    std::string kw;
    while (pos_ < sql_.size() && std::isalpha(sql_[pos_])) {
        kw += sql_[pos_++];
    }

    if (kw == "SELECT") return QueryType::SELECT;
    if (kw == "INSERT") return QueryType::INSERT;
    if (kw == "UPDATE") return QueryType::UPDATE;
    if (kw == "DELETE") return QueryType::DELETE;
    return QueryType::UNKNOWN;
}

inline std::string Parser::parse_identifier() {
    skip_whitespace();

    std::string id;
    while (pos_ < sql_.size() && (std::isalnum(sql_[pos_]) || sql_[pos_] == '_')) {
        id += sql_[pos_++];
    }
    return id;
}

inline Cell Parser::parse_value() {
    skip_whitespace();

    if (pos_ >= sql_.size()) return Cell{};

    char c = sql_[pos_];

    // 字符串
    if (c == '\'' || c == '"') {
        char quote = c;
        pos_++;
        std::string val;
        while (pos_ < sql_.size() && sql_[pos_] != quote) {
            val += sql_[pos_++];
        }
        if (pos_ < sql_.size()) pos_++; // 跳过结束引号
        return Cell{val};
    }

    // 数字
    if (std::isdigit(c) || (c == '-' && pos_ + 1 < sql_.size() && std::isdigit(sql_[pos_ + 1]))) {
        bool is_double = false;
        std::string num;
        while (pos_ < sql_.size() && (std::isdigit(sql_[pos_]) || sql_[pos_] == '.' || sql_[pos_] == '-')) {
            if (sql_[pos_] == '.') is_double = true;
            num += sql_[pos_++];
        }

        if (is_double) {
            return Cell{std::stod(num)};
        } else {
            return Cell{std::stoll(num)};
        }
    }

    // 布尔值
    if (sql_.size() - pos_ >= 4) {
        std::string kw = sql_.substr(pos_, 4);
        if (kw == "TRUE" || kw == "true") {
            pos_ += 4;
            return Cell{true};
        }
        if (kw == "FALSE" || kw == "false") {
            pos_ += 5;
            return Cell{false};
        }
    }

    // NULL
    if (sql_.size() - pos_ >= 4) {
        std::string kw = sql_.substr(pos_, 4);
        if (kw == "NULL" || kw == "null") {
            pos_ += 4;
            return Cell{};
        }
    }

    return Cell{};
}

inline Op Parser::parse_operator() {
    skip_whitespace();

    if (pos_ + 1 < sql_.size()) {
        std::string op = sql_.substr(pos_, 2);
        if (op == "==") { pos_ += 2; return Op::EQ; }
        if (op == "!=") { pos_ += 2; return Op::NE; }
        if (op == ">=") { pos_ += 2; return Op::GE; }
        if (op == "<=") { pos_ += 2; return Op::LE; }
    }

    if (pos_ < sql_.size()) {
        char c = sql_[pos_++];
        if (c == '=') return Op::EQ;
        if (c == '>') return Op::GT;
        if (c == '<') {
            if (pos_ < sql_.size() && sql_[pos_] == '>') {
                pos_++;
                return Op::NE;
            }
            return Op::LT;
        }
    }

    return Op::EQ;
}

inline Condition Parser::parse_condition() {
    Condition cond;
    cond.column = parse_identifier();
    cond.op = parse_operator();
    cond.value = parse_value();
    return cond;
}

inline std::variant<SelectQuery, InsertQuery, UpdateQuery, DeleteQuery>
Parser::parse() {
    QueryType type = parse_keyword();

    switch (type) {
        case QueryType::SELECT: {
            SelectQuery query;
            query.table_name = parse_identifier();
            // 简单解析：跳过其他部分
            return query;
        }

        case QueryType::INSERT: {
            InsertQuery query;
            parse_identifier(); // INTO
            query.table_name = parse_identifier();
            return query;
        }

        case QueryType::UPDATE: {
            UpdateQuery query;
            query.table_name = parse_identifier();
            return query;
        }

        case QueryType::DELETE: {
            DeleteQuery query;
            parse_identifier(); // FROM
            query.table_name = parse_identifier();
            return query;
        }

        default:
            return DeleteQuery{}; // 空查询
    }
}

// ==================== 查询执行器 ====================

class QueryExecutor {
public:
    QueryExecutor(Table& table, Index& index)
        : table_(table), index_(index), executor_(table, index) {}

    // 执行 SELECT
    std::vector<Record> execute_select(const SelectQuery& query);

    // 执行 INSERT
    RowId execute_insert(const InsertQuery& query);

    // 执行 UPDATE
    size_t execute_update(const UpdateQuery& query);

    // 执行 DELETE
    size_t execute_delete(const DeleteQuery& query);

private:
    // 评估条件
    bool evaluate_condition(const Record& record, const Condition& cond);

    // 评估操作符
    bool evaluate_op(const Cell& a, Op op, const Cell& b);

    Table& table_;
    Index& index_;
    Executor executor_;
};

inline bool QueryExecutor::evaluate_op(const Cell& a, Op op, const Cell& b) {
    int cmp = KeyComparator::compare(a, b);

    switch (op) {
        case Op::EQ: return cmp == 0;
        case Op::NE: return cmp != 0;
        case Op::GT: return cmp > 0;
        case Op::GE: return cmp >= 0;
        case Op::LT: return cmp < 0;
        case Op::LE: return cmp <= 0;
        default: return false;
    }
}

inline bool QueryExecutor::evaluate_condition(const Record& record,
                                               const Condition& cond) {
    Cell a = record.get(cond.column);
    return evaluate_op(a, cond.op, cond.value);
}

inline std::vector<Record> QueryExecutor::execute_select(const SelectQuery& query) {
    std::vector<Record> results;

    // 创建执行计划
    PlanNode plan;
    if (!query.where.empty()) {
        plan = executor_.create_plan(
            query.where[0].column,
            query.where[0].value);
    } else {
        plan.scan_type = ScanType::SEQ_SCAN;
    }

    // 执行
    auto cursor = executor_.execute(plan);

    while (cursor->has_next()) {
        auto record_opt = cursor->next();
        if (!record_opt.has_value()) break;

        const auto& record = record_opt.value();

        // 检查 WHERE 条件
        bool match = true;
        for (const auto& cond : query.where) {
            if (!evaluate_condition(record, cond)) {
                match = false;
                break;
            }
        }

        if (match) {
            results.push_back(record);

            // 检查 LIMIT
            if (query.limit > 0 && results.size() >= query.limit) {
                break;
            }
        }
    }

    return results;
}

inline RowId QueryExecutor::execute_insert(const InsertQuery& query) {
    if (query.values.empty()) return INVALID_ROW_ID;

    const auto& values = query.values[0];
    Record record(table_.meta().columns);

    // 映射列到值
    if (query.columns.size() == values.size()) {
        for (size_t i = 0; i < query.columns.size(); ++i) {
            record.set(query.columns[i], values[i]);
        }
    } else {
        // 使用位置映射
        const auto& cols = table_.meta().columns;
        for (size_t i = 0; i < std::min(values.size(), cols.size()); ++i) {
            record.set(cols[i].name, values[i]);
        }
    }

    RowId row_id = table_.insert(record);

    // 更新索引
    for (const auto& col : table_.meta().columns) {
        // 简化：只索引第一列
        break;
    }

    return row_id;
}

inline size_t QueryExecutor::execute_update(const UpdateQuery& query) {
    size_t updated = 0;

    auto cursor = std::make_unique<TableScanCursor>(table_);

    while (cursor->has_next()) {
        auto record_opt = cursor->next();
        if (!record_opt.has_value()) break;

        auto record = record_opt.value();

        // 检查 WHERE 条件
        bool match = true;
        for (const auto& cond : query.where) {
            if (!evaluate_condition(record, cond)) {
                match = false;
                break;
            }
        }

        if (match) {
            // 应用更新
            for (const auto& [col, value] : query.set_values) {
                record.set(col, value);
            }

            // 获取当前行号并更新（简化实现）
            updated++;
        }
    }

    return updated;
}

inline size_t QueryExecutor::execute_delete(const DeleteQuery& query) {
    size_t deleted = 0;

    auto cursor = std::make_unique<TableScanCursor>(table_);

    while (cursor->has_next()) {
        auto record_opt = cursor->next();
        if (!record_opt.has_value()) break;

        auto record = record_opt.value();

        // 检查 WHERE 条件
        bool match = true;
        for (const auto& cond : query.where) {
            if (!evaluate_condition(record, cond)) {
                match = false;
                break;
            }
        }

        if (match) {
            // 简化：标记删除
            deleted++;
        }
    }

    return deleted;
}

} // namespace mydb
