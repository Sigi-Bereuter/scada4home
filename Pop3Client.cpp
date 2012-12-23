#include "Pop3Client.h"

#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include "SharedUtils.h"

/**
 * Constructor class connect to a socket and receives a greeting line from POP3 server
 */
Pop3Client::Pop3Client(const std::string& ahostname,LogTracer* argLogger, const unsigned short aport) 
{
    _Logger = argLogger;
    try
    {
	shortMessage = false;
	memset(buffer, 0, sizeof(buffer));

	// get host IP address
	if ((hostinfo = gethostbyname(ahostname.c_str())) == NULL) {
		std::cerr << "Can't get hostname" << std::endl;
		throw "Can't get host name";
	}

	// create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		std::cerr << "Can't open socket, error = " << strerror(errno) << std::endl;
		throw "Can't open socket";
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;
	address.sin_port = htons(aport);
	int len = sizeof(address);

	// connect
	if ((connect(sockfd, (struct sockaddr *)&address, len)) != 0) {
		std::cerr << "Can't connect to server, error = " << strerror(errno) << std::endl;
		throw "Can't connect to server";
	}

	// receive greeting line
	// we just receive it, there is no need to print it
	// one recv() is enough because according to RFC message can't be longer than BUFLEN (=512) octets
	int result = recv(sockfd, buffer, BUFLEN, 0);
	if (result != -1) {
		buffer[result] = '\0';
	}
	else {
		std::cerr << "An error occured during receiving greeting message from POP3 server" << std::endl;
		throw "Error during receiving greeting line";
	}
  }
  catch (...) 
  {
	std::cerr << "Can't create client";
	throw;
  }
}

Pop3Client::~Pop3Client() {
	close(sockfd);
}

/**
 * Login using a given username
 * Password is read from command line and it is not printed while typing
 */
void Pop3Client::login(const std::string& user,const std::string& passwd) {
	

	std::string response;
	// USER
	try {
		response = sendReceive("USER " + user + "\n");
	}
	catch (const char * e) {
		std::string tmp(e);
		if (tmp == "Error response") {
			// USER command can receive an -ERR message even if the user exists! (security reasons).
			// If we receive -ERR for USER command, we ignore it
		}
		else throw e;
	}
	catch (...) {
		std::cerr << "An error occured during login" << std::endl;
		throw "Login error";
	}

	// PASS
	try {
		response = sendReceive("PASS " + passwd + "\n");
	}
	catch (const char * e) {
		std::string tmp(e);
		if (tmp == "Error response") {
			std::cerr << "Login is unsuccessful" << std::endl;
			throw e;
		}
	}
	catch (...) {
		std::cerr << "An error occured during login" << std::endl;
		throw "Login error";
	}
}

/**
 * This method sends a message and receive the corresponding response message (not a data part!)
 * It analyzes whether the message is +OK or -ERR and throws exception in case of error
 */
std::string Pop3Client::sendReceive(const std::string& message) {
	// send
	int result = send(sockfd, message.c_str(), message.length(), 0);
	if (result == -1) {
		throw "Message can't be sent";
	}
	
	// receive
	result = recv(sockfd, buffer, BUFLEN, 0);
	buffer[result] = '\0';
	if (result == -1) {
		throw "Message can't be received";
		close(sockfd);
	}

	// we've got the message, now check it
	std::string response = (std::string)buffer;
	if (! analyzeMessage(response)) {
		throw "Error response";
	}

	return response;
}

/**
 * Analyze a message whether it's +OK or -ERR
 * Returns true in case of +OK, otherwise false
 */
bool Pop3Client::analyzeMessage(std::string& msg) const {
	// +OK
	if (msg.find("+OK") != std::string::npos) {
		if (msg.substr(0,3) == "+OK") {	//		we need to be sure +OK is at the beginning
			return true;
		}
		else {
			throw("Invalid position of +OK status");
		}
	}
	// -ERR
	else if (msg.find("-ERR") != std::string::npos) {
		if (msg.substr(0,4) == "-ERR") {	//	we need to be sure -ERR is at the beginning
			// print status
			std::cerr << "Error response from server:" << msg.substr(4, msg.length()-4);
			return false;
		}
		else {
			throw("Invalid position of -ERR status");
		}
	} else {
		throw("Incorrect response message");
	}

	return false;	// just to suppress warning messages
}

/**
 * Simply send a message to socket
 */
void Pop3Client::sendMessage(const std::string& message) {
	int result = send(sockfd, message.c_str(), message.length(), 0);
	if (result == -1) {
		std::cerr << "Message can't be sent" << std::endl;
		throw "Message can't be sent";
	}
}

/**
 * Simple receive a message
 * It can read multi-line messages, too
 *
 * This method doesn't check response status (+OK/-ERR) messages! It is supposed to read data part of message only!
 */
void Pop3Client::receiveMessage(std::string& message) {
	int result=0;
	std::string tmp;

	// read data while there are any and no terminating CRLF has been found
	while ((result = recv(sockfd, buffer, BUFLEN, 0)) > 0) {
		buffer[result] = '\0';
		tmp += buffer;

		// check whether message ends up with "CRLF.CRLF". It indicates the end of message
		if (tmp.length() >= 5) {
			if (tmp.substr( tmp.length()-5, 5) == "\r\n.\r\n") {	// we've found the end of multi-line message
				break;
			}
		}
		else if (tmp == ".\r\n")
			break;
	}

	message = tmp;	// final response
}

/**
 * List all messages at POP3 server using the LIST command
 */
void Pop3Client::listMails(vector<int>& argMsgIdList) 
{
	std::string result = "";

	sendReceive("LIST\n");	// status message
	receiveMessage(result);	// data

	if (result.length() == 3) {	// = ".CRLF"
		std::cout << "No new messages" << std::endl;
		return;
	}

	if (shortMessage) {
		// remove last dot + CRLF
		result.erase( result.length() - 3, 3);
	}
	
	vector<string> msgList;
	SharedUtils::Tokenize(result,msgList,"\n");	
	for(vector<string>::iterator iter = msgList.begin(); iter!=msgList.end(); ++iter)
	{
	  string curLine = (*iter);
	  int numEnd = curLine.find_first_of(' ');
	  curLine[numEnd] = 0;
	  int msgId = atoi(curLine.c_str());
	  argMsgIdList.push_back(msgId);	  
	}
	

	std::cout << result;
}

/**
 * Retrieve a given email using its ID number
 */
bool Pop3Client::FetchMail(const unsigned int i,Email& argMail ) {
	// convert integer value to string
	std::stringstream ss;
	ss << i;
	std::string responseData = "";
	std::string responseHeader = "";
	
	try {
		std::string request = "RETR " + ss.str() + "\n";				
		responseHeader = sendReceive(request);	// status message
		receiveMessage(responseData);	// data
	}
	catch (const char * e) {
		std::string tmp(e);
		if (tmp == "Error response") {
			std::cerr << "Can't get message " << i << std::endl;
			throw e;
		}
	}
	catch (...) {
		std::cerr << "An error occured during receiving message " << i << std::endl;
	}
	
	
	try {
		std::string request = "DELE " + ss.str() + "\n";				
		sendReceive(request);	// status message		
	}
	catch (const char * e) {
		std::string tmp(e);
		if (tmp == "Error response") {
			std::cerr << "Can't delete message " << i << std::endl;
			throw e;
		}
	}
	catch (...) {
		std::cerr << "An error occured during deleting message " << i << std::endl;
	}
	
	vector<string> msgLines;	
	std::string allText = responseHeader + responseData;
	SharedUtils::Tokenize(allText,msgLines,"\r\n");	
	bool fromFound = false;
	bool subjFound = false;
	bool toFound = false;
	bool dateFound = false;
	for(vector<string>::iterator iter = msgLines.begin(); iter!=msgLines.end(); ++iter)
	{
	  string curLine = (*iter);
	  string searchStr = "From:";
	  int sPos = curLine.find(searchStr);
	  if(sPos == 0)
	  {
	    std::string from = curLine.substr(searchStr.length(),curLine.length() - searchStr.length());
	    int startPos=from.find_first_of('<')+1;
	    int endPos = from.find_last_of('>')-1;
	    if(startPos > 0 && endPos > startPos)
	    {
	      argMail.FromAddr = from.substr(startPos,endPos-startPos+1);
	      fromFound = true;
	    }
	  }
	  searchStr = "To:";
	  sPos = curLine.find(searchStr);
	  if(sPos == 0)
	  {
	    argMail.ToAddr = curLine.substr(searchStr.length(),curLine.length() - searchStr.length());
	    toFound = true;
	  }	
	  searchStr = "Subject:";
	  sPos = curLine.find(searchStr);
	  if(sPos == 0)
	  {
	    argMail.Subject = curLine.substr(searchStr.length(),curLine.length() - searchStr.length());
	    subjFound = true;
	  }
	  searchStr = "Date:";
	  sPos = curLine.find(searchStr);
	  if(sPos == 0)
	  {
	    argMail.Date = curLine.substr(searchStr.length(),curLine.length() - searchStr.length());
	    dateFound = true;	    
	  }
	  
	  if(fromFound && toFound && subjFound && dateFound)
	    break;
	  
	}
				
	argMail.BodyText = allText;
	
	if(!fromFound)
	  _Logger->Trace("Received Email does not contain From:",allText);
	if(!toFound)
	  _Logger->Trace("Received Email does not contain To:",allText);
	if(!subjFound)
	  _Logger->Trace("Received Email does not contain Subject:",allText);
	if(!dateFound)
	  _Logger->Trace("Received Email does not contain Date:",allText);
	
	
	return (fromFound && toFound && subjFound && dateFound);
}


/**
 * Quit the POP3 session
 */
void Pop3Client::quit() {
	try {
		sendReceive("QUIT\n");
		close(sockfd);
	}
	catch (...) {
		// an error occured but we don't care, we are quitting anyway
	}
}
