#include "cache-dict.h"
#include <map>

// 增加新对象或者更新对象
void CacheDict::addKeyValue(std::string key, void *value, CacheType valueType) {
    // 搜索key是否存在
    CacheObject *object = nullptr;
    if (m_map.find(key) != m_map.end()) {
        object = m_map[key];
    }
    // key不存在或者value类型不匹配
    if (!object || object->getValueType() != valueType) {
        m_map[key] = getInstance(key, value, valueType);
        if (object)
            destoryInstance(object);
        return;
    }
    object->setValue(value);
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
    : CacheObject(key, valueType) {
    m_list = dynamic_cast<CacheList *>(
        getInstance("myCacheList", nullptr, ListType));
}

LinkedDict::~LinkedDict() { destoryInstance(m_list); }

void LinkedDict::addKeyValue(std::string key, void *value,
                             CacheType valueType) {
    CacheListNode *node = nullptr;
    if (m_map.find(key) != m_map.end())
        node = m_map[key];
    if (node) {
        auto object = node->getValue();
        if (object->getValueType() == valueType) {
            object->setValue(value);
        } else {
            destoryInstance(node->getValue());
            node->setValue(getInstance(key, value, valueType));
        }
        m_list->popNode(node);
        m_list->addNode(node);
        return;
    }
    node = new CacheListNode(getInstance(key, value, valueType));
    m_map[key] = node;
    m_list->addNode(node);
}

CacheObject *LinkedDict::getKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end())
        return nullptr;
    auto node = m_map[key];
    // 节点被访问，移动到链表首部
    m_list->popNode(node);
    m_list->addNode(node);

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
    destoryInstance(node->getValue());
    delete node;
}

CacheList *LinkedDict::getList() { return m_list; }

bool LinkedDict::existKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    return true;
}

int LinkedDict::getSize() { return m_map.size(); }