// BoostSocks5Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "server.h"
#include "log.h"

int main(int argc, _TCHAR* argv[])
{
    //FILELog::ReportingLevel() = logDEBUG;
    //FILE_LOG(logINFO) << "Started logging";
    boost::asio::io_service io_service;

    Server server(io_service, 1081);

	io_service.run();
}

