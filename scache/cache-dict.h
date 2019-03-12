#pragma once

#include "cache-list.h"
#include "cache-object.h"
#include <condition_variable>
#include <mutex>
#include <unordered_map>

// 对unordered_map的简单封装
class CacheDict : public CacheObject {
  private:
    std::unordered_map<std::string, CacheObject *> m_map;

  protected:
    CacheDict(std::string key, CacheType valueType = DictType)
        : CacheObject(key, valueType) {
        ;
    }
    virtual ~CacheDict();

  public:
    virtual void addKeyValue(std::string key, void *value, CacheType valueType);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    virtual bool existKeyValue(std::string key);

    int getSize();

    friend CacheObject *getInstance(std::string key, void *value,
                                    CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};

class LinkedDict : public CacheObject {
  private:
    CacheList *m_list;
    std::unordered_map<std::string, CacheListNode *> m_map;

  protected:
    LinkedDict(std::string key, CacheType valueType = LinkType);
    virtual ~LinkedDict();

  public:
    virtual void addKeyValue(std::string key, void *value, CacheType valueType);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    virtual bool existKeyValue(std::string key);

    virtual int getSize();

    CacheList *getList();

    friend CacheObject *getInstance(std::string key, void *value,
                                    CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};