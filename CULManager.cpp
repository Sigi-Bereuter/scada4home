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


#include "CULManager.h"
#include "SharedUtils.h"
#include <sys/time.h>


CULManager::CULManager(ICULEventSubscriber *argEventSubsciber,ItemRepository *argItemRepo,string argDeviceName, LogTracer *argLogger) 
{  
  _DeviceName = argDeviceName;
  _ItemRepo = argItemRepo;
  _Logger = argLogger;  
  _EventSubscriber = argEventSubsciber;
  _ITfsm = new IntertechnoFSM(argLogger);
}



void * CULManager::ProcessingLoop()
{  
    timeval nowTime;
    timeval lastRCVTime;
    int rcvCount;
    const uint16_t RCV_BUFF_LEN = 512;
    char rcvBuff[RCV_BUFF_LEN];
    while(1)
    { 			
	
	//logTrace("Reading from CUL...") ;
	rcvCount = read(_DeviceHandle,rcvBuff,RCV_BUFF_LEN);
	if(rcvCount <= 0)
	{
	  usleep(100000);
	  return 0;
	}
	
	gettimeofday(&nowTime,NULL);
	long diffSec = nowTime.tv_sec - lastRCVTime.tv_sec;
        long diffUsec = nowTime.tv_usec - lastRCVTime.tv_usec;
	//if(diffSec > 0 || diffUsec > 500000)
	//  _ITfsm->Reset();
	
	lastRCVTime = nowTime;
	ScadaItemMessage newMsg;	
	string strRCV(rcvBuff);	
	_Logger->Trace("CULManager RCV:" + strRCV);
	
	if(rcvBuff[0] == 'F')
	{
	  HandleFS20(strRCV);
	}
	
	//bool telegramComplete = _ITfsm->Execute(rcvBuff,rcvCount,&newMsg);
		
    }
    return 0;
}

void CULManager::HandleFS20(string argCULRcvString)
{
  string houseCode = argCULRcvString.substr(1,4);
  string deviceCode = argCULRcvString.substr(5,2);
  string cmdCode = argCULRcvString.substr(7,2);
  
  if(houseCode != "D14B")
  {
    _Logger->Trace("CULManager: Unknown FS20 HouseCode %s",houseCode.c_str());
    return;
  } 
  
  ScadaItem *item = _ItemRepo->GetItem(deviceCode);
  if(item == NULL)
  {
     _Logger->Trace("CULManager: Unknown FS20 Device %s no Item-Config found for this",deviceCode.c_str());
    return;    
  }
  
  ScadaItemMessage newMsg;
  newMsg.MsgType = ItemMessageTypes::Command;
  newMsg.ItemType = item->ItemType;
  newMsg.ItemIndex = item->Index;
  newMsg.Property = ItemProperties::Func;
  
  if(item->ItemType == ItemTypes::Switch)
  {
    if(cmdCode == "00")
      newMsg.Value = 1;
    else if(cmdCode == "01")
       newMsg.Value = 0;
    else if(cmdCode == "10")
       newMsg.Value = 3;
    else if(cmdCode == "11")
       newMsg.Value = 2;
    else if(cmdCode == "13")
       newMsg.Value = 13;
    else if(cmdCode == "14")
       newMsg.Value = 14;
    else 
    {
       _Logger->Trace("CULManager: CommandCode %s is unhandled for ItemType %d (as FS20-Device %s) yet ",cmdCode.c_str(), item->ItemType,deviceCode.c_str());
       return; 
    }
    
  }
  else
  {
     _Logger->Trace("CULManager: ItemType %d (as FS20-Device %s) is unhandled for FS20 Devices yet ",item->ItemType,deviceCode.c_str());
    return; 
  }
    
  if(_EventSubscriber != NULL)
	    _EventSubscriber->CULMessageReceived(newMsg);
    
}



bool CULManager::InitCUL()
{
  termios newtio;
 
  
  _Logger->Trace("Init CUL..." );   
  
  const uint16_t RCV_BUFF_LEN = 512;
  const int BAUDRATE = 17;   //Kommt zustande via Macro BAUDRATE B38400;   
  const int FALSE = 0;
  const int TRUE = 1;


  /* open the device to be non-blocking (read will return immediatly) */
  /*fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);*/
  _DeviceHandle = open(_DeviceName.c_str(), O_RDWR | O_NOCTTY);
  if (_DeviceHandle <0) 
  {
    perror(_DeviceName.c_str()); 
    _Logger->Trace("Open Device failed",_DeviceName.c_str() );   
    return false;     
  }

  /* install the signal handler before making the device asynchronous */
  /*
  sigaction saio;            
  saio.sa_handler = signal_handler_IO;
  //saio.sa_mask = 0;
  saio.sa_flags = 0;
  saio.sa_restorer = NULL;
  sigaction(SIGIO,&saio,NULL);
  sigemptyset(&saio.sa_mask); */

  /* allow the process to receive SIGIO */
  //fcntl(fd, F_SETOWN, getpid());
  /* Make the file descriptor asynchronous (the manual page says only
      O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
  //fcntl(fd, F_SETFL, FASYNC);
  

  tcgetattr(_DeviceHandle,&_OldTIO); /* save current port settings */
  /* set new port settings for canonical input processing */
  newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR | ICRNL;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  
  newtio.c_cc[VMIN]=0;
  newtio.c_cc[VTIME]=1;
  tcflush(_DeviceHandle, TCIFLUSH);
  tcsetattr(_DeviceHandle,TCSANOW,&newtio);
  
  

  //write(_DeviceHandle,"X18\n",4);
  write(_DeviceHandle,"X01\n",4);
  write(_DeviceHandle,"V\n",2);
  usleep(1000000);
  uint8_t rcvBuff[255];
  int rcvCount = read(_DeviceHandle,rcvBuff,RCV_BUFF_LEN);
  while(rcvCount > 0)
  {
    rcvBuff[rcvCount] = 0;
    cout << rcvBuff; 
    rcvCount = read(_DeviceHandle,rcvBuff,RCV_BUFF_LEN);
  }
  _Logger->Trace("CUL Init completed with Return " , rcvCount);  
  
  newtio.c_cc[VMIN]=8;
  newtio.c_cc[VTIME]=0;
  tcflush(_DeviceHandle, TCIFLUSH);
  tcsetattr(_DeviceHandle,TCSANOW,&newtio);
  

  
  return true;
}


bool CULManager::Start()
{
  
  _Logger->Log(LogTypes::Audit,"Starting CULManager...");
  bool success = true;
  if(success)
    success = InitCUL();
   
  _ITfsm->Reset();
  pthread_create( &_ProcessingThread, NULL, LaunchMemberFunction,this); // create a thread running function1
    
  return success;
  
}

void CULManager::Stop()
{
  /* restore old port settings */
    tcsetattr(_DeviceHandle,TCSANOW,&_OldTIO);
    close(_DeviceHandle);
}

