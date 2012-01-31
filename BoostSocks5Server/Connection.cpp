
#include "stdafx.h"
#include "connection.h"
#include "SocksConnection.h"

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

    return version;
}

Connection::Connection(ba::io_service& ioService): ioService_(ioService), bsocket_(new ba::ip::tcp::socket(ioService)) {}

// We handle here both http and socks proxy requests.
// If case of socks protocol the first byte sent by the client represents the
// socks version. In case the first byte represents one of these we know to have a socks request and in contrary
// if no version is specified we consider the client to be wanting to use http.
// Of course in case the request would not be http conformant it will be discareded afterwards
void Connection::Start()
{
    boost::array<char, 1024> buffer;

    SocksVersion socksVer = ReadSocksVersion(*bsocket_, buffer);  

    if (socksVer == version5)
    {
        SocksConnection socksConnection(bsocket_, ioService_, buffer);
        if (socksConnection.HandleHandshake())
            socksConnection.HandleRequest();
    }
    else if (socksVer == version4)
    {
        //HandleSocks4Request(sock, buffer);
        return;
    }
    else // asume we have http
    {
        //
    }
}

