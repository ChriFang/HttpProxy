#ifndef _HTTP_PROXY_SERVER_H_
#define _HTTP_PROXY_SERVER_H_

#include <winsock2.h>
#include "HttpProxySession.h"
#include <map>


//#include <Base/Mutex.h>
//HTTP代理服务类，因为只用了一个线程，暂时不用加锁机制

class HttpProxyServer
{
public:
	HttpProxyServer();
	~HttpProxyServer();

	static HttpProxyServer* GetInstance();

	bool Init();
	void Term();

	bool SetProxyHost(const char* pszHost);
	void SetSaveFileName(const char* pszFileName);
	unsigned short GetProxyServerPort();

private:
	bool	CreateListen();
	bool	CreateLoop();
	int		Loop();
	void	CloseServer();
	bool	InitNetLib();
	void	UnInitNetLib();

	int		HandlePoll(long iTimeOutSec);
	void	AddFdSet(fd_set* pReadSet, fd_set* pWriteSet, fd_set* pErrorSet);
	void	DoNetEvent(fd_set* pReadSet, fd_set* pWriteSet, fd_set* pErrorSet);

	static unsigned __stdcall LoopProc(void *pVoid);

private:
	void	AcceptConnect(SOCKET sock);
	void	AddConnect(SOCKET sock, SOCKADDR_IN* addr);
	void	CloseAllConnect();

private:
	int SetNoBlock(SOCKET sock);

private:
	bool				m_bIsInit;
	volatile bool		m_bExitFlag;
	unsigned short		m_uPort;
	SOCKET				m_sockListen;
	DWORD				m_dwLoopThread;

	CStringA			m_strRemoteServerIP;
	CStringA			m_strHost;
	CStringA			m_strSaveFileName;
	unsigned short		m_uRemoteServerPort;

private:
	typedef std::map<SOCKET, HttpProxySession*>		HTTP_PROXY_SESSION_MAP;
	HTTP_PROXY_SESSION_MAP	m_mapSession;

private:
	static HttpProxyServer m_Instance;
};

#endif