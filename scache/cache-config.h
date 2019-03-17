#pragma once

#include<cstdint>

using int64 = long long;
using int32 = int32_t;
using int16 = int16_t;

// 相关配置项：直接暴露，没有提供相关的set/get
// 构造函数：私有，使用static成员函数创建，保证单例
class GlobalConfig {
private:
    GlobalConfig() = default;
    virtual ~GlobalConfig() = default;
public:
    int16 listeningPort = 2333;
    int64 maxCacheSize = 1000000; // 个
    int64 expireCycle = 25000; // ms
    int64 requestBufferSize = 20000; // 个
    int64 sessionBufferSize = 4096; // byte
    int64 sessionDuration = 1200000; // ms
    int64 lockDuration = 5000; // ms

    friend GlobalConfig* getGlobalConfig();
    friend void delGlobalConfig();
};

GlobalConfig* getGlobalConfig();
void delGlobalConfig();
void initGlobalConfig(int argc, char** argv);