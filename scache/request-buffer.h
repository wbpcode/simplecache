#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

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
    long long m_maxSize;

  public:
    RequestBuffer(long long maxBufferSize);
    virtual ~RequestBuffer() { ; }
    void addRequest(Request &rq);
    Request getRequest();
    void setMaxSize(long long maxSize);
    long long getMaxSize();
    long long getSize();
};