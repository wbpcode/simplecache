#pragma once
#include "cache-dict.h"

class ExpireTable {
  private:
    // 毫秒精度的计时系统
    std::unordered_map<std::string,
                       std::chrono::time_point<std::chrono::system_clock,
                                               std::chrono::milliseconds>>
        m_map;

  public:
    void set(std::string key, std::chrono::milliseconds time);
    void del(std::string key);
    bool checkExpire(std::string key);
};

class SimpleCache {
  private:
    LinkedDict *g_dict;
    ExpireTable *g_expire;

    int m_maxCacheSize;
    int m_expireCycle;

  public:
    virtual ~SimpleCache();
    SimpleCache(int maxCacheSize, int expireCycle);

    void addKeyValue(std::string key, void *value, CacheType valueType);
    CacheObject *getKeyValue(std::string key);
    void delKeyValue(std::string key);

    bool existKeyValue(std::string key);

    void setExpire(std::string key, long long time);
    bool checkExpire(std::string key);
    void delExpire(std::string key);

    int getCacheSize();

    int getMaxCacheSize();
    int getExpireCycle();

    CacheList *getList();
};

void startServer();
void startExpire();
