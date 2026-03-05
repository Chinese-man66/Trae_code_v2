/**
 * Stage 2: 进阶阶段 - 索引与查询优化
 *
 * 本阶段学习目标：
 * 1. 理解数据库索引：哈希索引、B+ 树索引
 * 2. 实现查询执行计划与成本估算
 * 3. 理解游标与迭代器模式
 * 4. 实现简单的 SQL 解析器
 */

#include "record.h"
#include "table.h"
#include "index.h"
#include "cursor.h"
#include "query.h"
#include <iostream>
#include <chrono>

using namespace mydb;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

// ==================== 演示：索引基础 ====================

void demo_index_basics() {
    print_separator("1. 索引基础演示");

    // 创建哈希索引
    HashIndex hash_idx("id_hash_idx", "id");

    // 插入测试数据
    std::vector<RowId> row_ids = {1, 5, 3, 7, 2, 8, 4, 6, 9, 10};

    for (size_t i = 0; i < row_ids.size(); ++i) {
        hash_idx.insert(Cell{static_cast<int64_t>(row_ids[i] * 10)}, row_ids[i]);
    }

    std::cout << "哈希索引插入 10 条记录\n";

    // 查询
    auto result = hash_idx.find(Cell{static_cast<int64_t>(50)});
    if (result.has_value()) {
        std::cout << "查找 key=50: row_id = " << result.value() << "\n";
    } else {
        std::cout << "查找 key=50: 未找到\n";
    }

    // 范围查询（哈希索引不支持）
    auto range = hash_idx.find_range(Cell{static_cast<int64_t>(20)},
                                      Cell{static_cast<int64_t>(80)});
    std::cout << "范围查询 [20, 80): 找到 " << range.size() << " 条\n";
}

// ==================== 演示：B+ 树索引 ====================

void demo_bplus_tree() {
    print_separator("2. B+ 树索引演示");

    BPlusTreeIndex btree_idx("id_btree_idx", "id", 4);

    // 插入有序数据
    std::vector<int64_t> keys = {10, 20, 5, 6, 12, 30, 7, 8, 25, 15};

    std::cout << "插入数据: ";
    for (auto k : keys) {
        std::cout << k << " ";
        btree_idx.insert(Cell{k}, static_cast<RowId>(k));
    }
    std::cout << "\n";

    // 查询
    auto find_result = btree_idx.find(Cell{static_cast<int64_t>(25)});
    if (find_result.has_value()) {
        std::cout << "精确查询 key=25: row_id = " << find_result.value() << "\n";
    }

    // 范围查询
    auto range_result = btree_idx.find_range(
        Cell{static_cast<int64_t>(10)},
        Cell{static_cast<int64_t>(25)}
    );

    std::cout << "范围查询 [10, 25): 找到 " << range_result.size() << " 条\n";
    std::cout << "结果: ";
    for (auto r : range_result) {
        std::cout << r << " ";
    }
    std::cout << "\n";
}

// ==================== 演示：游标与执行计划 ====================

void demo_cursor_and_plan() {
    print_separator("3. 游标与执行计划");

    // 创建表
    TableMeta student_meta;
    student_meta.name = "students";
    student_meta.columns = {
        {"id", Column::Type::INTEGER, false, Cell{0}},
        {"name", Column::Type::TEXT, false, Cell{std::string("")}},
        {"score", Column::Type::DOUBLE, true, Cell{0.0}}
    };

    Table table("./data/stage2/students", student_meta);

    // 插入数据
    for (int i = 1; i <= 100; ++i) {
        Record r(student_meta.columns);
        r.set("id", Cell{static_cast<int64_t>(i)});
        r.set("name", Cell{std::string("Student") + std::to_string(i)});
        r.set("score", Cell{60.0 + (i % 40)});
        table.insert(r);
    }

    std::cout << "插入 100 条学生记录\n";

    // 创建索引
    HashIndex index("score_idx", "score");

    // 执行计划演示
    std::cout << "\n--- 执行计划分析 ---\n";

    // 计划1: 全表扫描
    PlanNode plan1;
    plan1.scan_type = ScanType::SEQ_SCAN;
    plan1.table_name = "students";

    std::cout << "计划1 (SEQ_SCAN):\n";
    std::cout << "  预计成本: " << estimate_cost(plan1, table) << "\n";

    // 计划2: 索引扫描
    PlanNode plan2;
    plan2.scan_type = ScanType::IDX_SCAN;
    plan2.table_name = "students";
    plan2.start_key = Cell{static_cast<int64_t>(80)};

    std::cout << "计划2 (IDX_SCAN):\n";
    std::cout << "  预计成本: " << estimate_cost(plan2, table) << "\n";

    // 使用游标遍历
    std::cout << "\n--- 使用游标遍历 ---\n";

    auto cursor = std::make_unique<TableScanCursor>(table);
    size_t count = 0;
    double sum = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (cursor->has_next()) {
        auto rec_opt = cursor->next();
        if (rec_opt.has_value()) {
            sum += std::get<double>(rec_opt->get("score").value());
            count++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "遍历 " << count << " 条记录\n";
    std::cout << "平均分: " << (sum / count) << "\n";
    std::cout << "耗时: " << duration.count() << " 微秒\n";
}

// 辅助函数：估算成本
size_t estimate_cost(const PlanNode& plan, Table& table) {
    switch (plan.scan_type) {
        case ScanType::SEQ_SCAN:
            return table.row_count() * 10;  // 每行10单位成本
        case ScanType::IDX_SCAN:
            return 20;  // 索引查找成本
        case ScanType::IDX_RANGE:
            return 50;   // 范围扫描成本
        default:
            return table.row_count() * 10;
    }
}

// ==================== 演示：SQL 解析 ====================

void demo_sql_parser() {
    print_separator("4. SQL 解析演示");

    // 解析 SELECT
    Parser select_parser("SELECT * FROM students WHERE score >= 80");
    auto select_result = select_parser.parse();

    std::cout << "解析: SELECT * FROM students WHERE score >= 80\n";

    // 解析 INSERT
    Parser insert_parser("INSERT INTO students (id, name, score) VALUES (1, 'Alice', 95.5)");
    auto insert_result = insert_parser.parse();

    std::cout << "解析: INSERT INTO students (id, name, score) VALUES (1, 'Alice', 95.5)\n";

    // 解析 UPDATE
    Parser update_parser("UPDATE students SET score = 100 WHERE id = 1");
    auto update_result = update_parser.parse();

    std::cout << "解析: UPDATE students SET score = 100 WHERE id = 1\n";

    // 解析 DELETE
    Parser delete_parser("DELETE FROM students WHERE score < 60");
    auto delete_result = delete_parser.parse();

    std::cout << "解析: DELETE FROM students WHERE score < 60\n";
}

// ==================== 演示：查询执行 ====================

void demo_query_executor() {
    print_separator("5. 查询执行演示");

    // 创建表和索引
    TableMeta student_meta;
    student_meta.name = "students";
    student_meta.columns = {
        {"id", Column::Type::INTEGER, false, Cell{0}},
        {"name", Column::Type::TEXT, false, Cell{std::string("")}},
        {"age", Column::Type::INTEGER, true, Cell{18}},
        {"score", Column::Type::DOUBLE, true, Cell{0.0}}
    };

    Table table("./data/stage2/students2", student_meta);
    HashIndex index("id_idx", "id");

    // 插入数据
    for (int i = 1; i <= 10; ++i) {
        Record r(student_meta.columns);
        r.set("id", Cell{static_cast<int64_t>(i)});
        r.set("name", Cell{std::string("Student") + std::to_string(i)});
        r.set("age", Cell{static_cast<int64_t>(18 + i)});
        r.set("score", Cell{70.0 + i * 2.5});
        table.insert(r);
        index.insert(Cell{static_cast<int64_t>(i)}, static_cast<RowId>(i));
    }

    std::cout << "插入 10 条学生记录\n\n";

    // 创建查询执行器
    QueryExecutor executor(table, index);

    // SELECT 查询
    std::cout << "--- SELECT 查询 ---\n";
    SelectQuery select_query;
    select_query.table_name = "students";
    select_query.where = {
        {"score", Op::GE, Cell{85.0}}
    };
    select_query.limit = 5;

    auto results = executor.execute_select(select_query);
    std::cout << "找到 " << results.size() << " 条记录 (score >= 85):\n";
    for (const auto& r : results) {
        std::cout << "  " << r.to_string() << "\n";
    }

    // 条件评估演示
    std::cout << "\n--- 条件评估演示 ---\n";
    Record test_rec(student_meta.columns);
    test_rec.set("id", Cell{1});
    test_rec.set("name", Cell{std::string("Test")});
    test_rec.set("age", Cell{20});
    test_rec.set("score", Cell{90.0});

    Condition cond = {"score", Op::GE, Cell{85.0}};
    bool matches = executor.evaluate_condition(test_rec, cond);
    std::cout << "记录 " << test_rec.to_string() << "\n";
    std::cout << "条件 score >= 85: " << (matches ? "满足" : "不满足") << "\n";
}

// 添加 QueryExecutor 的 evaluate_condition 声明
namespace mydb {
class QueryExecutor;
bool QueryExecutor::* evaluate_condition(const Record&, const Condition&);
} // namespace mydb

// ==================== 主函数 ====================

int main() {
    std::cout << "======================================\n";
    std::cout << "   MyDB - Stage 2: 进阶阶段\n";
    std::cout << "   索引与查询优化\n";
    std::cout << "======================================\n";

    // 演示 1: 索引基础
    demo_index_basics();

    // 演示 2: B+ 树
    demo_bplus_tree();

    // 演示 3: 游标与执行计划
    demo_cursor_and_plan();

    // 演示 4: SQL 解析
    demo_sql_parser();

    // 演示 5: 查询执行
    demo_query_executor();

    print_separator("Stage 2 完成!");
    std::cout << "\n学习要点：\n";
    std::cout << "  1. 哈希索引：O(1) 查找，不支持范围查询\n";
    std::cout << "  2. B+ 树索引：O(log n) 查找，支持范围查询\n";
    std::cout << "  3. 游标模式：统一遍历接口，延迟加载\n";
    std::cout << "  4. 执行计划：选择最优查询路径\n";
    std::cout << "  5. SQL 解析：词法分析 + 语法分析\n";
    std::cout << "\n下一阶段：高级阶段 - 事务与并发\n";

    return 0;
}
