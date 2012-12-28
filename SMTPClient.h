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


#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H
#include <string>
#include "LogTracer.h"

using namespace std;

class SMTPClient
{
  private:
    LogTracer *_Logger;
    int _SocketHandle;
    void SendSocket(string argText);
    void ReadSocket();
    
  
  public:
    void SendMail(string argReceiverEmail,string argSubject,string argText);
    SMTPClient(LogTracer * argLogger);
    virtual ~SMTPClient();
};

#endif // SMTPCLIENT_H
