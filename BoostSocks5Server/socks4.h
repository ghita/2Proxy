#pragma once
#include "stdafx.h"


//data structures as defined by socks 4 protocol - see http://csocks.altervista.org/rfc/socks4.protocol.txt
class SOCKS4RequestHeader
{
public:
    enum commandType
    {
        connect = 0x1,
        bind    = 0x2
    };

    SOCKS4RequestHeader(): /*version_(),*/ cmd_(), dstPort_(), dstIp_()  {}

    boost::array<boost::asio::mutable_buffer, 3> buffers()
    {
        boost::array<boost::asio::mutable_buffer, 3> bufs =
        {
            {
                //boost::asio::buffer(&version_),
                boost::asio::buffer(&cmd_, 1),
                boost::asio::buffer(&dstPort_, 2),
                boost::asio::buffer(&dstIp_, 4)
            }
        };

        return bufs;
    }

    bool Success() const
    {
        return /*(version_ == 5) &&*/ (cmd_ == connect);
    }

public:
    //unsigned char version_;
    unsigned char cmd_;
    unsigned short dstPort_;
    unsigned int dstIp_;
};

class Socks4ResponseConnect
{
public:
    Socks4ResponseConnect(): vn(0), response(), dstPort(), dstIp(){}

    boost::array<boost::asio::mutable_buffer, 4> buffers()
    {
        boost::array<boost::asio::mutable_buffer, 4> bufs =
        {
            {
                boost::asio::buffer(&vn, 1),
                boost::asio::buffer(&response, 1),
                boost::asio::buffer(&dstPort, 2),
                boost::asio::buffer(&dstIp, 4)
            }
        };

        return bufs;
    }

public:
    unsigned char vn;
    unsigned char response;
    unsigned short dstPort;
    unsigned int dstIp;
};