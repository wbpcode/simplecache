#include"request-buffer.h"
#include<condition_variable>

RequestBuffer* g_requestBuffer;


void RequestBuffer::setMaxSize(int maxSize) {
    m_maxSize = maxSize;
}

int RequestBuffer::getMaxSize() {
    return m_maxSize;
}

int RequestBuffer::getSize() {
    return m_buffer.size();
}


void RequestBuffer::addRequest(Request &rq) {
    std::unique_lock<std::mutex> lock(m_lock);
    if (getSize() >= m_maxSize) {
        m_addCond.wait(lock, [this]() {
            return getSize() < getMaxSize() / 2;
        });
    }
    m_buffer.push(std::move(rq));
    m_getCond.notify_one();
}

Request RequestBuffer::getRequest() {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_buffer.size() <= 0) {
        m_getCond.wait(lock);
    }
    auto rq = m_buffer.front();
    m_buffer.pop();
    m_addCond.notify_one();
    return rq;
}