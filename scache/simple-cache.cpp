#include "cache-session.h"
#include "cache-config.h"
#include "cache-server.h"
#include "request-buffer.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include "cache-base.h"
#include "cache-dict.h"
#include "cache-list.h"

int main(int argc, char **argv) {
    initGlobalConfig(argc, argv);
    
    std::cout << "Start simple cache service." << std::endl;

    auto serverTask = std::thread(startServer);
    auto sessionTask = std::thread(startSession);
    auto expireTask = std::thread(startExpire);

    serverTask.join();
    sessionTask.join();
    expireTask.join();

    delSessionManager(); 
    delSimpleCache(); 
    delRequestBuffer();
    delGlobalConfig();

    std::cout << "Simple cache service closed." << std::endl;
}
