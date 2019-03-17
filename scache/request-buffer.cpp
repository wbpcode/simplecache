#include "request-buffer.h"
#include <condition_variable>



RequestBuffer::RequestBuffer() {
    m_globalConfig = getGlobalConfig();
}

void RequestBuffer::addRequest(Request &rq) {
    std::unique_lock<std::mutex> lock(m_lock);
    int64 maxSize = m_globalConfig->requestBufferSize;
    if (m_buffer.size() >= maxSize) {
        m_addCond.wait(lock, [this, maxSize]() 
        { return m_buffer.size() < maxSize / 2; });
    }
    m_buffer.push(std::move(rq));
    m_getCond.notify_one();
}

Request RequestBuffer::getRequest() {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_buffer.size() <= 0) {
        m_getCond.wait(lock);
    }
    auto request = m_buffer.front();
    m_buffer.pop();
    m_addCond.notify_one();
    return request;
}

RequestBuffer* getRequestBuffer() {
    static RequestBuffer* buffer = new RequestBuffer();
    return buffer;
}

void delRequestBuffer() {
    delete getRequestBuffer();
}