#ifndef __DOCMDER_H__
#define __DOCMDER_H__

#include <curl/curl.h>
#include <map>
#include <string>

using std::string;
using std::map;

class DoCmd{
	
public:
	DoCmd(int size_buf=4096);
	~DoCmd();
	
	int Perform(const char* request_url, map<string,string>& header,  const char* post_data, const char* method,
				size_t (*call_back)(void *ptr, size_t size, size_t nmemb, void *obj), void* carg,
				size_t (*header_callback)(char *buffer, size_t size, size_t nitems, void *userdata), void* header_arg,
				char* usrpwd=NULL, size_t (*readback)(void *ptr, size_t size, size_t nmemb, void *obj)=NULL, void* rarg=NULL );
	CURL* GetCurl(){return curl;}
private:
	CURL *curl;
	unsigned char* resPtr;
	char* string_data;
	
};


#endif