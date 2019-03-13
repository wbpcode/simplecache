#pragma once

#include "cache-list.h"
#include "cache-object.h"
#include <condition_variable>
#include <mutex>
#include <unordered_map>

// 对unordered_map的简单封装
class CacheDict : public CacheContainer {
  protected:
    std::unordered_map<std::string, CacheObject *> m_map;

    CacheDict(std::string key, CacheType valueType = DictType)
        : CacheContainer(key, valueType) {
        ;
    }
    virtual ~CacheDict();

  public:
    virtual void addKeyValue(CacheObject* o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    bool existKeyValue(std::string key);

    int getSize();

    friend CacheObject *getInstance(std::string key, CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};

class LinkedDict : public CacheContainer {
  protected:
    CacheList *m_list;
    std::unordered_map<std::string, CacheListNode *> m_map;

    LinkedDict(std::string key, CacheType valueType = LinkType);
    virtual ~LinkedDict();

  public:
    virtual void addKeyValue(CacheObject* o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    bool existKeyValue(std::string key);

    // 添加节点并且将其插入到m_map中
    CacheListNode *addNode(CacheListNode *node, CacheListNode *pos = nullptr);
    CacheListNode *addNode(CacheListNode *node, std::string key);
    // 返回节点
    CacheListNode *getNode(std::string key);
    // 弹出节点并且将其从m_map中擦除
    CacheListNode *popNode(CacheListNode *node = nullptr);
    CacheListNode *popNode(std::string key);

    int getSize();

    CacheListNode *getHead();
    CacheListNode *getTail();

    friend CacheObject *getInstance(std::string key, CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};