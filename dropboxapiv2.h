#ifndef __DROPBOXAPIV2_H__
#define __DROPBOXAPIV2_H__

#include <semaphore.h>
#include <pthread.h>
#include <queue>
#include <string.h>

#include "utf8.h"

using std::queue;

enum file_type{unknown,folder, file};
enum task_action{download,upload,del};
enum pic_format{jpg,png};
enum pic_size{w32h32, w64h64, w128h128, w640h480, w1024h768};


typedef struct task
{
	enum file_type type;
	enum task_action act;
	const char* dst_prefix;
	char* file_name;
	task(enum file_type _type, enum task_action _act,  const char* prefix, const char* file):  type(_type), act(_act),dst_prefix(NULL),file_name(NULL)
	{
		int len;
		if(prefix)
		{
			dst_prefix = prefix;
		}
		
		if(file)
		{
			len = strlen(file);
			file_name = new char [len+1] ;
			
			u8_unescape(file_name,len,(char*)file);
			file_name[len] = '\0';
		}
			
	}
	~task()
	{
		if(file_name)
			delete [] file_name;
	}
}task;

class DropboxApiV2
{
public:
	int DownloadFile(const char* src,const char* dst, bool sync=false, bool recursive=false, int nOfThread=1);
	int SetAccessToken(const char* _access_token);
	int GetMetaData(const char* path, char* buf, int buf_size);
	int RetrieveAccessToken(const char* authorization_token, const char* app_key, const char* app_secret, char* buf=NULL, int buf_size=0);		//retreive access token by authorization token
	int StopWorking();
	int UploadFile(const char* src,const char* dst, bool inclulde, bool recursive=false, int nOfThread=1);
	int DeleteFile(const char* file);
	int GetThumbnail(const char* file, unsigned char* buf, int buf_size,enum pic_format* format=NULL, enum pic_size* size=NULL);
public:
	DropboxApiV2();
	~DropboxApiV2();
	void DoWork();
	void FlushStore();
	char* GetCurPtr();
	int GetLeftBytes();
	void SetLeftBytes(int size);
	void MoveDataFromCurToP(char* start, const char* end);
	void AppeningData(char* s, int size);
	char* GetDSrc(){return d_src;}
	char* GetDDst(){return d_dst;}
	bool isDRecursive(){return d_recursive;}
	bool isURecursive(){return u_recursive;}
	void PushTask(task* t);
	char* GetResPtr(){return res_buf;}
	int SetCursor(const char* p,int bytes);
	void SetHasMore(bool v){has_more=v;}
	void addDownloadingTask(){downloading_task++;}
	bool isDoneParse(){return done_parse_task;}
	int GetTotalDownloadTask(){return downloading_task;}
	int GetCurrentDownload(){return current_download;}
	bool GetDSync(){return dSync;}
	bool GetUSync(){return uSync;}
private:
	pthread_t* pid;
	int num_thread;
	sem_t sig_task;
	pthread_mutex_t task_lock;
	queue<task*> tasks;
private:
	char* res_buf;
	char* cursor;
	int left_bytes;
	char* cur_p;
	const int MAX_BUF_SIZE;
	const int MAX_STRING_LEN;
	char *d_src, *d_dst;
	char *u_src, *u_dst;
	char* access_token;
	bool d_recursive, u_recursive;
	bool has_more;
	int downloading_task;
	int current_download;
	bool done_parse_task;
	bool _running;
	bool dSync;
	bool uSync;
private:
	void CreateThread(int nOfThread);
	void RecycleThread();
	void ClearTask();
	task* GetTask();
	void MakeUploadTask(char* src_folder, const char* dst_folder, bool recursive, const char* prefix, int level);
private:
	int CheckFileType(const char* path, enum file_type* type);
};

#endif


