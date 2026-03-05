#pragma once

#include "../stage1_basics/record.h"
#include "../stage1_basics/table.h"
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <set>
#include <optional>
#include <functional>
#include <memory>

namespace mydb {

// ==================== 事务管理 ====================

// 事务状态
enum class TransactionState {
    ACTIVE,      // 活跃
    COMMITTED,   // 已提交
    ABORTED      // 已中止
};

// 事务隔离级别
enum class IsolationLevel {
    READ_UNCOMMITTED,  // 读未提交
    READ_COMMITTED,    // 读已提交
    REPEATABLE_READ,   // 可重复读
    SERIALIZABLE       // 串行化
};

// 事务
class Transaction {
public:
    explicit Transaction(TransactionId tid, IsolationLevel level)
        : tid_(tid), isolation_level_(level), state_(TransactionState::ACTIVE) {
        start_timestamp_ = std::chrono::system_clock::now().time_since_epoch().count();
    }

    TransactionId id() const { return tid_; }
    IsolationLevel isolation_level() const { return isolation_level_; }
    TransactionState state() const { return state_; }

    void commit() { state_ = TransactionState::COMMITTED; }
    void abort() { state_ = TransactionState::ABORTED; }

    bool is_active() const { return state_ == TransactionState::ACTIVE; }

    // 获取/设置全局事务ID
    static TransactionId get_next_tid() {
        static std::atomic<TransactionId> counter{0};
        return ++counter;
    }

    uint64_t start_timestamp() const { return start_timestamp_; }

private:
    TransactionId tid_;
    IsolationLevel isolation_level_;
    TransactionState state_;
    uint64_t start_timestamp_;
};

// ==================== 锁管理器 ====================

// 锁类型
enum class LockType {
    SHARED,     // 读锁
    EXCLUSIVE   // 写锁
};

// 锁模式
enum class LockMode {
    LOCK_IS,    // 意图共享
    LOCK_IX,    // 意图排他
    LOCK_S,     // 共享
    LOCK_X,     // 排他
    LOCK_NOT    // 无锁
};

// 锁请求
struct LockRequest {
    TransactionId tid;
    LockType type;
    bool granted = false;
};

// 锁元素
struct LockEntry {
    std::string resource_id;
    LockType lock_type;
    std::set<TransactionId> holders;        // 持有者
    std::vector<LockRequest> wait 等待队列
_queue;   //};

// 锁管理器
class LockManager {
public:
    LockManager() = default;

    // 获取锁
    bool lock(TransactionId tid, const std::string& resource_id, LockType type);

    // 释放锁
    bool unlock(TransactionId tid, const std::string& resource_id);

    // 释放事务的所有锁
    void unlock_all(TransactionId tid);

    // 检查是否有冲突
    bool has_conflict(const LockEntry& entry, const LockRequest& request);

    // 死锁检测
    bool detect_deadlock();

    // 获取等待锁的事务
    std::set<TransactionId> get_blocked_transactions() const;

private:
    std::unordered_map<std::string, LockEntry> locks_;
    std::unordered_map<TransactionId, std::set<std::string>> transaction_locks_;
    std::mutex mutex_;
};

// 锁管理器实现
inline bool LockManager::lock(TransactionId tid, const std::string& resource_id, LockType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    LockRequest request{tid, type, false};

    // 如果资源没有锁，直接获取
    auto it = locks_.find(resource_id);
    if (it == locks_.end()) {
        LockEntry entry;
        entry.resource_id = resource_id;
        entry.lock_type = type;
        entry.holders.insert(tid);
        request.granted = true;
        locks_[resource_id] = entry;
        transaction_locks_[tid].insert(resource_id);
        return true;
    }

    // 检查冲突
    if (!has_conflict(it->second, request)) {
        request.granted = true;
        it->second.holders.insert(tid);
        transaction_locks_[tid].insert(resource_id);
        return true;
    }

    // 加入等待队列
    it->second.wait_queue.push_back(request);
    return false;
}

inline bool LockManager::unlock(TransactionId tid, const std::string& resource_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = locks_.find(resource_id);
    if (it == locks_.end()) return false;

    it->second.holders.erase(tid);
    transaction_locks_[tid].erase(resource_id);

    // 如果有等待者，尝试授予锁
    if (!it->second.wait_queue.empty() && it->second.holders.empty()) {
        auto& request = it->second.wait_queue.front();
        if (!has_conflict(it->second, request)) {
            request.granted = true;
            it->second.holders.insert(request.tid);
            transaction_locks_[request.tid].insert(resource_id);
            it->second.wait_queue.erase(it->second.wait_queue.begin());
        }
    }

    // 清理空资源
    if (it->second.holders.empty() && it->second.wait_queue.empty()) {
        locks_.erase(it);
    }

    return true;
}

inline void LockManager::unlock_all(TransactionId tid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = transaction_locks_.find(tid);
    if (it == transaction_locks_.end()) return;

    for (const auto& resource_id : it->second) {
        unlock(tid, resource_id);
    }

    transaction_locks_.erase(it);
}

inline bool LockManager::has_conflict(const LockEntry& entry, const LockRequest& request) {
    // 排他锁与任何锁都冲突
    if (entry.lock_type == LockType::EXCLUSIVE) {
        return true;
    }

    // 共享锁只与排他锁冲突
    if (request.type == LockType::EXCLUSIVE) {
        return true;
    }

    return false;
}

inline bool LockManager::detect_deadlock() {
    // 简化：超时检测
    return false;
}

inline std::set<TransactionId> LockManager::get_blocked_transactions() const {
    std::set<TransactionId> blocked;

    for (const auto& [resource_id, entry] : locks_) {
        for (const auto& request : entry.wait_queue) {
            blocked.insert(request.tid);
        }
    }

    return blocked;
}

// ==================== 缓冲池 ====================

// 缓冲池页
struct BufferFrame {
    uint64_t page_id = INVALID_PAGE_ID;
    std::vector<uint8_t> data;
    bool dirty = false;
    int pin_count = 0;
    std::mutex mutex;
};

constexpr uint64_t INVALID_PAGE_ID = 0;

// 缓冲池
class BufferPool {
public:
    explicit BufferPool(size_t pool_size = 100)
        : pool_size_(pool_size), frame_id_(1) {
        clock_hand_ = 0;
    }

    // 分配新页
    uint64_t allocate_page();

    // 获取页
    std::shared_ptr<BufferFrame> get_page(uint64_t page_id);

    // 固定页（防止被淘汰）
    void pin(uint64_t page_id);

    // 取消固定
    void unpin(uint64_t page_id);

    // 标记脏页
    void mark_dirty(uint64_t page_id);

    // 刷新所有脏页
    void flush_all();

    // 淘汰页
    uint64_t evict();

    // 大小
    size_t size() const { return pool_size_; }

private:
    // 时钟算法淘汰
    uint64_t clock_evict();

    size_t pool_size_;
    std::unordered_map<uint64_t, std::shared_ptr<BufferFrame>> frames_;
    std::unordered_map<uint64_t, uint64_t> page_to_frame_;
    std::vector<uint64_t> free_list_;
    std::mutex mutex_;
    size_t clock_hand_;
    std::atomic<uint64_t> frame_id_;
};

// 缓冲池实现
inline uint64_t BufferPool::allocate_page() {
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t page_id = frame_id_++;

    // 如果有空闲帧，优先使用
    if (!free_list_.empty()) {
        uint64_t frame_id = free_list_.back();
        free_list_.pop_back();

        auto frame = std::make_shared<BufferFrame>();
        frame->page_id = page_id;
        frame->data.resize(Page::PAGE_SIZE);
        frame->pin_count = 1;

        frames_[page_id] = frame;
        return page_id;
    }

    // 淘汰页
    if (frames_.size() >= pool_size_) {
        uint64_t evicted = evict();
        if (evicted == INVALID_PAGE_ID) {
            // 无法淘汰，分配新帧
        }
    }

    auto frame = std::make_shared<BufferFrame>();
    frame->page_id = page_id;
    frame->data.resize(Page::PAGE_SIZE);
    frame->pin_count = 1;

    frames_[page_id] = frame;
    return page_id;
}

inline std::shared_ptr<BufferFrame> BufferPool::get_page(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = frames_.find(page_id);
    if (it != frames_.end()) {
        it->second->pin_count++;
        return it->second;
    }

    return nullptr;
}

inline void BufferPool::pin(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = frames_.find(page_id);
    if (it != frames_.end()) {
        it->second->pin_count++;
    }
}

inline void BufferPool::unpin(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = frames_.find(page_id);
    if (it != frames_.end() && it->second->pin_count > 0) {
        it->second->pin_count--;
    }
}

inline void BufferPool::mark_dirty(uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = frames_.find(page_id);
    if (it != frames_.end()) {
        it->second->dirty = true;
    }
}

inline void BufferPool::flush_all() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [page_id, frame] : frames_) {
        if (frame->dirty) {
            // 写入磁盘（简化）
            frame->dirty = false;
        }
    }
}

inline uint64_t BufferPool::evict() {
    return clock_evict();
}

inline uint64_t BufferPool::clock_evict() {
    // 简化：随机淘汰
    for (auto& [page_id, frame] : frames_) {
        if (frame->pin_count == 0) {
            if (frame->dirty) {
                // 写回磁盘
            }
            free_list_.push_back(frame->page_id);
            frames_.erase(page_id);
            return page_id;
        }
    }
    return INVALID_PAGE_ID;
}

} // namespace mydb
