#include "ChatClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CChatClientDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CChatClientDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_SEND, &CChatClientDlg::OnBnClickedSend)
    ON_MESSAGE(WM_APP_CHAT, &CChatClientDlg::OnAppChat)
    ON_MESSAGE(WM_APP_STATUS, &CChatClientDlg::OnAppStatus)
    ON_MESSAGE(WM_APP_STATE, &CChatClientDlg::OnAppState)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

CChatClientDlg::CChatClientDlg(CWnd* pParent)
    : CDialogEx(IDD_CHATCLIENT_DIALOG, pParent) {
}

void CChatClientDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BOOL CChatClientDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        SetDlgItemText(IDC_STATIC_STATUS, _T("Winsock initialization failed"));
        GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
        return TRUE;
    }

    m_serverIp = "127.0.0.1";
    m_tcpPort = 5000;
    m_udpPort = 5001;
    m_hbIntervalMs = 5000;

    SetDlgItemTextA(IDC_EDIT_SERVERIP, m_serverIp.c_str());
    SetDlgItemInt(IDC_EDIT_PORT, static_cast<UINT>(m_tcpPort));
    SetDlgItemTextA(IDC_EDIT_USERID, "user1");
    SetDlgItemTextA(IDC_EDIT_PASSWORD, "");

    GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);

    SetDlgItemText(IDC_STATIC_STATUS, _T("Disconnected"));
    SetDlgItemText(IDC_STATIC_HEARTBEAT, _T("-"));

    return TRUE;
}

void CChatClientDlg::AppendChatLine(const CString& text) {
    CListBox* list = static_cast<CListBox*>(GetDlgItem(IDC_LIST_CHAT));
    if (list) {
        int idx = list->AddString(text);
        if (idx != LB_ERR && idx != LB_ERRSPACE) {
            list->SetCurSel(idx);
        }
    }
}

void CChatClientDlg::Disconnect() {
    m_running = false;
    m_connected = false;

    SOCKET old = m_socket;
    m_socket = INVALID_SOCKET;

    if (old != INVALID_SOCKET) {
        shutdown(old, SD_BOTH);
        closesocket(old);
    }

    if (m_recvThread.joinable()) m_recvThread.join();
    if (m_hbThread.joinable()) m_hbThread.join();
}

void CChatClientDlg::OnBnClickedConnect() {
    if (m_connected) return;

    Disconnect();

    char ip[128] = {};
    char user[128] = {};
    char password[256] = {};

    GetDlgItemTextA(IDC_EDIT_SERVERIP, ip, sizeof(ip));
    GetDlgItemTextA(IDC_EDIT_USERID, user, sizeof(user));
    GetDlgItemTextA(IDC_EDIT_PASSWORD, password, sizeof(password));

    UINT port = GetDlgItemInt(IDC_EDIT_PORT);
    if (!*ip || !*user || port == 0 || port > 65535) {
        SetDlgItemText(IDC_STATIC_STATUS, _T("Enter valid IP, port and user"));
        return;
    }

    m_serverIp = ip;
    m_tcpPort = static_cast<int>(port);
    m_userId = user;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        SetDlgItemText(IDC_STATIC_STATUS, _T("Cannot create TCP socket"));
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(m_tcpPort));
    inet_pton(AF_INET, m_serverIp.c_str(), &addr.sin_addr);

    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        SetDlgItemText(IDC_STATIC_STATUS, _T("TCP connection failed"));
        return;
    }

    std::string auth = std::string("{\"type\":\"auth\",\"userId\":\"") + m_userId +
        "\",\"password\":\"" + password + "\"}";

    std::string reply;
    if (!protocol::SendFrame(s, auth) || !protocol::RecvFrame(s, reply, 10000)) {
        shutdown(s, SD_BOTH);
        closesocket(s);
        SetDlgItemText(IDC_STATIC_STATUS, _T("Authentication failed"));
        return;
    }

    std::map<std::string, std::string> json;
    if (!JsonParse(reply, json) || JsonGet(json, "type") != "auth_ok") {
        shutdown(s, SD_BOTH);
        closesocket(s);
        SetDlgItemText(IDC_STATIC_STATUS, _T("Authentication rejected"));
        return;
    }

    DWORD noTimeout = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&noTimeout), sizeof(noTimeout));

    m_socket = s;
    m_connected = true;
    m_running = true;

    SetDlgItemText(IDC_STATIC_STATUS, _T("Connected"));
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);

    m_recvThread = std::thread(&CChatClientDlg::RecvWorker, this);
    m_hbThread = std::thread(&CChatClientDlg::HeartbeatWorker, this);
}

void CChatClientDlg::OnBnClickedDisconnect() {
    Disconnect();
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
    SetDlgItemText(IDC_STATIC_STATUS, _T("Disconnected"));
    SetDlgItemText(IDC_STATIC_HEARTBEAT, _T("-"));
}

void CChatClientDlg::OnDestroy() {
    Disconnect();
    WSACleanup();
    CDialogEx::OnDestroy();
}

void CChatClientDlg::OnBnClickedSend() {
    if (!m_connected) return;

    char buf[1024] = {};
    GetDlgItemTextA(IDC_EDIT_MESSAGE, buf, sizeof(buf));

    std::string message = buf;
    if (message.empty()) return;

    std::string json = std::string("{\"type\":\"chat\",\"message\":\"") + message + "\"}";

    if (protocol::SendFrame(m_socket, json)) {
        AppendChatLine(CString(("[You] " + message).c_str()));
        SetDlgItemTextA(IDC_EDIT_MESSAGE, "");
        SetDlgItemText(IDC_STATIC_STATUS, _T("Message sent"));
    }
}

void CChatClientDlg::RecvWorker(CChatClientDlg* self) {
    std::string frame;
    while (self->m_running) {
        frame.clear();
        if (!protocol::RecvFrame(self->m_socket, frame, 0)) {
            if (self->m_running) {
                self->PostMessage(WM_APP_STATUS, 0, reinterpret_cast<LPARAM>(_strdup("TCP connection lost")));
                self->PostMessage(WM_APP_STATE, 0, 0);
            }
            break;
        }

        self->PostMessage(WM_APP_CHAT, 0, reinterpret_cast<LPARAM>(_strdup(frame.c_str())));
    }
}

void CChatClientDlg::HeartbeatWorker(CChatClientDlg* self) {
    SOCKET udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp == INVALID_SOCKET) return;

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(static_cast<u_short>(self->m_udpPort));
    inet_pton(AF_INET, self->m_serverIp.c_str(), &dest.sin_addr);

    uint64_t seq = 0;

    while (self->m_running) {
        std::string hb = std::string("{\"userId\":\"") + self->m_userId +
            "\",\"seq\":" + std::to_string(++seq) + "}";

        sendto(udp, hb.data(), static_cast<int>(hb.size()), 0,
            reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

        std::string status = "Heartbeat sent (" + std::to_string(seq) + ")";
        self->PostMessage(WM_APP_STATUS, 1, reinterpret_cast<LPARAM>(_strdup(status.c_str())));

        for (int elapsed = 0; elapsed < self->m_hbIntervalMs && self->m_running; elapsed += 100) {
            Sleep(100);
        }
    }

    closesocket(udp);
}

LRESULT CChatClientDlg::OnAppChat(WPARAM, LPARAM lParam) {
    char* raw = reinterpret_cast<char*>(lParam);
    std::string frame = raw ? raw : "";
    free(raw);

    std::map<std::string, std::string> msg;
    if (!JsonParse(frame, msg)) return 0;

    std::string type = JsonGet(msg, "type");
    if (type == "chat") {
        std::string user = JsonGet(msg, "userId");
        std::string text = JsonGet(msg, "message");
        if (user != m_userId) {
            AppendChatLine(CString((user + ": " + text).c_str()));
        }
    }
    else if (type == "ack") {
        SetDlgItemText(IDC_STATIC_STATUS, _T("Message acknowledged"));
    }
    else if (type == "error") {
        std::string err = "[error] " + JsonGet(msg, "message");
        AppendChatLine(CString(err.c_str()));
    }

    return 0;
}

LRESULT CChatClientDlg::OnAppStatus(WPARAM wParam, LPARAM lParam) {
    char* raw = reinterpret_cast<char*>(lParam);
    CString msg(raw ? raw : "");
    free(raw);

    if (wParam == 1) {
        SetDlgItemText(IDC_STATIC_HEARTBEAT, msg);
    }
    else {
        SetDlgItemText(IDC_STATIC_STATUS, msg);
    }

    return 0;
}

LRESULT CChatClientDlg::OnAppState(WPARAM wParam, LPARAM) {
    if (wParam == 0) {
        OnBnClickedDisconnect();
    }
    return 0;
}
