/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "HMIManager.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <dirent.h> // directory header
#include <errno.h>
#include "SharedUtils.h"


using namespace std;

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";
  



HMIManager::HMIManager(ItemRepository *argItemRepo,IHMIEventSubscriber *argEventSubsciber,LogTracer *argLogger)
{
  _Logger = argLogger;
  _EventSubscriber = argEventSubsciber;
  _ItemRepo = argItemRepo;
  _Instance = this;
  
}

HMIManager* HMIManager::_Instance = NULL;
 
HMIManager* HMIManager::GetInstance()
{
  return _Instance;
}

int HMIManager::GetMessagCount()
{
  return 999;
}

void HMIManager::FireNewMessage(ItemUpdateMessage argMsg)
{
  if(_EventSubscriber != NULL)
    _EventSubscriber->HMIMessageReceived(argMsg);
}

string HMIManager::GetSiteMap(string argSiteMapName)
{
  return _SiteMaps[argSiteMapName];
}



static void get_qsvar(const struct mg_request_info *request_info,const char *name, char *dst, size_t dst_len) 
{
  const char *qs = request_info->query_string;
  mg_get_var(qs, strlen(qs == NULL ? "" : qs), name, dst, dst_len);
}

// If "callback" param is present in query string, this is JSONP call.
// Return 1 in this case, or 0 if "callback" is not specified.
// Wrap an output in Javascript function call.
static int handle_jsonp(struct mg_connection *conn,const struct mg_request_info *request_info) {
  char cb[64];

  get_qsvar(request_info, "callback", cb, sizeof(cb));
  if (cb[0] != '\0') 
  {
    mg_printf(conn, "%s(", cb);
  }

  return cb[0] == '\0' ? 0 : 1;
}

// A handler for the /ajax/get_messages endpoint.
// Return a list of messages with ID greater than requested.
static void ajax_static_version(struct mg_connection *conn,const struct mg_request_info *request_info)
{
  char last_id[32], *json;
  int is_jsonp;

  mg_printf(conn, "%s", ajax_reply_start);
  is_jsonp = handle_jsonp(conn, request_info);

  get_qsvar(request_info, "last_id", last_id, sizeof(last_id));
  json = "1.0.0.99";
  mg_printf(conn, "[%s]", json);
  

  if (is_jsonp) {
    mg_printf(conn, "%s", ")");
  }
}

static void ajax_sitemaps(struct mg_connection *conn,const struct mg_request_info *request_info,const char* argSiteMapName)
{  
  char last_id[32];
  int is_jsonp;
  mg_printf(conn, "%s", ajax_reply_start);
  is_jsonp = handle_jsonp(conn, request_info);

  get_qsvar(request_info, "last_id", last_id, sizeof(last_id));  
  string siteMap = HMIManager::GetInstance()->GetSiteMap(argSiteMapName); 
  const char *strContent = siteMap.c_str();
  mg_printf(conn, "%s", strContent);
  

  if (is_jsonp) {
    mg_printf(conn, "%s", ")");
  }
}



int16_t ConvertToItemValue(string argStringValue,ItemTypes::T argItemType)
{
  int16_t result = 0;
  if(argStringValue == "ON")
    return 1;
  else if(argStringValue == "OFF")
    return 0;
  else if(argStringValue == "UP")
    return 1;
  else if(argStringValue == "DOWN")
    return 2;
  else if(argStringValue == "STOP")
    return 0;
  else if(argItemType == ItemTypes::Switch)
  {
    result = atoi(argStringValue.c_str());
  }
  else
  {
    printf("ItemValue %s undefined\r\n",argStringValue.c_str());    
    return 0;  
  }
 
}

static void *WebServerCallback(enum mg_event event,struct mg_connection *conn)
{
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  void *processed = const_cast<char*>("yes");
  void *emptystring = const_cast<char*>("");
  
  

  if (event == MG_WEBSOCKET_READY) 
  {    
    printf("MG_WEBSOCKET_READY\r\n");
    unsigned char buf[40];
    buf[0] = 0x81;
    buf[1] = snprintf((char *) buf + 2, sizeof(buf) - 2, "%s", "server ready");
    mg_write(conn, buf, 2 + buf[1]);
    return emptystring;  // MG_WEBSOCKET_READY return value is ignored
  } 
  else if (event == MG_WEBSOCKET_CONNECT) 
  {   
    printf("MG_WEBSOCKET_CONNECT\r\n");    
    return NULL;  
  } 
  else if (event == MG_WEBSOCKET_MESSAGE)
  {
    printf("MG_WEBSOCKET_MESSAGE\r\n");
    unsigned char buf[200];
    unsigned char reply[200];
    int n, i, mask_len;
    int exor, msg_len, len;

    // Read message from the client.
    // Accept only small (<126 bytes) messages.
    len = 0;
    msg_len = mask_len = 0;
    for (;;) 
    {
      if ((n = mg_read(conn, buf + len, sizeof(buf) - len)) <= 0)
      {
        return emptystring;  // Read error, close websocket
      }
      len += n;
      if (len >= 2) {
        msg_len = buf[1] & 127;
        mask_len = (buf[1] & 128) ? 4 : 0;
        if (msg_len > 125) {
          return emptystring; // Message is too long, close websocket
        }
        // If we've buffered the whole message, exit the loop
        if (len >= 2 + mask_len + msg_len) {
          break;
        }
      }
    }

    // Prepare frame
    reply[0] = 0x81;  // text, FIN set
    reply[1] = msg_len;

    // Copy message from request to reply, applying the mask if required.
    for (i = 0; i < msg_len; i++) 
    {
      exor = mask_len == 0 ? 0 : buf[2 + (i % 4)];
      reply[i + 2] = buf[i + 2 + mask_len] ^ exor;
    }

    // Echo the message back to the client
    mg_write(conn, reply, 2 + msg_len);

    // Return non-NULL means stoping websocket conversation.
    // Close the conversation if client has sent us "exit" string.
    return memcmp(reply + 2, "exit", 4) == 0 ? emptystring : NULL;
  }
  else if (event == MG_NEW_REQUEST)
  {
    bool isPost = strcmp(request_info->request_method , "POST") == 0;
    char* sitemapsURI = "/rest/sitemaps"; 
    bool isSitemapRequest = strncmp(request_info->uri,sitemapsURI,strlen(sitemapsURI)) == 0;
    
    if(isPost)
    {              
      // Read POST data
      string baseUrl = "/rest/items/";
      string postURI =  request_info->uri;
      bool isSetItems = baseUrl.compare(0,baseUrl.length(),request_info->uri);
      if(isSetItems)
      {	
	// User has submitted a form, show submitted data and a variable value
	
	string itemName = postURI.substr(baseUrl.length());
	itemName.erase(itemName.length()-1,1);   //Remove trailing '/'	
	
	ScadaItem* item= NULL;
	item= HMIManager::GetInstance()->GetItem(itemName);
	if(item == NULL)
	{
	  printf("Item-Cfg not found in Repository %s \n",itemName.c_str());
	  return processed;
	}
		
	char value_data[2048];
	int value_data_len;	
	value_data_len = mg_read(conn, value_data, sizeof(value_data));
	value_data[value_data_len] = 0;	//Add Null-Termination manually
	
	ItemUpdateMessage msg;
	msg.MsgType = ItemMessageTypes::StatusUpdate;
	msg.ItemType = item->ItemType;
	msg.ItemIndex = item->Index;
	msg.Property = ItemProperties::Status;
	msg.Value = ConvertToItemValue(value_data,msg.ItemType);
	HMIManager::GetInstance()->FireNewMessage(msg);
		
	printf(" %s is %s \n ",request_info->uri,value_data);
      }
      else
	printf("Unhandled POST-Uri %s  \n ",request_info->uri);
	
      
      processed = NULL;
    }
    else if(isSitemapRequest)  
    { 
      char homeUri[128];
      strcpy (homeUri,sitemapsURI);
      strcat(homeUri,"/home/home");
      if(strcmp(request_info->uri, sitemapsURI) == 0) 
      {
	ajax_sitemaps(conn,request_info,"start");      
      }      
      else if (strcmp(request_info->uri, homeUri) == 0) 
      {
	ajax_sitemaps(conn,request_info,"home_home");      
      }
      else   
      {
	string strUri = string(request_info->uri);
	int posLastSlash = strUri.find_last_of('/');
	string sitemapName = strUri.substr(posLastSlash + 1);
	ajax_sitemaps(conn,request_info,sitemapName.c_str());      
      }
    }
    else
    {
      processed = NULL;
    }
  } 
  else
  {
    processed = NULL;
  }
  
  return processed;
}


bool HMIManager::InitWebserver()
{
  bool success = true;  
  _Logger->Trace("InitWebserver...");    
  const char *options[] = {"document_root", "greent","listening_ports", "8081","num_threads", "5",NULL};
  _Webserver = mg_start(&WebServerCallback, NULL, options);    
  return success;  
}

int HMIManager::GetFilesInDir (string argDir, vector<string> &argFiles)
{
    DIR *dp;
    dirent *dirp;
    if((dp  = opendir(argDir.c_str())) == NULL)
    {
        _Logger->Trace("Error opening " , argDir);
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        argFiles.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}


string HMIManager::ExpandLinkedPages(string argContent)
{
    string result = argContent;
    int startpos = result.find_first_of('$');
    string tagPrefix = "linkedpage.";
    while(startpos >= 0)            
    {
      //Nur linkedpages-Tags suchen
      if(result.compare(startpos+1,tagPrefix.length(),tagPrefix) != 0)
	continue;
      
      int endpos = result.find_first_of('$',startpos+1);
      if(result[startpos-1] == '\"' && result[endpos+1] == '\"')
      {
	startpos--;  //tags sind als string im JSON - Anführungszeichen wegtrimmen
        endpos++;
      }
      int len = endpos - startpos+1;
      string tagName = result.substr(startpos, len);
      string pageName = tagName;
      pageName.erase(0,2+tagPrefix.length());
      pageName.erase(pageName.length()-2,2);
      result = result.erase(startpos,tagName.length());
      result = result.insert(startpos, ExpandLinkedPages(_SiteMaps[pageName]));
      startpos = result.find_first_of('$', 0);
    }            
    return result;  
}

bool HMIManager::InitSiteMaps()
{
  string dirPath = "greent/sitemaps/";  
  vector<string> foundFiles;
  GetFilesInDir(dirPath,foundFiles);
  vector<string>::iterator iter;
  for (unsigned int i = 0;i < foundFiles.size();i++) 
  {
        vector<string> nameParts;	
	SharedUtils::Tokenize(foundFiles[i], nameParts,".");
	if(nameParts.size() < 2 || nameParts[nameParts.size()-1] != "sitemap")
	  continue;
	
	FILE *f;
	char fullPath[200];
	strcpy(fullPath,dirPath.c_str());	
	strcat(fullPath, foundFiles[i].c_str());
	if((f=fopen(fullPath,"r"))==NULL)
	{
	  _Logger->Trace("Unable t open sitemap file ",fullPath);
	  continue;
	}
	
	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	rewind(f); 
	if(size > 32000)
	  _Logger->Trace("Content Size does not match the reserved array: ",size);
	char strContent[32000];
	int len = fread(strContent,1,size,f);
	fclose(f);
	strContent[len] = 0; //Null termination	
	_SiteMaps[nameParts[0]] = strContent;
  }   
  
  for(map<string, string>::iterator iter=_SiteMaps.begin(); iter!=_SiteMaps.end(); ++iter)
  {
      _SiteMaps[(*iter).first] = ExpandLinkedPages((*iter).second);
  }
  
}

ScadaItem* HMIManager::GetItem(string argItemName)
{
  return _ItemRepo->GetItem(argItemName);
}


bool HMIManager::Start()
{
  bool  success = true;
  
  _Logger->Trace("Starting HMIManager... ");
  _Logger->Trace("Loading SiteMaps... ");
  success &= InitSiteMaps(); 
  _Logger->Trace("Init WebServer... ");
  success &= InitWebserver();    
      
  return success;

}

void HMIManager::CloseWebserver()
{
   mg_stop(_Webserver);  
}

void HMIManager::Stop()
{
  CloseWebserver();
}

HMIManager::~HMIManager()
{

}

