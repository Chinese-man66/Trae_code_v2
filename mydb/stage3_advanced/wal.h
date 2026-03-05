#pragma once

#include "transaction.h"
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <chrono>
#include <filesystem>

namespace mydb {

// ==================== WAL 日志 ====================

// 日志记录类型
enum class LogType {
    BEGIN,           // 事务开始
    COMMIT,          // 事务提交
    ABORT,           // 事务中止
    UPDATE,          // 更新操作
    INSERT,          // 插入操作
    DELETE,          // 删除操作
    CHECKPOINT       // 检查点
};

// 日志记录
struct LogRecord {
    uint64_t lsn;           // 日志序列号
    uint64_t transaction_id;
    LogType type;
    uint64_t prev_lsn;      // 前一个日志记录的 LSN
    uint64_t undo_next_lsn; // 可回滚的下一个 LSN

    // 业务数据
    std::string table_name;
    RowId row_id;
    std::vector<uint8_t> before_image; // 修改前的数据
    std::vector<uint8_t> after_image;  // 修改后的数据
};

// WAL (Write-Ahead Log) 管理器
class WALManager {
public:
    explicit WALManager(const std::string& log_dir);

    // 追加日志
    uint64_t append_log(const LogRecord& record);

    // 刷盘
    void flush();

    // 获取检查点位置
    uint64_t get_checkpoint_lsn() const { return checkpoint_lsn_; }

    // 设置检查点
    void set_checkpoint(uint64_t lsn) { checkpoint_lsn_ = lsn; }

    // 获取所有日志记录（用于恢复）
    std::vector<LogRecord> get_all_logs() const;

    // 读取日志文件
    void recover();

private:
    // 序列化日志记录
    std::vector<uint8_t> serialize(const LogRecord& record) const;

    // 反序列化日志记录
    LogRecord deserialize(const std::vector<uint8_t>& data) const;

    std::string log_dir_;
    std::ofstream log_stream_;
    std::mutex mutex_;
    uint64_t current_lsn_ = 0;
    uint64_t checkpoint_lsn_ = 0;
    std::string current_log_file_;
};

// WAL 实现
inline WALManager::WALManager(const std::string& log_dir)
    : log_dir_(log_dir) {

    std::filesystem::create_directories(log_dir_);

    // 创建日志文件
    auto now = std::chrono::system_clock::now();
    auto_t time = std::chrono::system_clock::to_time_t(now);
    current_log_file_ = log_dir_ + "/wal_" + std::to_string(time_t) + ".log";

    log_stream_.open(current_log_file_, std::ios::binary | std::ios::app);
}

inline uint64_t WALManager::append_log(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);

    LogRecord r = record;
    r.lsn = ++current_lsn_;

    auto data = serialize(r);
    log_stream_.write(reinterpret_cast<const char*>(data.data()), data.size());
    log_stream_.flush();

    return r.lsn;
}

inline void WALManager::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    log_stream_.flush();
}

inline std::vector<LogRecord> WALManager::get_all_logs() const {
    std::vector<LogRecord> records;

    if (!std::filesystem::exists(current_log_file_)) {
        return records;
    }

    std::ifstream file(current_log_file_, std::ios::binary);
    std::vector<uint8_t> buffer(Page::PAGE_SIZE);

    while (file.peek() != EOF) {
        // 简单读取：假设日志记录长度已知
        // 实际需要更复杂的解析
    }

    return records;
}

inline void WALManager::recover() {
    // 读取所有日志记录
    auto records = get_all_logs();

    // 分析阶段：确定需要重做的事务
    std::set<uint64_t> committed_transactions;

    // 回放阶段：从检查点开始重做
    for (const auto& record : records) {
        if (record.lsn < checkpoint_lsn_) continue;

        switch (record.type) {
            case LogType::COMMIT:
            case LogType::ABORT:
                // 标记事务结束
                break;

            case LogType::UPDATE:
            case LogType::INSERT:
            case LogType::DELETE:
                // 应用日志
                break;

            default:
                break;
        }
    }

    // 撤销未提交事务
    for (const auto& record : records) {
        if (committed_transactions.count(record.transaction_id)) {
            continue;
        }

        // 撤销操作
    }
}

// 日志序列化实现
inline std::vector<uint8_t> WALManager::serialize(const LogRecord& record) const {
    std::vector<uint8_t> data;

    // 写入基本字段
    auto write_u64 = [&data](uint64_t v) {
        data.insert(data.end(),
                   reinterpret_cast<const uint8_t*>(&v),
                   reinterpret_cast<const uint8_t*>(&v) + sizeof(v));
    };

    write_u64(record.lsn);
    write_u64(record.transaction_id);
    write_u64(static_cast<uint64_t>(record.type));
    write_u64(record.prev_lsn);
    write_u64(record.undo_next_lsn);

    // 写入表名
    write_u64(static_cast<uint64_t>(record.table_name.size()));
    data.insert(data.end(), record.table_name.begin(), record.table_name.end());

    // 写入行号
    write_u64(record.row_id);

    // 写入 before_image
    write_u64(static_cast<uint64_t>(record.before_image.size()));
    data.insert(data.end(), record.before_image.begin(), record.before_image.end());

    // 写入 after_image
    write_u64(static_cast<uint64_t>(record.after_image.size()));
    data.insert(data.end(), record.after_image.begin(), record.after_image.end());

    return data;
}

inline LogRecord WALManager::deserialize(const std::vector<uint8_t>& data) const {
    LogRecord record;
    size_t offset = 0;

    auto read_u64 = [&data, &offset]() -> uint64_t {
        uint64_t v;
        std::memcpy(&v, data.data() + offset, sizeof(v));
        offset += sizeof(v);
        return v;
    };

    record.lsn = read_u64();
    record.transaction_id = read_u64();
    record.type = static_cast<LogType>(read_u64());
    record.prev_lsn = read_u64();
    record.undo_next_lsn = read_u64();

    size_t name_len = read_u64();
    record.table_name = std::string(data.begin() + offset,
                                     data.begin() + offset + name_len);
    offset += name_len;

    record.row_id = read_u64();

    size_t before_len = read_u64();
    record.before_image = std::vector<uint8_t>(data.begin() + offset,
                                                data.begin() + offset + before_len);
    offset += before_len;

    size_t after_len = read_u64();
    record.after_image = std::vector<uint8_t>(data.begin() + offset,
                                               data.begin() + offset + after_len);

    return record;
}

// ==================== 恢复管理器 ====================

class RecoveryManager {
public:
    RecoveryManager(WALManager& wal, Table& table)
        : wal_(wal), table_(table) {}

    // 事务开始
    void begin(Transaction& tx);

    // 记录更新前镜像
    void log_before_image(Transaction& tx, RowId row_id,
                         const std::vector<uint8_t>& before);

    // 记录更新后镜像
    void log_after_image(Transaction& tx, RowId row_id,
                        const std::vector<uint8_t>& after);

    // 事务提交
    void commit(Transaction& tx);

    // 事务中止
    void abort(Transaction& tx);

    // 创建检查点
    void checkpoint();

    // 系统崩溃恢复
    void recover();

private:
    WALManager& wal_;
    Table& table_;
};

// 恢复管理器实现
inline void RecoveryManager::begin(Transaction& tx) {
    LogRecord record;
    record.transaction_id = tx.id();
    record.type = LogType::BEGIN;
    record.prev_lsn = 0;
    record.undo_next_lsn = 0;

    wal_.append_log(record);
}

inline void RecoveryManager::log_before_image(Transaction& tx, RowId row_id,
                                                const std::vector<uint8_t>& before) {
    LogRecord record;
    record.transaction_id = tx.id();
    record.type = LogType::UPDATE;
    record.table_name = table_.name();
    record.row_id = row_id;
    record.before_image = before;

    wal_.append_log(record);
}

inline void RecoveryManager::log_after_image(Transaction& tx, RowId row_id,
                                               const std::vector<uint8_t>& after) {
    LogRecord record;
    record.transaction_id = tx.id();
    record.type = LogType::UPDATE;
    record.table_name = table_.name();
    record.row_id = row_id;
    record.after_image = after;

    wal_.append_log(record);
}

inline void RecoveryManager::commit(Transaction& tx) {
    LogRecord record;
    record.transaction_id = tx.id();
    record.type = LogType::COMMIT;

    wal_.append_log(record);
    wal_.flush();

    tx.commit();
}

inline void RecoveryManager::abort(Transaction& tx) {
    // 回滚所有修改
    // 简化实现

    LogRecord record;
    record.transaction_id = tx.id();
    record.type = LogType::ABORT;

    wal_.append_log(record);
    wal_.flush();

    tx.abort();
}

inline void RecoveryManager::checkpoint() {
    // 刷新所有脏页
    // 写入检查点日志
    LogRecord record;
    record.transaction_id = 0;
    record.type = LogType::CHECKPOINT;

    wal_.append_log(record);
    wal_.set_checkpoint(record.lsn);
    wal_.flush();
}

inline void RecoveryManager::recover() {
    // 重新读取日志文件
    wal_.recover();
}

} // namespace mydb
