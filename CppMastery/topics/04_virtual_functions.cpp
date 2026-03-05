/**
 * C++ 高级特性学习 - 专题 4: 虚函数表与运行时多态
 *
 * 本文件讲解:
 * 1. 虚函数概念
 * 2. 虚函数表 (vtable) 原理
 * 3. 运行时绑定
 * 4. override 和 final 关键字
 * 5. 纯虚函数与抽象类
 * 6. 多态的实际应用
 */

#include <iostream>
#include <memory>
#include <vector>
#include <typeinfo>

namespace polymorphism_demo {

// ==================== 基础概念 ====================

// 基类：动物
class Animal {
public:
    // 构造函数
    Animal(const std::string& name) : name_(name) {
        std::cout << "Animal 构造: " << name_ << "\n";
    }

    // 虚析构函数 - 重要！
    virtual ~Animal() {
        std::cout << "Animal 析构: " << name_ << "\n";
    }

    // 虚函数 - 派生类可以重写
    virtual void speak() const {
        std::cout << name_ << " 发出了声音\n";
    }

    // 普通成员函数
    void introduce() const {
        std::cout << "我是一只 " << name_ << "\n";
    }

    // 纯虚函数 - 派生类必须实现
    virtual void move() const = 0;

protected:
    std::string name_;
};

// 派生类：狗
class Dog : public Animal {
public:
    Dog(const std::string& name, const std::string& breed)
        : Animal(name), breed_(breed) {
        std::cout << "Dog 构造: " << name_ << ", 品种: " << breed_ << "\n";
    }

    ~Dog() override {
        std::cout << "Dog 析构: " << name_ << "\n";
    }

    // 重写虚函数
    void speak() const override {
        std::cout << name_ << " 汪汪叫！\n";
    }

    // 实现纯虚函数
    void move() const override {
        std::cout << name_ << " 奔跑\n";
    }

    // Dog 特有的方法
    void fetch() const {
        std::cout << name_ << " 捡球\n";
    }

private:
    std::string breed_;
};

// 派生类：猫
class Cat : public Animal {
public:
    Cat(const std::string& name, bool indoor)
        : Animal(name), indoor_(indoor) {
        std::cout << "Cat 构造: " << name_ << ", 室内猫: " << (indoor_ ? "是" : "否") << "\n";
    }

    ~Cat() override {
        std::cout << "Cat 析构: " << name_ << "\n";
    }

    void speak() const override {
        std::cout << name_ << " 喵喵叫~\n";
    }

    void move() const override {
        std::cout << name_ << " 轻快地走\n";
    }

    void purr() const {
        std::cout << name_ << " 发出呼噜声\n";
    }

private:
    bool indoor_;
};

// ==================== 虚函数表原理 ====================

void demo_vtable_concept() {
    std::cout << "\n--- 虚函数表原理 ---\n";

    std::cout << "每个包含虚函数的类都有一个虚函数表 (vtable)\n";
    std::cout << "每个对象有一个指向 vtable 的指针 (vptr)\n";
    std::cout << "运行时通过 vptr 查找 vtable 调用正确的函数\n\n";

    std::cout << "Animal 类的 vtable:\n";
    std::cout << "  [0] Animal::~Animal\n";
    std::cout << "  [1] Animal::speak\n";
    std::cout << "  [2] Animal::move (纯虚)\n\n";

    std::cout << "Dog 类的 vtable:\n";
    std::cout << "  [0] Dog::~Dog\n";
    std::cout << "  [1] Dog::speak (重写)\n";
    std::cout << "  [2] Dog::move (实现)\n\n";

    std::cout << "Cat 类的 vtable:\n";
    std::cout << "  [0] Cat::~Cat\n";
    std::cout << "  [1] Cat::speak (重写)\n";
    std::cout << "  [2] Cat::move (实现)\n";
}

// ==================== 运行时多态演示 ====================

void demo_polymorphism() {
    std::cout << "\n--- 运行时多态演示 ---\n";

    // 静态绑定 vs 动态绑定
    std::cout << "\n1. 静态绑定 (编译时确定):\n";
    Dog dog1("旺财", "金毛");
    dog1.speak();  // 编译时确定调用 Dog::speak

    std::cout << "\n2. 动态绑定 (运行时确定):\n";
    Animal* animal_ptr = &dog1;
    animal_ptr->speak();  // 运行时通过 vptr 调用 Dog::speak

    std::cout << "\n3. 多态容器:\n";
    std::vector<std::unique_ptr<Animal>> animals;

    animals.push_back(std::make_unique<Dog>("小黑", "拉布拉多"));
    animals.push_back(std::make_unique<Cat>("小白", true));
    animals.push_back(std::make_unique<Dog>("大黄", "中华田园犬"));

    for (auto& animal : animals) {
        animal->speak();  // 多态调用
        animal->move();
    }

    std::cout << "\n4. 引用实现多态:\n";
    Dog dog2("二哈", "哈士奇");
    Animal& animal_ref = dog2;
    animal_ref.speak();  // 引用也实现多态

    // 释放资源
    animals.clear();
}

// ==================== override 和 final ====================

// final - 禁止派生
class Base {
public:
    virtual void method() {
        std::cout << "Base::method\n";
    }

    // final 虚函数 - 派生类不能重写
    virtual void final_method() {
        std::cout << "Base::final_method\n";
    }
};

class Derived final : public Base {
public:
    // override - 显式表明重写虚函数
    void method() override {
        std::cout << "Derived::method\n";
    }

    // Error! 不能重写 final 方法
    // void final_method() override {}
};

// 不能继承 final 类
// class MoreDerived : public Derived {};  // Error!

void demo_override_final() {
    std::cout << "\n--- override 和 final ---\n";

    Base b;
    Derived d;

    b.method();
    d.method();
    b.final_method();
    d.final_method();

    Base* ptr = &d;
    ptr->method();  // 动态绑定到 Derived::method
}

// ==================== 抽象类 ====================

// 抽象类：不能实例化
class Shape {
public:
    virtual ~Shape() = default;

    // 纯虚函数
    virtual double area() const = 0;
    virtual double perimeter() const = 0;

    // 实现的虚函数
    virtual void draw() const {
        std::cout << "绘制图形\n";
    }
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : width_(w), height_(h) {}

    double area() const override {
        return width_ * height_;
    }

    double perimeter() const override {
        return 2 * (width_ + height_);
    }

private:
    double width_;
    double height_;
};

class Circle : public Shape {
public:
    explicit Circle(double r) : radius_(r) {}

    double area() const override {
        return 3.14159 * radius_ * radius_;
    }

    double perimeter() const override {
        return 2 * 3.14159 * radius_;
    }

private:
    double radius_;
};

void demo_abstract_class() {
    std::cout << "\n--- 抽象类演示 ---\n";

    // 不能实例化抽象类
    // Shape s;  // Error!

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Rectangle>(3, 4));
    shapes.push_back(std::make_unique<Circle>(5));

    for (auto& shape : shapes) {
        std::cout << "面积: " << shape->area()
                  << ", 周长: " << shape->perimeter() << "\n";
    }
}

// ==================== typeid 运行时的类型信息 ====================

void demo_typeid() {
    std::cout << "\n--- typeid 演示 ---\n";

    Animal* dog = new Dog("旺财", "金毛");
    Animal* cat = new Cat("咪咪", true);

    std::cout << "dog 的类型: " << typeid(*dog).name() << "\n";
    std::cout << "cat 的类型: " << typeid(*cat).name() << "\n";

    // 运行时检查类型
    if (typeid(*dog) == typeid(Dog)) {
        std::cout << "dog 是 Dog 类型\n";

        // 向下转型
        Dog* real_dog = dynamic_cast<Dog*>(dog);
        if (real_dog) {
            real_dog->fetch();
        }
    }

    // dynamic_cast 安全转型
    Cat* cat_from_dog = dynamic_cast<Cat*>(dog);
    if (cat_from_dog) {
        std::cout << "转型成功\n";
    } else {
        std::cout << "dog 不能转型为 Cat\n";
    }

    delete dog;
    delete cat;
}

// ==================== 虚函数与内存布局 ====================

class Base1 {
public:
    virtual void vfunc1() {}
    virtual void vfunc2() {}
    int a;
};

class Derived1 : public Base1 {
public:
    void vfunc1() override {}  // 重写
    int b;
};

class Derived2 : public Base1 {
public:
    void vfunc2() override {}  // 重写另一个
    int c;
};

void demo_memory_layout() {
    std::cout << "\n--- 虚函数内存布局 ---\n";

    Base1 b;
    Derived1 d1;
    Derived2 d2;

    std::cout << "Base1 大小: " << sizeof(b) << " (vptr + int a)\n";
    std::cout << "Derived1 大小: " << sizeof(d1)
              << " (vptr + int a + int b)\n";
    std::cout << "Derived2 大小: " << sizeof(d2)
              << " (vptr + int a + int c)\n";

    // 注意：vptr 通常在对象开头
    std::cout << "\n对象布局 (通常):\n";
    std::cout << "  [vptr] --> vtable\n";
    std::cout << "  [a] (或 b, c)\n";
}

// ==================== 多态设计模式 ====================

// 策略模式示例
class Renderer {
public:
    virtual void render(const std::string& text) const = 0;
    virtual ~Renderer() = default;
};

class TextRenderer : public Renderer {
public:
    void render(const std::string& text) const override {
        std::cout << "文本: " << text << "\n";
    }
};

class HtmlRenderer : public Renderer {
public:
    void render(const std::string& text) const override {
        std::cout << "<div>" << text << "</div>\n";
    }
};

class JsonRenderer : public Renderer {
public:
    void render(const std::string& text) const override {
        std::cout << "{\"message\": \"" << text << "\"}\n";
    }
};

class Document {
public:
    Document(Renderer* r) : renderer_(r) {}

    void set_renderer(Renderer* r) {
        renderer_ = r;
    }

    void display(const std::string& text) {
        renderer_->render(text);
    }

private:
    Renderer* renderer_;
};

void demo_strategy_pattern() {
    std::cout << "\n--- 策略模式演示 ---\n";

    TextRenderer text_renderer;
    HtmlRenderer html_renderer;
    JsonRenderer json_renderer;

    Document doc(&text_renderer);
    doc.display("Hello World");

    doc.set_renderer(&html_renderer);
    doc.display("Hello World");

    doc.set_renderer(&json_renderer);
    doc.display("Hello World");
}

// ==================== 主函数 ====================

} // namespace polymorphism_demo

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 专题 4: 虚函数表与运行时多态\n";
    std::cout << "======================================\n";

    using namespace polymorphism_demo;

    demo_vtable_concept();
    demo_polymorphism();
    demo_override_final();
    demo_abstract_class();
    demo_typeid();
    demo_memory_layout();
    demo_strategy_pattern();

    std::cout << "\n======================================\n";
    std::cout << "虚函数与多态要点总结：\n";
    std::cout << "1. 虚函数: 运行时绑定，实现多态\n";
    std::cout << "2. vtable: 每个类一个，存放虚函数指针\n";
    std::cout << "3. vptr: 每个对象一个，指向 vtable\n";
    std::cout << "4. 纯虚函数: =0，抽象类不能实例化\n";
    std::cout << "5. override: 显式重写，检查基类虚函数\n";
    std::cout << "6. final: 禁止重写或继承\n";
    std::cout << "7. 虚析构: 确保正确析构派生类\n";
    std::cout << "======================================\n";

    return 0;
}
