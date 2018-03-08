#include "stdafx.h"
#include "HttpProxySession.h"
#include <Log/MyLogEx.h>


HttpProxySession::HttpProxySession() : 
	m_iState(SESSION_STATE_INIT),
	m_uRemoteServerPort(0)
{
	m_sessionPassive.SetListener(true);
	m_sessionInitiative.SetListener(false);

	m_strRemoteServerIP.Empty();
	m_strHost.Empty();
	m_strFileName.Empty();
}

HttpProxySession::~HttpProxySession()
{

}

int HttpProxySession::PassiveSession_Read()
{
	int iReadLen;
	iReadLen = m_sessionPassive.HandleRead();

	//�������0�ֽڣ�֤���Զ˶Ͽ�������
	if(iReadLen > 0)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "%s", m_sessionPassive.m_RecvBuf.GetData());
		if(m_sessionPassive.Parse())
		{
			LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "����HTTP���ɹ�");

#if 0
			//��ȡURI��ȥ��ǰ���/
			const HTTP_MESSAGE *pHttpMsg = m_sessionPassive.GetHttpMessage();
			std::string strUriName(pHttpMsg->m_strUri.m_pData+1, pHttpMsg->m_strUri.m_iLen-1);
			std::string strDecodeName = MyString::DecodeUrl(strUriName.c_str());
			MyString::UTF8toANSI(strDecodeName);
			m_strFileName = strDecodeName.c_str();
#endif
			if(m_sessionInitiative.DoConnect(m_strRemoteServerIP, m_uRemoteServerPort))
			{
				LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "�����������ӣ�IP=%s, �˿�=%u", m_strRemoteServerIP, m_uRemoteServerPort);
				m_iState = SESSION_STATE_INITIATIVE_CONNECTING;
			}
		}
	}

	return iReadLen;
}

int HttpProxySession::PassiveSession_Write()
{
	int iWriteLen;
	iWriteLen = m_sessionPassive.HandleWrite();

	return iWriteLen;
}

int HttpProxySession::PassiveSession_Error()
{
	return m_sessionPassive.HandleError();
}

int HttpProxySession::InitiativeSession_Read()
{
	int iReadLen;
	iReadLen = m_sessionInitiative.HandleRead();

	//�������0�ֽڣ�֤���Զ˶Ͽ�������
	if(iReadLen > 0)
	{
		//LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "iReadLen=%d", iReadLen);
		//LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "%s", m_sessionInitiative.m_RecvBuf.GetData());
		RecordFile(m_sessionInitiative.GetRecvBufData(), m_sessionInitiative.GetRecvBufDataLen());
		m_sessionPassive.Write(m_sessionInitiative.GetRecvBufData(), m_sessionInitiative.GetRecvBufDataLen());
		m_sessionInitiative.ResetRecvBuf();
	}

	return iReadLen;
}

bool HttpProxySession::RecordFile(const char* pszData, int iDataLen)
{
	int iHeadLen = m_sessionInitiative.ParseHead();
	if(iHeadLen > iDataLen)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "����HTTP��ͷ������");
		return false;
	}

	const char *pszContent	= pszData + iHeadLen;
	int	iWriteLen			= iDataLen - iHeadLen;

	return WriteFile(pszContent, iWriteLen);
}

bool HttpProxySession::WriteFile(const char *pszContent, int iContentLen)
{
	if(m_strFileName.IsEmpty())
	{
		return false;
	}

#if 1
	std::ofstream file;
	file.open(m_strFileName, std::ios::binary|std::ios::app);
	if (!file.is_open())
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "fstream���ļ�ʧ��");
		return false;
	}
	
	file.write(pszContent, iContentLen);
	file.flush();
	file.close();

	return true;

#else

	FILE *fp;
	fopen_s(&fp, m_strFileName, "ab");
	if(fp == NULL)
	{
		LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "fopen()ʧ��");
		return false;
	}

	if(iContentLen != fwrite(pszContent, 1, iContentLen, fp))
	{
		//LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "fwrite()����");
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
#endif
}

int HttpProxySession::InitiativeSession_Write()
{
	int iWriteLen;
	iWriteLen = m_sessionInitiative.HandleWrite();

	return iWriteLen;
}

int HttpProxySession::InitiativeSession_Error()
{
	return m_sessionInitiative.HandleError();
}

void HttpProxySession::InitiativeSession_Connected()
{
	//�����ʾ�Ѿ����ӳɹ��ˣ���������б�־�����������ӺͶ���־
	m_sessionInitiative.ClearConnectStatusConnecting();
	m_sessionInitiative.SetConnectStatusConnected();
	m_sessionInitiative.SetConnectStatusWantRead();

	m_iState = SESSION_STATE_INITIATIVE_CONNECTED;

	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "�������ӳɹ�����ʼת�������");
	TransferRequest();
}

void HttpProxySession::TransferRequest()
{
	int i = 0;
	const HTTP_MESSAGE *pMsg = m_sessionPassive.GetHttpMessage();

	m_sessionInitiative.Write(&pMsg->m_strMethod);
	m_sessionInitiative.Write(" ", 1);
	m_sessionInitiative.Write(&pMsg->m_strUri);
	m_sessionInitiative.Write(" ", 1);
	if(pMsg->m_strQueryString.m_iLen != 0)
	{
		m_sessionInitiative.Write("?", 1);
		m_sessionInitiative.Write(&pMsg->m_strQueryString);
	}
	m_sessionInitiative.Write(" ", 1);
	m_sessionInitiative.Write(&pMsg->m_strProto);
	m_sessionInitiative.Write("\r\n", 2);

	for(i=0; i<MAX_HTTP_HEADERS; i++)
	{
		const HTTP_STR* pStrName = &pMsg->m_strHeaderNames[i];
		const HTTP_STR* pStrValue = &pMsg->m_strHeaderValues[i];
		if(pStrName->m_iLen == 0)
		{
			break;
		}

		if(pStrName->vcasecmp("Host") == 0)
		{
			m_sessionInitiative.Write(pStrName);
			m_sessionInitiative.Write(": ", 2);
			m_sessionInitiative.Write(m_strHost, m_strHost.GetLength());
			m_sessionInitiative.Write("\r\n", 2);
		}
		else
		{
			m_sessionInitiative.Write(pStrName);
			m_sessionInitiative.Write(": ", 2);
			m_sessionInitiative.Write(pStrValue);
			m_sessionInitiative.Write("\r\n", 2);
		}
	}

	m_sessionInitiative.Write("\r\n", 2);

	//requestû��body��������������²���ִ�е�
	if(&pMsg->m_strBody.m_iLen != 0)
	{
		m_sessionInitiative.Write(&pMsg->m_strBody);
	}
}

void HttpProxySession::Close()
{
	LOG_PRINTEX(0, MyLogEx::LOG_LEVEL_DEBUG_1, "�ر�session");
	m_sessionInitiative.Close();
	m_sessionPassive.Close();
}