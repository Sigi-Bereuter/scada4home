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


#include "ControlManager.h"

ControlManager::ControlManager()
{
   std::cout << "Init Logtracer..." << std::endl;
   _Logger = new LogTracer();
   _Logger->Log(LogTypes::Audit, "LogTracer initilized successfully !");
   
   _Logger->Log(LogTypes::Audit, "Init ItemRepository...");
  _ItemRepo = new ItemRepository(_Logger);
   _Logger->Log(LogTypes::Audit, "Init PLCManager...");
  _PLC = new PLCManager(this,_Logger);
  _Logger->Log(LogTypes::Audit, "Init CULManager...");
  _CUL = new CULManager(this,_ItemRepo, _Logger); 
  _Logger->Log(LogTypes::Audit, "Init HMIManager...");
  _HMI = new HMIManager(_ItemRepo,this,_Logger);
   _Logger->Log(LogTypes::Audit, "Init RASManager...");
  _RAS = new RASManager(_ItemRepo,this,_Logger);

}

ControlManager::~ControlManager()
{
  
}

bool ControlManager::Start()
{
  _Logger->SetLogLevel(true);
  
  _Logger->Log(LogTypes::Audit, "Starting ControlManager...");
  _Logger->Log(LogTypes::Audit, "Loading ItemRepository...");
  _ItemRepo->Load();
  
  _PLC->Start();
  _CUL->Start();
  _HMI->Start();
  _RAS->Start(); 
  
  usleep(1000000);
  SyncPLCItems();  
  
}

void ControlManager::Stop()
{
  _PLC->Stop();
  _CUL->Stop();
  _HMI->Stop();
  _RAS->Stop();
}

void ControlManager::SyncPLCItems()
{
  
  vector<ScadaItem *> items = _ItemRepo->GetItems();
  for(vector<ScadaItem *>::iterator iter = items.begin();iter != items.end();++iter)
  {
    ScadaItemMessage msg;
    msg.MsgType = ItemMessageTypes::StatusRequest;
    msg.ItemType = (*iter)->ItemType;
    msg.ItemIndex = (*iter)->Index;
    msg.Property = ItemProperties::All;
    msg.Value = 0;
    _PLC->Send(msg);
    usleep(1000000);
  }  
}

void ControlManager::PLCMessageReceived(ScadaItemMessage argMsg)
{
  _Logger->Trace("Received PLC_NewMessage Callback");
  if(argMsg.MsgType == ItemMessageTypes::StatusUpdate)
  {
    ScadaItem *item = _ItemRepo->GetItem(argMsg.ItemType,argMsg.ItemIndex);
    if(item != NULL)
    {
      item->Properties[argMsg.Property] = argMsg.Value;
      if(argMsg.Property == ItemProperties::Position)
	_Logger->Trace("Item %d Position=%d",argMsg.ItemIndex,argMsg.Value);
    }
    
    _HMI->UpdateItemView(argMsg);
    
  }
}

void ControlManager::CULMessageReceived(ScadaItemMessage argMsg)
{
  _Logger->Trace("Received CUL_NewMessage Callback");
  _PLC->Send(argMsg);
}

void ControlManager::HMIMessageReceived(ScadaItemMessage argMsg)
{
  _Logger->Trace("Received HMI_NewMessage Callback");  
  _PLC->Send(argMsg);
}

void ControlManager::RASMessageReceived(ScadaItemMessage argMsg)
{
  _Logger->Trace("Received RAS_NewMessage Callback");  
  _PLC->Send(argMsg);
  
  
}

