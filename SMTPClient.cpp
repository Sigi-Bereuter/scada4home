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


#include "SMTPClient.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>



SMTPClient::SMTPClient()
{

}

SMTPClient::~SMTPClient()
{

}

void SMTPClient::SendSocket(string argText)
{
	write(_SocketHandle,argText.c_str(),argText.length());
	write(1,argText.c_str(),argText.length());
}

/*=====Read a string from the socket=====*/

void SMTPClient::ReadSocket()
{	
  char rcvBuffer[BUFSIZ+1];
  int len = read(_SocketHandle,rcvBuffer,BUFSIZ);	
  rcvBuffer[len] = 0;
  printf("RCV: %s\n",rcvBuffer);
}

void SMTPClient::SendMail(string argReceiverEmail,string argSubject,string argText)
{  
  struct sockaddr_in server;
  
  string host_id = "mailbox.bregenznet.at";
  string from_id = "home@bereuter.com"; 
  string username = "c2llZ2ZyaWVkLmJlcmV1dGVyQGJyZWdlbnoubmV0"; //"siegfried.bereuter@bregenz.net" base64 codiert
  string password = "azhJd2tvSXc=";				//"k8IwkoIw"  base64 codiert
  char wkstr[100];  
    
  /*=====Create Socket=====*/
  _SocketHandle = socket(AF_INET, SOCK_STREAM, 0);
  if (_SocketHandle==-1)
  {
    perror("opening stream socket");
    return;
  }

  /*=====Verify host=====*/
  server.sin_family = AF_INET;
  hostent *hp = gethostbyname(host_id.c_str());
  if (hp==(struct hostent *) 0)
  {
    fprintf(stderr, "%s: unknown host\n", host_id.c_str());
    return;;
  }

  /*=====Connect to port 25 on remote host=====*/
  printf ("hostent %s\n", hp->h_addr_list[0]);
  memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);

  server.sin_port=htons(25); /* SMTP PORT */

  if (connect(_SocketHandle, (struct sockaddr *) &server, sizeof server)==-1)
  {
    perror("connecting stream socket");
    return;
  }

  /*=====Write some data then read some =====*/
  

  ReadSocket(); /* SMTP Server logon string */

  SendSocket("EHLO scada4home\n"); /* introduce ourselves */
  ReadSocket(); /*Read reply */
  
  SendSocket("AUTH LOGIN\n"); /* start login sequence */
  ReadSocket(); /*Read reply */
  
  SendSocket(username); /* send username */
  SendSocket("\n");
  ReadSocket(); /*Read reply */
  
  SendSocket(password); /* ssend password */
  SendSocket("\n");
  ReadSocket(); /*Read reply */

  SendSocket("MAIL from: "); /* Mail from us */
  SendSocket(from_id);
  SendSocket("\n");
  ReadSocket(); /* Sender OK */

  SendSocket("RCPT To: "); /*Mail to*/
  SendSocket(argReceiverEmail);
  SendSocket("\n");
  ReadSocket(); /*Recipient OK*/

  SendSocket("DATA\n");/*body to follow*/
  ReadSocket(); /*ok to send */
  
  SendSocket("From: " + from_id); /*send file*/
  SendSocket("\n");
  SendSocket("To: " + argReceiverEmail); /*send file*/
  SendSocket("\n");
  SendSocket("Subject: " + argSubject); /*send file*/
  SendSocket("\n");
  SendSocket("\n");
  
  SendSocket(argText.c_str()); /*send file*/
  SendSocket("\r\n.\r\n");

  ReadSocket(); /* OK*/
  SendSocket("QUIT\n"); /* quit */  
  ReadSocket(); /* log off */

  /*=====Close socket and finish=====*/
  close(_SocketHandle);

}


