#include <iostream>
#include <type_traits>
#include <vector>

// ==================== 用途2: 类型操作（类型作为数据）====================

// 需求：判断两个类型是否能转换
// 传统方法：运行时检查
// 模板元：编译时检查

template<typename From, typename To>
struct CanConvert {
    static constexpr bool value = std::is_convertible<From, To>::value;
};

int main() {
    // 编译时判断类型关系
    std::cout << "int -> double: " << CanConvert<int, double>::value << "\n";
    std::cout << "double -> int: " << CanConvert<double, int>::value << "\n";
    std::cout << "int -> int: " << CanConvert<int, int>::value << "\n";
    std::cout << "int* -> void*: " << CanConvert<int*, void*>::value << "\n";

    // 这些都是在编译时确定的，不消耗运行时资源

    return 0;
}
