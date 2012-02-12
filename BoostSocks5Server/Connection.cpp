
#include "stdafx.h"
#include "connection.h"
#include "SocksConnection.h"
#include "HttpConnection.h"

typedef enum _SocksVersion
{
    version4,
    version5,
    versionUnknown
} SocksVersion;

SocksVersion ReadSocksVersion(ba::ip::tcp::socket& socket, boost::array<char, 1024>& buffer)
{
    SocksVersion version = versionUnknown;
    //socket->read_some(ba::buffer(buffer, 1));
    ba::read(socket, ba::buffer(buffer, 1));

    if (buffer[0] == 4)
        version = version4;
    else if (buffer[0] == 5)
        version = version5;
	else
		version = (SocksVersion) buffer[0];

    return version;
}

Connection::Connection(ba::io_service& ioService): ioService_(ioService), bsocket_(ioService), httpConnection_() {}

// We handle here both http and socks proxy requests.
// If case of socks protocol the first byte sent by the client represents the
// socks version. In case the first byte represents one of these we know to have a socks request and in contrary
// if no version is specified we consider the client to be wanting to use http.
// Of course in case the request would not be http conformant it will be discareded afterwards
void Connection::Start()
{
    boost::array<char, 1024> buffer;

    try 
    {
		SocksVersion socksVer = versionUnknown;
        //SocksVersion socksVer = ReadSocksVersion(bsocket_, buffer);

        //SocksConnection* socksConnection = nullptr;
        if (socksVer == version5)
        {
            socksConnection_.reset(new SocksConnection(bsocket_, ioService_, buffer, *this));
            if (socksConnection_->HandleHandshake())
            {
                if (!socksConnection_->HandleRequest())
                    Shutdown();
            }
            else
            {
                Shutdown();
            }
        }
        else if (socksVer == version4)
        {
            //HandleSocks4Request(sock, buffer);
            return;
        }
        else // asume we have http
        {
		    char firstByte = 0;
			httpConnection_ = HttpConnection::Create(ioService_, bsocket_, firstByte);
			httpConnection_->Start();
        }
    }
    catch (std::exception& exp)
    {
        //if (socksConnection != nullptr)
        //    delete 
        std::cout << "Exception occured while handling new connection, shutting down connection: " << exp.what() << std::endl;
        Shutdown();
    }
}

