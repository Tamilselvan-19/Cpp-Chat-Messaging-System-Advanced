#pragma once

#include <atomic>
#include <string>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <afxwin.h>
#include <afxdialogex.h>

#include "resource.h"

#define WM_APP_CHAT   (WM_APP + 1)
#define WM_APP_STATUS (WM_APP + 2)
#define WM_APP_STATE  (WM_APP + 3)

class CChatClientDlg : public CDialogEx {
public:
    CChatClientDlg(CWnd* pParent = nullptr);

    static void RecvWorker(CChatClientDlg* self);
    static void HeartbeatWorker(CChatClientDlg* self);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
    afx_msg void OnBnClickedSend();
    afx_msg LRESULT OnAppChat(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAppStatus(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAppState(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    void AppendChatLine(const CString& text);
    void Disconnect();

    std::string m_serverIp = "127.0.0.1";
    int m_tcpPort = 5000;
    int m_udpPort = 5001;
    int m_hbIntervalMs = 5000;

    SOCKET m_socket = INVALID_SOCKET;
    std::atomic<bool> m_connected{ false };
    std::atomic<bool> m_running{ false };
    std::thread m_recvThread;
    std::thread m_hbThread;
    std::string m_userId;
};
