#ifndef _HTTP_DEFINE_H_
#define _HTTP_DEFINE_H_

#include <Misc/MyString.h>


#define HTTP_DEFAULT_BUF_SIZE					(1024*4)
#define HTTP_BUF_GROWTH_RATE					2
#define HTTP_DEFAULT_PORT						80
#define HTTP_LEN_INFINITE						(~0)

#define MAX_HTTP_HEADERS						40


//Describes chunk of memory
struct HTTP_STR
{
public:
	HTTP_STR()
	{
		m_pData = NULL;
		m_iLen	= 0;
	}

	HTTP_STR(const char* s)
	{
		m_pData	= s;
		m_iLen	= strlen(s);
	}

	HTTP_STR(const char* s, size_t len)
	{
		m_pData = s;
		m_iLen	= len;
	}

	void Clear()
	{
		m_pData = NULL;
		m_iLen	= 0;
	}

public:
	int vcasecmp(const char *str) const
	{
		size_t iLen = strlen(str);
		int r = MyString::strncasecmp(m_pData, str, (m_iLen < iLen) ? m_iLen : iLen);
		if(r == 0)
		{
			return m_iLen - iLen;
		}

		return r;
	}

public:
	const char*		m_pData;			//Memory chunk pointer
	size_t			m_iLen;				//Memory chunk length
};

//HTTP message
struct HTTP_MESSAGE
{
public:
	  HTTP_STR	m_strMessage;			//Whole message: request line + headers + body

	  //HTTP Request line (or HTTP response line)
	  HTTP_STR	m_strMethod;			//"GET"
	  HTTP_STR	m_strUri;				//"/my_file.html"
	  HTTP_STR	m_strProto;				//"HTTP/1.1" -- for both request and response

	  //For responses, code and response status message are set
	  int		m_iRespCode;
	  HTTP_STR	m_strRespStatusMsg;

	  /*
	   * Query-string part of the URI. For example, for HTTP request
	   *    GET /foo/bar?param1=val1&param2=val2
	   *    |    uri    |     query_string     |
	   *
	   * Note that question mark character doesn't belong neither to the uri,
	   * nor to the query_string
	   */
	  HTTP_STR m_strQueryString;

	  //Headers
	  HTTP_STR m_strHeaderNames[MAX_HTTP_HEADERS];
	  HTTP_STR m_strHeaderValues[MAX_HTTP_HEADERS];

	  //Message body
	  HTTP_STR m_strBody;				//Zero-length for requests with no body

public:
	HTTP_MESSAGE()
	{
		Clear();
	}

	void Clear()
	{
		m_strMessage.Clear();
		m_strMethod.Clear();
		m_strUri.Clear();
		m_strProto.Clear();
		m_iRespCode = 0;
		m_strRespStatusMsg.Clear();
		m_strQueryString.Clear();

		int i = 0;
		for(i=0; i<MAX_HTTP_HEADERS; i++)
		{
			m_strHeaderNames[i].Clear();
			m_strHeaderValues[i].Clear();
		}

		m_strBody.Clear();
	}
};

#endif