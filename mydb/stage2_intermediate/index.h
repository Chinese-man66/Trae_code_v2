#pragma once

#include "../stage1_basics/record.h"
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace mydb {

// ==================== 索引抽象 ====================

// 索引接口
class Index {
public:
    virtual ~Index() = default;

    // 插入索引项
    virtual void insert(const Cell& key, RowId row_id) = 0;

    // 删除索引项
    virtual bool remove(const Cell& key, RowId row_id) = 0;

    // 查找键对应的行号
    virtual std::optional<RowId> find(const Cell& key) const = 0;

    // 范围查询：查找 >= start 且 < end 的所有行号
    virtual std::vector<RowId> find_range(const Cell& start,
                                          const Cell& end) const = 0;

    // 获取索引名称
    virtual const std::string& name() const = 0;

    // 获取索引列名
    virtual const std::string& column_name() const = 0;
};

// 比较函数类型
using CompareFunc = std::function<int(const Cell&, const Cell&)>;

// 键比较器
class KeyComparator {
public:
    explicit KeyComparator(const std::string& column_name)
        : column_name_(column_name) {}

    int operator()(const Cell& a, const Cell& b) const {
        return compare(a, b);
    }

    static int compare(const Cell& a, const Cell& b) {
        if (a.index() != b.index()) {
            return a.index() < b.index() ? -1 : 1;
        }

        return std::visit([&](auto&& arg_a, auto&& arg_b) -> int {
            using TA = std::decay_t<decltype(arg_a)>;
            using TB = std::decay_t<decltype(arg_b)>;

            if constexpr (std::is_same_v<TA, std::monostate> &&
                          std::is_same_v<TB, std::monostate>) {
                return 0;
            } else if constexpr (std::is_same_v<TA, std::monostate>) {
                return -1;
            } else if constexpr (std::is_same_v<TB, std::monostate>) {
                return 1;
            } else if constexpr (std::is_same_v<TA, int64_t> &&
                                  std::is_same_v<TB, int64_t>) {
                return arg_a < arg_b ? -1 : (arg_a > arg_b ? 1 : 0);
            } else if constexpr (std::is_same_v<TA, double> &&
                                  std::is_same_v<TB, double>) {
                return arg_a < arg_b ? -1 : (arg_a > arg_b ? 1 : 0);
            } else if constexpr (std::is_same_v<TA, std::string> &&
                                  std::is_same_v<TB, std::string>) {
                if (arg_a < arg_b) return -1;
                if (arg_a > arg_b) return 1;
                return 0;
            } else if constexpr (std::is_same_v<TA, bool> &&
                                  std::is_same_v<TB, bool>) {
                return (arg_a == arg_b) ? 0 : (arg_a ? -1 : 1);
            }
            return 0;
        }, a, b);
    }

private:
    std::string column_name_;
};

// ==================== 哈希索引（内存索引）====================

class HashIndex : public Index {
public:
    HashIndex(const std::string& name, const std::string& column_name)
        : name_(name), column_name_(column_name) {}

    void insert(const Cell& key, RowId row_id) override {
        // 一个键可能对应多行（哈希冲突时）
        index_[key].push_back(row_id);
    }

    bool remove(const Cell& key, RowId row_id) override {
        auto it = index_.find(key);
        if (it == index_.end()) return false;

        auto& row_ids = it->second;
        for (auto rit = row_ids.begin(); rit != row_ids.end(); ++rit) {
            if (*rit == row_id) {
                row_ids.erase(rit);
                if (row_ids.empty()) {
                    index_.erase(it);
                }
                return true;
            }
        }
        return false;
    }

    std::optional<RowId> find(const Cell& key) const override {
        auto it = index_.find(key);
        if (it != index_.end() && !it->second.empty()) {
            return it->second.front();
        }
        return std::nullopt;
    }

    std::vector<RowId> find_range(const Cell& start,
                                   const Cell& end) const override {
        // 哈希索引不支持范围查询，返回空
        return {};
    }

    const std::string& name() const override { return name_; }
    const std::string& column_name() const override { return column_name_; }

private:
    std::string name_;
    std::string column_name_;
    std::map<Cell, std::vector<RowId>, KeyComparator> index_;
};

// 哈希函数
struct CellHash {
    size_t operator()(const Cell& cell) const {
        return std::visit([](auto&& arg) -> size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return 0;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return std::hash<int64_t>{}(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                return std::hash<double>{}(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return std::hash<std::string>{}(arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                return std::hash<bool>{}(arg);
            }
            return 0;
        }, cell);
    }
};

// ==================== B+ 树索引 ====================

class BPlusTreeIndex : public Index {
public:
    // B+ 树节点
    struct Node {
        bool is_leaf = true;
        std::vector<Cell> keys;
        std::vector<RowId> values;  // 叶子节点：行号；内部节点：子节点指针（用 RowId 表示页号）
        std::shared_ptr<Node> next; // 叶子节点：指向下一个叶子
    };

    BPlusTreeIndex(const std::string& name,
                   const std::string& column_name,
                   size_t order = 4)
        : name_(name), column_name_(column_name), order_(order) {
        root_ = std::make_shared<Node>();
    }

    void insert(const Cell& key, RowId row_id) override;

    bool remove(const Cell& key, RowId row_id) override;

    std::optional<RowId> find(const Cell& key) const override;

    std::vector<RowId> find_range(const Cell& start,
                                  const Cell& end) const override;

    const std::string& name() const override { return name_; }
    const std::string& column_name() const override { return column_name_; }

private:
    // 分裂叶子节点
    std::shared_ptr<Node> split_leaf(std::shared_ptr<Node> node);

    // 分裂内部节点
    std::shared_ptr<Node> split_internal(std::shared_ptr<Node> node);

    // 查找键应该插入的位置
    size_t find_position(const std::shared_ptr<Node>& node,
                         const Cell& key) const;

    // 插入到叶子节点
    void insert_into_leaf(std::shared_ptr<Node> leaf,
                          const Cell& key, RowId row_id);

    // 插入到内部节点
    void insert_into_internal(std::shared_ptr<Node> node,
                              size_t pos,
                              const Cell& key,
                              std::shared_ptr<Node> right);

    std::string name_;
    std::string column_name_;
    size_t order_; // B+ 树的阶
    std::shared_ptr<Node> root_;
    KeyComparator comparator_;
};

// B+ 树实现
inline size_t BPlusTreeIndex::find_position(const std::shared_ptr<Node>& node,
                                            const Cell& key) const {
    return std::lower_bound(node->keys.begin(), node->keys.end(), key,
        [this](const Cell& a, const Cell& b) {
            return KeyComparator::compare(a, b) < 0;
        }) - node->keys.begin();
}

inline void BPlusTreeIndex::insert_into_leaf(std::shared_ptr<Node> leaf,
                                              const Cell& key,
                                              RowId row_id) {
    size_t pos = find_position(leaf, key);

    // 检查是否已存在
    if (pos < leaf->keys.size() &&
        KeyComparator::compare(leaf->keys[pos], key) == 0) {
        // 更新已存在的值
        leaf->values[pos] = row_id;
        return;
    }

    leaf->keys.insert(leaf->keys.begin() + pos, key);
    leaf->values.insert(leaf->values.begin() + pos, row_id);
}

inline std::shared_ptr<BPlusTreeIndex::Node>
BPlusTreeIndex::split_leaf(std::shared_ptr<Node> leaf) {
    auto new_leaf = std::make_shared<Node>();
    new_leaf->is_leaf = true;

    size_t mid = (leaf->keys.size() + 1) / 2;

    // 移动后半部分到新节点
    new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());

    // 更新原节点
    leaf->keys.erase(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->values.erase(leaf->values.begin() + mid, leaf->values.end());

    // 连接叶子节点
    new_leaf->next = leaf->next;
    leaf->next = new_leaf;

    return new_leaf;
}

inline std::shared_ptr<BPlusTreeIndex::Node>
BPlusTreeIndex::split_internal(std::shared_ptr<Node> internal) {
    auto new_internal = std::make_shared<Node>();
    new_internal->is_leaf = false;

    size_t mid = (internal->keys.size()) / 2;

    // 中间键上移到父节点
    Cell middle_key = internal->keys[mid];

    // 移动后半部分
    new_internal->keys.assign(internal->keys.begin() + mid + 1, internal->keys.end());
    new_internal->values.assign(internal->values.begin() + mid + 1, internal->values.end());

    // 更新原节点
    internal->keys.erase(internal->keys.begin() + mid + 1, internal->keys.end());
    internal->values.erase(internal->values.begin() + mid + 1, internal->values.end());

    return new_internal;
}

inline void BPlusTreeIndex::insert(const Cell& key, RowId row_id) {
    auto leaf = root_;

    // 如果根节点是叶子节点
    if (leaf->is_leaf) {
        if (leaf->keys.size() >= order_) {
            // 需要分裂
            auto new_leaf = split_leaf(leaf);

            // 创建新的根
            auto new_root = std::make_shared<Node>();
            new_root->is_leaf = false;
            new_root->keys.push_back(new_leaf->keys.front());
            new_root->values.push_back(0); // 伪行号，表示页指针
            new_root->values.push_back(reinterpret_cast<uintptr_t>(new_leaf.get()));

            root_ = new_root;
            leaf = root_;
        }
        insert_into_leaf(root_, key, row_id);
    } else {
        // 根是内部节点，递归向下查找
        size_t pos = find_position(root_, key);
        // 继续查找插入...
        insert_into_leaf(root_, key, row_id); // 简化实现
    }
}

inline bool BPlusTreeIndex::remove(const Cell& key, RowId row_id) {
    // 简化实现
    return true;
}

inline std::optional<RowId> BPlusTreeIndex::find(const Cell& key) const {
    if (root_->keys.empty()) return std::nullopt;

    auto node = root_;

    // 找到叶子节点
    while (!node->is_leaf) {
        size_t pos = find_position(node, key);
        if (pos >= node->keys.size()) {
            // 大于所有键，使用最后一个子节点
            pos = node->keys.size() - 1;
        }

        // 获取子节点指针（简化：用 values 存储指针）
        auto child_ptr = reinterpret_cast<Node*>(
            node->values[std::min(pos + 1, node->values.size() - 1)]);
        node = child_ptr->shared_from_this(); // 需要支持 shared_from_this
    }

    // 在叶子节点中查找
    size_t pos = find_position(node, key);
    if (pos < node->keys.size() &&
        KeyComparator::compare(node->keys[pos], key) == 0) {
        return node->values[pos];
    }

    return std::nullopt;
}

inline std::vector<RowId> BPlusTreeIndex::find_range(const Cell& start,
                                                      const Cell& end) const {
    std::vector<RowId> results;

    if (root_->keys.empty()) return results;

    // 找到起始叶子节点
    auto node = root_;
    while (!node->is_leaf) {
        size_t pos = find_position(node, start);
        auto child_ptr = reinterpret_cast<Node*>(
            node->values[std::min(pos, node->values.size() - 1)]);
        node = child_ptr->shared_from_this();
    }

    // 遍历叶子节点收集结果
    while (node) {
        for (size_t i = 0; i < node->keys.size(); ++i) {
            int cmp_start = KeyComparator::compare(node->keys[i], start);
            int cmp_end = KeyComparator::compare(node->keys[i], end);

            if (cmp_start >= 0 && cmp_end < 0) {
                results.push_back(node->values[i]);
            } else if (cmp_end >= 0) {
                return results;
            }
        }
        node = node->next;
    }

    return results;
}

} // namespace mydb
