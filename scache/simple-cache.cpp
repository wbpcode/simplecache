#include"cache-server.h"
#include"cache-session.h"
#include"request-buffer.h"
#include<thread>

extern SimpleCache *g_simpleCache;
extern RequestBuffer *g_requestBuffer;
extern SessionManager *g_sessionManager;

int main(int argc, char** argv){
    // 暂时未实现各种参数和接口配置
    g_simpleCache=new SimpleCache();
    g_requestBuffer=new RequestBuffer();
    g_sessionManager=new SessionManager(2333);

    auto serverTask=std::thread(startServer);
    auto expireTask=std::thread(startExpire);
    auto sessionTask=std::thread(startSession);

    serverTask.join();
    expireTask.join();
    sessionTask.join();
}
