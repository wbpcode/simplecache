#pragma once
#include "cache-dict.h"
#include "cache-config.h"
#include <mutex>

class CacheExpireDict {
  private:
    std::unordered_map<std::string, int64> m_map;
  public:
    void set(std::string &key, int64 time);
    void del(std::string &key);
    bool checkExpire(std::string &key);
};

struct CacheClientLock {
    std::string m_name;
    int64 m_expireTime;
    CacheClientLock(std::string& name, int64 time) : 
        m_name(name), m_expireTime(time) { ; }
    CacheClientLock() = default;
    virtual ~CacheClientLock() = default;
};

class CacheClientLockDict {
private:
    std::unordered_map<std::string, CacheClientLock> m_map;
public:
    void set(std::string &key, std::string &m_name, int64 time);
    void del(std::string &key);
    bool checkLock(std::string &key, std::string &m_name);
};

class SimpleCache : public LinkedDict {
  private:
    CacheExpireDict* m_cacheExpireDict;
    CacheClientLockDict* m_cacheClientLockDict;

    GlobalConfig* m_globalConfig;

    std::mutex m_lock;

    virtual ~SimpleCache();
    SimpleCache();

  public:
    virtual void addKeyValue(CacheObject *o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    void setExpire(std::string &key, int64 time);
    bool checkExpire(std::string &key);
    void delExpire(std::string &key);

    void setLock(std::string &key, std::string &name);
    bool checkLock(std::string &key, std::string &name);
    void delLock(std::string &key);

    void lock();
    void unlock();

    friend SimpleCache* getSimpleCache();
    friend void delSimpleCache();
};

SimpleCache* getSimpleCache();
void delSimpleCache();

void startServer();
void startExpire();