#include <iostream>
#include "dropboxapiv2.h"
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

#include <dirent.h>

 
int main(int argc, char* argv[])
{

	DropboxApiV2* api = new DropboxApiV2();
	//api->RetrieveAccessToken(<authorization token>, <app key>, <app secret>);
	api->SetAccessToken(<access token>);
	
	api->DownloadFile("/","./dropbox",true,5);

	delete api;

	return 0;
}
