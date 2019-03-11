#include"cache-list.h"

void CacheListNode::setNext(CacheListNode* next) {
    m_next = next;
}

CacheListNode* CacheListNode::getNext() {
    return m_next;
}

void CacheListNode::setPrev(CacheListNode* prev) {
    m_prev = prev;
}

CacheListNode* CacheListNode::getPrev() {
    return m_prev;
}

void CacheListNode::setValue(CacheObject* value) {
    m_value = value;
}

CacheObject* CacheListNode::getValue() {
    return m_value;
}

// 弹出一个节点时:节点本身销毁，但是对应的CacheObject需要保留
// 删除一个节点时:需要同时销毁保存的CacheObject
// 所以提供一个函数显式销毁所管理的CacheObject
void CacheListNode::destoryValue() {
    destoryInstance(m_value);
}


// 根据key搜索
CacheListNode* CacheList::serachByKey(std::string key) {
    CacheListNode *temp = m_head->getNext();
    while (temp) {
        if (temp->getValue()->getKey() == key) {
            return temp;
        }
        temp = temp->getNext();
    }
    return nullptr;
}

void CacheList::addKeyValue(std::string key, void *value, CacheType valueType) {
    auto temp = getInstance(key, value, valueType);
    auto node = new CacheListNode(temp);
    addNode(node);
}

CacheObject* CacheList::getKeyValue(std::string key) {
    auto node = serachByKey(key);
    if (node) return node->getValue();
    return nullptr;
}

void CacheList::delKeyValue(std::string key) {
    auto node = serachByKey(key);
    if (!node) return;
    popNode(node);
    node->destoryValue();
    delete node;
}

// 插入一个新节点
CacheListNode* CacheList::addNode(CacheListNode* node, CacheListNode* pos = nullptr) {
    if (!pos) pos = m_head; // 若没有指定位置，默认从头部插入
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
    return node;
}

// 将一个节点从链表移除
CacheListNode* CacheList::popNode(CacheListNode* node = nullptr) {
    if (m_size <= 0) return nullptr;
    // 默认从尾部弹出节点
    if (!node) node = m_tail;
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

CacheList::CacheList(std::string key, CacheType valueType = ListType) 
    : CacheObject(key, valueType) {
    m_head = new CacheListNode();
    m_head->setNext(nullptr);
    m_head->setPrev(nullptr);
    m_tail = nullptr;
}

CacheList::~CacheList() {
    CacheListNode* temp = nullptr;
    while (temp = m_head->getNext()) {
        popNode(temp);
        temp->destoryValue();
        delete temp;
    }
    delete m_head;
}

CacheListNode* CacheList::getHead() {
    return m_head;
}

CacheListNode* CacheList::getTail() {
    return m_tail;
}

int CacheList::getSize() {
    return m_size;
}