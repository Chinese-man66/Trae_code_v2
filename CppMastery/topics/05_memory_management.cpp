/**
 * C++ 高级特性学习 - 专题 5: 内存管理
 *
 * 本文件讲解:
 * 1. new/delete 操作符
 * 2. new/delete 数组
 * 3. placement new
 * 4. 内存池原理与实现
 * 5. 内存对齐
 * 6. 内存泄漏检测
 * 7. RAII 与内存管理
 */

#include <iostream>
#include <memory>
#include <vector>
#include <new>
#include <cstring>
#include <cstdint>
#include <cstddef>

namespace memory_demo {

// ==================== 1. new/delete 基础 ====================

class Widget {
public:
    int data;

    Widget() : data(0) {
        std::cout << "Widget 默认构造\n";
    }

    Widget(int d) : data(d) {
        std::cout << "Widget 构造: data = " << data << "\n";
    }

    ~Widget() {
        std::cout << "Widget 析构: data = " << data << "\n";
    }
};

void demo_new_delete() {
    std::cout << "\n--- new/delete 演示 ---\n";

    // 1. new 单个对象
    int* p1 = new int(42);
    std::cout << "*p1 = " << *p1 << "\n";
    delete p1;

    // 2. new 数组
    int* arr = new int[5];
    for (int i = 0; i < 5; ++i) {
        arr[i] = i * 10;
    }
    delete[] arr;

    // 3. new 自定义类型
    Widget* w = new Widget(100);
    delete w;

    // 4. new 不初始化
    double* d = new double;  // 未初始化
    *d = 3.14;
    std::cout << "*d = " << *d << "\n";
    delete d;

    // 5. new 并值初始化
    double* d2 = new double();  // 值初始化为 0
    std::cout << "*d2 = " << *d2 << "\n";
    delete d2;
}

// ==================== 2. Placement New ====================

void demo_placement_new() {
    std::cout << "\n--- Placement New 演示 ---\n";

    // 1. 在已分配的内存上构造对象
    char buffer[sizeof(Widget)];

    Widget* w1 = new (buffer) Widget(1);
    std::cout << "w1->data = " << w1->data << "\n";
    w1->~Widget();  // 手动调用析构

    // 2. 使用 std::align_val_t 对齐
    void* aligned = nullptr;
    size_t alignment = 64;

    void* ptr = ::operator new(sizeof(Widget), std::align_val_t(alignment));
    Widget* w2 = new (ptr) Widget(2);
    std::cout << "对齐地址: " << ptr << ", 能否被 64 整除: "
              << (reinterpret_cast<uintptr_t>(ptr) % 64 == 0) << "\n";

    w2->~Widget();
    ::operator delete(ptr, std::align_val_t(alignment));
}

// ==================== 3. 内存池实现 ====================

class MemoryPool {
public:
    // 构造函数
    explicit MemoryPool(size_t block_size, size_t pool_size = 1024)
        : block_size_(block_size), pool_size_(pool_size) {

        // 计算实际块大小（考虑对齐）
        block_size_ = (block_size + alignof(max_align_t) - 1)
                      & ~(alignof(max_align_t) - 1);

        // 分配内存池
        pool_ = ::operator new(pool_size_ * block_size_);

        // 初始化空闲链表
        free_list_ = nullptr;
        for (size_t i = 0; i < pool_size_; ++i) {
            char* block = static_cast<char*>(pool_) + i * block_size_;
            *reinterpret_cast<char**>(block) = free_list_;
            free_list_ = block;
        }

        std::cout << "内存池创建: " << pool_size_ << " 块, 每块 "
                  << block_size_ << " 字节\n";
    }

    // 析构函数
    ~MemoryPool() {
        ::operator delete(pool_);
        std::cout << "内存池释放\n";
    }

    // 分配内存
    void* allocate() {
        if (!free_list_) {
            throw std::bad_alloc("内存池已空");
        }

        void* result = free_list_;
        free_list_ = *reinterpret_cast<char**>(free_list_);
        allocated_++;

        return result;
    }

    // 释放内存
    void deallocate(void* ptr) {
        if (!ptr) return;

        *reinterpret_cast<char**>(ptr) = free_list_;
        free_list_ = static_cast<char*>(ptr);
        allocated_--;
    }

    // 统计信息
    void print_stats() const {
        std::cout << "已分配: " << allocated_
                  << ", 空闲: " << (pool_size_ - allocated_) << "\n";
    }

private:
    size_t block_size_;
    size_t pool_size_;
    void* pool_;
    char* free_list_;
    size_t allocated_ = 0;
};

// 自定义类型的内存池
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t count = 100) : pool_(sizeof(T), count) {}

    template<typename... Args>
    T* allocate(Args&&... args) {
        void* ptr = pool_.allocate();
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) {
        if (ptr) {
            ptr->~T();
            pool_.deallocate(ptr);
        }
    }

    void print_stats() const { pool_.print_stats(); }

private:
    MemoryPool pool_;
};

void demo_memory_pool() {
    std::cout << "\n--- 内存池演示 ---\n";

    // 使用通用内存池
    MemoryPool pool(sizeof(Widget), 10);

    Widget* w1 = static_cast<Widget*>(pool.allocate());
    new (w1) Widget(1);

    Widget* w2 = static_cast<Widget*>(pool.allocate());
    new (w2) Widget(2);

    pool.print_stats();

    w1->~Widget();
    pool.deallocate(w1);

    w2->~Widget();
    pool.deallocate(w2);

    pool.print_stats();

    // 使用模板化对象池
    std::cout << "\n--- 模板化对象池 ---\n";
    ObjectPool<Widget> obj_pool(5);

    Widget* w3 = obj_pool.allocate(100);
    Widget* w4 = obj_pool.allocate(200);
    Widget* w5 = obj_pool.allocate(300);

    obj_pool.print_stats();

    obj_pool.deallocate(w3);
    obj_pool.deallocate(w4);
    obj_pool.deallocate(w5);

    obj_pool.print_stats();
}

// ==================== 4. 内存对齐 ====================

struct Unaligned {
    char a;
    int b;
    char c;
};

struct Aligned {
    char a;
    char _pad1[3];  // 填充
    int b;
    char c;
    char _pad2[3];  // 填充
};

#pragma pack(push, 1)
struct Packed {
    char a;
    int b;
    char c;
};
#pragma pack(pop)

void demo_alignment() {
    std::cout << "\n--- 内存对齐演示 ---\n";

    std::cout << "Unaligned 大小: " << sizeof(Unaligned)
              << " (期望: 6, 实际: " << sizeof(Unaligned) << ")\n";
    std::cout << "Aligned 大小: " << sizeof(Aligned) << "\n";
    std::cout << "Packed 大小: " << sizeof(Packed) << "\n";

    // alignas 关键字
    struct alignas(16) Aligned16 {
        int x;
    };

    std::cout << "alignas(16) 大小: " << sizeof(Aligned16) << "\n";

    // alignof
    std::cout << "alignof(int): " << alignof(int) << "\n";
    std::cout << "alignof(double): " << alignof(double) << "\n";
    std::cout << "alignof(Aligned16): " << alignof(Aligned16) << "\n";
}

// ==================== 5. 自定义 new/delete ====================

// 全局自定义 operator new
void* operator new(size_t size) {
    std::cout << "自定义 new 分配: " << size << " 字节\n";
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept {
    std::cout << "自定义 delete 释放\n";
    ::operator delete(ptr);
}

void demo_custom_new_delete() {
    std::cout << "\n--- 自定义 new/delete ---\n";

    int* p = new int(42);
    delete p;
}

// ==================== 6. RAII 内存管理 ====================

class SmartBuffer {
public:
    explicit SmartBuffer(size_t size) : size_(size) {
        data_ = new char[size_];
        std::cout << "SmartBuffer 分配: " << size_ << " 字节\n";
    }

    ~SmartBuffer() {
        delete[] data_;
        std::cout << "SmartBuffer 释放\n";
    }

    // 禁止拷贝
    SmartBuffer(const SmartBuffer&) = delete;
    SmartBuffer& operator=(const SmartBuffer&) = delete;

    // 允许移动
    SmartBuffer(SmartBuffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    SmartBuffer& operator=(SmartBuffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    char* data() const { return data_; }
    size_t size() const { return size_; }

private:
    char* data_;
    size_t size_;
};

void demo_raii() {
    std::cout << "\n--- RAII 内存管理 ---\n";

    SmartBuffer buf1(100);

    SmartBuffer buf2 = std::move(buf1);
    std::cout << "移动后 buf1 大小: " << buf1.size() << "\n";
    std::cout << "移动后 buf2 大小: " << buf2.size() << "\n";
}

// ==================== 7. 智能指针的底层实现（简化版）====================

template<typename T>
class SimpleUniquePtr {
public:
    explicit SimpleUniquePtr(T* p = nullptr) : ptr_(p) {}

    ~SimpleUniquePtr() {
        delete ptr_;
    }

    // 禁止拷贝
    SimpleUniquePtr(const SimpleUniquePtr&) = delete;
    SimpleUniquePtr& operator=(const SimpleUniquePtr&) = delete;

    // 移动
    SimpleUniquePtr(SimpleUniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    SimpleUniquePtr& operator=(SimpleUniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    void reset(T* p = nullptr) {
        delete ptr_;
        ptr_ = p;
    }

    T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

    explicit operator bool() const { return ptr_ != nullptr; }

private:
    T* ptr_;
};

void demo_simple_unique_ptr() {
    std::cout << "\n--- 简化版 UniquePtr ---\n";

    SimpleUniquePtr<Widget> ptr(new Widget(999));
    std::cout << "ptr->data = " << ptr->data << "\n";

    SimpleUniquePtr<Widget> ptr2 = std::move(ptr);
    std::cout << "移动后 ptr 有效: " << static_cast<bool>(ptr) << "\n";
    std::cout << "移动后 ptr2->data = " << ptr2->data << "\n";
}

// ==================== 8. 内存泄漏检测基础 ====================

class LeakDetector {
public:
    static LeakDetector& instance() {
        static LeakDetector detector;
        return detector;
    }

    void track_alloc(void* ptr, size_t size, const char* file, int line) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, file, line};
        total_allocated_ += size;
    }

    void track_free(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            total_allocated_ -= it->second.size;
            allocations_.erase(it);
        }
    }

    void report() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\n=== 内存泄漏报告 ===\n";
        std::cout << "当前分配: " << allocations_.size() << " 块\n";
        std::cout << "总大小: " << total_allocated_ << " 字节\n";

        for (const auto& [ptr, info] : allocations_) {
            std::cout << "  泄漏: " << info.size << " 字节 at "
                      << info.file << ":" << info.line << "\n";
        }
    }

    ~LeakDetector() {
        if (!allocations_.empty()) {
            report();
        }
    }

private:
    struct AllocInfo {
        size_t size;
        const char* file;
        int line;
    };

    std::unordered_map<void*, AllocInfo> allocations_;
    size_t total_allocated_ = 0;
    std::mutex mutex_;
};

#define TRACK_NEW new(__FILE__, __LINE__)
// 简化的跟踪版本
void* tracked_new(size_t size, const char* file, int line) {
    void* ptr = ::operator new(size);
    LeakDetector::instance().track_alloc(ptr, size, file, line);
    return ptr;
}

void tracked_delete(void* ptr) {
    LeakDetector::instance().track_free(ptr);
    ::operator delete(ptr);
}

void demo_leak_detection() {
    std::cout << "\n--- 内存泄漏检测演示 ---\n";

    // 模拟泄漏
    void* leaked = tracked_new(100, "test.cpp", 42);

    // 正常释放
    int* normal = static_cast<int*>(tracked_new(sizeof(int), "test.cpp", 50));
    tracked_delete(normal);

    // LeakDetector 会在程序结束时报告泄漏
}

// ==================== 主函数 ====================

} // namespace memory_demo

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 专题 5: 内存管理\n";
    std::cout << "======================================\n";

    using namespace memory_demo;

    demo_new_delete();
    demo_placement_new();
    demo_memory_pool();
    demo_alignment();
    demo_custom_new_delete();
    demo_raii();
    demo_simple_unique_ptr();
    demo_leak_detection();

    std::cout << "\n======================================\n";
    std::cout << "内存管理要点总结：\n";
    std::cout << "1. new/delete: 堆内存分配释放\n";
    std::cout << "2. new[]/delete[]: 数组动态分配\n";
    std::cout << "3. Placement new: 在指定内存构造\n";
    std::cout << "4. 内存池: 预分配，减少碎片\n";
    std::cout << "5. 内存对齐: 提高访问效率\n";
    std::cout << "6. RAII: 自动管理资源\n";
    std::cout << "7. 自定义 new/delete: 追踪/日志\n";
    std::cout << "======================================\n";

    return 0;
}
