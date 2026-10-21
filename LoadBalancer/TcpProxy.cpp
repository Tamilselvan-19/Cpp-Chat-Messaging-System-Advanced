#include "TcpProxy.h"

#include "../Common/Logger.h"
#include "../Common/Protocol.h"

namespace {
    void ForwardLoop(SOCKET from, SOCKET to, bool clientToBackend) {
        char buffer[4096];
        while (true) {
            int n = recv(from, buffer, sizeof(buffer), 0);
            if (n <= 0) break;

            if (!protocol::SendAll(to, buffer, n)) break;

            if (clientToBackend) {
                Logger::Info("Proxy    : forwarding client -> backend");
            }
        }
    }
}

void TcpProxy::StartProxyPair(std::shared_ptr<ProxySession> session) {
    if (!session || session->clientSocket == INVALID_SOCKET || session->backendSocket == INVALID_SOCKET) return;

    std::thread([session]() {
        ForwardLoop(session->clientSocket, session->backendSocket, true);
    }).detach();

    std::thread([session]() {
        ForwardLoop(session->backendSocket, session->clientSocket, false);
    }).detach();
}
