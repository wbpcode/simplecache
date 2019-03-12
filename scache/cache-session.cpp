#include "cache-session.h"
#include "request-buffer.h"
#include <regex>

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

Session::Session(TcpSocket sock, size_t bufferSize)
    : m_sock(std::move(sock)), m_bufferSize(bufferSize),
      m_buffer(std::string(m_bufferSize, '0')) {
    auto remoteEndpoint = m_sock.remote_endpoint();
    m_name = remoteEndpoint.address().to_string() + ":" +
             std::to_string(remoteEndpoint.port());
}

Session::~Session() {
    m_sock.shutdown(m_sock.shutdown_both);
    m_sock.close();
}

void Session::async_recv() {
    m_sock.async_read_some(
        boost::asio::buffer(m_buffer),
        [this](const boost::system::error_code &ec, size_t size) {
            if (!ec && m_recvHandler) {
                std::string temp = std::string(m_buffer, 0, size);
                m_recvHandler(m_name, temp);
            }
        });
}
void Session::aysnc_send(const std::string &result) {
    m_buffer.replace(0, result.size(), result);
    boost::asio::async_write(
        m_sock, boost::asio::buffer(m_buffer, result.size()),
        [this](const boost::system::error_code &ec, size_t size) {
            if (!ec && m_sendHandler) {
                std::string temp = std::string(m_buffer, 0, size);
                m_sendHandler(m_name, temp);
            }
        });
}

void Session::setSendHandler(Handler handler) { m_sendHandler = handler; }
void Session::setRecvHandler(Handler handler) { m_recvHandler = handler; }

std::string Session::getPeer() { return m_name; }

SessionManager::SessionManager(short port, int sessionDuration)
    : m_acceptor(m_io, Endpoint(boost::asio::ip::tcp::v4(), port)),
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
            auto newSession = new Session(std::move(m_sock), 4096);
            m_lock.lock();
            m_sockMap[newSession->getPeer()] = newSession;
            m_lock.unlock();
            newSession->setRecvHandler(revcHandlerImpl);
            newSession->setSendHandler(nullptr);
            std::cout << "New session: " + newSession->getPeer() << std::endl;
            newSession->async_recv();
        } else {
            std::cout << ec.message() << std::endl;
        }
        async_accept();
    });
}

int SessionManager::getSessionCount() { return m_sockMap.size(); }
int SessionManager::getSessionDuration() { return m_sessionDuration; }

void startSession() {
    if (!g_sessionManager || !g_requestBuffer) {
        std::cout << "No session manager and request buffer." << std::endl;
        exit(0);
    }
    g_sessionManager->runManager();
}