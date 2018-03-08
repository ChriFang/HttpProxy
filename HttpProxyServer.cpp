#include "stdafx.h"
#include "HttpProxyServer.h"
#include <Log/MyLogEx.h>


#define	DEFAULT_BIND_PORT			53311
#define BIND_RETRY_TIME				10
#define DEFAULT_SELECT_TIME_OUT		1


HttpProxyServer::HttpProxyServer() : 
	m_bIsInit(false),
	m_bExitFlag(false),
	m_uPort(DEFAULT_BIND_PORT),
	m_sockListen(INVALID_SOCKET),
	m_uRemoteServerPort(HTTP_DEFAULT_PORT)
{
	m_strRemoteServerIP.Empty();
	m_strHost.Empty();
}

HttpProxyServer::~HttpProxyServer()
{
}

HttpProxyServer HttpProxyServer::m_Instance;

HttpProxyServer* HttpProxyServer::GetInstance()
{
	return &m_Instance;
}

bool HttpProxyServer::Init()
{
	if(m_bIsInit)
	{
		return true;
	}

	if(!InitNetLib())
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "初始化网络库失败！！");
		return false;
	}

	if(!CreateListen())
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "创建代理服务器失败！！");
		return false;
	}

	if(!CreateLoop())
	{
		return false;
	}

	m_bIsInit = true;
	return true;
}

void HttpProxyServer::Term()
{
	CloseServer();

	UnInitNetLib();

	m_bIsInit = false;
}

bool HttpProxyServer::SetProxyHost(const char* pszHost)
{
	//Host字符串如：127.0.0.1:53311      www.baidu.com
	if(pszHost == NULL)
	{
		return false;
	}

	m_strHost = pszHost;

	char		szDomain[MAX_PATH];
	const char*	index;

	index = strchr(pszHost, ':');
	if(index == NULL)
	{
		if(strlen(pszHost) >= MAX_PATH)
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "SetProxyHost()出错，远端主机地址过长：%s", pszHost);
			return false;
		}
		else
		{
			strncpy_s(szDomain, MAX_PATH, pszHost, _TRUNCATE);
		}

		m_uRemoteServerPort = HTTP_DEFAULT_PORT;
	}
	else
	{
		int iDomainLen = (long)index - (long)pszHost;
		if(iDomainLen >= MAX_PATH)
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "SetProxyHost()出错，远端主机地址过长：%s", pszHost);
			return false;
		}
		else
		{
			strncpy_s(szDomain, MAX_PATH, pszHost, iDomainLen);
		}

		m_uRemoteServerPort = (unsigned short)atoi(index+1);
	}

	HOSTENT *hostent=gethostbyname(szDomain);
	if(hostent)
	{
		in_addr inad = *( (in_addr*) *hostent->h_addr_list);
		m_strRemoteServerIP = inet_ntoa(inad);
	}
	else
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "gethostbyname()出错！");
		return false;
	}

	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "Host=%s\nRemoteIp=%s\nRemotePort=%u", m_strHost, m_strRemoteServerIP, m_uRemoteServerPort);
	return true;
}

void HttpProxyServer::SetSaveFileName(const char* pszFileName)
{
	m_strSaveFileName = pszFileName;
}

unsigned short HttpProxyServer::GetProxyServerPort()
{
	return m_uPort;
}

bool HttpProxyServer::InitNetLib()
{
	WSADATA wsa_data;
	if(0 != WSAStartup(MAKEWORD(2, 0), &wsa_data))
	{
		return false;
	}

	return true;
}

void HttpProxyServer::UnInitNetLib()
{
	WSACleanup();
}

bool HttpProxyServer::CreateListen()
{
	SOCKADDR_IN serv_addr;

	if((m_sockListen = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "socket()失败，错误码=%d", WSAGetLastError());
		return false;
	}

	if(NO_ERROR != SetNoBlock(m_sockListen))
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "设置非阻塞套接字失败！！");
		return false;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(m_uPort);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int i = 0;
	for(; i<BIND_RETRY_TIME; i++)
	{
		if(SOCKET_ERROR != bind(m_sockListen, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)))
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "绑定端口%u", m_uPort);
			break;
		}

		m_uPort++;
		serv_addr.sin_port = htons(m_uPort);
	}

	if(i == BIND_RETRY_TIME)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "无法绑定合适端口！！！");
		closesocket(m_sockListen);
		return false;
	}

	if(SOCKET_ERROR == listen(m_sockListen, SOMAXCONN))
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "listen()失败，错误码=%d", WSAGetLastError());
		closesocket(m_sockListen);
		return false;
	}

	return true;
}

bool HttpProxyServer::CreateLoop()
{
	unsigned int ThreadAddr;
	m_dwLoopThread = _beginthreadex(
		NULL,			// Security
		0,				// Stack size
		LoopProc,		// Function address
		this,			// Argument
		0,				// Init flag
		&ThreadAddr);	// Thread address
	if (!m_dwLoopThread)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "创建线程失败，错误码=%d", GetLastError());
		return false;
	}

	return true;
}

unsigned __stdcall HttpProxyServer::LoopProc(void *pVoid)
{
	HttpProxyServer* pServer = (HttpProxyServer*)pVoid;
	return pServer->Loop();
}

void HttpProxyServer::CloseServer()
{
	m_bExitFlag = true;
	Sleep(DEFAULT_SELECT_TIME_OUT*1000);
}

int HttpProxyServer::SetNoBlock(SOCKET sock)
{
	unsigned long ul = 1;
	return ioctlsocket(sock, FIONBIO, (unsigned long *)&ul);
}

int	HttpProxyServer::Loop()
{
	while(true)
	{
		if(HandlePoll(DEFAULT_SELECT_TIME_OUT) < 0)
		{
			break;
		}
		if(m_bExitFlag)
		{
			HandlePoll(0);
			break;
		}
	}

	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "代理服务线程准备退出，清理所有连接");
	CloseAllConnect();

	closesocket(m_sockListen);
	m_sockListen = INVALID_SOCKET;

	return 0;
}

int HttpProxyServer::HandlePoll(long iTimeOutSec)
{
	int ret;
	fd_set rset, wset, eset;
	struct timeval tv = {iTimeOutSec, 0};

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	AddFdSet(&rset, &wset, &eset);

	ret = select(0, &rset, &wset, &eset, &tv);
	if(0 == ret)
	{
		return 0;	// time out
	}
	else if(SOCKET_ERROR == ret)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "select()出错，错误码=%d", WSAGetLastError());
		return -1;
	}

	DoNetEvent(&rset, &wset, &eset);

	return 0;
}

void HttpProxyServer::AddFdSet(fd_set* pReadSet, fd_set* pWriteSet, fd_set* pErrorSet)
{
	FD_SET(m_sockListen, pReadSet);
	FD_SET(m_sockListen, pErrorSet);

	HTTP_PROXY_SESSION_MAP::iterator iter = m_mapSession.begin();
	for(; iter != m_mapSession.end(); iter++)
	{
		HttpProxySession* pSession = iter->second;
		if(pSession->m_sessionPassive.IsConnectStatusConnected())
		{
			if(pSession->m_sessionPassive.IsConnectStatusWantRead())
			{
				FD_SET(pSession->GetPassiveSocket(), pReadSet);
				FD_SET(pSession->GetPassiveSocket(), pErrorSet);
			}
			if(pSession->m_sessionPassive.IsConnectStatusWantWrite())
			{
				FD_SET(pSession->GetPassiveSocket(), pWriteSet);
			}
		}

		if(pSession->m_sessionInitiative.IsConnectStatusConnected())
		{
			if(pSession->m_sessionInitiative.IsConnectStatusWantRead())
			{
				FD_SET(pSession->GetInitiativeSocket(), pReadSet);
				FD_SET(pSession->GetInitiativeSocket(), pErrorSet);
			}
			if(pSession->m_sessionInitiative.IsConnectStatusWantWrite())
			{
				FD_SET(pSession->GetInitiativeSocket(), pWriteSet);
			}
		}

		if(pSession->m_sessionInitiative.IsConnectStatusConnecting())
		{
			//FD_SET(pSession->GetInitiativeSocket(), pReadSet);
			FD_SET(pSession->GetInitiativeSocket(), pWriteSet);
			FD_SET(pSession->GetInitiativeSocket(), pErrorSet);
		}
	}
}

void HttpProxyServer::DoNetEvent(fd_set* pReadSet, fd_set* pWriteSet, fd_set* pErrorSet)
{
	//LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "DoNetEvent");
	if(FD_ISSET(m_sockListen, pReadSet))
	{
		AcceptConnect(m_sockListen);
	}

	if(FD_ISSET(m_sockListen, pErrorSet))
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "监听socket出现错误");
	}

	HTTP_PROXY_SESSION_MAP::iterator iter = m_mapSession.begin();
	for(; iter != m_mapSession.end();)
	{
		HttpProxySession* pSession = iter->second;
		SOCKET sock;

		if(pSession->m_sessionPassive.IsConnectStatusConnected())
		{
			sock = pSession->GetPassiveSocket();
			if(FD_ISSET(sock, pReadSet))
			{
				if(pSession->PassiveSession_Read() <= 0)
				{
					LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "被动连接端的连接断开");
					pSession->Close();

					iter = m_mapSession.erase(iter);
					delete pSession;

					continue;
				}
			}
			if(FD_ISSET(sock, pWriteSet))
			{
				pSession->PassiveSession_Write();
			}
			if(FD_ISSET(sock, pErrorSet))
			{
				pSession->PassiveSession_Error();
			}
		}

		if(pSession->m_sessionInitiative.IsConnectStatusConnected())
		{
			sock = pSession->GetInitiativeSocket();
			if(FD_ISSET(sock, pReadSet))
			{
				if(pSession->InitiativeSession_Read() <= 0)
				{
					if(pSession->m_sessionPassive.IsConnectStatusWantWrite())
					{
						pSession->m_sessionPassive.SetConnectStatusWriteAndClose();
					}
					else
					{
						pSession->Close();

						iter = m_mapSession.erase(iter);
						delete pSession;

						continue;
					}
				}
			}
			if(FD_ISSET(sock, pWriteSet))
			{
				pSession->InitiativeSession_Write();
			}
			if(FD_ISSET(sock, pErrorSet))
			{
				pSession->InitiativeSession_Error();
			}
		}

		if(pSession->m_sessionInitiative.IsConnectStatusConnecting())
		{
			sock = pSession->GetInitiativeSocket();
			if(FD_ISSET(sock, pWriteSet))
			{
				pSession->InitiativeSession_Connected();
			}
		}

		iter++;
	}
}

void HttpProxyServer::AcceptConnect(SOCKET sock)
{
	SOCKADDR_IN add_client;
	int iaddrSize = sizeof(SOCKADDR_IN);

	SOCKET sClient = accept(sock, (struct sockaddr *)&add_client, &iaddrSize);
	SetNoBlock(sClient);
	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "收到一个新的连接, socket=%d", sClient);

	AddConnect(sClient, &add_client);
}

void HttpProxyServer::AddConnect(SOCKET sock, SOCKADDR_IN* addr)
{
	HttpProxySession* pSession = new HttpProxySession();
	if(pSession != NULL)
	{
		pSession->m_sessionPassive.m_sock = sock;
		pSession->m_sessionPassive.SetConnectStatusWantRead();
		pSession->m_sessionPassive.SetConnectStatusConnected();
		pSession->m_iState = HttpProxySession::SESSION_STATE_PASSIVE_CONNECT;

		pSession->SetProxyInfo(m_strRemoteServerIP, m_uRemoteServerPort, m_strHost, m_strSaveFileName);
	}

	m_mapSession.insert(std::make_pair(sock, pSession));
}

void HttpProxyServer::CloseAllConnect()
{
	HTTP_PROXY_SESSION_MAP::iterator iter = m_mapSession.begin();
	for(; iter != m_mapSession.end(); iter++)
	{
		HttpProxySession* pSession = iter->second;
		pSession->Close();
		delete pSession;
	}

	m_mapSession.clear();
}
