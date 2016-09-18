# dropboxapiv2
This is an api, written in c++, to access dropbox data with Oauth2.0 and dropbox api v2.

Currently, this project only supports the download and upload functionality.

Dependencies
------
libcurl and libssl

Compile
------
make

(create the folder libs which contains lib and include)

make samples

(before compile the sample, you must modify the sample/download/main.cpp. Change the access token to yours)

Usage
------
###
    DropboxApiV2* api = new DropboxApiV2();
    api->SetAccessToken("access token"); //or
    api->RetrieveAccessToken("authorization token", "app key", "app secret");
    api->DownloadFile("/","./dropbox",true,5);
### Api arguments
DownloadFile
    The first argument is the dropbox directory or file name with fully path you want to download.
    The second argument is the directory or file name in the local machine you want to save the file.
    The third argument is to download recursively or not.
    The fourth argument is number of thread that you want to use simultaneously to download.

   
Access token
------
You can use the following url to get "authorization token" without redirect url at the time the user grant you permission.

https://www.dropbox.com/1/oauth2/authorize?client_id=appkey&response_type=code

Then you can use the api RetrieveAccessToken to get access token.
