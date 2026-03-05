/**
 * C++ 高级特性学习 - 专题 3: 智能指针
 *
 * 本文件讲解:
 * 1. unique_ptr - 独占所有权的智能指针
 * 2. shared_ptr - 共享所有权的智能指针
 * 3. weak_ptr - 弱引用，不增加引用计数
 * 4. 自定义删除器
 * 5. 智能指针的线程安全性
 */

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>

namespace smart_ptr_demo {

// ==================== 资源类 ====================

class Resource {
public:
    int id_;

    explicit Resource(int id) : id_(id) {
        std::cout << "Resource " << id_ << " 构造\n";
    }

    ~Resource() {
        std::cout << "Resource " << id_ << " 析构\n";
    }

    void use() {
        std::cout << "Resource " << id_ << " 被使用\n";
    }
};

// ==================== 1. unique_ptr ====================

void demo_unique_ptr() {
    std::cout << "\n--- unique_ptr 演示 ---\n";

    // 创建 unique_ptr
    std::unique_ptr<Resource> ptr1 = std::make_unique<Resource>(1);
    std::cout << "ptr1 指向: Resource " << ptr1->id_ << "\n";

    // 使用 *
    (*ptr1).use();

    // unique_ptr 不能直接拷贝
    // std::unique_ptr<Resource> ptr2 = ptr1;  // Error!

    // 可以移动
    std::unique_ptr<Resource> ptr2 = std::move(ptr1);
    std::cout << "移动后 ptr2 指向: Resource " << ptr2->id_ << "\n";

    // ptr1 现在为空
    std::cout << "ptr1 是否为空: " << (ptr1 ? "否" : "是") << "\n";

    // 释放
    ptr2.reset();
    std::cout << "reset 后 ptr2 是否为空: " << (ptr2 ? "否" : "是") << "\n";

    // 离开作用域自动释放
    {
        auto ptr = std::make_unique<Resource>(99);
        std::cout << "在作用域内\n";
    }
    std::cout << "离开作用域\n";
}

// 自定义删除器
void demo_unique_ptr_custom_deleter() {
    std::cout << "\n--- unique_ptr 自定义删除器 ---\n";

    // 使用 lambda 作为删除器
    auto deleter = [](Resource* r) {
        std::cout << "自定义删除 Resource " << r->id_ << "\n";
        delete r;
    };

    std::unique_ptr<Resource, decltype(deleter)> ptr(
        new Resource(100), deleter);

    // 使用数组
    std::unique_ptr<Resource[]> arr = std::make_unique<Resource[]>(3);
    for (int i = 0; i < 3; ++i) {
        arr[i].use();
    }
}

// ==================== 2. shared_ptr ====================

void demo_shared_ptr() {
    std::cout << "\n--- shared_ptr 演示 ---\n";

    // 创建 shared_ptr
    std::shared_ptr<Resource> ptr1 = std::make_shared<Resource>(1);
    std::cout << "ptr1 引用计数: " << ptr1.use_count() << "\n";

    // 拷贝 - 引用计数增加
    std::shared_ptr<Resource> ptr2 = ptr1;
    std::cout << "拷贝后 ptr1 计数: " << ptr1.use_count() << "\n";
    std::cout << "拷贝后 ptr2 计数: " << ptr2.use_count() << "\n";

    // 移动 - 引用计数不变
    std::shared_ptr<Resource> ptr3 = std::move(ptr1);
    std::cout << "移动后 ptr2 计数: " << ptr2.use_count() << "\n";
    std::cout << "移动后 ptr3 计数: " << ptr3.use_count() << "\n";

    // 多个 shared_ptr 指向同一对象
    std::cout << "\n--- shared_ptr 多个引用 ---\n";
    {
        std::shared_ptr<Resource> ptr4 = ptr2;
        std::cout << "ptr4 创建后计数: " << ptr2.use_count() << "\n";
    }
    std::cout << "ptr4 销毁后计数: " << ptr2.use_count() << "\n";

    // reset - 重置
    ptr2.reset();
    std::cout << "reset 后 ptr3 计数: " << ptr3.use_count() << "\n";
}

// shared_ptr 循环引用问题
class Node {
public:
    int value_;
    std::shared_ptr<Node> next_;

    explicit Node(int v) : value_(v) {
        std::cout << "Node " << v << " 构造\n";
    }

    ~Node() {
        std::cout << "Node " << value_ << " 析构\n";
    }
};

void demo_shared_ptr_cycle() {
    std::cout << "\n--- shared_ptr 循环引用 ---\n";

    // 创建循环引用 - 内存泄漏！
    std::cout << "创建循环引用:\n";
    {
        auto node1 = std::make_shared<Node>(1);
        auto node2 = std::make_shared<Node>(2);

        node1->next_ = node2;
        node2->next_ = node1;

        std::cout << "node1 计数: " << node1.use_count() << "\n";
        std::cout << "node2 计数: " << node2.use_count() << "\n";
    }
    std::cout << "离开作用域 - 循环引用导致内存泄漏!\n";
}

// ==================== 3. weak_ptr ====================

void demo_weak_ptr() {
    std::cout << "\n--- weak_ptr 演示 ---\n";

    // 创建 weak_ptr
    std::shared_ptr<Resource> shared = std::make_shared<Resource>(1);
    std::weak_ptr<Resource> weak = shared;

    std::cout << "weak_ptr 有效: " << !weak.expired() << "\n";
    std::cout << "shared 引用计数: " << shared.use_count() << "\n";

    // 转换为 shared_ptr
    if (auto locked = weak.lock()) {
        std::cout << "lock 成功: Resource " << locked->id_ << "\n";
    }

    // 释放 shared
    shared.reset();

    std::cout << "shared reset 后 weak_ptr 有效: " << !weak.expired() << "\n";

    // lock 返回空
    if (auto locked = weak.lock()) {
        std::cout << "lock 成功\n";
    } else {
        std::cout << "lock 失败 - weak_ptr 已过期\n";
    }
}

// 使用 weak_ptr 打破循环引用
class NodeSafe {
public:
    int value_;
    std::shared_ptr<NodeSafe> next_;
    std::weak_ptr<NodeSafe> prev_;  // 使用 weak_ptr 打破循环

    explicit NodeSafe(int v) : value_(v) {
        std::cout << "NodeSafe " << v << " 构造\n";
    }

    ~NodeSafe() {
        std::cout << "NodeSafe " << value_ << " 析构\n";
    }
};

void demo_weak_ptr_broke_cycle() {
    std::cout << "\n--- weak_ptr 打破循环引用 ---\n";

    {
        auto node1 = std::make_shared<NodeSafe>(1);
        auto node2 = std::make_shared<NodeSafe>(2);

        node1->next_ = node2;
        node2->prev_ = node1;  // weak_ptr 不会增加计数

        std::cout << "node1 计数: " << node1.use_count() << "\n";
        std::cout << "node2 计数: " << node2.use_count() << "\n";
    }
    std::cout << "离开作用域 - 正常析构!\n";
}

// ==================== 4. 自定义删除器 ====================

void demo_custom_deleter() {
    std::cout << "\n--- 自定义删除器 ---\n";

    // 数组删除器
    auto array_deleter = [](Resource* arr) {
        std::cout << "删除数组\n";
        delete[] arr;
    };

    std::shared_ptr<Resource> arr(
        new Resource[3]{1, 2, 3},
        array_deleter);

    // 文件删除器模拟
    struct FileCloser {
        void operator()(FILE* f) {
            if (f) {
                std::cout << "关闭文件\n";
                fclose(f);
            }
        }
    };

    // shared_ptr 用于管理文件
    // FILE* f = fopen("test.txt", "w");
    // std::shared_ptr<FILE> file(f, FileCloser());
}

// ==================== 5. 智能指针的线程安全性 ====================

std::mutex cout_mutex;

void thread_safe_demo() {
    std::cout << "\n--- 智能指针线程安全演示 ---\n";

    std::shared_ptr<int> counter = std::make_shared<int>(0);

    auto worker = [&counter](int iterations) {
        for (int i = 0; i < iterations; ++i) {
            // shared_ptr 的引用计数操作是线程安全的
            // 但需要注意：多个线程同时读写同一个对象是不安全的
            auto local = counter;  // 拷贝 - 线程安全
            (*local)++;

            // 正确的做法：使用原子操作或互斥锁保护
        }
    };

    std::thread t1(worker, 1000);
    std::thread t2(worker, 1000);

    t1.join();
    t2.join();

    std::cout << "Counter: " << *counter << "\n";
}

// ==================== 智能指针的使用场景 ====================

class Database {
public:
    Database() { std::cout << "Database 连接\n"; }
    ~Database() { std::cout << "Database 断开\n"; }

    void query(const std::string& sql) {
        std::cout << "执行: " << sql << "\n";
    }
};

// 工厂函数返回 unique_ptr
std::unique_ptr<Database> create_database() {
    return std::make_unique<Database>();
}

// 共享所有权的情况
void demo_ownership() {
    std::cout << "\n--- 智能指针使用场景 ---\n";

    // 场景1: 工厂函数返回 unique_ptr
    auto db = create_database();
    db->query("SELECT * FROM users");

    // 场景2: 容器中的智能指针
    std::vector<std::unique_ptr<Resource>> vec;
    vec.push_back(std::make_unique<Resource>(1));
    vec.push_back(std::make_unique<Resource>(2));

    // 遍历
    for (auto& ptr : vec) {
        ptr->use();
    }

    // 场景3: 打破所有权的传递
    std::unique_ptr<Resource> p1 = std::make_unique<Resource>(3);

    // 在函数中使用 unique_ptr
    auto use_resource = [](std::unique_ptr<Resource> r) {
        r->use();
    };

    // 传递所有权
    use_resource(std::move(p1));

    // 场景4: shared_ptr 用于共享资源
    std::shared_ptr<Database> db1 = std::make_shared<Database>();
    std::shared_ptr<Database> db2 = db1;

    std::cout << "引用计数: " << db1.use_count() << "\n";
}

// ==================== make_unique 和 make_shared ====================

void demo_make_functions() {
    std::cout << "\n--- make_unique 和 make_shared ---\n";

    // make_unique (C++14)
    auto p1 = std::make_unique<Resource>(1);

    // make_shared (更高效，一次分配)
    auto p2 = std::make_shared<Resource>(2);

    // make_unique 支持数组 (C++20)
    auto arr = std::make_unique<Resource[]>(3);

    // 注意：make_shared 比 new + shared_ptr 更高效
    // 因为只需要一次内存分配
}

// ==================== 主函数 ====================

} // namespace smart_ptr_demo

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 专题 3: 智能指针\n";
    std::cout << "======================================\n";

    using namespace smart_ptr_demo;

    demo_unique_ptr();
    demo_unique_ptr_custom_deleter();
    demo_shared_ptr();
    demo_shared_ptr_cycle();
    demo_weak_ptr();
    demo_weak_ptr_broke_cycle();
    demo_custom_deleter();
    thread_safe_demo();
    demo_ownership();
    demo_make_functions();

    std::cout << "\n======================================\n";
    std::cout << "智能指针要点总结：\n";
    std::cout << "1. unique_ptr: 独占所有权，可移动\n";
    std::cout << "2. shared_ptr: 共享所有权，引用计数\n";
    std::cout << "3. weak_ptr: 弱引用，不增加计数\n";
    std::cout << "4. make_unique/make_shared: 安全创建\n";
    std::cout << "5. 循环引用用 weak_ptr 打破\n";
    std::cout << "======================================\n";

    return 0;
}
