#include <iostream>
#include <type_traits>
#include <vector>
#include <string>

// ==================== 用途4: 类型安全检查 ====================

// 需求：只有支持加法的类型才能调用 add 函数
template<typename T>
auto add(const T& a, const T& b) -> std::enable_if_t<std::is_arithmetic<T>::value, T> {
    return a + b;
}

// 需求：只有指针类型才能调用 deref
template<typename T>
auto deref(T* ptr) -> std::enable_if_t<std::is_pointer<T>::value, T> {
    return *ptr;
}

int main() {
    // 加法：只有数值类型可以
    std::cout << add(1, 2) << "\n";        // OK: 3
    std::cout << add(1.5, 2.5) << "\n";    // OK: 4.0
    // add("hello", "world");              // Error! 字符串不支持

    // 解引用：只有指针可以
    int x = 10;
    int* p = &x;
    std::cout << deref(p) << "\n";         // OK: 10
    // deref(123);                         // Error! 不是指针

    return 0;
}

// ==================== 用途5: 优化性能 ====================

// 需求：对于小类型，直接返回；对于大类型，做特殊处理
template<typename T>
struct OptimizedContainer {
    // 小类型（<= 8字节）：直接存储
    // 大类型（> 8字节）：用指针存储

    // 使用类型萃取选择正确的存储方式
    using StorageType = std::conditional_t<
        sizeof(T) <= 8,
        T,          // 直接存储
        T*          // 指针存储
    >;

    StorageType data;
};

// 编译器会根据 T 的大小自动选择最优的存储方式
