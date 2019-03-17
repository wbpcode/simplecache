#include"cache-config.h"
#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

GlobalConfig* getGlobalConfig() {
    // 静态局部变量，全局初始化一次
    static GlobalConfig* config = new GlobalConfig();
    return config;
}

void delGlobalConfig() {
    delete getGlobalConfig();
}

// 使用命令行参数初始化CacheGlobalConfig
void initGlobalConfig(int argc, char** argv) {
    auto config = getGlobalConfig();
    bpo::options_description desc("Allowed options...");
    desc.add_options()
        ("listeningPort,p", 
            bpo::value<int16>(&config->listeningPort)->default_value(2333),
            "Listening port.")
        ("maxCacheSize,m", 
            bpo::value<int64>(&config->maxCacheSize)->default_value(1000000),
            "The maximum number of key-value pairs that can be stored.")
        ("expireCycle,c", 
            bpo::value<int64>(&config->expireCycle)->default_value(25000),
            "The period(ms) in which the key-value is checked for expiration.")
        ("requestBufferSize,b", 
            bpo::value<int64>(&config->requestBufferSize)->default_value(20000),
            "The maximum number of requests that can be buffered.")
        ("sessionBufferSize,s",
            bpo::value<int64>(&config->sessionBufferSize)->default_value(4096),
            "The maxinum bytes of session that can be buffered.")
        ("sessionDuration,d", 
            bpo::value<int64>(&config->sessionDuration)->default_value(1200000),
            "Duration(ms) of session.")
        ("lockDuration,l", 
            bpo::value<int64>(&config->lockDuration)->default_value(5000),
            "Duration(ms) of lock.");

    bpo::variables_map parameterTable;
    bpo::store(bpo::parse_command_line(argc, argv, desc), parameterTable);
    parameterTable.notify();
}