#include "stdafx.h"
#pragma once

//FWD
#include "SocksConnection.h"

class Connection : public boost::enable_shared_from_this<Connection> {
public:
	//typedef boost::shared_ptr<Connection> Pointer;
    typedef Connection* Pointer;

    ~Connection()
    {
        std::cout << "Connection closed " << std::endl;  
    }
	static Pointer Create(ba::io_service& io_service) {
		return /*Pointer(*/new Connection(io_service);
	}

    void Shutdown()
    {
        delete this;
    }

	ba::ip::tcp::socket& Socket() {
		return bsocket_;
	}

	void Start();

private:
	Connection(ba::io_service& io_service);

private:
    ba::io_service& ioService_;
    //socket_ptr bsocket_;
    ba::ip::tcp::socket bsocket_;
    std::unique_ptr<SocksConnection> socksConnection_;
};