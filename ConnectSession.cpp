#include "stdafx.h"
#include "ConnectSession.h"
#include <Log/MyLogEx.h>
#include <Misc/MySocketOption.h>


ConnectSession::ConnectSession() : 
	m_sock(INVALID_SOCKET),
	m_uFlags(0),
	m_bListener(false),
	m_iPacketLen(0)
{

}

ConnectSession::~ConnectSession()
{

}

int	ConnectSession::HandleRead()
{
	if(m_iPacketLen > 0)
	{
		m_RecvBuf.MoveBackBuffer(m_iPacketLen);
		m_httpMsg.Clear();
		m_iPacketLen = 0;
	}

	int iRead = ::recv(m_sock, m_szBlock, READ_BLOCK_SIZE, 0);
	if(iRead == 0)
	{
		return 0;
	}
	else if(iRead == SOCKET_ERROR)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "recv()出错，错误码=%d", WSAGetLastError());
		return SOCKET_ERROR;
	}
	else
	{
		m_RecvBuf.SetSize(m_RecvBuf.GetOffset() + iRead);
		m_RecvBuf.ExpandData(m_szBlock, iRead);
	}

	return iRead;
}

int	ConnectSession::HandleWrite()
{
	//LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "Write=%s", m_SendBuf.GetData());
	int iWriteLen = ::send(m_sock, m_SendBuf.GetData(), m_SendBuf.GetDataLen(), 0);
	if(iWriteLen == SOCKET_ERROR)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "send()出错，错误码=%d", WSAGetLastError());
		return SOCKET_ERROR;
	}
	else
	{
		m_SendBuf.MoveBuf(iWriteLen);
	}

	if(m_SendBuf.GetDataLen() <= 0)
	{
		if(IsConnectStatusWriteAndClose())
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "写完关闭");
			closesocket(m_sock);
			//这里应该关闭连接
		}

		//已经写完，清楚些标志
		ClearConnectStatusWantWrite();
		ClearConnectStatusWriteAndClose();
	}

	return iWriteLen;
}

int	ConnectSession::HandleError()
{
	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "HandleError()，连接socket出错");
	return 0;
}

bool ConnectSession::Parse()
{
	int iPacketLen = m_httpParse.Parse(m_RecvBuf.GetData(), m_RecvBuf.GetDataLen(), m_bListener, &m_httpMsg);
	if(iPacketLen > 0)
	{
		m_iPacketLen = iPacketLen;
		return true;
	}

	return false;
}

int ConnectSession::ParseHead()
{
	int iHeadLen = m_httpParse.ParseHead(m_RecvBuf.GetData(), m_RecvBuf.GetDataLen(), m_bListener, &m_httpMsg);
	if(iHeadLen > 0)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "is Head");
		return iHeadLen;
	}

	return 0;
}

bool ConnectSession::DoConnect(const char* pszRemoteServerIP, unsigned short uRemoteServerPort)
{
	if(m_sock != INVALID_SOCKET)
	{
		//已经连接
		return true;
	}

	if(uRemoteServerPort == 0 || strlen(pszRemoteServerIP) == 0)
	{
		return false;
	}

	struct sockaddr_in	addrRemote;
	addrRemote.sin_family = AF_INET;
	addrRemote.sin_port = htons(uRemoteServerPort);
	addrRemote.sin_addr.s_addr = inet_addr(pszRemoteServerIP);

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(m_sock == INVALID_SOCKET)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "socket()出错，错误码=%d", WSAGetLastError());
		return false;
	}

	MySocketOption::SetNonBlockingMode(m_sock);

	int iRet = connect(m_sock, (struct sockaddr *)&addrRemote, sizeof(struct sockaddr));
	if(iRet < 0)
	{
		if(WSAGetLastError() != WSAEINTR && 
			WSAGetLastError() != WSAEWOULDBLOCK && 
			errno != EINPROGRESS && 
			errno != EWOULDBLOCK)
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "connect()返回错误，错误码=%d", WSAGetLastError());
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			return false;
		}
	}
	else if(iRet == 0)
	{
		//这种情况很少存在，暂时不做处理 :=(
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "连接马上完成");
	}

	SetConnectStatusConnecting();

	return true;
}

bool ConnectSession::Write(const char* pszData, int iDataLen)
{
	SetConnectStatusWantWrite();
	return m_SendBuf.Write(pszData, iDataLen);
}

bool ConnectSession::Write(const HTTP_STR* pStr)
{
	return Write(pStr->m_pData, pStr->m_iLen);
}

void ConnectSession::Close()
{
	if(m_sock != INVALID_SOCKET)
	{
		::closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	
	m_RecvBuf.Release();
	m_SendBuf.Release();
}
