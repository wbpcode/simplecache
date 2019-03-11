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
using Endpoint = boost::asio::ip::tcp::endpoint;
using Address = boost::asio::ip::address;
using IOService = boost::asio::io_service;
using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::milliseconds>;
using Handler = void (*)(std::string &, std::string &);

class Session {
  private:
    TcpSocket m_sock;
    size_t m_bufferSize;
    std::string m_buffer;
    std::string m_name;

    TimePoint m_lastAccess;

    Handler m_sendHandler;
    Handler m_recvHandler;

  public:
    Session(TcpSocket sock, size_t bufferSize);
    ~Session();
    void async_recv();
    void aysnc_send(const std::string &result);
    void setSendHandler(Handler handler);
    void setRecvHandler(Handler handler);
    std::string getPeer();
};

class SessionManager {
  private:
    std::unordered_map<std::string, Session *> m_sockMap;
    IOService m_io;
    Acceptor m_acceptor;
    TcpSocket m_sock;
    std::mutex m_lock;

  public:
    SessionManager(short port);
    ~SessionManager();

    void runManager();

    void async_send(std::string &name, const std::string &result);

  private:
    void async_accept();
};

void startSession();