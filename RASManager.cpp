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
#include "SharedUtils.h"
#include "ItemRepository.h"
#include <sys/time.h> 
#include <sstream>


RASManager::RASManager(ItemRepository *argItemRepo,IRASEventSubscriber *argEventSubsciber,string argPOP3Server,string argPOP3User,string argPOP3Password,LogTracer *argLogger)
{
  _EventSubscriber = argEventSubsciber;
  _Logger = argLogger;
  _ItemRepo = argItemRepo;
  _POP3Server = argPOP3Server;
  _POP3User = argPOP3User;
  _POP3Password= argPOP3Password;
}

RASManager::~RASManager()
{

}



bool RASManager::Start()
{
 _Logger->Log(LogTypes::Audit,"Starting RASManager...");  
      
  pthread_create( &_ProcessingThread, NULL, LaunchMemberFunction,this); // create a thread running function1
    
  return true;

}

void* RASManager::ProcessingLoop()
{  
    timeval nowTime;
    gettimeofday(&_LastPop3Fetch,NULL);
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
    Pop3Client client = Pop3Client(_POP3Server,_Logger, 110);
    client.login(_POP3User,_POP3Password);
    client.setShortMessage(true);   
    vector<int> msgIdList;
    client.listMails(msgIdList);
   
    for(vector<int>::iterator iter = msgIdList.begin(); iter!=msgIdList.end(); ++iter)
    {
      int curId = (*iter);
      Email curMail;
      bool rcvOK = client.FetchMail(curId,curMail);
      if(rcvOK)
      {
	AnalyzeMail(curMail);		  
      }
    }
    
    client.quit();
    
   
  }
  catch (const char *e) 
  {
    _Logger->Log(LogTypes::Error, "POP3 client failed: %s ", e);
    return ;
  }
  catch (...)
  {
    _Logger->Log(LogTypes::Error,"An unknown error occured, quitting...");
    return ;
  }  
}

void RASManager::AnalyzeMail(Email argMail)
{
    
  if(argMail.Subject.find("scada4home") < 0 )
  {
    HandleAnalyzeError(argMail,"Unknown Mail Subject received " + argMail.Subject);
    return;
  }    
  
  int commandsFound=0;
  string marker = "##";
  int startPos = argMail.BodyText.find(marker);
  int endPos = argMail.BodyText.find(marker,startPos + marker.length());
  
  if(startPos < 0 || endPos <= startPos)
    return;  
  
  commandsFound++;
  
  string strCommand = argMail.BodyText.substr(startPos+marker.length(),endPos - startPos - marker.length());
  vector<string> cmdTokens;
  SharedUtils::Tokenize(strCommand,cmdTokens,"|");
  if(cmdTokens.size() == 0)
  {
    HandleAnalyzeError(argMail,"Received empty command " + argMail.BodyText);	
    return;  
  }
  
  if(cmdTokens.size() == 1)
  {
    string strOperator = cmdTokens[0];
    if(strOperator == "HELP")
    {
      SendHelp(argMail);
    }
    else
      HandleAnalyzeError(argMail,"Operators not yet implemented " + argMail.BodyText);
  }
  else 
  {
    string actionType = cmdTokens[0]; 
    string strItem = cmdTokens[1];
    
    ScadaItem *item = _ItemRepo->GetItem(strItem);
    if(item != NULL)
    {
      if(actionType == "GET")
      {
	ScadaItemMessage msg;
	msg.MsgType = ItemMessageTypes::StatusRequest;
	msg.ItemType = item->ItemType;
	msg.ItemIndex = item->Index;
	msg.Property = ItemProperties::All;
	
	_EventSubscriber->RASMessageReceived(msg);
	
	
	//Let Repository be updated meanwhile
	//TODO wait for the corresponding PLC-message in ControlManager instead of time
	usleep(1000000);
	
	
	stringstream sstream;
	sstream << " Status=" << item->Properties[ItemProperties::Status] << "\r\n Position=" << item->Properties[ItemProperties::Position] << "\r\n Value=" << item->Properties[ItemProperties::Value];
	SendMail(argMail.FromAddr,"Re:" + argMail.Subject + "(GET Result)", "Item " + item->Name + "\r\n" + sstream.str());
      }
      else if(actionType == "SET")
      {
	if(cmdTokens.size() < 3)
	{
	  HandleAnalyzeError(argMail,"Received command not valid for a SET Action, no Value parameter provided " + strCommand);	
	  return;  
	}
	
	string strValue = cmdTokens[2];	    
	
	ScadaItemMessage msg;
	msg.MsgType = ItemMessageTypes::Command;
	msg.ItemType = item->ItemType;
	msg.ItemIndex = item->Index;
	msg.Property = ItemProperties::Func;
	msg.Value = SharedUtils::ConvertToItemValue(strValue,item->ItemType,msg.Property);
    
	_EventSubscriber->RASMessageReceived(msg);
	SendMail(argMail.FromAddr,"Re:" + argMail.Subject + "(SET Result)","Command " + strCommand +" processed successfully");
      }
      else
	HandleAnalyzeError(argMail,"Received command not valid for a SET Action, no Value parameter provided " + strCommand);	
	
    }
    else
      HandleAnalyzeError(argMail,"Received command for unknown Item " + strCommand);	
  }  
   
  if(commandsFound == 0)
    HandleAnalyzeError(argMail,"Message does no contain line with ##<instruction>## pattern !");
  
}

void RASManager::SendHelp(Email argMail)
{
  string helpText = "Command synopsis:\r\n";
  helpText.append("##SET|itemname|value##\r\n");
  helpText.append("##GET|itemname##\r\n");
  helpText.append("##HELP##\r\n");
  helpText.append("\r\n");
  helpText.append("Available Items:\r\n");
  
  vector<ScadaItem*> itemList = _ItemRepo->GetItems();
  for(vector<ScadaItem*>::iterator iter=itemList.begin();iter != itemList.end();++iter)
  {
    
    helpText.append((*iter)->Name + "\r\n");      
  }    
  
  SendMail(argMail.FromAddr,"Re:" + argMail.Subject + " (HELP)",helpText);  
  
}


void RASManager::HandleAnalyzeError(Email argMail, string argText)
{
  _Logger->Trace(argText);
  SendMail(argMail.FromAddr,"Re:" + argMail.Subject + " (ERR)",argText);  
  
}



void RASManager::SendMail(string argReceiverEmail,string argSubject,string argBodyText)
{
  SMTPClient client(_Logger);
  client.SendMail(argReceiverEmail,argSubject,argBodyText); 
  
}

void RASManager::Stop()
{
  
  
}

