#include "cache-server.h"
#include "cache-session.h"
#include "request-buffer.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

namespace bpo = boost::program_options;

extern SimpleCache *g_simpleCache;
extern RequestBuffer *g_requestBuffer;
extern SessionManager *g_sessionManager;

int main(int argc, char **argv) {

    bpo::options_description desc("Allowed options...");

    desc.add_options()("portNumber,p", bpo::value<short>()->default_value(2333),
                       "Listening port")(
        "maxCacheSize,m", bpo::value<long long>()->default_value(1000000),
        "The maximum number of key-value pairs that can be stored in the "
        "cache.")(
        "expireCycle,c", bpo::value<long long>()->default_value(25000),
        "The period(ms) in which the key-value is checked for expiration.")(
        "maxBufferSize,b", bpo::value<long long>()->default_value(20000),
        "The maximum number of requests that can be buffered.")(
        "sessionDuration,s", bpo::value<long long>()->default_value(30000),
        "Duration(ms) of session.");

    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
    vm.notify();

    g_simpleCache = new SimpleCache(vm["maxCacheSize"].as<long long>(),
                                    vm["expireCycle"].as<long long>());

    g_requestBuffer = new RequestBuffer(vm["maxBufferSize"].as<long long>());

    g_sessionManager = new SessionManager(
        vm["portNumber"].as<short>(), vm["sessionDuration"].as<long long>());

    std::cout << "Start simple cache service." << std::endl;
    auto serverTask = std::thread(startServer);
    auto expireTask = std::thread(startExpire);
    auto sessionTask = std::thread(startSession);

    serverTask.join();
    expireTask.join();
    sessionTask.join();
    std::cout << "Simple cache service closed." << std::endl;
}
