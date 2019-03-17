#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include "cache-config.h"

struct Request {
    std::string m_name;
    std::vector<std::string> cmd;
};

class RequestBuffer {
private:
    std::queue<Request> m_buffer;
    std::mutex m_lock;
    std::condition_variable m_addCond;
    std::condition_variable m_getCond;

    GlobalConfig* m_globalConfig;

    RequestBuffer();
    virtual ~RequestBuffer() = default;

public:
    void addRequest(Request &rq);
    Request getRequest();

    friend RequestBuffer* getRequestBuffer();
    friend void delRequestBuffer();
};

RequestBuffer* getRequestBuffer();
void delRequestBuffer();