#pragma once

#include "request-buffer.h"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

using TcpSocket = boost::asio::ip::tcp::socket;
using Acceptor = boost::asio::ip::tcp::acceptor;
using EndPoint = boost::asio::ip::tcp::endpoint;
using Address = boost::asio::ip::address;
using IOService = boost::asio::io_service;
using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::milliseconds>;
using DeadTimer = boost::asio::deadline_timer;

using Handler = void (*)(std::string &, std::string &);

class Session {
  private:
    TcpSocket m_sock;
    std::string m_buffer;
    std::string m_name;

    DeadTimer m_deadTimer;

    Handler m_sendHandler;
    Handler m_recvHandler;
    Handler m_shutHandler;

    long long m_sessionDuration;
    long long m_lastAccess;

    void setDeadTimer(long long time);

  public:
    Session(TcpSocket sock, size_t bufferSize,
            long long sessionDuration = 1200000);
    ~Session();
    void async_recv();
    void aysnc_send(const std::string &result);
    void setSendHandler(Handler handler);
    void setRecvHandler(Handler handler);
    void setShutHandler(Handler handler);
    // ����������߳�ʱʱ��ִ��
    std::string getPeer();
};

class SessionManager {
  private:
    std::unordered_map<std::string, Session *> m_sockMap;
    IOService m_io;
    Acceptor m_acceptor;
    TcpSocket m_sock;
    std::mutex m_lock;
    long long m_sessionDuration;

  public:
    SessionManager(short port, long long sessionDuration);
    ~SessionManager();

    void runManager();

    void async_send(std::string &name, const std::string &result);

    long long getSessionCount();
    long long getSessionDuration();

    void shutSession(std::string &peer);

  private:
    void async_accept();
};

void startSession();