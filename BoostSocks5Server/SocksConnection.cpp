#include "stdafx.h"
#include "SocksConnection.h"
#include "socks5.h"

#define SERVER_PORT 555

// handle handshake with socks5 client
bool SocksConnection::HandleHandshake()
{
	const int METHOD_NOT_AVAILABLE = 0xFF;
	const int METHOD_NO_AUTH = 0x0;
	const int METHOD_AUTH = 0x2;

	MethodIdentification metIdentif;
	ba::read(*csocket_, metIdentif.buffers());

	if (!metIdentif.Success())
		return false;

	int readSize = 0;
	if (readSize = ba::read(*csocket_, ba::buffer(buffer_, metIdentif.Nmethods())) != metIdentif.Nmethods())
		return false;

	MethodSelectionPacket response(METHOD_NOT_AVAILABLE);
	for (unsigned int i = 0; i < metIdentif.Nmethods(); i ++)
	{
		if (buffer_[i] == METHOD_NO_AUTH)
			response.method = METHOD_NO_AUTH;

		if (buffer_[i] == METHOD_AUTH)
			response.method = METHOD_AUTH;
	}

	int writeSize = 0;
	writeSize = ba::write(*csocket_, response.buffers()); //add checks !!

	return true;
}

bool SocksConnection::HandleRequest()
{
	SOCKS5RequestHeader header;
	ba::read(*csocket_, header.buffers());

	if (!header.Success())
		return false;

	socket_ptr remoteSock(new ba::ip::tcp::socket(ioService_));
	switch(header.Atyp())
	{
		case SOCKS5RequestHeader::addressingType::atyp_ipv4:
			{
				SOCK5IP4RequestBody req;

				ba::read(*csocket_, req.buffers()); // if length is not OK does booost ASIO throw ?
				//socket_ptr clientSock(new ba::ip::tcp::socket(ioService_));

				ba::ip::tcp::endpoint endpoint( ba::ip::address(ba::ip::address_v4(ntohl(req.IpDst()))), ntohs(req.Port()));
				remoteSock->connect(endpoint); //connect throws if connection failed
				break;
			}

		case SOCKS5RequestHeader::addressingType::atyp_dname:
			break;

		default:
			return false;
	}

	// connection to endpoint was succesfull
	SOCK5Response response;

	response.ipSrc = 0;
	response.portSrc = SERVER_PORT; // TODO: clear things out here

	ba::write(*csocket_, response.buffers());

    //TODO: remove socket_ptre from here
	ProxySocksSession* proxySession = new ProxySocksSession(ioService_, csocket_, remoteSock);

	proxySession->Start();

	//TODO clientSock should be automatically be closed before returning. The client socks outlives this function.

	return true;
}

#if 0
//handle request commig from a socks 4 client
bool Server::HandleSocks4Request(socket_ptr& socket, char* buffer)
{
    SOCKS4RequestHeader header;
    ba::read(*socket, header.buffers());

    if (!header.Success())
        return false;

    socket->read_some(ba::buffer(buffer, 256));

    //ba::streambuf sBuf;
    //int len = ba::read_until(*socket, sBuf, 0);

    //std::string name;
    //std::istream is(&sBuf);
    //is >> name;

    std::cout << "Socks User: " << buffer << std::endl;

    Socks4ResponseConnect response;
    response.response = 92;
    int writeSize = 0;

    writeSize = ba::write(*socket, response.buffers());

    //close imediately socket
    socket->close();

    // read some undefined that (if clients sends any) now
    //int read = socket->read_some(ba::buffer(buffer
}

#endif

ProxySocksSession::ProxySocksSession(ba::io_service& ioService, socket_ptr socket, socket_ptr  remoteSock): 
    ioService_(ioService), socket_(socket), remoteSock_(remoteSock)
{

}

void ProxySocksSession::Start()
{
	
	socket_->async_read_some(boost::asio::buffer(dataClient_, 1024),
        boost::bind(&ProxySocksSession::HandleClientProxyRead, this,
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred));

	remoteSock_->async_read_some(boost::asio::buffer(dataRemote_, 1024),
        boost::bind(&ProxySocksSession::HandleRemoteProxyRead, this,
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred));
}

// received data from socks5 client - completion handler
void ProxySocksSession::HandleClientProxyRead(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	if (!error)
    {
		//std::cout << "Received from client: \r\n" << std::string(dataClient_, bytes_transferred) << "\r\n\r\n";
        
		// forward data read from client to remote endpoint
		if (bytes_transferred > 0)
		  boost::asio::async_write(*remoteSock_,
			  boost::asio::buffer(dataClient_, bytes_transferred),
			  boost::bind(&ProxySocksSession::HandleRemoteProxyWrite, this,
				boost::asio::placeholders::error));

		// read some more data from socks client
		socket_->async_read_some(boost::asio::buffer(dataClient_, 1024),
			boost::bind(&ProxySocksSession::HandleClientProxyRead, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
    }
	else
	{
		socket_->close();
	}
}

//received data from socks5 remote endpoint (socks5 client destination) - completion handler
void ProxySocksSession::HandleRemoteProxyRead(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	if (!error)
    {
		//std::cout << "Received from remote end: \r\n" << std::string(dataRemote_, bytes_transferred) << "\r\n\r\n";
		if (bytes_transferred > 0)
		boost::asio::async_write(*socket_,
          boost::asio::buffer(dataRemote_, bytes_transferred),
          boost::bind(&ProxySocksSession::HandleClientProxyWrite, this,
            boost::asio::placeholders::error));

		//read some more data from remote endpoint
		remoteSock_->async_read_some(boost::asio::buffer(dataRemote_, 1024),
			boost::bind(&ProxySocksSession::HandleRemoteProxyRead, this,
			 boost::asio::placeholders::error,
			 boost::asio::placeholders::bytes_transferred));
    }
	else
	{
		remoteSock_->close();
	}
}

// completion event for remote endpoint write
void ProxySocksSession::HandleRemoteProxyWrite(const boost::system::error_code& error)
{
	if (error)
	{
		std::cerr << "Write to remote socket failed: " << error.message() << std::endl;
		remoteSock_->close();
	}
}

// completion event for client write
void ProxySocksSession::HandleClientProxyWrite(const boost::system::error_code& error)
{
	if (error)
	{
		std::cerr << "Write to client socket failed: " << error.message() << std::endl;
		socket_->close();
	}
}
