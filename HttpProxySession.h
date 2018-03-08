#ifndef _HTTP_PROXY_SESSION_H_
#define _HTTP_PROXY_SESSION_H_

#include "ConnectSession.h"


class HttpProxySession
{
public:
	HttpProxySession();
	~HttpProxySession();

	enum SESSION_STATE
	{
		SESSION_STATE_INIT,
		SESSION_STATE_PASSIVE_CONNECT,
		SESSION_STATE_INITIATIVE_CONNECTING,
		SESSION_STATE_INITIATIVE_CONNECTED,
		SESSION_STATE_DEAD,
	};

	inline SOCKET GetPassiveSocket()
	{
		return m_sessionPassive.m_sock;
	}

	inline SOCKET GetInitiativeSocket()
	{
		return m_sessionInitiative.m_sock;
	}

	inline void SetProxyInfo(const char* pszRemoteServerIP, unsigned short uRemoteServerPort, const char* pszHost, const char* pszFileName)
	{
		m_strRemoteServerIP	= pszRemoteServerIP;
		m_strHost			= pszHost;
		m_uRemoteServerPort	= uRemoteServerPort;
		m_strFileName		= pszFileName;
	}

	int		PassiveSession_Read();
	int		PassiveSession_Write();
	int		PassiveSession_Error();
	int		InitiativeSession_Read();
	int		InitiativeSession_Write();
	int		InitiativeSession_Error();
	void	InitiativeSession_Connected();

	void	Close();

private:
	CStringA			m_strRemoteServerIP;
	CStringA			m_strHost;
	CStringA			m_strFileName;
	unsigned short		m_uRemoteServerPort;

	void	TransferRequest();

private:
	bool	RecordFile(const char *pszData, int iDataLen);
	bool	WriteFile(const char *pszContent, int iContentLen);

public:
	ConnectSession	m_sessionPassive;		//被动连接session（代理服务接收终端的请求）
	ConnectSession	m_sessionInitiative;	//主动连接session（代理服务向远端发起的请求）

	int			m_iState;					//会话的状态
};


#endif