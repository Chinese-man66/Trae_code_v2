#pragma once

#include "../stage1_basics/record.h"
#include "../stage1_basics/table.h"
#include "index.h"
#include <memory>
#include <vector>
#include <optional>

namespace mydb {

// ==================== 游标（用于遍历）====================

// 游标类型
enum class CursorType {
    TABLE_SCAN,  // 全表扫描
    INDEX_SCAN,  // 索引扫描
    INDEX_RANGE  // 索引范围扫描
};

// 游标抽象
class Cursor {
public:
    virtual ~Cursor() = default;

    // 是否还有下一条记录
    virtual bool has_next() const = 0;

    // 获取下一条记录
    virtual std::optional<Record> next() = 0;

    // 游标类型
    virtual CursorType type() const = 0;

    // 估计剩余记录数
    virtual size_t remaining() const = 0;
};

// 表扫描游标
class TableScanCursor : public Cursor {
public:
    explicit TableScanCursor(Table& table)
        : table_(table), current_row_(1) {
        total_rows_ = table.row_count();
    }

    bool has_next() const override {
        return current_row_ <= total_rows_;
    }

    std::optional<Record> next() override {
        if (!has_next()) return std::nullopt;

        auto record = table_.get(current_row_);
        current_row_++;
        return record;
    }

    CursorType type() const override { return CursorType::TABLE_SCAN; }

    size_t remaining() const override {
        return total_rows_ >= current_row_ ? total_rows_ - current_row_ + 1 : 0;
    }

private:
    Table& table_;
    RowId current_row_;
    size_t total_rows_;
};

// 索引扫描游标
class IndexScanCursor : public Cursor {
public:
    IndexScanCursor(Index& index, Table& table, const Cell& key)
        : index_(index), table_(table), key_(key), consumed_(false) {

        auto row_id = index.find(key);
        if (row_id.has_value()) {
            row_ids_.push_back(row_id.value());
        }
    }

    bool has_next() const override {
        return !consumed_ || current_idx_ < row_ids_.size();
    }

    std::optional<Record> next() override {
        if (current_idx_ >= row_ids_.size()) {
            consumed_ = true;
            return std::nullopt;
        }

        auto row_id = row_ids_[current_idx_++];
        return table_.get(row_id);
    }

    CursorType type() const override { return CursorType::INDEX_SCAN; }

    size_t remaining() const override {
        return row_ids_.size() >= current_idx_ ? row_ids_.size() - current_idx_ : 0;
    }

private:
    Index& index_;
    Table& table_;
    Cell key_;
    std::vector<RowId> row_ids_;
    size_t current_idx_ = 0;
    bool consumed_ = false;
};

// 索引范围扫描游标
class IndexRangeCursor : public Cursor {
public:
    IndexRangeCursor(Index& index, Table& table,
                     const Cell& start, const Cell& end)
        : index_(index), table_(table),
          start_(start), end_(end), current_idx_(0) {

        row_ids_ = index.find_range(start, end);
    }

    bool has_next() const override {
        return current_idx_ < row_ids_.size();
    }

    std::optional<Record> next() override {
        if (current_idx_ >= row_ids_.size()) {
            return std::nullopt;
        }

        auto row_id = row_ids_[current_idx_++];
        return table_.get(row_id);
    }

    CursorType type() const override { return CursorType::INDEX_RANGE; }

    size_t remaining() const override {
        return row_ids_.size() >= current_idx_ ? row_ids_.size() - current_idx_ : 0;
    }

private:
    Index& index_;
    Table& table_;
    Cell start_;
    Cell end_;
    std::vector<RowId> row_ids_;
    size_t current_idx_;
};

// ==================== 执行计划 ====================

// 扫描方式
enum class ScanType {
    SEQ_SCAN,    // 顺序扫描
    IDX_SCAN,    // 索引扫描
    IDX_RANGE    // 索引范围扫描
};

// 执行计划节点
struct PlanNode {
    ScanType scan_type;
    std::string table_name;
    std::string index_name;
    std::optional<Cell> start_key;  // 范围查询起始键
    std::optional<Cell> end_key;    // 范围查询结束键
    std::vector<std::string> target_columns; // 要返回的列
};

// 查询执行器
class Executor {
public:
    Executor(Table& table, Index& index)
        : table_(table), index_(index) {}

    // 创建执行计划
    PlanNode create_plan(const std::string& column,
                        const Cell& value,
                        bool is_range = false);

    // 执行计划，返回游标
    std::unique_ptr<Cursor> execute(const PlanNode& plan);

    // 估算扫描成本
    size_t estimate_cost(const PlanNode& plan) const;

private:
    Table& table_;
    Index& index_;
};

inline PlanNode Executor::create_plan(const std::string& column,
                                      const Cell& value,
                                      bool is_range) {
    PlanNode plan;
    plan.table_name = table_.name();
    plan.index_name = column + "_idx";
    plan.scan_type = is_range ? ScanType::IDX_RANGE : ScanType::IDX_SCAN;
    plan.start_key = value;
    plan.target_columns = {"*"}; // 所有列

    return plan;
}

inline std::unique_ptr<Cursor> Executor::execute(const PlanNode& plan) {
    switch (plan.scan_type) {
        case ScanType::SEQ_SCAN:
            return std::make_unique<TableScanCursor>(table_);

        case ScanType::IDX_SCAN:
            if (plan.start_key.has_value()) {
                return std::make_unique<IndexScanCursor>(
                    index_, table_, plan.start_key.value());
            }
            return std::make_unique<TableScanCursor>(table_);

        case ScanType::IDX_RANGE:
            if (plan.start_key.has_value()) {
                return std::make_unique<IndexRangeCursor>(
                    index_, table_, plan.start_key.value(),
                    plan.end_key.value_or(plan.start_key.value()));
            }
            return std::make_unique<TableScanCursor>(table_);

        default:
            return std::make_unique<TableScanCursor>(table_);
    }
}

inline size_t Executor::estimate_cost(const PlanNode& plan) const {
    switch (plan.scan_type) {
        case ScanType::SEQ_SCAN:
            // 全表扫描成本：读取所有页
            return table_.row_count() * 100;

        case ScanType::IDX_SCAN:
            // 索引扫描：索引成本 + 1次表查找
            return 10 + 1;

        case ScanType::IDX_RANGE:
            // 范围扫描：索引成本 + 范围行数
            return 10 + 50; // 假设返回50行

        default:
            return table_.row_count() * 100;
    }
}

} // namespace mydb
