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

using namespace std;

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";
  



HMIManager::HMIManager(IHMIEventSubscriber *argEventSubsciber,LogTracer *argLogger)
{
  _Logger = argLogger;
  _EventSubscriber = argEventSubsciber;
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
  char str_sitemap[32000];
  
  char filePath[200] = "greent/";
  strcat(filePath, argSiteMapName);
  
  FILE *f;
  if((f=fopen(filePath,"r"))==NULL)
  {
    printf("\nUnable t open sitemap file");
    return;
  }
  fseek(f, 0, SEEK_END);
  long int size = ftell(f);
  rewind(f); 
  fread(str_sitemap,1,size,f);
      
  int is_jsonp;

  mg_printf(conn, "%s", ajax_reply_start);
  is_jsonp = handle_jsonp(conn, request_info);

  get_qsvar(request_info, "last_id", last_id, sizeof(last_id));  
  mg_printf(conn, "%s", str_sitemap);
  

  if (is_jsonp) {
    mg_printf(conn, "%s", ")");
  }
}

static void *WebServerCallback(enum mg_event event,struct mg_connection *conn)
{
  const struct mg_request_info *request_info = mg_get_request_info(conn);
  void *processed = const_cast<char*>("yes");

  if (event == MG_NEW_REQUEST)
  {
    /*
    if (strcmp(request_info->uri, "/static/version") == 0) 
    {
      ajax_static_version(conn,request_info);
      
    }*/
    if(strcmp(request_info->uri, "/rest/sitemaps") == 0) 
    {
      ajax_sitemaps(conn,request_info,"start.sitemap");      
    }
    else if (strcmp(request_info->uri, "/rest/sitemaps/demo") == 0) 
    {
      ajax_sitemaps(conn,request_info,"demo.sitemap");      
    }
    else if (strcmp(request_info->uri, "/rest/sitemaps/demo/demo") == 0) 
    {
      ajax_sitemaps(conn,request_info,"demo_demo.sitemap");      
    }
    else
    {
      processed = NULL;
    }
    /*
    HMIManager* hmi = HMIManager::GetInstance();
    int recs = hmi->GetMessagCount();
    
    char content[1024];
    int content_length = snprintf(content, sizeof(content),"Hello from mongoose! Remote port: %d records %d",request_info->remote_port,recs);
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %d\r\n"        // Always set Content-Length
              "\r\n"
              "%s",
              content_length, content);
    
    // Mark as processed
    string result = "";
    return &result;
    
    */
				  
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


bool HMIManager::Start()
{
  _Logger->Trace("Starting HMIManager... ");
 
  bool  success = InitWebserver();    
      
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

