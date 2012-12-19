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


#include "RASManager.h"
#include "Pop3Client.h"
#include <sys/time.h> 


RASManager::RASManager(IPLCEventSubscriber *argEventSubsciber,LogTracer *argLogger)
{
  _EventSubscriber = argEventSubsciber;
  _Logger = argLogger;
  //_POP3Client = new Pop3Client("mail.bereuter.com",110);
}

RASManager::~RASManager()
{

}



bool RASManager::Start()
{
  _Logger->Trace("Starting RASManager...");  
      
  pthread_create( &_ProcessingThread, NULL, LaunchMemberFunction,this); // create a thread running function1
    
  return true;

}

void* RASManager::ProcessingLoop()
{  
    timeval nowTime;
    while(1)
    { 		
	gettimeofday(&nowTime,NULL);
	long diffSec = nowTime.tv_sec - _LastPop3Fetch.tv_sec;
        long diffUsec = nowTime.tv_usec - _LastPop3Fetch.tv_usec;
	if(diffSec > 10 )
	{	 
	  _Logger->Trace("Fetching POP3 Server...");	  
	  _LastPop3Fetch = nowTime;
	  FetchPop3Mails();
	}
					
	usleep(1000000);
	
    }
    return 0;

}

void RASManager::FetchPop3Mails()
{
  try
  {    
    Pop3Client client = Pop3Client("mail.bereuter.com",110);
    client.login("home@bereuter.com","Quattro1");
    client.setShortMessage(true);   
    client.listMails();
    client.quit();
    
    AnalyzeMail("rollo_eltern_sued|status|1");

   
  }
  catch (const char *e) 
  {
    cerr << "POP3 client failed: " << e << endl;
    return ;
  }
  catch (...)
  {
    cerr << "An unknown error occured, quitting..." << endl;
    return ;
  }  
}

void RASManager::AnalyzeMail(string argMailText)
{
  SendMail("info@bereuter.com","scada4home response",argMailText);
  
}

void RASManager::SendMail(string argReceiverEmail,string argSubject,string argBodyText)
{
  SMTPClient client;
  client.SendMail(argReceiverEmail,argSubject,argBodyText); 
  
}

void RASManager::Stop()
{
  
  
}

