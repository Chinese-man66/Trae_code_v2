/**
 * Stage 3: 高级阶段 - 事务与并发
 *
 * 本阶段学习目标：
 * 1. 理解 ACID 事务特性
 * 2. 实现两阶段锁协议
 * 3. 理解并发控制：死锁检测、锁粒度
 * 4. 理解缓冲池管理：LRU/Clock 算法
 * 5. 理解 WAL 日志与崩溃恢复
 */

#include "transaction.h"
#include "wal.h"
#include "../stage1_basics/record.h"
#include "../stage1_basics/table.h"
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <random>

using namespace mydb;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(50, '=') << "\n";
}

// ==================== 演示：事务基础 ====================

void demo_transaction_basic() {
    print_separator("1. 事务基础演示");

    // 创建事务
    auto tx1 = std::make_shared<Transaction>(
        Transaction::get_next_tid(),
        IsolationLevel::SERIALIZABLE
    );

    std::cout << "创建事务:\n";
    std::cout << "  事务ID: " << tx1->id() << "\n";
    std::cout << "  隔离级别: SERIALIZABLE\n";
    std::cout << "  状态: " << (tx1->is_active() ? "ACTIVE" : "ENDED") << "\n";

    tx1->commit();
    std::cout << "\n提交后状态: " << (tx1->is_active() ? "ACTIVE" : "ENDED") << "\n";
}

// ==================== 演示：锁管理器 ====================

void demo_lock_manager() {
    print_separator("2. 锁管理器演示");

    LockManager lock_mgr;

    // 事务1 获取排他锁
    TransactionId tid1 = 1;
    TransactionId tid2 = 2;

    bool lock1 = lock_mgr.lock(tid1, "page:1", LockType::EXCLUSIVE);
    std::cout << "事务1 获取 page:1 排他锁: " << (lock1 ? "成功" : "失败") << "\n";

    // 事务2 尝试获取共享锁
    bool lock2 = lock_mgr.lock(tid2, "page:1", LockType::SHARED);
    std::cout << "事务2 获取 page:1 共享锁: " << (lock2 ? "成功" : "失败") << "\n";

    // 事务1 释放锁
    lock_mgr.unlock(tid1, "page:1");
    std::cout << "事务1 释放 page:1 排他锁\n";

    // 事务2 再次尝试获取排他锁
    bool lock3 = lock_mgr.lock(tid2, "page:1", LockType::EXCLUSIVE);
    std::cout << "事务2 获取 page:1 排他锁: " << (lock3 ? "成功" : "失败") << "\n";

    // 多资源锁演示
    std::cout << "\n--- 多资源锁演示 ---\n";
    lock_mgr.lock(tid1, "page:2", LockType::SHARED);
    lock_mgr.lock(tid1, "page:3", LockType::SHARED);

    auto blocked = lock_mgr.get_blocked_transactions();
    std::cout << "被阻塞的事务数: " << blocked.size() << "\n";
}

// ==================== 演示：并发事务 ====================

void demo_concurrent_transactions() {
    print_separator("3. 并发事务演示");

    LockManager lock_mgr;
    TableMeta meta;
    meta.name = "accounts";
    meta.columns = {
        {"id", Column::Type::INTEGER},
        {"balance", Column::Type::DOUBLE}
    };

    Table table("./data/stage3/accounts", meta);

    // 插入初始数据
    Record r1(meta.columns);
    r1.set("id", Cell{1});
    r1.set("balance", Cell{1000.0});
    table.insert(r1);

    Record r2(meta.columns);
    r2.set("id", Cell{2});
    r2.set("balance", Cell{2000.0});
    table.insert(r2);

    std::cout << "初始数据:\n";
    std::cout << "  账户1: " << table.get(1)->to_string() << "\n";
    std::cout << "  账户2: " << table.get(2)->to_string() << "\n";

    // 转账事务函数
    auto transfer = [&](TransactionId tid, int from, int to, double amount) {
        std::cout << "\n事务" << tid << " 开始转账: " << from << " -> " << to
                  << ", 金额: " << amount << "\n";

        // 获取锁
        std::string from_key = "row:" + std::to_string(from);
        std::string to_key = "row:" + std::to_string(to);

        // 按顺序获取锁避免死锁
        if (from > to) std::swap(from_key, to_key);

        lock_mgr.lock(tid, from_key, LockType::EXCLUSIVE);
        lock_mgr.lock(tid, to_key, LockType::EXCLUSIVE);

        // 读取余额
        auto rec1 = table.get(from);
        auto rec2 = table.get(to);

        double balance1 = std::get<double>(rec1->get("balance").value());
        double balance2 = std::get<double>(rec2->get("balance").value());

        std::cout << "  当前余额: 账户" << from << "=" << balance1
                  << ", 账户" << to << "=" << balance2 << "\n";

        // 更新余额
        Record new_rec1(meta.columns);
        new_rec1.set("id", Cell{static_cast<int64_t>(from)});
        new_rec1.set("balance", Cell{balance1 - amount});
        table.update(from, new_rec1);

        Record new_rec2(meta.columns);
        new_rec2.set("id", Cell{static_cast<int64_t>(to)});
        new_rec2.set("balance", Cell{balance2 + amount});
        table.update(to, new_rec2);

        // 释放锁
        lock_mgr.unlock(tid, from_key);
        lock_mgr.unlock(tid, to_key);

        std::cout << "  事务" << tid << " 提交完成\n";
    };

    // 并发执行两个转账
    std::cout << "\n--- 并发转账 ---\n";

    std::future<void> f1 = std::async(std::launch::async, transfer, 1, 1, 2, 500.0);
    std::future<void> f2 = std::async(std::launch::async, transfer, 2, 2, 1, 300.0);

    f1.get();
    f2.get();

    std::cout << "\n转账后数据:\n";
    std::cout << "  账户1: " << table.get(1)->to_string() << "\n";
    std::cout << "  账户2: " << table.get(2)->to_string() << "\n";
}

// ==================== 演示：缓冲池 ====================

void demo_buffer_pool() {
    print_separator("4. 缓冲池演示");

    BufferPool pool(3);

    // 分配页
    uint64_t page1 = pool.allocate_page();
    uint64_t page2 = pool.allocate_page();
    uint64_t page3 = pool.allocate_page();

    std::cout << "分配页: " << page1 << ", " << page2 << ", " << page3 << "\n";

    // 获取页
    auto frame1 = pool.get_page(page1);
    auto frame2 = pool.get_page(page2);

    std::cout << "获取页 " << page1 << ": pin_count = " << frame1->pin_count << "\n";
    std::cout << "获取页 " << page2 << ": pin_count = " << frame2->pin_count << "\n";

    // 标记脏页
    pool.mark_dirty(page1);
    std::cout << "标记页 " << page1 << " 为脏页\n";

    // 取消固定
    pool.unpin(page1);
    std::cout << "取消固定页 " << page1 << ": pin_count = "
              << frame1->pin_count << "\n";

    // 尝试淘汰页
    uint64_t evicted = pool.evict();
    std::cout << "淘汰页: " << evicted << "\n";

    // 刷新所有脏页
    pool.flush_all();
    std::cout << "刷新所有脏页完成\n";
}

// ==================== 演示：WAL 日志 ====================

void demo_wal_log() {
    print_separator("5. WAL 日志演示");

    WALManager wal("./data/stage3/wal");

    // 记录事务日志
    LogRecord begin_record;
    begin_record.transaction_id = 1;
    begin_record.type = LogType::BEGIN;

    uint64_t lsn1 = wal.append_log(begin_record);
    std::cout << "写入 BEGIN 日志: LSN = " << lsn1 << "\n";

    // 记录更新
    LogRecord update_record;
    update_record.transaction_id = 1;
    update_record.type = LogType::UPDATE;
    update_record.table_name = "accounts";
    update_record.row_id = 1;
    update_record.before_image = {0x01, 0x02, 0x03};
    update_record.after_image = {0x04, 0x05, 0x06};

    uint64_t lsn2 = wal.append_log(update_record);
    std::cout << "写入 UPDATE 日志: LSN = " << lsn2 << "\n";

    // 记录提交
    LogRecord commit_record;
    commit_record.transaction_id = 1;
    commit_record.type = LogType::COMMIT;

    uint64_t lsn3 = wal.append_log(commit_record);
    std::cout << "写入 COMMIT 日志: LSN = " << lsn3 << "\n";

    // 刷盘
    wal.flush();
    std::cout << "刷盘完成\n";

    // 设置检查点
    wal.set_checkpoint(lsn3);
    std::cout << "设置检查点: LSN = " << wal.get_checkpoint_lsn() << "\n";
}

// ==================== 演示：恢复流程 ====================

void demo_recovery() {
    print_separator("6. 崩溃恢复演示");

    TableMeta meta;
    meta.name = "test";
    meta.columns = {
        {"id", Column::Type::INTEGER},
        {"value", Column::Type::TEXT}
    };

    Table table("./data/stage3/test", meta);
    WALManager wal("./data/stage3/wal2");
    RecoveryManager recovery(wal, table);

    // 模拟事务
    auto tx = std::make_shared<Transaction>(
        Transaction::get_next_tid(),
        IsolationLevel::SERIALIZABLE
    );

    std::cout << "--- 模拟正常事务 ---\n";
    recovery.begin(*tx);

    Record r(meta.columns);
    r.set("id", Cell{1});
    r.set("value", Cell{std::string("test_data")});

    RowId row_id = table.insert(r);
    std::cout << "插入数据: row_id = " << row_id << "\n";

    // 记录修改（模拟）
    std::vector<uint8_t> before_image = {0x00};
    std::vector<uint8_t> after_image = {0x01};
    recovery.log_after_image(*tx, row_id, after_image);

    recovery.commit(*tx);
    std::cout << "事务提交\n";

    // 创建检查点
    std::cout << "\n--- 创建检查点 ---\n";
    recovery.checkpoint();
    std::cout << "检查点创建完成\n";

    // 模拟崩溃恢复
    std::cout << "\n--- 模拟崩溃恢复 ---\n";
    recovery.recover();
    std::cout << "恢复完成\n";
}

// ==================== 演示：两阶段锁 ====================

void demo_two_phase_locking() {
    print_separator("7. 两阶段锁协议演示");

    LockManager lock_mgr;

    std::cout << "两阶段锁协议 (2PL):\n";
    std::cout << "  阶段1: 增长阶段 (Growing) - 只获取锁\n";
    std::cout << "  阶段2: 缩减阶段 (Shrinking) - 只释放锁\n\n";

    TransactionId tid = 1;

    // 增长阶段
    std::cout << "--- 增长阶段 ---\n";
    lock_mgr.lock(tid, "resource:A", LockType::SHARED);
    std::cout << "获取 resource:A 共享锁\n";

    lock_mgr.lock(tid, "resource:B", LockType::EXCLUSIVE);
    std::cout << "获取 resource:B 排他锁\n";

    // 业务逻辑执行...

    // 缩减阶段
    std::cout << "\n--- 缩减阶段 ---\n";
    lock_mgr.unlock(tid, "resource:A");
    std::cout << "释放 resource:A 锁\n";

    lock_mgr.unlock(tid, "resource:B");
    std::cout << "释放 resource:B 锁\n";

    std::cout << "\n事务完成\n";
}

// ==================== 主函数 ====================

int main() {
    std::cout << "======================================\n";
    std::cout << "   MyDB - Stage 3: 高级阶段\n";
    std::cout << "   事务与并发控制\n";
    std::cout << "======================================\n";

    // 演示 1: 事务基础
    demo_transaction_basic();

    // 演示 2: 锁管理器
    demo_lock_manager();

    // 演示 3: 并发事务
    demo_concurrent_transactions();

    // 演示 4: 缓冲池
    demo_buffer_pool();

    // 演示 5: WAL 日志
    demo_wal_log();

    // 演示 6: 恢复流程
    demo_recovery();

    // 演示 7: 两阶段锁
    demo_two_phase_locking();

    print_separator("Stage 3 完成!");
    std::cout << "\n学习要点：\n";
    std::cout << "  1. ACID 特性：原子性、一致性、隔离性、持久性\n";
    std::cout << "  2. 两阶段锁协议 (2PL)：增长阶段 + 缩减阶段\n";
    std::cout << "  3. 锁粒度：行锁、表锁、页锁\n";
    std::cout << "  4. 死锁处理：超时检测、等待图检测\n";
    std::cout << "  5. 缓冲池：Clock/LRU 算法、脏页刷盘\n";
    std::cout << "  6. WAL：Write-Ahead Logging、Redo/Undo\n";
    std::cout << "  7. 恢复：ARIES 算法、检查点\n";
    std::cout << "\n恭喜完成 MyDB 完整学习路径！\n";

    return 0;
}
