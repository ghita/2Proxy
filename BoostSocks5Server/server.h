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

private:
	ba::io_service& ioService_;
	int port_;
	ba::ip::tcp::acceptor acceptor_;
};
