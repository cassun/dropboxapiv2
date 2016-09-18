# dropboxapiv2
This is an api, written in c++, to access dropbox data with Oauth2.0 and dropbox api v2.

Currently, this project only supports the download and upload functionality.

Compile
------
make

make samples

Usage
------
###
    DropboxApiV2* api = new DropboxApiV2();
    api->SetAccessToken("access token"); //or
    api->RetrieveAccessToken("authorization token", "app key", "app secret");
    api->DownloadFile("/","./dropbox",true,5);
### api arguments
DownloadFile("dropbox src","local dst",recursive or not, number of thread to dowload);

   
