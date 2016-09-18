#ifndef __DROPBOXURL_H__
#define __DROPBOXURL_H__

#define URL_LIST_FOLDER 		"https://api.dropboxapi.com/2/files/list_folder"
#define URL_DOWNLOAD			"https://content.dropboxapi.com/2/files/download"
#define URL_DOWNLOAD_CONTINUE	"https://api.dropboxapi.com/2/files/list_folder/continue"
#define URL_GET_META			"https://api.dropboxapi.com/2/files/alpha/get_metadata"
#define URL_AUTH2_AT			"https://api.dropbox.com/1/oauth2/token"
#define URL_UPLOAD				"https://content.dropboxapi.com/2/files/upload"
#define URL_CREATE_FOLER		"https://api.dropboxapi.com/2/files/create_folder"
#define URL_DELETE				"https://api.dropboxapi.com/2/files/delete"

#define TAGTAG					"\".tag\""
#define TAGFILE					"\"path_display\""
#define TAGNAME					"\"name\""
#define TAGACCESSTOKEN			"\"access_token\""
#define TAGHASMORE				"\"has_more\""
#define TAGCURSOR				"\"cursor\""
#define TAGCLIENTMOD			"\"client_modified\""

#define FOLDER_NAME "folder"
#define FILE_NAME 	"file"


#define WORD_AUTH 				"Authorization"
#define WORD_APIARG 			"Dropbox-API-Arg"
#define WORD_CONTENTTYPE		"Content-Type"



//https://www.dropbox.com/1/oauth2/authorize?response_type=code&client_id=vcuroftli68jr62
#endif