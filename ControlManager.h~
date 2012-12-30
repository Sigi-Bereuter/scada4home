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


#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include "LogTracer.h"
#include "PLCManager.h"
#include "CULManager.h"
#include "HMIManager.h"
#include "RASManager.h"
#include "SharedTypes.h"
#include <termios.h>



class ControlManager : public IPLCEventSubscriber,ICULEventSubscriber,IHMIEventSubscriber,IRASEventSubscriber
{
  private:
    LogTracer *_Logger;
    PLCManager *_PLC;
    CULManager *_CUL;
    HMIManager *_HMI;
    RASManager * _RAS;
    ItemRepository *_ItemRepo;
    void PLCMessageReceived(ScadaItemMessage argMsg);
    void CULMessageReceived(ScadaItemMessage argMsg);
    void HMIMessageReceived(ScadaItemMessage argMsg);
    void RASMessageReceived(ScadaItemMessage argMsg);
    

  public:
    ControlManager();
    virtual ~ControlManager();
    bool Start();
    void Stop();
};

#endif // CONTROLMANAGER_H
