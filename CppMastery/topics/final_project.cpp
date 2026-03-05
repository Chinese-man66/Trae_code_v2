/**
 * C++ 高级特性综合项目 - 事件系统 (Event System)
 *
 * 本项目整合了以下知识点：
 * 1. 模板元编程 - 类型萃取、模板特化、SFINAE
 * 2. 移动语义 - 移动构造函数、移动赋值、完美转发
 * 3. 智能指针 - unique_ptr、shared_ptr、weak_ptr
 * 4. 虚函数表 - 多态、抽象类、override/final
 * 5. 内存管理 - 内存池、自定义 new/delete、RAII
 */

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <typeindex>
#include <mutex>

// ==================== 第一部分：模板元编程 ====================

namespace template_meta {

// 类型萃取：判断是否支持事件处理
template<typename T, typename = void>
struct has_handle_event : std::false_type {};

template<typename T>
struct has_handle_event<T,
    std::void_t<decltype(std::declval<T>().on_event(std::declval<const std::string&>()))>
> : std::true_type {};

// 类型萃取：判断是否为指针类型
template<typename T>
struct is_smart_pointer : std::false_type {};

template<typename T>
struct is_smart_pointer<std::unique_ptr<T>> : std::true_type {};

template<typename T>
struct is_smart_pointer<std::shared_ptr<T>> : std::true_type {};

// 编译时if - 条件编译
template<bool Condition, typename T>
using Conditional_t = std::conditional_t<Condition, T, void>;

// ==================== 第二部分：移动语义 ====================

namespace move_semantics {

// 可移动但不可拷贝的资源
class Buffer {
public:
    Buffer() : data_(nullptr), size_(0) {}

    explicit Buffer(size_t size) : size_(size) {
        data_ = new char[size];
        std::cout << "Buffer 构造: " << size_ << " 字节\n";
    }

    // 移动构造函数
    Buffer(Buffer&& other) noexcept : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "Buffer 移动构造\n";
    }

    // 移动赋值
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
            std::cout << "Buffer 移动赋值\n";
        }
        return *this;
    }

    // 禁用拷贝
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer() {
        delete[] data_;
    }

    char* data() const { return data_; }
    size_t size() const { return size_; }

private:
    char* data_;
    size_t size_;
};

// 完美转发包装器
template<typename Func, typename... Args>
decltype(auto) forward_call(Func&& func, Args&&... args) {
    return func(std::forward<Args>(args)...);
}

} // namespace move_semantics

// ==================== 第三部分：智能指针 ====================

namespace smart_ptr {

// 事件监听器 - 使用 shared_ptr
class EventListener {
public:
    explicit EventListener(const std::string& name) : name_(name) {
        std::cout << "EventListener 构造: " << name_ << "\n";
    }

    virtual ~EventListener() {
        std::cout << "EventListener 析构: " << name_ << "\n";
    }

    virtual void on_event(const std::string& event) = 0;

    const std::string& name() const { return name_; }

protected:
    std::string name_;
};

// 具体监听器
class ClickListener : public EventListener {
public:
    using EventListener::EventListener;

    void on_event(const std::string& event) override {
        std::cout << "[" << name_ << "] 处理点击事件: " << event << "\n";
    }
};

class KeyListener : public EventListener {
public:
    using EventListener::EventListener;

    void on_event(const std::string& event) override {
        std::cout << "[" << name_ << "] 处理键盘事件: " << event << "\n";
    }
};

// 使用 weak_ptr 避免循环引用
class ListenerRegistry {
public:
    void register_listener(std::shared_ptr<EventListener> listener) {
        listeners_[listener->name()] = listener;
    }

    void unregister_listener(const std::string& name) {
        listeners_.erase(name);
    }

    // 返回 weak_ptr，不增加引用计数
    std::weak_ptr<EventListener> get_listener(const std::string& name) {
        auto it = listeners_.find(name);
        if (it != listeners_.end()) {
            return it->second;
        }
        return {};
    }

    void notify_all(const std::string& event) {
        for (auto& [name, listener] : listeners_) {
            listener->on_event(event);
        }
    }

private:
    std::map<std::string, std::shared_ptr<EventListener>> listeners_;
};

// 工厂函数返回 unique_ptr
std::unique_ptr<EventListener> create_click_listener(const std::string& name) {
    return std::make_unique<ClickListener>(name);
}

std::unique_ptr<EventListener> create_key_listener(const std::string& name) {
    return std::make_unique<KeyListener>(name);
}

} // namespace smart_ptr

// ==================== 第四部分：虚函数与多态 ====================

namespace polymorphism {

// 事件基类 - 抽象类
class Event {
public:
    virtual ~Event() = default;

    // 纯虚函数
    virtual std::string get_type() const = 0;
    virtual void process() = 0;

    // 虚函数 - 可选重写
    virtual void log() const {
        std::cout << "[Event] " << get_type() << "\n";
    }

protected:
    Event() = default;

    // 允许派生类拷贝
    Event(const Event&) = default;
};

// 具体事件类型
class MouseEvent : public Event {
public:
    enum class Type { CLICK, MOVE, DRAG };

    MouseEvent(Type type, int x, int y) : type_(type), x_(x), y_(y) {}

    std::string get_type() const override {
        switch (type_) {
            case Type::CLICK: return "MouseClick";
            case Type::MOVE: return "MouseMove";
            case Type::DRAG: return "MouseDrag";
            default: return "Unknown";
        }
    }

    void process() override {
        std::cout << "处理鼠标事件: (" << x_ << ", " << y_ << ")\n";
    }

    void log() const override {
        std::cout << "[MouseEvent] " << get_type()
                  << " at (" << x_ << ", " << y_ << ")\n";
    }

    int x() const { return x_; }
    int y() const { return y_; }

private:
    Type type_;
    int x_, y_;
};

class KeyboardEvent : public Event {
public:
    enum class Type { PRESS, RELEASE };

    KeyboardEvent(Type type, int keycode) : type_(type), keycode_(keycode) {}

    std::string get_type() const override {
        return type_ == Type::PRESS ? "KeyPress" : "KeyRelease";
    }

    void process() override {
        std::cout << "处理键盘事件: keycode=" << keycode_ << "\n";
    }

    int keycode() const { return keycode_; }

private:
    Type type_;
    int keycode_;
};

// 事件处理器 - 使用多态
class EventHandler {
public:
    virtual ~EventHandler() = default;

    virtual void handle(std::unique_ptr<Event> event) = 0;

    // 模板方法
    template<typename T>
    T* as() { return dynamic_cast<T*>(this); }
};

class ConcreteHandler : public EventHandler {
public:
    void handle(std::unique_ptr<Event> event) override {
        event->process();
        event->log();
    }
};

// 事件分发器 - 使用类型信息
class EventDispatcher {
public:
    template<typename T>
    void register_handler(EventHandler* handler) {
        handlers_[std::type_index(typeid(T))] = handler;
    }

    template<typename T, typename... Args>
    void dispatch(Args&&... args) {
        auto event = std::make_unique<T>(std::forward<Args>(args)...);

        auto it = handlers_.find(std::type_index(typeid(T)));
        if (it != handlers_.end()) {
            it->second->handle(std::move(event));
        } else {
            std::cout << "没有处理器处理: " << event->get_type() << "\n";
        }
    }

private:
    std::map<std::type_index, EventHandler*> handlers_;
};

} // namespace polymorphism

// ==================== 第五部分：内存管理 ====================

namespace memory {

// 简单内存池
class MemoryPool {
public:
    explicit MemoryPool(size_t block_size, size_t count = 100)
        : block_size_(block_size), count_(count) {

        // 对齐
        block_size_ = (block_size + alignof(max_align_t) - 1)
                      & ~(alignof(max_align_t) - 1);

        pool_ = ::operator new(block_size_ * count_);

        // 初始化空闲链表
        free_list_ = nullptr;
        for (size_t i = 0; i < count_; ++i) {
            char* block = static_cast<char*>(pool_) + i * block_size_;
            *reinterpret_cast<void**>(block) = free_list_;
            free_list_ = block;
        }

        std::cout << "MemoryPool 构造: " << count_ << " x "
                  << block_size_ << " 字节\n";
    }

    ~MemoryPool() {
        ::operator delete(pool_);
        std::cout << "MemoryPool 析构\n";
    }

    void* allocate() {
        if (!free_list_) throw std::bad_alloc();

        void* result = free_list_;
        free_list_ = *reinterpret_cast<void**>(free_list_);
        return result;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        *reinterpret_cast<void**>(ptr) = free_list_;
        free_list_ = ptr;
    }

private:
    size_t block_size_;
    size_t count_;
    void* pool_;
    void* free_list_;
};

// 对象池 - 基于内存池
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t count = 50)
        : pool_(sizeof(T), count) {}

    template<typename... Args>
    T* create(Args&&... args) {
        void* ptr = pool_.allocate();
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            pool_.deallocate(ptr);
        }
    }

private:
    MemoryPool pool_;
};

// RAII 包装器
template<typename T>
class ScopedPointer {
public:
    explicit ScopedPointer(T* p = nullptr) : ptr_(p) {}
    ~ScopedPointer() { delete ptr_; }

    ScopedPointer(const ScopedPointer&) = delete;
    ScopedPointer& operator=(const ScopedPointer&) = delete;

    ScopedPointer(ScopedPointer&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ScopedPointer& operator=(ScopedPointer&& other) noexcept {
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

private:
    T* ptr_;
};

// 自定义删除器的 unique_ptr
struct CustomDeleter {
    void operator()(polymorphism::Event* ptr) {
        std::cout << "自定义删除 Event\n";
        delete ptr;
    }
};

} // namespace memory

// ==================== 主函数：整合演示 ====================

int main() {
    std::cout << "======================================\n";
    std::cout << "   C++ 高级特性综合项目 - 事件系统\n";
    std::cout << "======================================\n\n";

    // ===== 1. 模板元编程 =====
    std::cout << "=== 1. 模板元编程 ===\n";
    {
        // 类型萃取
        struct HasHandler {
            void on_event(const std::string&) {}
        };
        struct NoHandler {};

        std::cout << "has_handle_event<HasHandler>: "
                  << template_meta::has_handle_event<HasHandler>::value << "\n";
        std::cout << "has_handle_event<NoHandler>: "
                  << template_meta::has_handle_event<NoHandler>::value << "\n";

        // is_smart_pointer
        std::cout << "is_smart_pointer<unique_ptr<int>>: "
                  << template_meta::is_smart_pointer<std::unique_ptr<int>>::value << "\n";
    }

    // ===== 2. 移动语义 =====
    std::cout << "\n=== 2. 移动语义 ===\n";
    {
        using namespace move_semantics;

        // 移动构造
        Buffer b1(100);
        Buffer b2 = std::move(b1);

        // 完美转发
        auto lambda = [](int a, int b) { return a + b; };
        std::cout << "forward_call 结果: "
                  << forward_call(lambda, 3, 4) << "\n";
    }

    // ===== 3. 智能指针 =====
    std::cout << "\n=== 3. 智能指针 ===\n";
    {
        using namespace smart_ptr;

        ListenerRegistry registry;

        // 创建监听器
        auto listener1 = std::make_unique<ClickListener>("ClickHandler");
        auto listener2 = std::make_unique<KeyListener>("KeyHandler");

        // 注册
        registry.register_listener(std::move(listener1));
        registry.register_listener(std::move(listener2));

        // 触发事件
        registry.notify_all("button_click");
        registry.notify_all("key_press");

        // 使用 weak_ptr
        auto weak = registry.get_listener("ClickHandler");
        if (auto locked = weak.lock()) {
            locked->on_event("weak_ptr test");
        }
    }

    // ===== 4. 虚函数与多态 =====
    std::cout << "\n=== 4. 虚函数与多态 ===\n";
    {
        using namespace polymorphism;

        // 多态事件处理
        std::vector<std::unique_ptr<Event>> events;
        events.push_back(std::make_unique<MouseEvent>(MouseEvent::Type::CLICK, 100, 200));
        events.push_back(std::make_unique<KeyboardEvent>(KeyboardEvent::Type::PRESS, 65));
        events.push_back(std::make_unique<MouseEvent>(MouseEvent::Type::MOVE, 150, 250));

        for (auto& event : events) {
            event->process();
            event->log();
        }

        // 事件分发器
        EventDispatcher dispatcher;
        ConcreteHandler handler;
        dispatcher.register_handler<MouseEvent>(&handler);
        dispatcher.register_handler<KeyboardEvent>(&handler);

        std::cout << "\n使用分发器:\n";
        dispatcher.dispatch<MouseEvent>(MouseEvent::Type::CLICK, 300, 400);
        dispatcher.dispatch<KeyboardEvent>(KeyboardEvent::Type::PRESS, 27);
    }

    // ===== 5. 内存管理 =====
    std::cout << "\n=== 5. 内存管理 ===\n";
    {
        using namespace memory;

        // 内存池
        ObjectPool<polymorphism::Event> event_pool(10);

        auto* e1 = event_pool.create<polymorphism::MouseEvent>(
            polymorphism::MouseEvent::Type::CLICK, 10, 20);
        auto* e2 = event_pool.create<polymorphism::KeyboardEvent>(
            polymorphism::KeyboardEvent::Type::PRESS, 65);

        e1->process();
        e2->process();

        event_pool.destroy(e1);
        event_pool.destroy(e2);

        // 自定义删除器
        std::unique_ptr<polymorphism::Event, CustomDeleter> custom_event(
            new polymorphism::MouseEvent(polymorphism::MouseEvent::Type::MOVE, 0, 0));

        std::cout << "\n程序结束\n";
    }

    std::cout << "\n======================================\n";
    std::cout << "项目演示完成！\n";
    std::cout << "整合的知识点：\n";
    std::cout << "1. 模板元编程: has_handle_event, is_smart_pointer\n";
    std::cout << "2. 移动语义: Buffer 移动构造/赋值, 完美转发\n";
    std::cout << "3. 智能指针: unique_ptr, shared_ptr, weak_ptr\n";
    std::cout << "4. 虚函数: 多态Event, override/final\n";
    std::cout << "5. 内存管理: MemoryPool, ObjectPool, RAII\n";
    std::cout << "======================================\n";

    return 0;
}
