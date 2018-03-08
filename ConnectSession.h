#ifndef _CONNECT_SESSION_H_
#define _CONNECT_SESSION_H_

#include <stdlib.h>
#include "HttpDefine.h"
#include "HttpParse.h"


struct HTTP_RECV_BUF
{
public:
	HTTP_RECV_BUF(int iSize = HTTP_DEFAULT_BUF_SIZE)
	{
		m_pszData		= (char*)malloc(HTTP_DEFAULT_BUF_SIZE);
		m_iOffset		= 0;
		m_iSize			= iSize;
	}

	~HTTP_RECV_BUF()
	{
		if(m_pszData)
		{
			free(m_pszData);
			m_pszData = NULL;
		}
	}

	inline void Reset()
	{
		m_iOffset = 0;
		memset(m_pszData, 0, m_iSize);
	}

	inline int GetLeftLen()
	{
		return (m_iSize - m_iOffset);
	}

	inline int GetOffset()
	{
		return m_iOffset;
	}

	inline char* GetDataTail()
	{
		return m_pszData + m_iOffset;
	}

	inline char* GetData()
	{
		return m_pszData;
	}

	inline int GetDataLen() 
	{
		return m_iOffset;
	}

	inline void ExpandData(const char* pszData, int iLen)
	{
		memcpy(m_pszData + m_iOffset, pszData, iLen);
		m_iOffset += iLen;
		*(m_pszData + m_iOffset) = 0;
	}

	inline bool MoveBackBuffer(int iBackLen)
	{
		if(m_iOffset >= iBackLen)
		{
			memmove(m_pszData, m_pszData+iBackLen, m_iOffset-iBackLen);
			m_iOffset -= iBackLen;
			return true;
		}
		
		return false;
	}

	inline void SetSize(int iSize)
	{
		if(iSize >= m_iSize)
		{
			char* pszTmp = (char*)realloc(m_pszData, iSize + 1);
			if(pszTmp == NULL)
			{
				return ;
			}

			m_pszData = pszTmp;
			m_iSize		= iSize + 1;
		}
	}

	inline void Release()
	{
		free(m_pszData);
		m_pszData	= NULL;
		m_iSize		= 0;
		m_iOffset	= 0;
	}

private:
	char*		m_pszData;					// 数据缓冲区指针
	int			m_iOffset;					// 缓冲区指针偏移
	int			m_iSize;					// 缓冲区大小
};


struct HTTP_SEND_BUF
{
public:
	HTTP_SEND_BUF()
	{
		m_pszData	= (char*)malloc(HTTP_DEFAULT_BUF_SIZE);
		m_iOffset	= 0;
		m_iSize		= HTTP_DEFAULT_BUF_SIZE;
	}

	~HTTP_SEND_BUF()
	{
		if(m_pszData != NULL)
		{
			free(m_pszData);
			m_pszData	= NULL;
			m_iOffset	= 0;
			m_iSize		= 0;
		}
	}

	inline void Release()
	{
		free(m_pszData);
		m_pszData	= NULL;
		m_iOffset	= 0;
		m_iSize		= 0;
	}

	inline bool Write(const char* pszData, int iLen)
	{
		if(m_iOffset + iLen > m_iSize)
		{
			if(!ExpandBuf(m_iOffset + iLen))
			{
				return false;
			}
		}

		memcpy_s(m_pszData + m_iOffset, m_iSize - m_iOffset, pszData, iLen);
		m_iOffset += iLen;

		return true;
	}

	inline void MoveBuf(int iMoveOffset)
	{
		if(m_iOffset < iMoveOffset)
		{
			return ;
		}

		memmove_s(m_pszData, m_iSize, m_pszData + iMoveOffset, m_iOffset - iMoveOffset);
		m_iOffset -= iMoveOffset;
	}

	inline const char* GetData() const
	{
		return m_pszData;
	}

	inline int GetDataLen() const
	{
		return m_iOffset;
	}

private:
	inline bool ExpandBuf(int iSize)
	{
		char* pszTmp = (char*)realloc(m_pszData, iSize * HTTP_BUF_GROWTH_RATE);
		if(pszTmp == NULL)
		{
			return false;
		}

		m_pszData	= pszTmp;
		m_iSize		= iSize * HTTP_BUF_GROWTH_RATE;

		return true;
	}

private:
	char*		m_pszData;					// 数据缓冲区指针
	int			m_iOffset;					// 有效数据偏移
	int			m_iSize;					// 缓冲区大小
};

class ConnectSession
{
public:
	ConnectSession();
	~ConnectSession();


#define CONNECT_STATUS_UNCONNECT			(1 << 0)
#define CONNECT_STATUS_CONNECTING			(1 << 1)
#define CONNECT_STATUS_CONNECTED			(1 << 2)
#define CONNECT_STATUS_WANT_READ			(1 << 3)
#define CONNECT_STATUS_WANT_WRITE			(1 << 4)
#define CONNECT_STATUS_WRITE_AND_CLOSE		(1 << 5)


#define READ_BLOCK_SIZE						(1024*2)


	inline void SetConnectStatusUnConnect()
	{
		m_uFlags |= CONNECT_STATUS_UNCONNECT;
	}

	inline void ClearConnectStatusUnConnect()
	{
		m_uFlags &= ~CONNECT_STATUS_UNCONNECT;
	}

	inline bool IsConnectStatusUnConnect()
	{
		return ((m_uFlags & CONNECT_STATUS_UNCONNECT) != 0);
	}

	inline void SetConnectStatusConnecting()
	{
		m_uFlags |= CONNECT_STATUS_CONNECTING;
	}

	inline void ClearConnectStatusConnecting()
	{
		m_uFlags &= ~CONNECT_STATUS_CONNECTING;
	}

	inline bool IsConnectStatusConnecting()
	{
		return ((m_uFlags & CONNECT_STATUS_CONNECTING) != 0);
	}

	inline void SetConnectStatusConnected()
	{
		m_uFlags |= CONNECT_STATUS_CONNECTED;
	}

	inline void ClearConnectStatusConnected()
	{
		m_uFlags &= ~CONNECT_STATUS_CONNECTED;
	}

	inline bool IsConnectStatusConnected()
	{
		return ((m_uFlags & CONNECT_STATUS_CONNECTED) != 0);
	}

	inline void SetConnectStatusWantRead()
	{
		m_uFlags |= CONNECT_STATUS_WANT_READ;
	}

	inline void ClearConnectStatusWantRead()
	{
		m_uFlags &= ~CONNECT_STATUS_WANT_READ;
	}

	inline bool IsConnectStatusWantRead()
	{
		return ((m_uFlags & CONNECT_STATUS_WANT_READ) != 0);
	}

	inline void SetConnectStatusWantWrite()
	{
		m_uFlags |= CONNECT_STATUS_WANT_WRITE;
	}

	inline void ClearConnectStatusWantWrite()
	{
		m_uFlags &= ~CONNECT_STATUS_WANT_WRITE;
	}

	inline bool IsConnectStatusWantWrite()
	{
		return ((m_uFlags & CONNECT_STATUS_WANT_WRITE) != 0);
	}

	inline void SetConnectStatusWriteAndClose()
	{
		m_uFlags |= CONNECT_STATUS_WRITE_AND_CLOSE;
	}

	inline void ClearConnectStatusWriteAndClose()
	{
		m_uFlags &= ~CONNECT_STATUS_WRITE_AND_CLOSE;
	}

	inline bool IsConnectStatusWriteAndClose()
	{
		return ((m_uFlags & CONNECT_STATUS_WRITE_AND_CLOSE) != 0);
	}

	inline bool IsListener()
	{
		return m_bListener;
	}

	inline void SetListener(bool bListener)
	{
		m_bListener = bListener;
	}

	inline const HTTP_MESSAGE *GetHttpMessage() const
	{
		return &m_httpMsg;
	}

	inline const char *GetRecvBufData()
	{
		return m_RecvBuf.GetData();
	}

	inline int GetRecvBufDataLen()
	{
		return m_RecvBuf.GetDataLen();
	}

	inline void ResetRecvBuf()
	{
		m_RecvBuf.Reset();
	}

	//读处理
	int		HandleRead();

	//写处理
	int		HandleWrite();

	//错误处理
	int		HandleError();

	//协议解析
	bool	Parse();

	//HTTP包头解析
	int		ParseHead();

	//发起主动连接
	bool	DoConnect(const char* pszRemoteServerIP, unsigned short uRemoteServerPort);

	//写数据
	bool	Write(const char* pszData, int iDataLen);
	bool	Write(const HTTP_STR* pStr);

	//关闭session
	void	Close();

public:
	SOCKET			m_sock;
	unsigned long	m_uFlags;
	HTTP_RECV_BUF	m_RecvBuf;
	HTTP_SEND_BUF	m_SendBuf;
	char			m_szBlock[READ_BLOCK_SIZE];

private:
	bool			m_bListener;

	int				m_iPacketLen;
	HTTP_MESSAGE	m_httpMsg;
	HttpParse		m_httpParse;
};

#endif