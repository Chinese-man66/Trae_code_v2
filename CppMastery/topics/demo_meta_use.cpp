#include <iostream>

// ==================== 用途1: 编译时计算 ====================

// 运行时计算
int fib_runtime(int n) {
    if (n <= 1) return n;
    return fib_runtime(n-1) + fib_runtime(n-2);
}

// 编译时计算（模板元编程）
template<int N>
struct Fib {
    static constexpr int value = Fib<N-1>::value + Fib<N-2>::value;
};

template<>
struct Fib<0> { static constexpr int value = 0; };

template<>
struct Fib<1> { static constexpr int value = 1; };

int main() {
    // 运行时计算：程序执行时才算
    std::cout << "运行时计算 fib(30): " << fib_runtime(30) << "\n";

    // 编译时计算：程序编译完结果就确定了
    std::cout << "编译时计算 Fib<30>::value: " << Fib<30>::value << "\n";

    // 实际效果：
    // - 运行时计算需要递归 100 多万次
    // - 编译时计算在编译时完成，运行时直接用结果

    return 0;
}
