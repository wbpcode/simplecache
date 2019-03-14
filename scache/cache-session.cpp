#include "cache-session.h"
#include "request-buffer.h"
#include <boost/bind.hpp>
#include <regex>

namespace bpt = boost::posix_time;

SessionManager *g_sessionManager;
extern RequestBuffer *g_requestBuffer;

void revcHandlerImpl(std::string &peer, std::string &rawData) {
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
    g_requestBuffer->addRequest(rq);
}

void shutHandlerImpl(std::string &peer, std::string &message) {
    // �漰��Session�����٣����ȱ���peer��message
    std::string tempPeer = std::move(peer);
    std::string tempMessage = std::move(message);
    g_sessionManager->shutSession(peer);
    std::cout << "Session: " + tempPeer + " is shutdowned: " + tempMessage
              << std::endl;
}

Session::Session(TcpSocket sock, size_t bufferSize, long long sessionDuration)
    : m_sock(std::move(sock)), m_buffer(std::string(bufferSize, '0')),
      m_sessionDuration(sessionDuration), m_deadTimer(m_sock.get_io_service()) {
    auto remoteEndpoint = m_sock.remote_endpoint();

    m_name = remoteEndpoint.address().to_string() + ":" +
             std::to_string(remoteEndpoint.port());

    m_lastAccess = std::chrono::time_point_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now())
                       .time_since_epoch()
                       .count();

    setDeadTimer(m_sessionDuration);
}

void Session::setDeadTimer(long long time) {
    m_deadTimer.expires_from_now(bpt::millisec(time));
    m_deadTimer.async_wait([this](const boost::system::error_code &ec) {
        if (ec) {
            std::cout << "Dead Timer is cancelled." << std::endl;
            return;
        }
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now())
                       .time_since_epoch()
                       .count();
        auto interval = now - m_lastAccess;
        if (interval >= m_sessionDuration) {
            std::string messages = "Session expired.";
            m_shutHandler(m_name, messages);
        } else {
            setDeadTimer(m_sessionDuration - interval);
        }
    });
}

Session::~Session() {
    m_sock.shutdown(m_sock.shutdown_both);
    m_sock.close();
}

void Session::async_recv() {
    m_sock.async_read_some(
        boost::asio::buffer(m_buffer),
        [this](const boost::system::error_code &ec, size_t size) {
            if (!ec) {
                if (m_recvHandler) {
                    std::string temp = std::string(m_buffer, 0, size);
                    m_recvHandler(m_name, temp);
                }
                m_lastAccess =
                    std::chrono::time_point_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now())
                        .time_since_epoch()
                        .count();
            } else {
                if (m_shutHandler) {
                    m_shutHandler(m_name, ec.message());
                }
            }
        });
}
void Session::aysnc_send(const std::string &result) {
    m_buffer.replace(0, result.size(), result);
    boost::asio::async_write(
        m_sock, boost::asio::buffer(m_buffer, result.size()),
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
                }
            }
        });
}

void Session::setSendHandler(Handler handler) { m_sendHandler = handler; }
void Session::setRecvHandler(Handler handler) { m_recvHandler = handler; }
void Session::setShutHandler(Handler handler) { m_shutHandler = handler; }

std::string Session::getPeer() { return m_name; }

SessionManager::SessionManager(short port, long long sessionDuration)
    : m_acceptor(m_io, EndPoint(boost::asio::ip::tcp::v4(), port)),
      m_sock(m_io), m_sessionDuration(sessionDuration) {
    ;
}

SessionManager::~SessionManager() {
    for (auto &e : m_sockMap) {
        delete e.second;
    }
    m_io.stop();
}

void SessionManager::runManager() {
    async_accept();
    m_io.run();
}

void SessionManager::async_send(std::string &name, const std::string &result) {
    if (m_sockMap.find(name) == m_sockMap.end())
        return;
    m_lock.lock();
    auto session = m_sockMap[name];
    m_lock.unlock();
    session->aysnc_send(result);
}

void SessionManager::async_accept() {
    m_acceptor.async_accept(m_sock, [this](
                                        const boost::system::error_code &ec) {
        if (!ec) {
            auto newSession =
                new Session(std::move(m_sock), 4096, m_sessionDuration);
            m_lock.lock();
            m_sockMap[newSession->getPeer()] = newSession;
            m_lock.unlock();
            newSession->setRecvHandler(revcHandlerImpl);
            newSession->setSendHandler(nullptr);
            newSession->setShutHandler(shutHandlerImpl);
            std::cout << "New session: " + newSession->getPeer() << std::endl;
            newSession->async_recv();
        } else {
            std::cout << ec.message() << std::endl;
        }
        async_accept();
    });
}

long long SessionManager::getSessionCount() { return m_sockMap.size(); }
long long SessionManager::getSessionDuration() { return m_sessionDuration; }

void SessionManager::shutSession(std::string &peer) {
    if (m_sockMap.find(peer) == m_sockMap.end()) {
        return;
    }
    auto session = m_sockMap[peer];
    delete session;
    m_sockMap.erase(peer);
}

void startSession() {
    if (!g_sessionManager || !g_requestBuffer) {
        std::cout << "No session manager and request buffer." << std::endl;
        exit(0);
    }
    g_sessionManager->runManager();
}