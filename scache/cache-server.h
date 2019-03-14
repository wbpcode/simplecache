#pragma once
#include "cache-dict.h"

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::milliseconds>;

class ExpireTable {
  private:
    // 毫秒精度的计时系统
    std::unordered_map<std::string, TimePoint> m_map;

  public:
    void set(std::string key, std::chrono::milliseconds time);
    void del(std::string key);
    bool checkExpire(std::string key);
};

class SimpleCache : public LinkedDict {
  private:
    ExpireTable *m_expire;
    long long m_maxCacheSize;
    long long m_expireCycle;

  public:
    virtual ~SimpleCache();
    SimpleCache(long long maxCacheSize, long long expireCycle);

    virtual void addKeyValue(CacheObject *o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    void setExpire(std::string key, long long time);
    bool checkExpire(std::string key);
    void delExpire(std::string key);

    long long getMaxCacheSize();
    long long getExpireCycle();
};

void startServer();
void startExpire();
