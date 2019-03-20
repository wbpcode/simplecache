#pragma once
#include "cache-config.h"
#include "cache-dict.h"
#include "cache-list.h"
#include "cache-base.h"

class SimpleCache {
public:
    using ClientLock = struct {
        std::string m_name;
        int64 m_expireTime;
    };
    using CacheTable = CacheDict<std::string, CacheListNode<CacheBase*>>;
    using ExpireTable = CacheDict<std::string, int64>;
    using ClientLockTable = CacheDict<std::string, ClientLock>;
    using LinkedList = CacheList<CacheBase*>;
    using PairType = CachePair<std::string, CacheListNode<CacheBase*>>;
    using NodeType = CacheListNode<CacheBase*>;

private:
    ExpireTable* m_expireTable;
    ClientLockTable* m_clientLockTable;
    LinkedList* m_linkedList;
    CacheTable* m_cacheTable;
    GlobalConfig* m_globalConfig;

    std::mutex m_simpleCacheLock;

    SimpleCache();
    virtual ~SimpleCache();

public:
    void set(std::string &key, CacheBase* value);
    CacheBase* get(std::string &key);
    void del(std::string &key);
    bool has(std::string &key);

    int64 getSize();
    NodeType* getHead();
    NodeType* getTail();
    
    int64 walk(std::function<void(const NodeType*)> func,
        int64 maxSize = LLONG_MAX);

    void setClientLock(std::string& key, std::string& name);
    void delClientLock(std::string& key);
    bool getClientLock(std::string& key, std::string& name);

    void setExpire(std::string& key, int64 time);
    void delExpire(std::string& key);
    bool getExpire(std::string& key);

    friend SimpleCache* getSimpleCache();
    friend void delSimpleCache();
};

SimpleCache* getSimpleCache();
void delSimpleCache();

void startServer();
void startExpire();