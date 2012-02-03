#include "stdafx.h"
#include "SocksBindingServer.h"
 
 
SocksBindingServer::SocksBindingServer(ba::io_service& ioService, unsigned int cAddress, unsigned short cPort, int bindingPort): 
ioService_(ioService), cAddress_(cAddress), cPort_(cPort), bindingPort_(bindingPort), 
    acceptor_(ioService_, ba::ip::tcp::endpoint(ba::ip::tcp::v4(), bindingPort))
{
}
 
void SocksBindingServer::StartAccept()
{
	ba::ip::tcp::socket* newConnection = new ba::ip::tcp::socket(ioService_);
    acceptor_.async_accept(*newConnection,
        boost::bind(&SocksBindingServer::HandleAccept, this, newConnection,
          boost::asio::placeholders::error));
}
 
void SocksBindingServer::HandleAccept(ba::ip::tcp::socket* csocket, const boost::system::error_code& error)
{
    std::cout << "\tSocksBindingServer::HandleAccept entry:" << std::endl;
    if (!error)
    {
        //process data comming to this "data channel"
		char data[1024] = "";
		int len = csocket->read_some(ba::buffer(data, 1024));
        std::cout << "\tSocksBindingServer::HandleAccept data:" << data << std::endl;

        socket_ptr clBindSock(new ba::ip::tcp::socket(ioService_));
        ba::ip::tcp::endpoint endpoint(ba::ip::address(ba::ip::address_v4(cAddress_)), cPort_);
		clBindSock->connect(endpoint); //connect throws if connection failed

        int written = ba::write(*clBindSock, ba::buffer(data, len));
    }
	else
	{
		delete csocket;
		std::cout << error.message() << std::endl;
	}
    //TODO: this has to be here even if error was reported at previous connect ?
}