#pragma once
#include "stdafx.h"
#include "connection.h"


class Server
{
public:

	Server(ba::io_service& ioService, int port);

private:
	void StartAccept();
    void HandleAccept(Connection::Pointer newConnection, const boost::system::error_code& error);

    //SocksVersion ReadSocksVersion(socket_ptr& sock, char* buffer);
	//bool HandleHandshake(socket_ptr& socket, char* buffer);
	//bool HandleRequest(socket_ptr& socket, char* buffer); 

    //bool HandleSocks4Request(socket_ptr& socket, char* buffer);
	//void DoProxy(socket_ptr& client, socket_ptr& conn, char* buffer);

	//void HandleProxyRead(const boost::system::error_code& error,
    //  size_t bytes_transferred, socket_ptr& conn);

private:
	ba::io_service& ioService_;
	int port_;
	ba::ip::tcp::acceptor acceptor_;
};
