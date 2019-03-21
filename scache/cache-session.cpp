#include "cache-session.h"
#include "request-buffer.h"
#include "cache-tool.h"
#include <boost/bind.hpp>
#include <regex>
#include <iostream>

namespace bpt = boost::posix_time;

void revcHandlerImpl(std::string &peer, std::string &rawData) {
    auto requestBuffer = getRequestBuffer();
    static std::regex re("(\\S+)|(\"[^\"]*\")");
    Request rq;
    rq.m_name = peer;
    std::sregex_iterator it(rawData.begin(), rawData.end(), re);
    std::sregex_iterator endIt;
    for (; it != endIt; ++it) {
        rq.cmd.push_back(std::move(it->str()));
    }
    if (rq.cmd.size() == 0) {
        rq.cmd.push_back(std::move(rawData));
    }
    requestBuffer->addRequest(rq);
}

void shutHandlerImpl(std::string &peer, std::string &message) {
    std::string tempPeer = std::move(peer);
    std::string tempMessage = std::move(message);
    auto sessionManager = getSessionManager();
    sessionManager->shutSession(peer);
    std::cout << "Session: " + tempPeer + " is shutdowned: " + tempMessage
              << std::endl;
}

Session::Session(TcpSocket sock)
    : m_tcpSocket(std::move(sock)), m_deadTimer(m_tcpSocket.get_io_service()) {
    m_globalConfig = getGlobalConfig();
    m_buffer = std::move(std::string(m_globalConfig->sessionBufferSize, '0'));
    auto remoteEndpoint = m_tcpSocket.remote_endpoint();
    m_name = remoteEndpoint.address().to_string() + ":" +
             std::to_string(remoteEndpoint.port());

    m_lastAccess = getCurrentTime();

    setDeadTimer(m_globalConfig->sessionDuration);
}

void Session::setDeadTimer(int64 time) {
    m_deadTimer.expires_from_now(bpt::millisec(time));
    m_deadTimer.async_wait([this](const boost::system::error_code &ec) {
        if (ec) {
            std::cout << "Dead Timer is cancelled." << std::endl;
            return;
        }
        auto now = getCurrentTime();
        auto interval = now - m_lastAccess;
        if (interval >= m_globalConfig->sessionDuration) {
            std::string messages = "Session expired.";
            m_shutHandler(m_name, messages);
        } else {
            setDeadTimer(m_globalConfig->sessionDuration - interval);
        }
    });
}

Session::~Session() {
    m_tcpSocket.shutdown(m_tcpSocket.shutdown_both);
    m_tcpSocket.close();
}

void Session::async_recv() {
    m_tcpSocket.async_read_some(
        boost::asio::buffer(m_buffer),
        [this](const boost::system::error_code &ec, size_t size) {
            if (!ec) {
                if (m_recvHandler) {
                    std::string temp = std::string(m_buffer, 0, size);
                    m_recvHandler(m_name, temp);
                }
                m_lastAccess = getCurrentTime();
            } else {
                if (m_shutHandler) {
                    m_shutHandler(m_name, ec.message());
                    m_deadTimer.cancel();
                }
            }
        });
}
void Session::aysnc_send(const std::string &result) {
    m_buffer.replace(0, result.size(), result);
    boost::asio::async_write(
        m_tcpSocket, boost::asio::buffer(m_buffer, result.size()),
        [this](const boost::system::error_code &ec, size_t size) {
            if (!ec) {
                if (m_sendHandler) {
                    std::string temp = std::string(m_buffer, 0, size);
                    m_sendHandler(m_name, temp);
                }
                async_recv();
            } else {
                if (m_shutHandler) {
                    m_shutHandler(m_name, ec.message());
                    m_deadTimer.cancel();
                }
            }
        });
}

void Session::setSendHandler(Handler handler) { m_sendHandler = handler; }
void Session::setRecvHandler(Handler handler) { m_recvHandler = handler; }
void Session::setShutHandler(Handler handler) { m_shutHandler = handler; }

std::string Session::getPeer() { return m_name; }

SessionManager::SessionManager()
    : m_globalConfig(getGlobalConfig()),
    m_endpoint(boost::asio::ip::tcp::v4(), m_globalConfig->listeningPort),
    m_acceptor(m_ioService, m_endpoint), m_tcpSocket(m_ioService) {
    std::cout << "Lisening port: " + std::to_string(m_endpoint.port()) << std::endl;
}

SessionManager::~SessionManager() {
    for (auto &e : m_sessionTable) {
        delete e.second;
    }
    m_ioService.stop();
}

void SessionManager::runManager() {
    async_accept();
    m_ioService.run();
}

void SessionManager::async_send(const std::string &name, const std::string &result) {
    if (m_sessionTable.find(name) == m_sessionTable.end())
        return;
    m_sessionTableLock.lock();
    auto session = m_sessionTable[name];
    m_sessionTableLock.unlock();
    session->aysnc_send(result);
}

void SessionManager::async_accept() {
    m_acceptor.async_accept(m_tcpSocket, 
        [this](const boost::system::error_code &ec) {
        if (!ec) {
            auto newSession = new Session(std::move(m_tcpSocket));
            m_sessionTableLock.lock();
            auto sessionName = newSession->getPeer();
            m_sessionTable[sessionName] = newSession;
            m_sessionTableLock.unlock();
            newSession->setRecvHandler(revcHandlerImpl);
            newSession->setSendHandler(nullptr);
            newSession->setShutHandler(shutHandlerImpl);
            std::cout << "New session: " + sessionName << std::endl;
            newSession->async_recv();
        } else {
            std::cout << ec.message() << std::endl;
        }
        async_accept();
    });
}

int64 SessionManager::getSessionCount() { return m_sessionTable.size(); }

void SessionManager::shutSession(const std::string &peer) {
    if (m_sessionTable.find(peer) == m_sessionTable.end()) {
        return;
    }
    auto session = m_sessionTable[peer];
    delete session;
    m_sessionTable.erase(peer);
}

SessionManager* getSessionManager() {
    static SessionManager* manager = new SessionManager();
    return manager;
}

void delSessionManager() {
    delete getSessionManager();
}


void startSession() {
    std::cout << "Session task is started." << std::endl;
    auto manager = getSessionManager();
    manager->runManager();
    std::cout << "Session task is closed." << std::endl;
}