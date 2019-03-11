#pragma once

#include<vector>
#include<string>
#include<queue>
#include<mutex>
#include<condition_variable>

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
    int m_maxSize = 200000;

public:
    void addRequest(Request &rq);
    Request getRequest();
    void setMaxSize(int maxSize);
    int getMaxSize();
    int getSize();
};