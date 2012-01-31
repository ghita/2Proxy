#include "stdafx.h"
#pragma once

class Connection : public boost::enable_shared_from_this<Connection> {
public:
	typedef boost::shared_ptr<Connection> Pointer;

	static Pointer Create(ba::io_service& io_service) {
		return Pointer(new Connection(io_service));
	}

	ba::ip::tcp::socket& Socket() {
		return *bsocket_;
	}

	void Start();

private:
	Connection(ba::io_service& io_service);

private:
    ba::io_service& ioService_;
    socket_ptr bsocket_;
};