/**
 * C++ 高级特性学习 - 专题 1: 模板元编程
 *
 * 本文件讲解:
 * 1. 模板参数推导
 * 2. 模板特化 (偏特化/全特化)
 * 3. 类型萃取 (Type Traits)
 * 4. 编译时计算
 * 5. SFINAE
 */

#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
#include <memory>
#include <array>

namespace template_demo {

// ==================== 1. 基础模板参数 ====================

// 基础模板
template<typename T>
class Container {
public:
    Container(T value) : value_(value) {}
    T get() const { return value_; }
private:
    T value_;
};

// 模板参数默认值
template<typename T, typename Allocator = std::allocator<T>>
class Vector {
public:
    void push(const T& value) { data_.push_back(value); }
    size_t size() const { return data_.size(); }
private:
    std::vector<T, Allocator> data_;
};

// ==================== 2. 模板特化 ====================

// 全特化：为特定类型提供特殊实现
template<>
class Container<bool> {
public:
    Container(bool value) : value_(value) {}
    bool get() const { return value_; }

    // bool 特化版特有的方法
    void flip() { value_ = !value_; }
private:
    bool value_;
};

// 偏特化：指针类型
template<typename T>
class Container<T*> {
public:
    Container(T* ptr) : ptr_(ptr) {}
    T* get() const { return ptr_; }
    bool is_null() const { return ptr_ == nullptr; }
private:
    T* ptr_;
};

// 偏特化：引用类型
template<typename T>
class Container<T&> {
public:
    Container(T& ref) : ptr_(std::make_shared<T>(ref)) {}
    T& get() const { return *ptr_; }
private:
    std::shared_ptr<T> ptr_;
};

// ==================== 3. 类型萃取 (Type Traits) ====================

// 编译时判断类型
void demo_type_traits() {
    std::cout << "\n--- 类型萃取演示 ---\n";

    // is_integral: 是否为整数类型
    std::cout << "is_integral<int>: " << std::is_integral<int>::value << "\n";
    std::cout << "is_integral<double>: " << std::is_integral<double>::value << "\n";
    std::cout << "is_integral<char>: " << std::is_integral<char>::value << "\n";

    // is_pointer: 是否为指针
    int* p = nullptr;
    std::cout << "is_pointer<int*>: " << std::is_pointer<decltype(p)>::value << "\n";
    std::cout << "is_pointer<int>: " << std::is_pointer<int>::value << "\n";

    // is_const: 是否为 const
    const int ci = 10;
    std::cout << "is_const<const int>: " << std::is_const<const int>::value << "\n";
    std::cout << "is_const<int>: " << std::is_const<int>::value << "\n";

    // is_reference: 是否为引用
    int x = 5;
    int& rx = x;
    std::cout << "is_reference<int&>: " << std::is_reference<int&>::value << "\n";
    std::cout << "is_reference<int>: " << std::is_reference<int>::value << "\n";

    // remove_reference: 移除引用
    using RefInt = int&;
    using PlainInt = std::remove_reference<RefInt>::type;
    std::cout << "remove_reference<int&>: " << std::is_same<int, PlainInt>::value << "\n";

    // add_pointer: 添加指针
    using PtrInt = std::add_pointer<int>::type;
    std::cout << "add_pointer<int>: " << std::is_same<int*, PtrInt>::value << "\n";
}

// 自定义类型萃取
template<typename T>
struct is_small {
    static constexpr bool value = sizeof(T) < 8;
};

template<typename T>
struct is_pointer_type : std::false_type {};

template<typename T>
struct is_pointer_type<T*> : std::true_type {};

// ==================== 4. 编译时计算 ====================

// 斐波那契数列 - 编译时计算
template<int N>
struct Fibonacci {
    static constexpr int value = Fibonacci<N-1>::value + Fibonacci<N-2>::value;
};

template<>
struct Fibonacci<0> {
    static constexpr int value = 0;
};

template<>
struct Fibonacci<1> {
    static constexpr int value = 1;
};

// 编译时计算数组大小
template<typename T, int N>
constexpr int array_size(T (&arr)[N]) {
    return N;
}

// 编译时判断数组长度
template<int N>
struct StaticArray {
    int data[N];
    constexpr int size() const { return N; }
};

// ==================== 5. SFINAE (Substitution Failure Is Not An Error) ====================

// 使用 enable_if 实现函数重载
template<typename T>
std::enable_if_t<std::is_integral<T>::value, T>
square(T n) {
    return n * n;
}

template<typename T>
std::enable_if_t<std::is_floating_point<T>::value, T>
square(T n) {
    return n * n;
}

// 检测类型是否有某个成员
template<typename T, typename = void>
struct has_size : std::false_type {};

template<typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

// 检测类型是否可以迭代
template<typename T, typename = void>
struct is_iterable : std::false_type {};

template<typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end())>>
    : std::true_type {};

// ==================== 6. 模板元编程实用例子 ====================

// 类型选择器
template<bool Condition, typename TrueType, typename FalseType>
struct Conditional {
    using type = TrueType;
};

template<typename TrueType, typename FalseType>
struct Conditional<false, TrueType, FalseType> {
    using type = FalseType;
};

template<bool Condition, typename TrueType, typename FalseType>
using Conditional_t = typename Conditional<Condition, TrueType, FalseType>::type;

// 类型列表
template<typename... Types>
struct TypeList {};

template<typename List>
struct Size;

template<typename... Types>
struct Size<TypeList<Types...>> {
    static constexpr size_t value = sizeof...(Types);
};

// 获取第 N 个类型
template<typename List, size_t N>
struct GetType;

template<typename Head, typename... Tail>
struct GetType<TypeList<Head, Tail...>, 0> {
    using type = Head;
};

template<typename Head, typename... Tail, size_t N>
struct GetType<TypeList<Head, Tail...>, N> {
    using type = typename GetType<TypeList<Types...>, N-1>::type;  // 这里有问题，应该用 Tail
};

// 修正：获取第 N 个类型
template<typename Head, typename... Tail, size_t N>
struct GetType<TypeList<Head, Tail...>, N> {
    using type = typename GetType<TypeList<Tail...>, N-1>::type;
};

// ==================== 主函数演示 ====================

void demo_basic_template() {
    std::cout << "\n--- 基础模板演示 ---\n";

    Container<int> ci(42);
    std::cout << "Container<int>: " << ci.get() << "\n";

    Container<std::string> cs("hello");
    std::cout << "Container<string>: " << cs.get() << "\n";
}

void demo_specialization() {
    std::cout << "\n--- 模板特化演示 ---\n";

    // 使用基础模板
    Container<int> ci(100);
    std::cout << "Container<int>: " << ci.get() << "\n";

    // 使用 bool 特化版
    Container<bool> cb(true);
    std::cout << "Container<bool>: " << cb.get() << "\n";
    cb.flip();
    std::cout << "After flip: " << cb.get() << "\n";

    // 使用指针特化版
    int x = 999;
    Container<int*> cp(&x);
    std::cout << "Container<int*>: " << *cp.get() << "\n";
    std::cout << "is_null: " << cp.is_null() << "\n";
}

void demo_compile_time() {
    std::cout << "\n--- 编译时计算演示 ---\n";

    // 斐波那契数列 - 编译时计算
    std::cout << "Fibonacci<10>: " << Fibonacci<10>::value << "\n";
    std::cout << "Fibonacci<20>: " << Fibonacci<20>::value << "\n";
    std::cout << "Fibonacci<30>: " << Fibonacci<30>::value << "\n";

    // 数组大小
    int arr[] = {1, 2, 3, 4, 5};
    std::cout << "array_size(arr): " << array_size(arr) << "\n";

    // 自定义类型萃取
    std::cout << "is_small<int>: " << is_small<int>::value << "\n";
    std::cout << "is_small<long>: " << is_small<long>::value << "\n";
    std::cout << "is_pointer_type<int*>: " << is_pointer_type<int*>::value << "\n";
    std::cout << "is_pointer_type<int>: " << is_pointer_type<int>::value << "\n";
}

void demo_sfinae() {
    std::cout << "\n--- SFINAE 演示 ---\n";

    // 函数重载 - 根据类型选择不同实现
    std::cout << "square(5): " << square(5) << "\n";
    std::cout << "square(3.14): " << square(3.14) << "\n";

    // 检测是否有 size 成员
    std::vector<int> vec;
    std::string str;
    int arr[5];

    std::cout << "has_size<vector<int>>: " << has_size<std::vector<int>>::value << "\n";
    std::cout << "has_size<string>: " << has_size<std::string>::value << "\n";
    std::cout << "has_size<int[5]>: " << has_size<int[5]>::value << "\n";

    // 检测是否可迭代
    std::cout << "is_iterable<vector<int>>: " << is_iterable<std::vector<int>>::value << "\n";
    std::cout << "is_iterable<string>: " << is_iterable<std::string>::value << "\n";
    std::cout << "is_iterable<int>: " << is_iterable<int>::value << "\n";
}

void demo_type_list() {
    std::cout << "\n--- 类型列表演示 ---\n";

    using MyTypes = TypeList<int, double, std::string, char>;

    std::cout << "TypeList size: " << Size<MyTypes>::value << "\n";
}

} // namespace template_demo

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 专题 1: 模板元编程\n";
    std::cout << "======================================\n";

    using namespace template_demo;

    demo_basic_template();
    demo_specialization();
    demo_compile_time();
    demo_sfinae();
    demo_type_list();

    // 类型萃取演示
    demo_type_traits();

    std::cout << "\n======================================\n";
    std::cout << "模板元编程要点总结：\n";
    std::cout << "1. 模板特化：全特化 + 偏特化\n";
    std::cout << "2. 类型萃取：编译时类型检查\n";
    std::cout << "3. SFINAE：函数重载的编译时选择\n";
    std::cout << "4. 编译时计算：constexpr + 模板递归\n";
    std::cout << "======================================\n";

    return 0;
}
