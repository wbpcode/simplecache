#pragma once
#include "cache-dict.h"

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::milliseconds>;

class ExpireTable {
  private:
    // 毫秒精度的计时系统
    std::unordered_map<std::string,TimePoint> m_map;

  public:
    void set(std::string key, std::chrono::milliseconds time);
    void del(std::string key);
    bool checkExpire(std::string key);
};

class SimpleCache: public LinkedDict {
  private:
    ExpireTable *m_expire;
    int m_maxCacheSize;
    int m_expireCycle;

  public:
    virtual ~SimpleCache();
    SimpleCache(int maxCacheSize, int expireCycle);


    virtual void addKeyValue(CacheObject* o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    void setExpire(std::string key, long long time);
    bool checkExpire(std::string key);
    void delExpire(std::string key);

    int getMaxCacheSize();
    int getExpireCycle();
};

void startServer();
void startExpire();
