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

CULManager::CULManager(LogTracer *argLogger)
{
  _Logger = argLogger;  

}

bool CULManager::Start()
{
  
  InitCUL();
  
}

void CULManager::Stop()
{
}

bool CULManager::InitCUL()
{
  termios newtio;
 
  
  _Logger->Trace("Init CUL..." );   
  
  const uint16_t RCV_BUFF_LEN = 512;
  const int BAUDRATE = 17;   //Kommt zustande via Macro BAUDRATE B38400;
  const char* MODEMDEVICE = "/dev/ttyACM0";  
  const int FALSE = 0;
  const int TRUE = 1;


  /* open the device to be non-blocking (read will return immediatly) */
  /*fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);*/
  _DeviceHandle = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
  if (_DeviceHandle <0) 
  {
    perror(MODEMDEVICE); 
    _Logger->Trace("Open Device failed",MODEMDEVICE );   
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
  
  

  write(_DeviceHandle,"X18\n",4);    
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
  
  newtio.c_cc[VMIN]=16;
  newtio.c_cc[VTIME]=0;
  tcflush(_DeviceHandle, TCIFLUSH);
  tcsetattr(_DeviceHandle,TCSANOW,&newtio);
  
  _Logger->Trace("CUL Init completed with Return " , rcvCount);  
  
  return true;
}

