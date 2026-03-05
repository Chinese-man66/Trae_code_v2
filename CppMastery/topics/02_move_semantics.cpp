/**
 * C++ 高级特性学习 - 专题 2: 移动语义与右值引用
 *
 * 本文件讲解:
 * 1. 左值 vs 右值
 * 2. 右值引用 (T&&)
 * 3. 移动构造函数
 * 4. 移动赋值运算符
 * 5. std::move
 * 6. 完美转发
 * 7. 返回值优化 (RVO/NRVO)
 */

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace move_demo {

// ==================== 基础概念：左值 vs 右值 ====================

void demo_lvalue_rvalue() {
    std::cout << "\n--- 左值 vs 右值 ---\n";

    int a = 10;      // a 是左值，10 是右值
    int b = a;       // b 是左值，a 是左值

    // 左值：有持久状态，可以取地址
    int* p = &a;     // OK：a 是左值
    // int* q = &10;  // Error：10 是右值，不能取地址

    // 右值：临时对象，生命周期短
    int c = a + 5;   // a + 5 是右值
    // (a + 5) = 10;  // Error：不能对右值赋值

    std::cout << "a = " << a << ", b = " << b << ", c = " << c << "\n";
    std::cout << "Address of a: " << p << "\n";
}

// ==================== 右值引用 ====================

void demo_rvalue_ref() {
    std::cout << "\n--- 右值引用 ---\n";

    int x = 10;
    int& lref = x;        // 左值引用
    // int& rref = 10;    // Error：不能绑定到右值

    int&& rref = 10;     // 右值引用，可以绑定到右值
    std::cout << "rref = " << rref << "\n";

    // 右值引用本身是左值！
    int y = rref;        // OK：rref 是左值（它有名字）
    std::cout << "y = " << y << "\n";
}

// ==================== 移动语义类 ====================

class Buffer {
public:
    int* data_;
    size_t size_;

    // 构造函数
    explicit Buffer(size_t size) : size_(size) {
        data_ = new int[size];
        std::cout << "Buffer 构造函数: 分配 " << size << " 个 int\n";
    }

    // 析构函数
    ~Buffer() {
        delete[] data_;
        std::cout << "Buffer 析构函数: 释放 " << size_ << " 个 int\n";
    }

    // 拷贝构造函数 (深拷贝)
    Buffer(const Buffer& other) : size_(other.size_) {
        data_ = new int[size_];
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
        std::cout << "Buffer 拷贝构造函数 (深拷贝)\n";
    }

    // 移动构造函数 (转移资源所有权)
    Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_) {
        // 将原对象的指针置空，避免析构时释放
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "Buffer 移动构造函数\n";
    }

    // 拷贝赋值运算符
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = new int[size_];
            for (size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
            std::cout << "Buffer 拷贝赋值运算符\n";
        }
        return *this;
    }

    // 移动赋值运算符
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
            std::cout << "Buffer 移动赋值运算符\n";
        }
        return *this;
    }

    void fill(int value) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = value;
        }
    }

    bool is_empty() const { return data_ == nullptr; }
    size_t size() const { return size_; }
};

void demo_move_constructor() {
    std::cout << "\n--- 移动构造函数演示 ---\n";

    std::cout << "\n1. 创建 buffer1:\n";
    Buffer buffer1(10);
    buffer1.fill(42);

    std::cout << "\n2. 拷贝构造 buffer2 (从 buffer1):\n";
    Buffer buffer2 = buffer1;  // 调用拷贝构造函数

    std::cout << "\n3. 移动构造 buffer3 (从 buffer1):\n";
    Buffer buffer3 = std::move(buffer1);  // 调用移动构造函数

    std::cout << "\n检查:\n";
    std::cout << "buffer1 是否为空: " << buffer1.is_empty() << "\n";
    std::cout << "buffer2 大小: " << buffer2.size() << "\n";
    std::cout << "buffer3 大小: " << buffer3.size() << "\n";
}

void demo_move_assignment() {
    std::cout << "\n--- 移动赋值运算符演示 ---\n";

    Buffer buffer1(5);
    buffer1.fill(100);

    Buffer buffer2(20);
    buffer2.fill(200);

    std::cout << "\n移动赋值前:\n";
    std::cout << "buffer1 大小: " << buffer1.size() << "\n";
    std::cout << "buffer2 大小: " << buffer2.size() << "\n";

    std::cout << "\n执行 buffer1 = std::move(buffer2):\n";
    buffer1 = std::move(buffer2);

    std::cout << "\n移动赋值后:\n";
    std::cout << "buffer1 大小: " << buffer1.size() << "\n";
    std::cout << "buffer2 是否为空: " << buffer2.is_empty() << "\n";
}

// ==================== std::move 的本质 ====================

void demo_std_move() {
    std::cout << "\n--- std::move 本质 ---\n";

    int x = 10;
    int&& rref = std::move(x);  // std::move 只是强制转换为右值引用

    // 注意：调用 std::move 后，原对象可能被"偷取"
    // 但对于 int 这种内置类型，没有移动构造函数，所以行为等同于拷贝

    std::cout << "x = " << x << ", rref = " << rref << "\n";

    // std::move 的实现原理：
    // template<typename T>
    // decltype(auto) std::move(T&& param) {
    //     return static_cast<T&&>(param);
    // }
}

// ==================== 完美转发 ====================

// 接受任意参数的函数包装器
template<typename Func, typename... Args>
void wrapper(Func&& func, Args&&... args) {
    // 完美转发：保持参数的左右值属性
    func(std::forward<Args>(args)...);
}

void print(int& x) { std::cout << "左值引用: " << x << "\n"; }
void print(int&& x) { std::cout << "右值引用: " << x << "\n"; }

void demo_forward() {
    std::cout << "\n--- 完美转发演示 ---\n";

    int a = 10;

    std::cout << "传递左值:\n";
    wrapper(print, a);  // 应该调用 print(int&)

    std::cout << "传递右值:\n";
    wrapper(print, 20);  // 应该调用 print(int&&)
}

// ==================== 返回值优化 (RVO/NRVO) ====================

// 编译器的返回值优化
struct Heavy {
    int data[1000];

    Heavy() {
        std::cout << "Heavy 构造函数\n";
    }

    Heavy(const Heavy&) {
        std::cout << "Heavy 拷贝构造函数\n";
    }

    // 即使有移动构造函数，RVO 也会跳过
    Heavy(Heavy&&) noexcept {
        std::cout << "Heavy 移动构造函数\n";
    }

    ~Heavy() {
        std::cout << "Heavy 析构函数\n";
    }
};

// 具名返回值优化 (NRVO)
Heavy create_heavy_named() {
    Heavy h;  // 局部变量
    return h; // 可能触发 NRVO
}

// 非具名返回值优化 (RVO)
Heavy create_heavy_unnamed() {
    return Heavy();  // 临时对象
}

void demo_rvo() {
    std::cout << "\n--- 返回值优化演示 ---\n";

    std::cout << "\n1. 创建具名返回值:\n";
    Heavy h1 = create_heavy_named();

    std::cout << "\n2. 创建非具名返回值:\n";
    Heavy h2 = create_heavy_unnamed();

    std::cout << "\n离开作用域:\n";
}

// ==================== 移动语义与 STL 容器 ====================

void demo_stl_move() {
    std::cout << "\n--- STL 容器中的移动语义 ---\n";

    std::vector<std::string> v1;
    v1.push_back("hello");  // 字符串字面量转为 std::string

    std::string s = "world";
    v1.push_back(s);         // 拷贝
    v1.push_back(std::move(s));  // 移动

    std::cout << "s after move: " << (s.empty() ? "empty" : s) << "\n";

    // emplace_back 直接在容器中构造
    std::vector<std::string> v2;
    v2.emplace_back(5, 'x');  // 直接构造 "xxxxx"

    // 移动 vector
    std::vector<std::string> v3 = std::move(v1);
    std::cout << "v1 是否为空: " << v1.empty() << "\n";
    std::cout << "v3 大小: " << v3.size() << "\n";
}

// ==================== 移动语义的最佳实践 ====================

class Resource {
public:
    Resource() { std::cout << "Resource 构造\n"; }
    ~Resource() { std::cout << "Resource 析构\n"; }

    Resource(const Resource&) { std::cout << "Resource 拷贝\n"; }
    Resource(Resource&&) noexcept { std::cout << "Resource 移动\n"; }

    Resource& operator=(const Resource&) {
        std::cout << "Resource 拷贝赋值\n";
        return *this;
    }

    Resource& operator=(Resource&&) noexcept {
        std::cout << "Resource 移动赋值\n";
        return *this;
    }
};

class Widget {
public:
    // 默认构造函数
    Widget() : resource_(std::make_unique<Resource>()) {
        std::cout << "Widget 默认构造\n";
    }

    // 拷贝构造函数
    Widget(const Widget& other)
        : resource_(std::make_unique<Resource>(*other.resource_)) {
        std::cout << "Widget 拷贝构造\n";
    }

    // 移动构造函数
    Widget(Widget&& other) noexcept
        : resource_(std::move(other.resource_)) {
        std::cout << "Widget 移动构造\n";
    }

    // 拷贝赋值
    Widget& operator=(const Widget& other) {
        if (this != &other) {
            resource_ = std::make_unique<Resource>(*other.resource_);
        }
        std::cout << "Widget 拷贝赋值\n";
        return *this;
    }

    // 移动赋值
    Widget& operator=(Widget&& other) noexcept {
        if (this != &other) {
            resource_ = std::move(other.resource_);
        }
        std::cout << "Widget 移动赋值\n";
        return *this;
    }

private:
    std::unique_ptr<Resource> resource_;
};

void demo_best_practice() {
    std::cout << "\n--- 最佳实践演示 ---\n";

    std::cout << "\n1. 拷贝构造:\n";
    Widget w1;
    Widget w2 = w1;

    std::cout << "\n2. 移动构造:\n";
    Widget w3 = std::move(w1);

    std::cout << "\n3. 移动赋值:\n";
    Widget w4;
    w4 = std::move(w3);
}

// ==================== 主函数 ====================

void demo_move_vector() {
    std::cout << "\n--- 移动语义与 Vector ---\n";

    std::cout << "\n创建包含大对象的 vector:\n";
    std::vector<Buffer> vec;

    std::cout << "\npush_back Buffer(1000):\n";
    vec.push_back(Buffer(1000));  // 临时对象，可能被移动

    std::cout << "\npush_back(std::move(Buffer(2000))):\n";
    vec.push_back(std::move(Buffer(2000)));

    std::cout << "\nemplace_back:\n";
    vec.emplace_back(3000);
}

} // namespace move_demo

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 专题 2: 移动语义与右值引用\n";
    std::cout << "======================================\n";

    using namespace move_demo;

    demo_lvalue_rvalue();
    demo_rvalue_ref();
    demo_move_constructor();
    demo_move_assignment();
    demo_std_move();
    demo_forward();
    demo_rvo();
    demo_stl_move();
    demo_best_practice();
    demo_move_vector();

    std::cout << "\n======================================\n";
    std::cout << "移动语义要点总结：\n";
    std::cout << "1. 左值：有持久状态，可取地址\n";
    std::cout << "2. 右值：临时对象，生命周期短\n";
    std::cout << "3. 移动语义：转移资源所有权，避免拷贝\n";
    std::cout << "4. std::move：将左值强制转为右值引用\n";
    std::cout << "5. 完美转发：std::forward 保持属性\n";
    std::cout << "6. RVO/NRVO：编译器的返回值优化\n";
    std::cout << "======================================\n";

    return 0;
}
