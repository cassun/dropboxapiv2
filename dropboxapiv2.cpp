#include <unistd.h>
#include "dropboxapiv2.h"
#include "docmder.h"
#include "dropboxurl.h"

#include "utility.h"
#include "statuscodelist.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 



DropboxApiV2::DropboxApiV2():MAX_BUF_SIZE(4096),MAX_STRING_LEN(256)
{
	pid = NULL;
	
	_running = true;
	
	pthread_mutex_init(&task_lock,NULL);
	
	res_buf = new char [MAX_BUF_SIZE];
	left_bytes = MAX_BUF_SIZE-1;
	access_token = new char [96];
	access_token[0] = '\0';
	d_recursive = false;
	u_recursive = false;
	done_parse_task = false;
	cursor = new char [MAX_STRING_LEN];
	
	
	d_dst = NULL;
	d_src = NULL;
	
	u_src = NULL;
	u_dst = NULL;

	dSync = false;
	uSync = false;
	
	sem_init(&sig_task,0,0);

}

DropboxApiV2::~DropboxApiV2()
{
	sem_destroy(&sig_task);
	if(pid)
		delete [] pid;
	pthread_mutex_destroy(&task_lock);
	
	delete [] res_buf;
	delete [] access_token;
	delete [] cursor;

	ClearTask();
}

void* Working(void* arg)
{
	DropboxApiV2* api = (DropboxApiV2*)arg;
	api->DoWork();
	return NULL;
}


size_t DownloadParsing(void *ptr, size_t size, size_t nmemb, void *obj)
{
	DropboxApiV2* api = (DropboxApiV2*) obj;
	const char *tag_s, *tag_e, *display_s, *display_e, *clientT_s, *clientT_e;
	int copy_bytes, has_bytes;
	const char *cur_p;
	int  left_bytes;
	const char* end_p;
	bool has_more;
	
	has_bytes = size*nmemb;

	while(has_bytes > 0)
	{
		cur_p = api->GetCurPtr();
		left_bytes = api->GetLeftBytes();
		
		copy_bytes = (has_bytes > left_bytes) ? left_bytes : has_bytes;
		api->AppeningData((char*)ptr, copy_bytes);	
		end_p = cur_p + copy_bytes;
		
		
		ptr = (char*)ptr + copy_bytes;
		has_bytes-=copy_bytes;
		
		cur_p = api->GetResPtr();
		
		
		if(get_tag_string_value(cur_p,TAGCURSOR, &tag_s, &tag_e))
			api->SetCursor(tag_s,tag_e-tag_s+1);
		if(get_tag_boolen_value(cur_p,TAGHASMORE, &has_more))
			api->SetHasMore(has_more);
		
		while(1)
		{
			if(get_tag_string_value(cur_p,TAGTAG, &tag_s, &tag_e))
			{
				if(get_tag_string_value(tag_e+1, TAGFILE, &display_s, &display_e))
				{
					if(!api->GetDSync() || get_tag_string_value(display_e+1, TAGCLIENTMOD, &clientT_s, &clientT_e))
					{
						//MAKE TASK HERE
						if(!strncmp(tag_s, FILE_NAME, strlen(FILE_NAME)))
						{
							*((char*)display_e+1)='\0';
							
							if(api->GetDSync())
							{
								char path[1024];
								struct stat filestat;
								int ret;
								
								ret = snprintf(path,1024,"%s",api->GetDDst());
								
								u8_unescape(path+ret,1024-ret,(char*)display_s);

								if(!stat(path,&filestat))
								{
									time_t dropboxt;
									struct tm t;
									
									*((char*)clientT_e+1)='\0';
									
									sscanf(clientT_s,"%d-%d-%dT%d:%d:%dZ",&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
									
									t.tm_isdst = -1;
									t.tm_year-=1900;
									t.tm_mon-=1;
									dropboxt = mktime(&t);
									
									
									if(dropboxt <= filestat.st_mtime)
									{
										cur_p = clientT_e + 1 +1;
										continue;
									}
									
								}
								
								display_e = clientT_e;
							}
							
							task* t = new task(file,download,  api->GetDDst(),display_s );
							api->PushTask(t);
							api->addDownloadingTask();
						}
						else if(!strncmp(tag_s, FOLDER_NAME, strlen(FOLDER_NAME)))
						{
							//only recursive makes folder
							if(api->isDRecursive())
							{
								*((char*)display_e+1)='\0';							
								task* t = new task(folder,download, api->GetDDst(), display_s);
								api->PushTask(t);
							}
						}
						else
						{
							*((char*)(tag_e+1)) = '\0';
							fprintf(stderr,"[%s:%d] Warning: tag name %s is out of knowledge\n",__FILE__,__LINE__,tag_s);
						}
					
						cur_p = display_e + 1 +1;
					}
					else
						break;
				}
				else
					break;
			}
			else
				break;
		}
		api->MoveDataFromCurToP((char*)cur_p,end_p);
	}

	
	return size*nmemb;
}

int DropboxApiV2::CheckFileType(const char* path, enum file_type* type)
{
	int ret;
	const char *tag_s,  *tag_e;
	char* meta_buf;
	
	meta_buf = new char [MAX_BUF_SIZE];
	
	ret = GetMetaData(path, meta_buf, MAX_BUF_SIZE);
	
	switch(ret)
	{
		case SUCCESS:
			if(get_tag_string_value(meta_buf,TAGTAG, &tag_s, &tag_e))
			{
				*((char*)(tag_e+1)) = '\0';
				if(!strncmp(tag_s, FOLDER_NAME, strlen(FOLDER_NAME)))
					*type = folder;
				else if(!strncmp(tag_s, FILE_NAME, strlen(FOLDER_NAME)))
					*type = file;
				else 
					*type = unknown;
			}
			break;
		case ROOT_FOLDER:
			*type = folder;
			break;
		default:
			ret = ERR_NO_SUCH_FILE;
			break;
	}
	
	delete [] meta_buf;
	
	return ret;
	
	
}


int DropboxApiV2::DownloadFile(const char* src, const char* dst, bool sync, bool recursive,int nOfThread)
{
	if(!dst || !src || src[0]!='/')
		return ERR_PARAMETERS_NULL;
	
	int ret;
	enum file_type type;
	
	_running = true;
	
	//check file exists and its type
	ret = CheckFileType(src, &type);
	if(ret < 0)
		return ret;
	
	//if its folder, create the folder at the local
	if(ret == SUCCESS || ret == ROOT_FOLDER)
	{
	
		string folder_path;
		
		const char *s, *e;
		
		folder_path = dst;
		
		
		if(type == folder)
			folder_path+=src;
		else
		{
			s = strchr(src,'/');
			e = strrchr(src,'/');
			
			if(s!=e)
			{
				string postfix(src,e-s);
				folder_path+=postfix;
			}	
		}
		
		create_folder(folder_path.c_str());
	}
	

	dSync = sync;
	
	
	FlushStore();
	
	CreateThread(nOfThread);
	
	DoCmd* doc = new DoCmd();
	map<string,string> headers;
	char* post_data, *p;
	int need_len;
	
	malloc_and_copy_string(&d_src, MAX_STRING_LEN, src);
	malloc_and_copy_string(&d_dst, MAX_STRING_LEN, dst);
	
	
	//set header
	headers[WORD_AUTH] = access_token;
	headers[WORD_CONTENTTYPE] = "application/json";
	
	//set the data path and recursive
	need_len = strlen(src);
	post_data = new char [need_len + 64];
	p = post_data;
	ret = sprintf(p, "{\"path\":\"");
	p+=ret;

	if(strlen(src)!=1)//root folder
	{
		ret = sprintf(p, "%s",src);
		p+=ret;
	}
	ret = sprintf(p,"\"");
	p+=ret;
	if(recursive)
	{
		ret = sprintf(p,",\"recursive\":true");
		p+=ret;
	}
	ret = sprintf(p,"}");
	d_recursive = recursive;
	
	

	
	current_download = 0;
	done_parse_task = false;
	
	//do the downloading
	if( type == folder)
	{
		
		downloading_task = 0;
		//only folder need to parse the list
		Openfile of(doc->GetCurl(),NULL,NULL,&_running);
		
		
		has_more = false;
		
		ret = doc->Perform(URL_LIST_FOLDER, headers, post_data, "POST", DownloadParsing, (void*)this, NULL, NULL);
		
		
		while(has_more)
		{
			FlushStore();
			has_more=false;
			ret = doc->Perform(URL_DOWNLOAD_CONTINUE, headers, cursor, "POST", DownloadParsing, (void*)this, NULL,NULL);
		}
		
		
		done_parse_task = true;
	}
	else
	{
		downloading_task = 1;
		done_parse_task = true;
		task* t = new task(file,download, d_dst, d_src);
		PushTask(t);
	}

	
	RecycleThread();
	ClearTask();
	
	delete doc;
	delete [] post_data;
	delete [] d_src;
	delete [] d_dst;
	
	return ret;
	
}

void DropboxApiV2::MakeUploadTask(char* src_folder, const char* dst_folder, bool recursive, const char* prefix, int level)
{
	
	DIR *dir;
	struct dirent *entry;
	int count = 0;
	
	dir = opendir(src_folder);
	if(dir){
		while ((entry = readdir(dir)) != NULL)
		{

			if (entry->d_type == DT_DIR) {
				
				if (!recursive || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				
				char path[1024];
				char* p;
				
				snprintf(path, sizeof(path), "%s/%s", src_folder, entry->d_name);
				//printf("path:%s\n",path);
				p = path;
				for(int i=0; i<level; i++)
				{
					p = strrchr(path,'/');
					if(p) *p = '\0';
				}
					
				snprintf(path, sizeof(path), "%s/%s", src_folder, entry->d_name);
				if(p)p++;
				
				MakeUploadTask(path, dst_folder, recursive, p,level+1);
				
			}
			else{
				string full_path;

				full_path="";
				if(prefix && strlen(prefix)>0)
				{
					full_path+=prefix;
					full_path+='/';
				}
				full_path+=entry->d_name;
								
				task* t = new task(file, upload, dst_folder, full_path.c_str());
				PushTask(t);
				
				count++;
			}
			
		}
		
		if(count == 0 && recursive)
		{
			task* t = new task(folder, upload, dst_folder, prefix);
			PushTask(t);
		}
		
	}

	
}

int DropboxApiV2::UploadFile(const char* src,const char* dst, bool inclulde, bool recursive, int nOfThread)
{
	if(!dst || !src || dst[0]!='/')
		return ERR_PARAMETERS_NULL;
	
	struct stat filestat;
	int ret;
	
	ret = stat(src,&filestat);
	if(ret != 0)
		return ERR_NO_SUCH_FILE;
	
	_running = true;

	malloc_and_copy_string(&u_src, MAX_STRING_LEN*4, src);
	malloc_and_copy_string(&u_dst, MAX_STRING_LEN*4, dst);
	
	
	if(S_ISDIR(filestat.st_mode)){
		
		CreateThread(nOfThread);
		
		int len,len2;
		char *s;
		
		len = strlen(u_dst);
		while(u_dst[len-1] == '/' && len > 1)u_dst[--len]='\0';
		
		s = u_src;
		len2 = strlen(s);
		while(s[len2-1]=='/' && len2 > 1) s[--len2] = '\0';
		
		if(inclulde)
		{
			s = strrchr(u_src,'/');
			if(!s) s = u_src;
			else s++;
			
			if(u_dst[len-1] == '/')
				snprintf(u_dst+len, MAX_STRING_LEN*4-len, "%s",s);
			else
				snprintf(u_dst+len, MAX_STRING_LEN*4-len, "/%s",s);
			
			len = strlen(u_dst);
			while(u_dst[len-1] == '/')u_dst[--len]='\0';
		}
		
		MakeUploadTask(u_src, u_dst,recursive,"", 1);
		
		RecycleThread();
		
		ClearTask();
		
	}
	else{
		
		DoCmd* doc = new DoCmd();
		map<string,string> headers;
		string path;
		FILE* fin;
	
		Openfile ofread(doc->GetCurl(),&fin,u_src,&_running);
	
		path =  "{\"path\":\"";
		path+=dst;
		path+="\"}";
		
		headers[WORD_AUTH] = access_token;
		headers[WORD_CONTENTTYPE] = "application/octet-stream";
		headers[WORD_APIARG] = path;
		
		ret = doc->Perform(URL_UPLOAD, headers, NULL, "POST", NULL, NULL, NULL,NULL, NULL, UploadReadBack, (void*)&ofread);
		if(ofread.is_open )
			fclose(fin);
		
		
		delete doc;
	}
	

	delete [] u_src;
	delete [] u_dst;
	
	return ret;
	
}

int DropboxApiV2::DeleteFile(const char* file)
{
	
	if(!file || file[0] != '/')
	{
		fprintf(stderr,"file name should starts with /\n");
		return ERR_PARAMETERS_NULL;
	}
	DoCmd* doc = new DoCmd();
	map<string,string> headers;
	string post_data;
	int ret;
	Openfile emptyof(doc->GetCurl(),NULL,NULL,&_running);
	
	headers[WORD_AUTH] = access_token;
	headers[WORD_CONTENTTYPE] = "application/json";
	
	post_data = "{\"path\":\"";
	post_data+=file;
	post_data+="\"}";
	
	ret = doc->Perform(URL_DELETE, headers, post_data.c_str(), "POST", NULLEverything, (void*)&emptyof, NULL, NULL);
	
	delete doc;
	
	return ret;
	
	
}

void DropboxApiV2::CreateThread(int nOfThread)
{

	if(nOfThread > 0)
		num_thread = nOfThread;
	else
		num_thread = 1;
	pid = new pthread_t[num_thread];
	
	
	for(int i=0; i<num_thread; i++)
		pthread_create(&pid[i],NULL,Working,(void*)this);
}

void DropboxApiV2::RecycleThread()
{
	if(num_thread > 0)
	{
		for(int i=0; i<num_thread; i++)
			sem_post(&sig_task);
		for(int i=0; i<num_thread; i++)
			pthread_join(pid[i],NULL);	
	}
}

int DropboxApiV2::GetMetaData(const char* path, char* buf, int buf_size)
{
	
	if(!path || !buf || buf_size <= 0)
		return ERR_PARAMETERS_NULL;
		
	if(strlen(path) == 1 && path[0] == '/')
		return ROOT_FOLDER;
	
	_running = true;
	
	DoCmd* doc = new DoCmd();
	map<string,string> headers;
	Openfile of(doc->GetCurl(),NULL,NULL,&_running);
	DCopyToBuf ctb(doc->GetCurl(), buf, buf+buf_size-1);
	string post_data;
	int ret;
	
	headers[WORD_AUTH] = access_token;
	headers[WORD_CONTENTTYPE] = "application/json";
	
	post_data = "{\"path\":\"";
	post_data+=path;
	post_data+="\"}";
	
	ret = doc->Perform(URL_GET_META, headers, post_data.c_str(), "POST", CopyToBuf, (void*)&ctb, NULL,NULL);
	
	*(ctb.c) = '\0';
	
	delete doc;
	
	return ret;
}



void DropboxApiV2::DoWork()
{
	task* t;
	char* buf = new char [MAX_STRING_LEN*4];
	DoCmd* dc = new DoCmd();
	map<string,string> dheaders;
	FILE* fout;
	Openfile of(dc->GetCurl(),&fout,buf,&_running);
	map<string,string> uheaders;
	FILE* fin;
	Openfile emptyof(dc->GetCurl(),NULL,NULL,&_running);	
	Openfile ofread(dc->GetCurl(),&fin, buf,&_running);
	string path;
	
	
	dheaders[WORD_AUTH] = access_token;
	
	uheaders[WORD_AUTH] = access_token;
	

	while(_running)
	{
		sem_wait(&sig_task);
		if(!(t = GetTask()) || !_running)
			break;
		
		switch(t->act)
		{
			case download:
				snprintf(buf,MAX_STRING_LEN*4, "%s%s",t->dst_prefix, t->file_name);
				switch (t->type)
				{	
					case folder:		
						create_folder(buf);
						break;
					case file:
						
						path =  "{\"path\":\"";
						path+=t->file_name;
						path+="\"}";
						dheaders[WORD_APIARG] = path;
						
						
						printf("dowloading: %s\n",buf);
						
						dc->Perform(URL_DOWNLOAD, dheaders, NULL, "POST", CopyFile, (void*)&of, NULL,NULL);
						if(of.is_open)
						{
							of.is_open = false;
							fclose(fout);
							current_download++;
							if(!of.finished)
								unlink(of.path);
						}
						break;
					default:
						break;
				}
				break;
			case upload:
				snprintf(buf,MAX_STRING_LEN*4, "%s/%s",u_src, t->file_name);
				
				path =  "{\"path\":\"";
				path+=t->dst_prefix;
				if(strlen(t->dst_prefix) > 1)
					path+="/";
				path+=t->file_name;
				path+="\"}";
				
				
				switch (t->type)
				{
					case folder:
						uheaders[WORD_CONTENTTYPE] = "application/json";
						dc->Perform(URL_CREATE_FOLER, uheaders, path.c_str(), "POST", NULLEverything, &emptyof, NULL,NULL);
						break;
					case file:
					
						uheaders[WORD_CONTENTTYPE] = "application/octet-stream";
						uheaders[WORD_APIARG] = path;
					
						dc->Perform(URL_UPLOAD, uheaders, NULL, "POST", NULLEverything, &emptyof, NULL,NULL, NULL, UploadReadBack, (void*)&ofread);
						if(ofread.is_open ){
							fclose(fin);
							ofread.is_open = false;
						}
						break;
					default:
						break;
				}
				break;
			case del:
				uheaders[WORD_CONTENTTYPE] = "application/json";
				path =  "{\"path\":\"";
				path+=t->file_name;
				path+="\"}";
				dc->Perform(URL_DELETE, uheaders, path.c_str(), "POST", NULLEverything, &emptyof, NULL,NULL);
				break;
			default:
				break;
		}
		
		delete t;
	}
	delete [] buf;
	delete dc;
	
}

task* DropboxApiV2::GetTask()
{
	task* t = NULL;
	
	pthread_mutex_lock(&task_lock);
		if(tasks.size() > 0)
		{
			t = tasks.front();
			tasks.pop();
		}
	pthread_mutex_unlock(&task_lock);
	
	return t;
}

void DropboxApiV2::PushTask(task* t)
{
	if(!t)
		return ;
	
	pthread_mutex_lock(&task_lock);
		tasks.push(t);
		sem_post(&sig_task);
	pthread_mutex_unlock(&task_lock);
	
}

void DropboxApiV2::FlushStore()
{
	left_bytes = MAX_BUF_SIZE - 1;
	cur_p = res_buf;
}

char* DropboxApiV2::GetCurPtr()
{
	return cur_p;
}
int DropboxApiV2::GetLeftBytes()
{
	return left_bytes;
}
void DropboxApiV2::SetLeftBytes(int size)
{
	left_bytes = size;
}
void DropboxApiV2::MoveDataFromCurToP(char* start, const char* toP)
{
	int move_bytes = toP-start;
	memmove(res_buf,start,move_bytes);

	left_bytes = MAX_BUF_SIZE -1 - move_bytes;
	cur_p = res_buf + move_bytes;
}
void DropboxApiV2::AppeningData(char* s, int size)
{
	memcpy(cur_p, s, size);
	*(cur_p + size) = '\0';
	left_bytes-=size;
}


int DropboxApiV2::SetAccessToken(const char* _access_token)
{
	if(!_access_token)
		return ERR_PARAMETERS_NULL;
	
	int len = strlen(_access_token);
	if(len > 95)
		return ERR_BUFFER_OVERFLOW;
	strncpy(access_token, _access_token, len);
	access_token[len] = '\0';
	
	return SUCCESS;
}

int DropboxApiV2::SetCursor(const char* p,int bytes)
{
	if(!p || bytes<=0)
		return ERR_PARAMETERS_NULL;
	
	int format_bytes;
	
	format_bytes = sprintf(cursor,"{\"cursor\": \"");
	
	memcpy(cursor+format_bytes, p, bytes);
	cursor[bytes+format_bytes] = '\"';
	cursor[bytes+format_bytes+1] = '}';
	cursor[bytes+format_bytes+2] = '\0';
	
	return SUCCESS;
}


int DropboxApiV2::RetrieveAccessToken(const char* authorization_token, const char* app_key, const char* app_secret, char* buf, int buf_size)
{
	if(!authorization_token || !app_key || !app_secret)
		return ERR_PARAMETERS_NULL;
	
	_running = true;
	
	DoCmd* doc = new DoCmd();
	map<string,string> headers;
	
	Openfile of(doc->GetCurl(),NULL,NULL,&_running);
	string post_data;
	int ret;
	int key_len, secret_len;
	char* usrpwd;
	char* returnString = new char[MAX_STRING_LEN];
	DCopyToBuf ctb(doc->GetCurl(), returnString, returnString + MAX_STRING_LEN-1);
	
	post_data = "code=";
	post_data+=authorization_token;
	post_data+="&grant_type=authorization_code";

	key_len = strlen(app_key);
	secret_len = strlen(app_secret);
	
	usrpwd = new char [key_len + secret_len + 2];
	sprintf(usrpwd,"%s:%s",app_key, app_secret);
	
	ret = doc->Perform(URL_AUTH2_AT, headers, post_data.c_str(), "POST", CopyToBuf, (void*)&ctb,NULL,NULL, usrpwd);
	
	if(ret == SUCCESS)
	{
		*(ctb.c) = '\0';

		if(buf)
		{
			int copy_bytes;
			int len;
			
			len = strlen(returnString);
			copy_bytes = buf_size > len ? len : buf_size-1;
			
			strncpy(buf, returnString, copy_bytes);
			buf[copy_bytes] = '\0';
		}
		
		const char* s;
		const char* e;
		
		if(get_tag_string_value(returnString, TAGACCESSTOKEN, &s, &e))
		{
			*((char*)e+1)='\0';
			sprintf(access_token,"%s", s);
		}
		else
			ret = ERR_UNKNOWN;
	}
	
	
	delete doc;
	delete [] usrpwd;
	delete [] returnString;
	//printf("access_token: %s\n",access_token);
	
	return ret;
}

void DropboxApiV2::ClearTask()
{
	task* t;
	while(tasks.size() > 0)
	{
		t = tasks.front();
		delete t;
		tasks.pop();
	}
	
}

int DropboxApiV2::StopWorking()
{
	_running = false;
	for(int i=0; i<num_thread; i++)
		sem_post(&sig_task);
	return SUCCESS;
}



