#include "stdafx.h"
#include "server.h"

#include "socks5.h"
#include "socks4.h"

#include "Connection.h"


Server::Server(ba::io_service& ioService, int port): ioService_(ioService), port_(port), acceptor_(ioService_, ba::ip::tcp::endpoint(ba::ip::tcp::v4(), port_))
{
	StartAccept();
}

void Server::StartAccept()
{
    Connection::Pointer newConnection = Connection::Create(ioService_);
    acceptor_.async_accept(newConnection->Socket(),
        boost::bind(&Server::HandleAccept, this, newConnection,
          boost::asio::placeholders::error));
}

void Server::HandleAccept(Connection::Pointer newConnection, const boost::system::error_code& error)
{
    if (!error)
    {
        newConnection->Start();
    }
    else
    {
        delete newConnection;
    }
    //TODO: this has to be here even if error was reported at previous connect ?
	StartAccept();
}