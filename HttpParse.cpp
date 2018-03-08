#include "stdafx.h"
#include "HttpParse.h"
#include <Misc/MyString.h>


/**
 *参数：pszBuf		网络上接收到的数据缓冲区
 *		iLen		数据缓冲区长度
 *		bIsReq		是否请求包
 *		pHttpMsg	解析后的http信息指针
 *
 *返回值：返回>0	表示整个http包已经缓冲完成，返回值是http包的长度
 *		  返回<=0	表示http包未缓冲完成
**/
int HttpParse::Parse(const char* pszBuf, int iLen, bool bIsReq, HTTP_MESSAGE* pHttpMsg)
{
	const char *pszEnd, *pszIndex, *pszIndex2;

	int iHeadLen = GetHeadLen(pszBuf, iLen);
	if(iHeadLen <= 0)
	{
		return iHeadLen;
	}

	pHttpMsg->m_strMessage.m_pData	= pszBuf;
	pHttpMsg->m_strBody.m_pData		= pszBuf + iHeadLen;
	pHttpMsg->m_strMessage.m_iLen	= HTTP_LEN_INFINITE;
	pHttpMsg->m_strBody.m_iLen		= HTTP_LEN_INFINITE;
	pszEnd = pszBuf + iHeadLen;

	//去掉空格
	pszIndex = pszBuf;
	while(pszIndex < pszEnd && isspace(*(unsigned char *) pszIndex))
	{
		pszIndex++;
	}

	if(bIsReq)
	{
		//解析method, URI, proto
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strMethod);
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strUri);
		pszIndex = Skip(pszIndex, pszEnd, "\r\n", &pHttpMsg->m_strProto);
		if(pHttpMsg->m_strUri.m_pData <= pHttpMsg->m_strMethod.m_pData ||
			pHttpMsg->m_strProto.m_pData <= pHttpMsg->m_strUri.m_pData)
		{
			return 0;
		}

		//如果URI中有 '?' 字符，需要解析 query_string
		pszIndex2 = (char *)memchr(pHttpMsg->m_strUri.m_pData, '?', pHttpMsg->m_strUri.m_iLen);
		if(pszIndex2 != NULL)
		{
			pHttpMsg->m_strQueryString.m_pData	= pszIndex2 + 1;
			pHttpMsg->m_strQueryString.m_iLen	= &pHttpMsg->m_strUri.m_pData[pHttpMsg->m_strUri.m_iLen] - (pszIndex2 + 1);
			pHttpMsg->m_strUri.m_iLen			= pszIndex2 - pHttpMsg->m_strUri.m_pData;
		}
	}
	else
	{
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strProto);
		if(pszEnd - pszIndex < 4 || pszIndex[3] != ' ')
		{
			return 0;
		}

		pHttpMsg->m_iRespCode = atoi(pszIndex);
		if(pHttpMsg->m_iRespCode < 100 || pHttpMsg->m_iRespCode > 600)
		{
			return 0;
		}

		pszIndex += 4;
		pszIndex = Skip(pszIndex, pszEnd, "\r\n", &pHttpMsg->m_strRespStatusMsg);
	}

	pszIndex = ParseHeaders(pszIndex, pszEnd, iHeadLen, pHttpMsg);

	if(pHttpMsg->m_strBody.m_iLen == HTTP_LEN_INFINITE && bIsReq && 
		pHttpMsg->m_strMethod.vcasecmp("PUT") != 0 && 
		pHttpMsg->m_strMethod.vcasecmp("POST") != 0)
	{
		pHttpMsg->m_strBody.m_iLen = 0;
		pHttpMsg->m_strMessage.m_iLen = iHeadLen;
	}

	HTTP_STR *strContentLen = NULL;
	int		 iContentLen = 0;
	if((strContentLen = GetHttpHeader(pHttpMsg, "Content-Length")) != NULL)
	{
		iContentLen = atoi(strContentLen->m_pData);
		if(iLen < iHeadLen + iContentLen)
		{
			//body not fully buffered
			return 0;
		}
	}

	return (iHeadLen + iContentLen);
}

int HttpParse::ParseHead(const char* pszBuf, int iLen, bool bIsReq, HTTP_MESSAGE* pHttpMsg)
{
	const char *pszEnd, *pszIndex, *pszIndex2;

	int iHeadLen = GetHeadLen(pszBuf, iLen);
	if(iHeadLen <= 0)
	{
		return iHeadLen;
	}

	pHttpMsg->m_strMessage.m_pData	= pszBuf;
	pHttpMsg->m_strBody.m_pData		= pszBuf + iHeadLen;
	pHttpMsg->m_strMessage.m_iLen	= HTTP_LEN_INFINITE;
	pHttpMsg->m_strBody.m_iLen		= HTTP_LEN_INFINITE;
	pszEnd = pszBuf + iHeadLen;

	//去掉空格
	pszIndex = pszBuf;
	while(pszIndex < pszEnd && isspace(*(unsigned char *) pszIndex))
	{
		pszIndex++;
	}

	if(bIsReq)
	{
		//解析method, URI, proto
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strMethod);
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strUri);
		pszIndex = Skip(pszIndex, pszEnd, "\r\n", &pHttpMsg->m_strProto);
		if(pHttpMsg->m_strUri.m_pData <= pHttpMsg->m_strMethod.m_pData ||
			pHttpMsg->m_strProto.m_pData <= pHttpMsg->m_strUri.m_pData)
		{
			return 0;
		}

		//如果URI中有 '?' 字符，需要解析 query_string
		pszIndex2 = (char *)memchr(pHttpMsg->m_strUri.m_pData, '?', pHttpMsg->m_strUri.m_iLen);
		if(pszIndex2 != NULL)
		{
			pHttpMsg->m_strQueryString.m_pData	= pszIndex2 + 1;
			pHttpMsg->m_strQueryString.m_iLen	= &pHttpMsg->m_strUri.m_pData[pHttpMsg->m_strUri.m_iLen] - (pszIndex2 + 1);
			pHttpMsg->m_strUri.m_iLen			= pszIndex2 - pHttpMsg->m_strUri.m_pData;
		}
	}
	else
	{
		pszIndex = Skip(pszIndex, pszEnd, " ", &pHttpMsg->m_strProto);
		if(pszEnd - pszIndex < 4 || pszIndex[3] != ' ')
		{
			return 0;
		}

		pHttpMsg->m_iRespCode = atoi(pszIndex);
		if(pHttpMsg->m_iRespCode < 100 || pHttpMsg->m_iRespCode > 600)
		{
			return 0;
		}

		pszIndex += 4;
		pszIndex = Skip(pszIndex, pszEnd, "\r\n", &pHttpMsg->m_strRespStatusMsg);
	}

	pszIndex = ParseHeaders(pszIndex, pszEnd, iHeadLen, pHttpMsg);

	if(pHttpMsg->m_strBody.m_iLen == HTTP_LEN_INFINITE && bIsReq && 
		pHttpMsg->m_strMethod.vcasecmp("PUT") != 0 && 
		pHttpMsg->m_strMethod.vcasecmp("POST") != 0)
	{
		pHttpMsg->m_strBody.m_iLen = 0;
		pHttpMsg->m_strMessage.m_iLen = iHeadLen;
	}

	return iHeadLen;
}

/*
 * Check whether full request is buffered. Return:
 *   -1  if request is malformed
 *    0  if request is not yet fully buffered
 *   >0  actual request length, including last \r\n\r\n
 */
int HttpParse::GetHeadLen(const char* pszBuf, int iLen)
{
	const unsigned char *buf = (unsigned char *) pszBuf;
	int i;

	for (i = 0; i < iLen; i++) {
		if (!isprint(buf[i]) && buf[i] != '\r' && buf[i] != '\n' && buf[i] < 128) {
			return -1;
		} else if (buf[i] == '\n' && i + 1 < iLen && buf[i + 1] == '\n') {
			return i + 2;
		} else if (buf[i] == '\n' && i + 2 < iLen && buf[i + 1] == '\r' &&
			buf[i + 2] == '\n') {
				return i + 3;
		}
	}

	return 0;
}

const char*	HttpParse::Skip(const char *s, const char *end, const char *delims, HTTP_STR* v)
{
	v->m_pData = s;
	while (s < end && strchr(delims, *(unsigned char *) s) == NULL) s++;
	v->m_iLen = s - v->m_pData;
	while (s < end && strchr(delims, *(unsigned char *) s) != NULL) s++;
	return s;
}

const char*	HttpParse::ParseHeaders(const char *s, const char *end, int len, HTTP_MESSAGE *req)
{
	int i = 0;
	while(i < (int)ARRAY_SIZE(req->m_strHeaderNames) - 1)
	{
		HTTP_STR *k = &req->m_strHeaderNames[i];
		HTTP_STR *v = &req->m_strHeaderValues[i];

		s = Skip(s, end, ": ", k);
		s = Skip(s, end, "\r\n", v);

		while(v->m_iLen > 0 && v->m_pData[v->m_iLen - 1] == ' ')
		{
			//Trim trailing spaces in header value
			v->m_iLen--;
		}

		/*
		 * If header value is empty - skip it and go to next (if any).
		 * NOTE: Do not add it to headers_values because such addition changes API
		 * behaviour
		 */
		if(k->m_iLen != 0 && v->m_iLen == 0)
		{
			continue;
		}

		if(k->m_iLen == 0 || v->m_iLen == 0)
		{
			k->m_pData = v->m_pData = NULL;
			k->m_iLen = v->m_iLen = 0;
			break;
		}

		if(!MyString::strncasecmp(k->m_pData, "Content-Length", 14))
		{
			req->m_strBody.m_iLen = (size_t)atoi(v->m_pData);
			req->m_strMessage.m_iLen = len + req->m_strBody.m_iLen;
		}

		i++;
	}

	return s;
}

HTTP_STR *HttpParse::GetHttpHeader(HTTP_MESSAGE *hm, const char *name)
{
	size_t i, len = strlen(name);

	for (i = 0; hm->m_strHeaderNames[i].m_iLen > 0; i++) 
	{
		HTTP_STR *h = &hm->m_strHeaderNames[i];
		HTTP_STR *v = &hm->m_strHeaderValues[i];

		if (h->m_pData != NULL && 
			h->m_iLen == len && 
			!MyString::strncasecmp(h->m_pData, name, len))
		{
			return v;
		}
	}

	return NULL;
}
