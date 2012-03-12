#pragma once
#include "stdafx.h"
#include "connection.h"


class Server
{
public:

	Server(ba::io_service& ioService, unsigned short port);

private:
	void StartAccept();
    void HandleAccept(Connection::Pointer newConnection, const boost::system::error_code& error);

private:
	ba::io_service& ioService_;
	unsigned short port_;
	ba::ip::tcp::acceptor acceptor_;
};
