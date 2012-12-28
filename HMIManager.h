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


#ifndef HMIMANAGER_H
#define HMIMANAGER_H


#include "LogTracer.h"
#include "SharedTypes.h"
#include "ScadaItem.h"
#include "ItemRepository.h"
#include "mongoose.h"
#include <map>


class HMIManager
{
  private:
    LogTracer *_Logger;
    IHMIEventSubscriber *_EventSubscriber;
    mg_context *_Webserver;    
    static HMIManager *_Instance;
    std::map<string,ScadaItem*> _HmiToScadaMappings;    
    ItemRepository *_ItemRepo;
    std::map<string,string> _SiteMaps;
    mg_connection *_ClientConnection;
    string ExpandLinkedPages(string argContent);
    string GetLabelForItemValue(int16_t argItemValue,ItemProperties::T argProperty,ItemTypes::T  argItemType);
    string GetIconForItemValue(int16_t argItemValue,ItemProperties::T argProperty,ItemTypes::T  argItemType);
    int GetFilesInDir (string argDir, vector<string> &argFiles);
    bool InitSiteMaps();
    bool InitWebserver();
    void CloseWebserver();    

  public:    
    static HMIManager *GetInstance();
    virtual ~HMIManager();
    HMIManager(ItemRepository *argItemRepo, IHMIEventSubscriber *argEventSubsciber,LogTracer *argLogger);
    bool Start();
    void Stop();
    int GetMessagCount();
    void NotifyUserMessage(ScadaItemMessage argMsg);
    void UpdateItemView(ScadaItemMessage argMsg);
    string GetSiteMap(string argSiteMapName);
    ScadaItem* GetItem(string argItemName);
    void SetWebSocketClient(mg_connection* argConnection);
      
};

#endif // HMIMANAGER_H
