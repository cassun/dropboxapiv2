# dropboxapiv2
This is an api, written in c++, to access dropbox data by dropbox api v2.

Currently, this project supports the download, upload and delete file.

To decrease dependencies, i didn't use json library. Instead, I parse the only information I need.

Dependencies
------
libcurl and libssl

Compile
------
###
    make
    (create the folder libs which contains lib and include)
###
    make samples
    (before compile the sample, you must modify the sample/download/main.cpp. Change the access token to yours)

Usage
------
Downloading example
###
    DropboxApiV2* api = new DropboxApiV2();
    api->SetAccessToken("access token");
    api->DownloadFile("/","./dropbox",true,true,5);
    delete api;
### Api arguments
Basically, api return an integer value. If the value is negative, something is wrong.

Must call RetrieveAccessToken or SetAccessToken before use any other api.

####int RetrieveAccessToken(const char* authorization_token, const char* app_key, const char* app_secret, char* buf=NULL, int buf_size=0)
This function would retreive the access token with authorization token.

        authorization_token: authorization token get from user
        app_key: your app key
        app_secret: your app secret
        buf: if you want the access token, you can allocate the buffer to store it.
####int SetAccessToken(const char* _access_token)
        _access_token: if user retrieve access token by yourself, you can set access token before use api
####int GetMetaData(const char* path, char* buf, int buf_size)
        path: the file name with fully path
        buf: buffer to store the metadata, user must allocate by his/herself
        buf_size: buffer size
####int DownloadFile(const char* src, const char* dst, bool sync, bool recursive, int numOfThread)
        src: the dropbox directory or file name with fully path you want to download.
        dst: the directory or file name in the local machine you want to save the file.
        sync: if the dst directory has the same file name, dowloading file if and only if the file in dropbox is newer than locals
        recursive: download recursively or not.
        numOfThread: number of thread that you want to use simultaneously to download.
####int UploadFile(const char* src,const char* dst, bool inclulde, bool recursive=false, int nOfThread=1);
        src: the directory or file in the local
        dst: the directory  in the dropbox or file name with fully path
        inculde: if src is a directory, create this folder in the dropbox
        recursive: recursively scan src directory
        nOfThread: number of thread to use
####int DeleteFile(const char* file)
        file: delete a file or directory with fully path in dropbox
####int GetThumbnail(const char* file, unsigned char* buf, int buf_size,enum pic_format* format=NULL, enum pic_size* size=NULL)
	file: the file name, which is picture, with fully path in dropbox
	buf: the buffer that user has to allocate to store thumbnail
	buf_size: buffer size
	format: the thumbnail format, dropbox only support png and jpg currently.
	size: the thumbnail size
####
	enum pic_format{jpg,png};
	enum pic_size{w32h32, w64h64, w128h128, w640h480, w1024h768};
####int StopWorking()
you can call this function with another thread to stop downloading or uploading.
        
        
        

   
Access token
------
You can use the following url to get "authorization token" without redirect url at the time the user grant you permission.

https://www.dropbox.com/1/oauth2/authorize?client_id=appkey&response_type=code

Then you can use the api RetrieveAccessToken to get access token.
