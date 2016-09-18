#include "docmder.h"
#include <string.h>


DoCmd::DoCmd(int size_buf)
{
	//resPtr = new unsigned char [size_buf];
	string_data = new char[256];
	curl = curl_easy_init();
	
}

DoCmd::~DoCmd()
{
	//delete [] resPtr;
	delete [] string_data;
	curl_easy_cleanup(curl);
}


int DoCmd::Perform(const char* request_url, map<string,string>& header,  const char* post_data, const char* method,
					size_t (*call_back)(void *ptr, size_t size, size_t nmemb, void *obj), void* carg,
					size_t (*header_callback)(char *buffer, size_t size, size_t nitems, void *userdata), void* header_arg,
					char* usrpwd,  size_t (*readback)(void *ptr, size_t size, size_t nmemb, void *obj), void* rarg)
{
	if(!request_url)
		return -1;
		
	CURLcode res;
	struct curl_slist *slist = NULL;
	map<string,string>::iterator it;
	long http_code;
	
	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_URL, request_url);
	
	
	//download setup
	if(call_back){
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, call_back);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, carg); 
	}
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
	
	if(header_callback){
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, header_arg); 
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	}
	
	//upload set up
	if(readback){
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, readback);
		curl_easy_setopt(curl, CURLOPT_READDATA, rarg); 
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); 
	}
	
	
	if(usrpwd)
		curl_easy_setopt(curl,CURLOPT_USERPWD,usrpwd);
	
	if(post_data)
	{
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	}
	
	if(method)
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method); 
	//set header data
	for(it=header.begin();it!=header.end();it++)
	{

		if( it->first.compare("Authorization") == 0)
			sprintf(string_data, "Authorization: Bearer %s",it->second.c_str()); 
		else
			sprintf(string_data, "%s: %s",it->first.c_str(), it->second.c_str()); 

		slist = curl_slist_append(slist, string_data);

	}
	
	if(header.size() > 0)
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
	
	res = curl_easy_perform(curl);
	
	//long http_code;
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	//printf("http code:%ld\n",http_code);
	
	if(slist)
		curl_slist_free_all(slist);
	
	
	if(res != CURLE_OK){
		printf("\n");
		
		//curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
		//fprintf(stderr,"error: http code:%ld\n",http_code);
		//fprintf(stderr,"error code:%d\n",res);
		return -res;
	}
	
	if(http_code == 401)
	{
		fprintf(stderr,"401 Unauthorized\n");
		return -http_code;
	}
	
	return 0;
}


