#pragma once
#include "cache-dict.h"
#include "cache-config.h"

class CacheExpireTable {
  private:
    std::unordered_map<std::string, int64> m_map;
  public:
    void set(std::string &key, int64 time);
    void del(std::string &key);
    bool checkExpire(std::string &key);
};

class CacheLock {
public:
    std::string m_name;
    int64 m_expireTime;
    CacheLock(std::string& name, int64 time);
    CacheLock() = default;
    virtual ~CacheLock() = default;
};

class CacheLockTable {
private:
    std::unordered_map<std::string, CacheLock> m_map;
public:
    void set(std::string &key, std::string &m_name, int64 time);
    void del(std::string &key);
    bool checkLock(std::string &key, std::string &m_name);
};

class SimpleCache : public LinkedDict {
  private:
    CacheExpireTable *m_expireTable;
    CacheLockTable* m_lockTable;
    GlobalConfig* m_globalConfig;

    SimpleCache();
    virtual ~SimpleCache();

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

    friend SimpleCache* getSimpleCache();
    friend void delSimpleCache();
};

SimpleCache* getSimpleCache();
void delSimpleCache();

void startServer();
void startExpire();