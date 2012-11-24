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


#ifndef CULMANAGER_H
#define CULMANAGER_H

#include "LogTracer.h"
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <cstdio>
#include "IntertechnoFSM.h"

class CULManager
{
  private:     
    
    termios _OldTIO;
    int _DeviceHandle;
    LogTracer *_Logger;
    pthread_t _ProcessingThread;
    IntertechnoFSM *_ITfsm;
    ICULEventSubscriber *_EventSubscriber;
    bool InitCUL();
    void * ProcessingLoop();
    static void *LaunchMemberFunction(void *obj)
    {
	CULManager *targetObj = reinterpret_cast<CULManager *>(obj);
	return targetObj->ProcessingLoop();
    }
    
  public:
    CULManager(ICULEventSubscriber *argEventSubsciber,LogTracer *argLogger);
    bool Start();
    void Stop();

};

#endif // CULMANAGER_H
