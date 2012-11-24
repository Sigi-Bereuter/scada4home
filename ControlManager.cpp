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
  _Logger = new LogTracer();
  _PLC = new PLCManager(this,_Logger);
  _CUL = new CULManager(this,_Logger); 

}

ControlManager::~ControlManager()
{
  
}

bool ControlManager::Start()
{
  _Logger->SetLogLevel(true);
  
  _Logger->Trace("Starting ControlManager...");
  _PLC->Start();
  _CUL->Start();
  while(true)
  {    
    usleep(1000000);
  }
}

void ControlManager::Stop()
{
  _PLC->Stop();
  _CUL->Stop();
}

void ControlManager::PLCMessageReceived(PLCMessage argMsg)
{
  _Logger->Trace("Received PLC_NewMessage Callback");
}

void ControlManager::CULMessageReceived(CULMessage argMsg)
{
  _Logger->Trace("Received CUL_NewMessage Callback");
}


