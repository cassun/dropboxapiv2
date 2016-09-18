#include "utility.h"
#include <stdio.h>
#include <string.h>

#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "statuscodelist.h"


using std::string;

bool get_tag_string_value(const char *p , const char* tag, const char** s, const char** e)
{
	const char *_s, *_e;
	
	_s = strstr(p,tag);

	if(!_s) 
		return false;
	
	_s += strlen(tag);
	
	_s = strchr(_s,'"');
	if(!_s)
		return false;
	_s++;
	_e = strchr(_s,'"');
	if(!_e)
		return false;
	_e--;
	
	*s = _s;
	*e = _e;
	return true;	
}

bool get_tag_boolen_value(const char* p , const char* tag, bool* value)
{
	const char *_s;
	
	_s = strstr(p,tag);

	if(!_s) 
		return false;
	
	_s += strlen(tag);
	
	if(strstr(_s,"true"))
		*value = true;
	else if(strstr(_s,"false"))
		*value = false;
	else
		return false;
	
	return true;
	
}


int malloc_and_copy_string(char** dst, int limit_buf_size, const char* src)
{
	
	if(!src || limit_buf_size<=0)
		return ERR_PARAMETERS_NULL;
	int max_len;
	
	max_len = strlen(src);
	if(max_len > limit_buf_size-1)
		max_len = limit_buf_size-1;
	*dst = new char [max_len+1];
	strncpy(*dst,src,max_len);
	(*dst)[max_len] = '\0';
	
	return SUCCESS;
}

int create_folder(const char* path)
{
	string cmd;
	int ret;
	if(!path)
		return -1;
	
	cmd = "mkdir -p \"";
	cmd+= path;
	cmd+="\"";
	ret = system(cmd.c_str());
	//ret = mkdir(cmd.c_str(),0777);
	
	return ret;
}

int CheckHttpCode(CURL* curl,void *ptr, size_t size, size_t nmemb)
{
	long http_code;
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	if(http_code != 200 && http_code != 100)
	{
		int len;
		char* p;
		char tmp;
		
		p = (char*)ptr;
		len = size*nmemb;
		
		if(len > 0)
		{
			tmp = p[len-1];
			p[len-1] = '\0';
			fprintf(stderr,"%s%c",p,tmp);
		}
		return -1;
	}
	
	return 0;
	
}

size_t CopyFile(void *ptr, size_t size, size_t nmemb, void *obj)
{
	int ret = 0;
	Openfile* of = (Openfile*)obj;

	
	if(CheckHttpCode(of->curl, ptr, size, nmemb))
		return 0;
	
	
	if(!of->is_open && of->path)
	{
		*(of->fout) = fopen(of->path,"wb");
		
		if(*(of->fout))
			of->is_open = true;
		else
			return 0;
	}
	
	
	if(of->is_open)
	{
		if(!*(of->_running))
			of->finished = false;
		else
			ret =  fwrite(ptr,size,nmemb,*(of->fout));
	}

	return ret;
}

size_t CopyToBuf(void *ptr, size_t size, size_t nmemb, void *obj)
{
	DCopyToBuf* ctb = (DCopyToBuf*)obj;
	
	int data_size = size*nmemb;
	
	if(CheckHttpCode(ctb->curl, ptr, size, nmemb))
		return 0;
	
	//printf("size:%d\n",data_size);
	
	if(ctb->c + data_size <= ctb->e)
		memcpy(ctb->c,ptr,data_size);
	else
	{
		data_size = ctb->e - ctb->c;
		memcpy(ctb->c,ptr,data_size);
	}
	ctb->c+=data_size;
	return data_size;
	
}

size_t download_header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	
	Openfile* of = (Openfile*)userdata;
	long http_code;
	
	curl_easy_getinfo (of->curl, CURLINFO_RESPONSE_CODE, &http_code);
	
	printf("header:%s\n",buffer);
	printf("http_code:%ld\n",http_code);
	
	
	
	//if(http_code != 200 && http_code != 100)
		//return 0;
	
	if(!of->is_open && of->path)
	{
		
		//printf("open file:%s\n",of->dst_path);
		*(of->fout) = fopen(of->path,"wb");
		
		if(*(of->fout))
			of->is_open = true;
		else
			return 0;
	}
	
	
	//printf("out hree\n");
	return nitems * size;
}


size_t UploadReadBack(void *ptr, size_t size, size_t nmemb, void *obj)
{
	Openfile* of = (Openfile*)obj;
	int ret = 0;
	
	if(CheckHttpCode(of->curl, ptr, size, nmemb))
		return 0;
	
	if(!of->is_open)
	{
		if(of->path)
		{
			*(of->fout) = fopen(of->path,"rb");
			of->is_open = true;
		}
		else
			return 0;
	}

	if(!*(of->_running))
		of->finished = false;
	else
		ret = fread(ptr, 1, size*nmemb, *(of->fout));
	
	return ret;
	
}

size_t NULLEverything(void *ptr, size_t size, size_t nmemb, void *obj)
{
	Openfile* of = (Openfile*)obj;
	if(CheckHttpCode(of->curl, ptr, size, nmemb))
		return 0;
	
	return size*nmemb;
}


