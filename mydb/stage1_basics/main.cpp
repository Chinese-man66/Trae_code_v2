/**
 * Stage 1: 基础阶段 - 基本存储与 CRUD
 *
 * 本阶段学习目标：
 * 1. 理解数据库基本概念：表、记录、列
 * 2. 掌握 C++ 类设计和内存管理
 * 3. 实现基本的增删改查操作
 * 4. 理解数据序列化与反序列化
 */

#include "record.h"
#include "table.h"
#include <iostream>
#include <cassert>

using namespace mydb;

// ==================== 辅助函数 ====================

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

// ==================== Stage 1 演示 ====================

void demo_basic_types() {
    print_separator("1. 基本数据类型演示");

    // 创建表元数据：学生表
    TableMeta student_meta;
    student_meta.name = "students";

    // 定义列
    student_meta.columns = {
        {"id", Column::Type::INTEGER, false, 0},
        {"name", Column::Type::TEXT, false, ""},
        {"age", Column::Type::INTEGER, true, 18},
        {"score", Column::Type::DOUBLE, true, 0.0},
        {"active", Column::Type::BOOLEAN, true, true}
    };

    std::cout << "表结构: students\n";
    std::cout << "  id (INTEGER, NOT NULL)\n";
    std::cout << "  name (TEXT, NOT NULL)\n";
    std::cout << "  age (INTEGER, DEFAULT 18)\n";
    std::cout << "  score (DOUBLE)\n";
    std::cout << "  active (BOOLEAN)\n";
}

void demo_record_operations() {
    print_separator("2. 记录操作演示");

    // 创建记录
    std::vector<Column> schema = {
        {"id", Column::Type::INTEGER},
        {"name", Column::Type::TEXT},
        {"age", Column::Type::INTEGER}
    };

    Record record(schema);

    // 设置字段值
    record.set("id", Cell{1001});
    record.set("name", Cell{std::string("Alice")});
    record.set("age", Cell{20});

    std::cout << "插入记录:\n";
    std::cout << "  " << record.to_string() << "\n";

    // 读取字段值
    Cell id = record.get("id");
    Cell name = record.get("name");

    std::cout << "\n读取字段:\n";
    std::cout << "  id = " << CellToString(id) << "\n";
    std::cout << "  name = " << CellToString(name) << "\n";

    // 序列化
    std::vector<uint8_t> serialized = record.serialize();
    std::cout << "\n序列化后的字节数: " << serialized.size() << "\n";

    // 反序列化
    Record restored = Record::deserialize(serialized, schema);
    std::cout << "反序列化后: " << restored.to_string() << "\n";
}

void demo_table_operations() {
    print_separator("3. 表操作演示");

    // 创建学生表
    TableMeta student_meta;
    student_meta.name = "students";
    student_meta.columns = {
        {"id", Column::Type::INTEGER, false, Cell{0}},
        {"name", Column::Type::TEXT, false, Cell{std::string("")}},
        {"age", Column::Type::INTEGER, true, Cell{18}},
        {"score", Column::Type::DOUBLE, true, Cell{0.0}}
    };

    Table table("./data/stage1/students", student_meta);

    // 插入数据
    std::cout << "插入 5 条学生记录:\n";

    for (int i = 1; i <= 5; ++i) {
        Record r(student_meta.columns);
        r.set("id", Cell{static_cast<int64_t>(1000 + i)});
        r.set("name", Cell{std::string("Student") + std::to_string(i)});
        r.set("age", Cell{static_cast<int64_t>(18 + i)});
        r.set("score", Cell{80.0 + i * 2.5});

        RowId row_id = table.insert(r);
        std::cout << "  插入 row_id=" << row_id << ": " << r.to_string() << "\n";
    }

    std::cout << "\n表中共 " << table.row_count() << " 条记录\n";

    // 查询数据
    std::cout << "\n查询 row_id=3:\n";
    auto record = table.get(3);
    if (record.has_value()) {
        std::cout << "  " << record->to_string() << "\n";
    }

    // 全表扫描
    std::cout << "\n全表扫描:\n";
    auto all_records = table.scan();
    for (const auto& r : all_records) {
        std::cout << "  " << r.to_string() << "\n";
    }

    // 更新数据
    std::cout << "\n更新 row_id=2:\n";
    Record update_r(student_meta.columns);
    update_r.set("id", Cell{1002});
    update_r.set("name", Cell{std::string("Alice Updated")});
    update_r.set("age", Cell{static_cast<int64_t>(22)});
    update_r.set("score", Cell{95.5});

    if (table.update(2, update_r)) {
        auto updated = table.get(2);
        if (updated.has_value()) {
            std::cout << "  " << updated->to_string() << "\n";
        }
    }

    // 再次扫描
    std::cout << "\n更新后全表扫描:\n";
    for (const auto& r : table.scan()) {
        std::cout << "  " << r.to_string() << "\n";
    }
}

void demo_serialization() {
    print_separator("4. 序列化格式演示");

    // 演示不同类型的序列化
    std::vector<Column> schema = {
        {"id", Column::Type::INTEGER},
        {"name", Column::Type::TEXT},
        {"price", Column::Type::DOUBLE},
        {"active", Column::Type::BOOLEAN},
        {"null_val", Column::Type::INTEGER}
    };

    Record r(schema);
    r.set("id", Cell{42});
    r.set("name", Cell{std::string("Product")});
    r.set("price", Cell{19.99});
    r.set("active", Cell{true});
    // null_val 保持默认值 NULL

    std::cout << "原始记录: " << r.to_string() << "\n";

    std::vector<uint8_t> data = r.serialize();
    std::cout << "序列化后: " << data.size() << " 字节\n";

    // 打印字节内容
    std::cout << "字节内容: [";
    for (size_t i = 0; i < std::min(data.size(), size_t(30)); ++i) {
        std::cout << std::hex << (int)data[i] << " ";
    }
    std::cout << std::dec << "]\n";

    Record restored = Record::deserialize(data, schema);
    std::cout << "恢复记录: " << restored.to_string() << "\n";
}

// ==================== 主函数 ====================

int main() {
    std::cout << "======================================\n";
    std::cout << "   MyDB - Stage 1: 基础阶段\n";
    std::cout << "   基本存储与 CRUD 操作\n";
    std::cout << "======================================\n";

    // 演示 1: 基本数据类型
    demo_basic_types();

    // 演示 2: 记录操作
    demo_record_operations();

    // 演示 3: 表操作
    demo_table_operations();

    // 演示 4: 序列化
    demo_serialization();

    print_separator("Stage 1 完成!");
    std::cout << "\n学习要点：\n";
    std::cout << "  1. Cell 变体类型实现多态数据存储\n";
    std::cout << "  2. Record 类封装行数据操作\n";
    std::cout << "  3. Table 类实现基本的文件存储\n";
    std::cout << "  4. 序列化为二进制格式便于持久化\n";
    std::cout << "\n下一阶段：进阶阶段 - 索引与查询优化\n";

    return 0;
}
