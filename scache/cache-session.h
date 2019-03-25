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
#include "cache-config.h"

using TcpSocket = boost::asio::ip::tcp::socket;
using Acceptor = boost::asio::ip::tcp::acceptor;
using Endpoint = boost::asio::ip::tcp::endpoint;
using Address = boost::asio::ip::address;
using IOService = boost::asio::io_service;
using DeadTimer = boost::asio::deadline_timer;

using Handler = void (*)(std::string&, std::string&);

class Session {
private:
    TcpSocket m_tcpSocket;
    std::string m_buffer;
    std::string m_name;

    DeadTimer m_deadTimer;

    GlobalConfig* m_globalConfig;

    Handler m_sendHandler;
    Handler m_recvHandler;
    Handler m_shutHandler;

    int64 m_lastAccess;

    void setDeadTimer(int64 time);

public:
    Session(TcpSocket sock);
    virtual ~Session();
    void async_recv();
    void aysnc_send(const std::string &result);
    void setSendHandler(Handler handler);
    void setRecvHandler(Handler handler);
    void setShutHandler(Handler handler);

    std::string getPeer();
};

class SessionManager {
  private:
    std::unordered_map<std::string, Session*> m_sessionTable;
    std::mutex m_sessionTableLock;

    GlobalConfig* m_globalConfig;

    Endpoint  m_endpoint;
    IOService m_ioService;
    Acceptor  m_acceptor;
    TcpSocket m_tcpSocket;

    SessionManager();
    virtual ~SessionManager();

public:
    void runManager();

    void async_send(const std::string &name, const std::string &result);

    int64 getSessionCount();

    void shutSession(const std::string &peer);

    friend SessionManager* getSessionManager();
    friend void delSessionManager();

  private:
    void async_accept();
};

SessionManager* getSessionManager();
void delSessionManager();

inline int64 getCurrentTime();

void startSession();