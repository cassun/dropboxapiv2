#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdlib.h>

#include <curl/curl.h>

bool get_tag_string_value(const char* p , const char* tag,  const char** s,  const char** e);
bool get_tag_boolen_value(const char* p , const char* tag, bool* value);
int create_folder(const char* path);
int malloc_and_copy_string(char** dst, int limit_buf_size, const char* src);


//for callback 

typedef struct Openfile
{
	CURL* curl;
	FILE** fout;
	bool is_open;
	const char* path;
	bool* _running;
	bool finished;
	Openfile(CURL* _curl, FILE** _fout, char* _path, bool* running):curl(_curl), fout(_fout), is_open(false), path(_path), _running(running), finished(true){}
}Openfile;

typedef struct DCopyToBuf
{
	CURL* curl;
	char* e;	//end position
	char* c;	//current position
	DCopyToBuf(CURL* _curl, char* _s, char* _e):curl(_curl), e(_e),c(_s){}
}DCopyToBuf;

size_t CopyFile(void *ptr, size_t size, size_t nmemb, void *obj);
size_t CopyToBuf(void *ptr, size_t size, size_t nmemb, void *obj);
size_t download_header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
size_t UploadReadBack(void *ptr, size_t size, size_t nmemb, void *obj);
size_t NULLEverything(void *ptr, size_t size, size_t nmemb, void *obj);


#endif