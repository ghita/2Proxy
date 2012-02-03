#include "stdafx.h"
#include "SocksConnection.h"
#include "socks5.h"

#include "SocksBindingServer.h"
#include "connection.h"
#include "stringUtil.h"


#define SERVER_PORT 1081 // have to match the socks5 server port

// handle handshake with socks5 client
bool SocksConnection::HandleHandshake()
{
    std::cout << "\tHandle handshake started" << std::endl;
	const int METHOD_NOT_AVAILABLE = 0xFF;
	const int METHOD_NO_AUTH = 0x0;
	const int METHOD_AUTH = 0x2;

	MethodIdentification metIdentif;
	ba::read(csocket_, metIdentif.buffers());

	if (!metIdentif.Success())
		return false;

	int readSize = 0;
	if (readSize = ba::read(csocket_, ba::buffer(buffer_, metIdentif.Nmethods())) != metIdentif.Nmethods())
		return false;

	MethodSelectionPacket response(METHOD_NOT_AVAILABLE);
	for (unsigned int i = 0; i < metIdentif.Nmethods(); i ++)
	{
		if (buffer_[i] == METHOD_NO_AUTH)
			response.method = METHOD_NO_AUTH;

        //TODO: not support auth for now !!
		//if (buffer_[i] == METHOD_AUTH)
		//	response.method = METHOD_AUTH;
	}

	int writeSize = 0;
	writeSize = ba::write(csocket_, response.buffers()); //add checks !!

    std::cout << "\tHandle handshake ended" << std::endl;
	return true;
}

bool SocksConnection::HandleRequest()
{
	bool isBinding = false;
	SOCKS5RequestHeader header;
	ba::read(csocket_, header.buffers());
 
	if (!header.Success())
		return false;
 
    remoteSocket_.reset(new ba::ip::tcp::socket(ioService_));
	switch(header.Atyp())
	{
		case SOCKS5RequestHeader::addressingType::atyp_ipv4:
			{
					SOCK5IP4RequestBody req;
					ba::read(csocket_, req.buffers()); // if length is not OK does booost ASIO throw ?
 
				if (header.cmd_ == SOCKS5RequestHeader::connect)
				{
					ba::ip::tcp::endpoint endpoint( ba::ip::address(ba::ip::address_v4(ntohl(req.IpDst()))), ntohs(req.Port()));
					remoteSocket_->connect(endpoint); //connect throws if connection failed
				}
#if 0 // TODO: BIND funtionality does not work as expected. In any case this functionality requires a firewall port to be opened
      // in order for the remote endpoint to connect to the port opened by the socks server. Most protocols (like FTP) might use
      // PASIVE MODE, case in which BIND is not necessary because data is transfered using same channel as CONNECT. In case of FTP,
      // ftp server can send data using control channel instead of needing an aditional channel for data
				else if(header.cmd_ == SOCKS5RequestHeader::bind)
				{
					//TODO: when this should be removed
					SocksBindingServer* bindSrv = new SocksBindingServer(ioService_, ntohl(req.IpDst()), ntohs(req.Port()), /*ntohs(req.Port())*/ 14596);
                    //bindSrv->StartAccept();
					//isBinding = true;
				}
#endif
				break;
			}
 
		case SOCKS5RequestHeader::addressingType::atyp_dname:
            {
                // the format for request body is following: xFQDNAddress|Port
                // where x represents nb. of bytes from FQDNAddress
                ba::read(csocket_, ba::buffer(buffer_, 1));
                int addrLen = ba::read(csocket_, ba::buffer(buffer_, buffer_[0]));

                std::string fqdnAddress;
                for ( int i = 0; i < addrLen; i ++)
                    fqdnAddress += buffer_[i];

                unsigned short port = 0;
                ba::read(csocket_, ba::buffer(&port, 2));

                std::string portStr = toString<unsigned short>(ntohs(port));

                std::cout << "Connectig to : " << fqdnAddress << ":" << portStr << "..." << std::endl;
                ba::ip::tcp::resolver::query query(fqdnAddress, portStr);

                ba::ip::tcp::resolver::iterator iter = resolver_.resolve(query);

                remoteSocket_.reset(new ba::ip::tcp::socket(ioService_));
                remoteSocket_->connect(*iter); //connect throws if connection failed. TODO: try different endpoint in case of error

                // connection to endpoint was succesfull
		        SOCK5Response response;
 
		        response.ipSrc = 0;
		        response.portSrc = SERVER_PORT; // TODO: clear things out here
 
		        ba::write(csocket_, response.buffers());
 
		        //TODO: remove socket_ptre from here, when ProxySocksSession should be deleted
                proxySocksSession_.reset(new ProxySocksSession(ioService_, csocket_, *remoteSocket_.get(), *this));
 
		        proxySocksSession_->Start();
		        // resolver_.async_resolve(query,
								//boost::bind(&SocksConnection::HandleResolve, shared_from_this(),
								//			boost::asio::placeholders::error,
								//			boost::asio::placeholders::iterator));

                return true;
            }
			break;
 
		default:
			return false;
	}
 
	if (!isBinding)
	{
 
		// connection to endpoint was succesfull
		SOCK5Response response;
 
		response.ipSrc = 0;
		response.portSrc = SERVER_PORT; // TODO: clear things out here
 
		ba::write(csocket_, response.buffers());
 
		//TODO: remove socket_ptre from here, when ProxySocksSession should be deleted
		proxySocksSession_.reset(new ProxySocksSession(ioService_, csocket_, *remoteSocket_.get(), *this));
 
		proxySocksSession_->Start();
	}
	else
	{
		SOCK5Response response;
 
		std::array< unsigned char, 4 > bytesAddr = csocket_.local_endpoint().address().to_v4().to_bytes();
		response.ipSrc = 0x0100007F; /* 127.0.0.1 written in newtwork byte order*/
        response.portSrc = htons(14596);
		ba::write(csocket_, response.buffers());
	}
 
	//TODO clientSock should be automatically be closed before returning. The client socks outlives this function.
 
	return true;
}

void SocksConnection::Shutdown()
{
    parentConn_.Shutdown();
}

void SocksConnection::HandleResolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator) {
    if (!err) {
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		//ssocket_.async_connect(endpoint,
		//					  boost::bind(&HttpConnection::HandleConnect, shared_from_this(),
		//								  boost::asio::placeholders::error,
		//								  ++endpoint_iterator));
        socket_ptr remoteSock(new ba::ip::tcp::socket(ioService_));
        remoteSock->connect(endpoint); //connect throws if connection failed

        // connection to endpoint was succesfull
		SOCK5Response response;
 
		response.ipSrc = 0;
		response.portSrc = SERVER_PORT; // TODO: clear things out here
 
		ba::write(csocket_, response.buffers());
 
		//TODO: remove socket_ptre from here, when ProxySocksSession should be deleted
		proxySocksSession_.reset(new ProxySocksSession(ioService_, csocket_, *remoteSocket_.get(), *this));
 
		proxySocksSession_->Start();
    }else {
        std::cout << "Resolve failed for: ' '" << err.message() << std::endl; 
	}
}

ProxySocksSession::ProxySocksSession(ba::io_service& ioService, ba::ip::tcp::socket&  socket, ba::ip::tcp::socket&  remoteSock, SocksConnection& parrentConn): 
    ioService_(ioService), socket_(socket), remoteSock_(remoteSock), parentConnection_(parrentConn)
{

}

void ProxySocksSession::Start()
{
//std::cout << "\tProxySocksSession::Start" << std::endl;	
	socket_.async_read_some(boost::asio::buffer(dataClient_, 1024),
        boost::bind(&ProxySocksSession::HandleClientProxyRead, this,
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred));

	remoteSock_.async_read_some(boost::asio::buffer(dataRemote_, 1024),
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
        
        //have to make a copy of the received data because dataClient_ is used for reading some more data
        std::copy_n(dataClient_, bytes_transferred, dataClientCopy_);

		// forward data read from client to remote endpoint
		if (bytes_transferred > 0)
		  boost::asio::async_write(remoteSock_,
			  boost::asio::buffer(dataClientCopy_, bytes_transferred),
			  boost::bind(&ProxySocksSession::HandleRemoteProxyWrite, this,
				boost::asio::placeholders::error));

		// read some more data from socks client
		socket_.async_read_some(boost::asio::buffer(dataClient_, 1024),
			boost::bind(&ProxySocksSession::HandleClientProxyRead, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
    }
	else
	{
        //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        //socket_.close();
        parentConnection_.Shutdown();
	}
}

//received data from socks5 remote endpoint (socks5 client destination) - completion handler
void ProxySocksSession::HandleRemoteProxyRead(const boost::system::error_code& error,
      size_t bytes_transferred)
{
	if (!error)
    {
		//std::cout << "Received from remote end: \r\n" << std::string(dataRemote_, bytes_transferred) << "\r\n\r\n";
        //std::cout << "Start relaying data to socks client" << std::endl;

        //have to make a copy of the received data because dataRemote_ is used for reading some more data
        std::copy_n(dataRemote_, bytes_transferred, dataRemoteCopy_);

		if (bytes_transferred > 0)
		boost::asio::async_write(socket_,
          boost::asio::buffer(dataRemote_, bytes_transferred),
          boost::bind(&ProxySocksSession::HandleClientProxyWrite, this,
            boost::asio::placeholders::error));

		//read some more data from remote endpoint
		remoteSock_.async_read_some(boost::asio::buffer(dataRemote_, 1024),
			boost::bind(&ProxySocksSession::HandleRemoteProxyRead, this,
			 boost::asio::placeholders::error,
			 boost::asio::placeholders::bytes_transferred));
    }
	else
	{
        //remoteSock_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        //remoteSock_.close();
        parentConnection_.Shutdown();
	}
}

// completion event for remote endpoint write
void ProxySocksSession::HandleRemoteProxyWrite(const boost::system::error_code& error)
{
	if (error)
	{
		std::cerr << "Write to remote socket failed: " << error.message() << std::endl;
		//remoteSock_->close();
		//socket_->close();
        //delete this;

        //remoteSock_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        remoteSock_.close();
        //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();
        parentConnection_.Shutdown();
        //if (error != boost::asio::error::eof)
	}
}

// completion event for client write
void ProxySocksSession::HandleClientProxyWrite(const boost::system::error_code& error)
{
	if (error)
	{
		std::cerr << "Write to client socket failed: " << error.message() << std::endl;
        //remoteSock_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        remoteSock_.close();
        //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();
        parentConnection_.Shutdown();
	}

    //std::cout << "HandleClientProxyWrite - data was sent to client proxy" << std::endl;
}
