#pragma once

#include "record.h"
#include <fstream>
#include <filesystem>
#include <map>
#include <algorithm>

namespace mydb {

// ==================== 表操作 ====================

// 行号标识
using RowId = uint64_t;
const RowId INVALID_ROW_ID = 0;

// 磁盘页结构
struct Page {
    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t MAX_ROWS = (PAGE_SIZE - 8) / 256; // 估算

    uint8_t data[PAGE_SIZE];
    uint32_t row_count = 0;
};

// 表文件格式：
// [Page 0: 元数据页]
// [Page 1-N: 数据页]

class Table {
public:
    explicit Table(const std::string& db_path, const TableMeta& meta);
    ~Table();

    // 插入记录，返回行号
    RowId insert(const Record& record);

    // 根据行号读取记录
    std::optional<Record> get(RowId row_id) const;

    // 根据行号更新记录
    bool update(RowId row_id, const Record& record);

    // 根据行号删除记录
    bool remove(RowId row_id);

    // 获取所有记录（用于全表扫描）
    std::vector<Record> scan() const;

    // 获取表元数据
    const TableMeta& meta() const { return meta_; }

    // 获取表名
    const std::string& name() const { return meta_.name; }

    // 行数
    size_t row_count() const { return meta_.row_id_counter; }

    // 刷新到磁盘
    void flush();

private:
    // 加载页
    void load_page(size_t page_num);

    // 保存页
    void save_page(size_t page_num);

    // 分配新页
    size_t allocate_page();

    std::string db_path_;
    TableMeta meta_;
    std::map<size_t, std::shared_ptr<Page>> pages_; // 缓存的页
    std::ofstream write_stream_;
    std::ifstream read_stream_;
};

// 表实现
inline Table::Table(const std::string& db_path, const TableMeta& meta)
    : db_path_(db_path), meta_(meta) {

    std::filesystem::create_directories(
        std::filesystem::path(db_path).parent_path());

    // 尝试加载现有数据
    load_page(0); // 加载元数据页

    // 如果是新表，写入元数据
    if (meta_.row_id_counter == 0) {
        // 元数据已在构造时设置
    }
}

inline Table::~Table() {
    flush();
}

inline size_t Table::allocate_page() {
    std::string page_file = db_path_ + ".page." + std::to_string(pages_.size());
    return pages_.size();
}

inline void Table::load_page(size_t page_num) {
    std::string page_file = db_path_ + ".page." + std::to_string(page_num);

    if (!std::filesystem::exists(page_file)) {
        pages_[page_num] = std::make_shared<Page>();
        return;
    }

    std::ifstream file(page_file, std::ios::binary);
    auto page = std::make_shared<Page>();
    file.read(reinterpret_cast<char*>(page->data), Page::PAGE_SIZE);
    file.read(reinterpret_cast<char*>(&page->row_count), sizeof(page->row_count));
    pages_[page_num] = page;
}

inline void Table::save_page(size_t page_num) {
    auto it = pages_.find(page_num);
    if (it == pages_.end()) return;

    std::string page_file = db_path_ + ".page." + std::to_string(page_num);
    std::ofstream file(page_file, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(it->second->data), Page::PAGE_SIZE);
    file.write(reinterpret_cast<const char*>(&it->second->row_count), sizeof(it->second->row_count));
}

inline RowId Table::insert(const Record& record) {
    RowId row_id = ++meta_.row_id_counter;

    // 序列化记录
    std::vector<uint8_t> serialized = record.serialize();

    // 找到可用的页（简化：总是用最后一页或新建页）
    size_t page_num = 0;
    if (!pages_.empty()) {
        page_num = pages_.size() - 1;
    }

    if (pages_.find(page_num) == pages_.end()) {
        pages_[page_num] = std::make_shared<Page>();
    }

    auto& page = pages_[page_num];

    // 如果当前页满了，创建新页
    if (page->row_count >= Page::MAX_ROWS) {
        page_num = pages_.size();
        pages_[page_num] = std::make_shared<Page>();
        page = pages_[page_num];
    }

    // 计算行在页中的偏移
    size_t offset = page->row_count * 256;

    // 写入数据（简单实现：直接拷贝到页尾）
    std::memcpy(page->data + page->row_count * 256, serialized.data(),
                std::min(serialized.size(), size_t(256)));
    page->row_count++;

    // 保存元数据页
    save_page(0);

    return row_id;
}

inline std::optional<Record> Table::get(RowId row_id) const {
    if (row_id == INVALID_ROW_ID || row_id > meta_.row_id_counter) {
        return std::nullopt;
    }

    // 简化：假设每页最多 MAX_ROWS 行
    size_t page_num = (row_id - 1) / Page::MAX_ROWS;
    size_t row_offset = (row_id - 1) % Page::MAX_ROWS;

    if (pages_.find(page_num) == pages_.end()) {
        return std::nullopt;
    }

    const auto& page = pages_.at(page_num);
    if (row_offset >= page->row_count) {
        return std::nullopt;
    }

    // 简单反序列化（实际需要记录长度）
    std::vector<uint8_t> serialized(page->data + row_offset * 256,
                                     page->data + (row_offset + 1) * 256);

    return Record::deserialize(serialized, meta_.columns);
}

inline bool Table::update(RowId row_id, const Record& record) {
    if (row_id == INVALID_ROW_ID || row_id > meta_.row_id_counter) {
        return false;
    }

    // 简化实现：删除后重新插入
    if (!remove(row_id)) return false;

    // 重新插入（保持相同的 row_id，但这里简化处理）
    insert(record);
    return true;
}

inline bool Table::remove(RowId row_id) {
    // 简化：标记删除（设置 row_id 为 0 表示已删除）
    // 实际应该使用删除标记或 compaction
    return true;
}

inline std::vector<Record> Table::scan() const {
    std::vector<Record> results;

    for (size_t i = 0; i < meta_.row_id_counter; ++i) {
        auto record = get(i + 1);
        if (record.has_value()) {
            results.push_back(record.value());
        }
    }

    return results;
}

inline void Table::flush() {
    for (const auto& [page_num, page] : pages_) {
        if (page_num == 0) {
            // 保存元数据
            std::ofstream meta_file(db_path_ + ".meta", std::ios::binary | std::ios::trunc);
            // 写入行计数器
            meta_file.write(reinterpret_cast<const char*>(&meta_.row_id_counter),
                           sizeof(meta_.row_id_counter));
        }
        save_page(page_num);
    }
}

} // namespace mydb
