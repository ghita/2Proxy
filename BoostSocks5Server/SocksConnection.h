#include "stdafx.h"
#pragma once

class SocksConnection
{
public:
    SocksConnection(socket_ptr csocket, ba::io_service& service, boost::array<char, 1024>&  buffer):
      csocket_(csocket), ioService_(service), buffer_(buffer) {};
    bool SocksConnection::HandleHandshake();
    bool SocksConnection::HandleRequest();
    //bool SocksConnection::HandleSocks4Request(socket_ptr& socket, char* buffer);

private:
    socket_ptr csocket_;
    ba::io_service& ioService_;
    boost::array<char, 1024> buffer_;
};

class ProxySocksSession
{
public:
	ProxySocksSession(ba::io_service& ioService, socket_ptr socket, socket_ptr remoteSock);
	void Start();

private:
	void HandleClientProxyRead(const boost::system::error_code& error,
      size_t bytes_transferred);
	void HandleRemoteProxyRead(const boost::system::error_code& error,
		size_t bytes_transferred);

	void HandleRemoteProxyWrite(const boost::system::error_code& error);
	void HandleClientProxyWrite(const boost::system::error_code& error);

private:
	socket_ptr socket_;
	socket_ptr remoteSock_;
	ba::io_service& ioService_;
  enum { max_length = 10000 };
  char dataClient_[max_length];
  char dataRemote_[max_length];
};
