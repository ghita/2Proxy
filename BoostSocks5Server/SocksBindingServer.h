#pragma once
#include "stdafx.h"
 
class SocksBindingServer
{
public:
 
	SocksBindingServer(ba::io_service& ioService, unsigned int cAddress, unsigned short cPort, unsigned short bindingPort);
    void StartAccept();
 
private:
    void HandleAccept(ba::ip::tcp::socket* csocket, const boost::system::error_code& error);
 
private:
	ba::io_service& ioService_;
    unsigned int cAddress_;
    unsigned short cPort_;
	int bindingPort_;
	ba::ip::tcp::acceptor acceptor_;
};