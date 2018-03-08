#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_

#include "HttpDefine.h"


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

class HttpParse
{
public:
	int Parse(const char* pszBuf, int iLen, bool bIsReq, HTTP_MESSAGE* pHttpMsg);
	int ParseHead(const char* pszBuf, int iLen, bool bIsReq, HTTP_MESSAGE* pHttpMsg);

private:
	int				GetHeadLen(const char* pszBuf, int iLen);
	const char		*Skip(const char *s, const char *end, const char *delims, HTTP_STR* v);
	const char		*ParseHeaders(const char *s, const char *end, int len, HTTP_MESSAGE *req);

	HTTP_STR		*GetHttpHeader(HTTP_MESSAGE *hm, const char *name);
};


#endif