#pragma once
#include "cache-base.h"
#include <functional>
#include <climits>

template<class T>
class CacheListNode {
public:
    using ValueType = T;
    using NodeType = CacheListNode<ValueType>;

private:
    NodeType *m_next;
    NodeType *m_prev;
    ValueType m_value;

public:
    void setNext(NodeType *next) { m_next = next; }
    NodeType *getNext() const { return m_next; }

    void setPrev(NodeType *prev) { m_prev = prev; }
    NodeType *getPrev() const { return m_prev; }

    void setValue(ValueType v) { m_value = v; }
    const ValueType& getValue() const { return m_value; }
    ValueType& getValue() { return m_value; }

    CacheListNode() = default;
    CacheListNode(ValueType& v) : m_value(v) { ; }
};

template<class T>
class CacheList : public CacheBase {
public:
    using ValueType = T;
    using NodeType = CacheListNode<ValueType>;

private:
    int64 m_size = 0;
    NodeType *m_head = nullptr;
    NodeType *m_tail = nullptr;

public:
    CacheList() :CacheBase(ListType) { 
        m_head = new NodeType();
        m_head->setNext(nullptr);
        m_head->setPrev(nullptr);
        m_tail = nullptr;
    }

    virtual ~CacheList() {
        NodeType *temp = m_head;
        NodeType *next = nullptr;
        while (next = temp->getNext()) {
            delete temp;
            temp = next;
        }
        delete temp;
    }


    void addNode(NodeType *node, NodeType *pos = nullptr) {
        if (!pos)
            pos = m_head; // 若没有指定位置，默认从头部插入
        auto temp = pos->getNext();

        pos->setNext(node);
        node->setPrev(pos);

        if (!temp) {
            node->setNext(temp);
            m_tail = node;
        }
        else {
            node->setNext(temp);
            temp->setPrev(node);
        }
        m_size++;
    }

    // 新建节点，添加到列表中，并返回该节点指针
    NodeType* add(ValueType& value) {
        auto node = new NodeType(value);
        addNode(node);
        return node;
    }


    template<class K>
    NodeType* getNode(K& t, std::function<bool(K&, NodeType*)> comp) {
        NodeType *temp = m_head->getNext();
        while (temp) {
            if (comp(t, temp)) return temp;
            temp = temp->getNext();
        }
        return nullptr;
    }

    NodeType* popNode(NodeType* node = nullptr) {
        if (m_size <= 0)
            return nullptr;
        // 默认从尾部弹出节点
        if (!node)
            node = m_tail;
        auto prev = node->getPrev();
        auto next = node->getNext();

        prev->setNext(next);
        if (next)
            next->setPrev(prev);
        else
            m_tail = prev;
        m_size--;
        return node;
    }

    ValueType pop(NodeType* node = nullptr) {
        node = popNode(node);
        if (!node) {
            throw std::string("Try pop empty list.");
        }
        ValueType temp = node->getValue();
       
        delete node;
        return temp;
    }

    int64 walk(std::function<void(const NodeType*)> func, int64 maxSize = LLONG_MAX,
        bool headToTail = true) {
        if (headToTail) {
            int64 count = 0;
            NodeType* temp = m_head->getNext();
            while (temp && count < maxSize) {
                func(temp);
                temp = temp->getNext();
                ++count;
            }
            return count;
        }
        else {
            int64 count = 0;
            NodeType* temp = m_tail;
            while (temp && count < maxSize) {
                func(temp);
                temp = temp->getPrev();
                ++count;
            }
            return count;
        }
    }

    NodeType* getHead() { return m_head; }
    NodeType* getTail() { return m_tail; }
    int64 getSize() { return m_size; }
};