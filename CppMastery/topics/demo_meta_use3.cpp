#include <iostream>
#include <vector>
#include <array>

// ==================== 用途3: 自动生成代码 ====================

// 需求：打印任意容器的所有元素
// 传统方法：为每种容器写一个函数
// 模板元：一个函数搞定所有容器

template<typename Container>
void print_all(const Container& c) {
    for (const auto& elem : c) {
        std::cout << elem << " ";
    }
    std::cout << "\n";
}

// 需求：创建任意类型的数组
template<typename T, size_t N>
std::array<T, N> create_array(const T& default_value) {
    std::array<T, N> arr;
    arr.fill(default_value);
    return arr;
}

int main() {
    // 自动适配不同容器
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::array<double, 4> arr = {1.1, 2.2, 3.3, 4.4};
    int raw_arr[] = {10, 20, 30};

    print_all(vec);   // 自动识别 vector
    print_all(arr);  // 自动识别 array
    // print_all(raw_arr); // 也可以识别原生数组

    // 自动创建数组
    auto int_arr = create_array<int, 5>(99);
    auto str_arr = create_array<std::string, 3>("hi");

    for (auto& x : int_arr) std::cout << x << " ";
    std::cout << "\n";

    for (auto& x : str_arr) std::cout << x << " ";
    std::cout << "\n";

    return 0;
}
