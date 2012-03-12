#pragma once

#include "stdafx.h"
#include <map>
#include "message.h"
#include "RequestParser.h"
#include "ResponseParser.h"
#include "connection.h"

//FWD
class HttpConnectionLifeManagement;

class HttpConnection : public boost::enable_shared_from_this<HttpConnection> {
public:
	typedef boost::shared_ptr<HttpConnection> Pointer;

	static Pointer Create(ba::io_service& io_service, ba::ip::tcp::socket& bsocket, char firstByte, Connection& parrentConn) {
		return Pointer(new HttpConnection(io_service,  bsocket, firstByte, parrentConn));
	}

	HttpConnection(ba::io_service& io_service, ba::ip::tcp::socket& bsocket, char firstByte, Connection& parrentConn);
	void Start();

	ba::ip::tcp::socket& Socket() {
		return bsocket_;
	}

public:

	//TODO: remove me
	void HandleServerReadHeaders(const bs::error_code& err, size_t len);
	void HandleServerReadBody(const bs::error_code& err, size_t len);
	void HandleBrowserWrite(const bs::error_code& err, size_t len);

	void Shutdown();
	bool IsStopped();

private:
	enum ClientRequestState
	{
		Method,
		Uri,
		Version,
		Headers
	};

	enum ServerResponseState
	{
		ReadUntilBody,
		Body
	};

private:
	void HandleBrowserReadHeaders(const bs::error_code& err, size_t len);
	void ReadMoreFromBrowser(ClientRequestState state);
	void StartConnect();
	void HandleResolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleConnect(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator);
	void StartWriteToServer();
	void HandleServerWrite(const bs::error_code& err, size_t len);
	// read from server related methods
	void HandleServerRead(const bs::error_code& err, size_t len);
	void SendDataToClient(int len);
	void HandleClientWrite(const bs::error_code& err, size_t len);
	void ReadMoreFromServer(ServerResponseState state);

private:
	ba::io_service& io_service_;
	ba::ip::tcp::socket& bsocket_;
	ba::ip::tcp::socket ssocket_;
	char firstByte_;
	Connection& parrentConn_;
	ba::ip::tcp::resolver resolver_;
	bool proxy_closed;
	bool isPersistent;
	int32_t RespLen;
	int32_t RespReaded;

	std::string fNewURL;
	std::string fMethod;
	std::string fReqVersion;
	std::string fServer;
	std::string fPort;
	bool isOpened;
    bool isFirstByteAdded;

	std::string fReq;

	typedef boost::array<char, 1024uL> BufferType;
	typedef std::vector<char> DataType;
	
	//TODO: keep only one version of headers
	typedef std::map<std::string,std::string> HeadersMap;


	HeadersMap reqHeaders;
	DataType reqHeaders_;

	BufferType bbuffer;
	BufferType sbuffer;
	DataType partialParsed_;
	DataType partialParsedServer_;
	BufferType::iterator newStart_;//, dataEnd_;
	BufferType::iterator newServerReadStart_;//, serverDataEnd_;

private:

	ClientRequestState clientRequestState_;
	ServerResponseState serverResponseState_;

	network::RequestParser parser_;
	network::ResponseParser responseParser_;
	network::BasicMessage message_;
	bool isShuttingDown_;
	std::shared_ptr<HttpConnectionLifeManagement> lifeMng_;

};
/*
class HttpConnectionLifeManagement
{
public:
	HttpConnectionLifeManagement(Connection& parrentConn): parrentConn_(parrentConn), isShuttingDown_(false) {}
	void Shutdown()
	{
		if (!isShuttingDown_)
		{
			isShuttingDown_ = true;
			parrentConn_.Shutdown();
		}
	}

private:
	HttpConnectionLifeManagement& operator = (const HttpConnectionLifeManagement& );
private:
	Connection& parrentConn_;
	bool isShuttingDown_;
};*/


