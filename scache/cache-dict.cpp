#include "cache-dict.h"
#include <map>

// 增加新对象或者更新对象
void CacheDict::addKeyValue(CacheObject *o) {
    // 搜索key是否存在
    CacheObject *old = nullptr;
    std::string key = o->getKey();
    if (m_map.find(key) != m_map.end()) {
        old = m_map[key];
    }
    m_map[key] = o;
    destoryInstance(old);
}

CacheObject *CacheDict::getKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return nullptr;
    }
    return m_map[key];
}

void CacheDict::delKeyValue(std::string key) {
    // 如果key不存在，直接返回;否者获取value，方便后续回收内存空间
    if (m_map.find(key) == m_map.end())
        return;
    auto value = m_map[key];
    m_map.erase(key);
    destoryInstance(value);
}

bool CacheDict::existKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    return true;
}

CacheDict::~CacheDict() {
    for (auto &e : m_map) {
        destoryInstance(e.second);
    }
}

int CacheDict::getSize() { return m_map.size(); }

LinkedDict::LinkedDict(std::string key, CacheType valueType = LinkType)
    : CacheContainer(key, valueType) {
    m_list =
        dynamic_cast<CacheList *>(getInstance(key + ":myCacheList", ListType));
}

LinkedDict::~LinkedDict() { destoryInstance(m_list); }

void LinkedDict::addKeyValue(CacheObject *o) {
    CacheListNode *node = nullptr;
    std::string key = o->getKey();
    if (m_map.find(key) != m_map.end())
        node = m_map[key];
    if (node) {
        CacheObject *old = node->getValue();
        node->setValue(o);
        destoryInstance(old);
        return;
    }
    node = new CacheListNode(o);
    m_map[key] = node;
    m_list->addNode(node);
}

CacheObject *LinkedDict::getKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end())
        return nullptr;
    auto node = m_map[key];
    return node->getValue();
}

void LinkedDict::delKeyValue(std::string key) {
    // 如果key不存在，直接返回;否者获取value，方便后续回收内存空间
    if (m_map.find(key) == m_map.end())
        return;
    auto node = m_map[key];
    m_map.erase(key);
    // 移除并销毁节点
    m_list->popNode(node);
    node->destoryValue();
    delete node;
}

CacheListNode *LinkedDict::addNode(CacheListNode *node,
                                   CacheListNode *pos = nullptr) {
    m_list->addNode(node, pos);
    m_map[node->getValue()->getKey()] = node;
    return node;
}
CacheListNode *LinkedDict::addNode(CacheListNode *node, std::string key) {
    CacheListNode *pos = getNode(key);
    return addNode(node, pos);
}
CacheListNode *LinkedDict::getNode(std::string key) {
    if (m_map.find(key) != m_map.end()) {
        return m_map[key];
    }
    return nullptr;
}
CacheListNode *LinkedDict::popNode(CacheListNode *node = nullptr) {
    CacheListNode *node = m_list->popNode(node);
    m_map.erase(node->getValue()->getKey());
    return node;
}
CacheListNode *LinkedDict::popNode(std::string key) {
    CacheListNode *node = getNode(key);
    return popNode(node);
}

CacheListNode *LinkedDict::getHead() { return m_list->getHead(); }
CacheListNode *LinkedDict::getTail() { return m_list->getTail(); }

bool LinkedDict::existKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    return true;
}

int LinkedDict::getSize() { return m_map.size(); }