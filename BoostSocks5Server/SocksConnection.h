#include "stdafx.h"
#pragma once

//FWD
class Connection;
class ProxySocksSession;

class SocksConnection: public boost::enable_shared_from_this<SocksConnection>
{
public:
    SocksConnection(ba::ip::tcp::socket& csocket, ba::io_service& service, boost::array<char, 1024>&  buffer, Connection& parrentConn):
      csocket_(csocket), ioService_(service), resolver_(service), buffer_(buffer), parentConn_(parrentConn) {};
    bool SocksConnection::HandleHandshake();
    bool SocksConnection::HandleRequest();
    void Shutdown();
    ~SocksConnection()
    {
        std::cout << "ending";
    };
    //bool SocksConnection::HandleSocks4Request(socket_ptr& socket, char* buffer);

private:
    void SocksConnection::HandleResolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator);

private:
    //socket_ptr csocket_;
    ba::ip::tcp::socket& csocket_;
    ba::io_service& ioService_;
    ba::ip::tcp::resolver resolver_;
    boost::array<char, 1024> buffer_;
    Connection& parentConn_;

    std::unique_ptr<ba::ip::tcp::socket> remoteSocket_;
    boost::shared_ptr<ProxySocksSession> proxySocksSession_;
    //ba::ip::tcp::socket& remoteSocket_;
};

class ProxySocksSession: public boost::enable_shared_from_this<ProxySocksSession>
{
public:
	ProxySocksSession(ba::io_service& ioService, ba::ip::tcp::socket& socket, ba::ip::tcp::socket& remoteSock, SocksConnection& parentConnection);
    ~ProxySocksSession()
    {
        std::cout << "ending";
    };
	void Start();

private:
	void HandleClientProxyRead(const boost::system::error_code& error,
      size_t bytes_transferred);
	void HandleRemoteProxyRead(const boost::system::error_code& error,
		size_t bytes_transferred);

	void HandleRemoteProxyWrite(const boost::system::error_code& error);
	void HandleClientProxyWrite(const boost::system::error_code& error);

	void Shutdown();

private:
	//socket_ptr socket_;
    ba::ip::tcp::socket& socket_;
	ba::io_service& ioService_;
    ba::ip::tcp::socket& remoteSock_;
    SocksConnection& parentConnection_;
    //ba::ip::tcp::socket& remoteSock_;

	bool shutdownInProgress_;

  enum { max_length = 10000 };
  char dataClient_[max_length];
  char dataClientCopy_[max_length];
  char dataRemote_[max_length];
  char dataRemoteCopy_[max_length];
};
