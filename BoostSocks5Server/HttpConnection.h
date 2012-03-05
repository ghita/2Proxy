#pragma once

#include "stdafx.h"
#include <map>
#include "message.h"
#include "RequestParser.h"

class HttpConnection : public boost::enable_shared_from_this<HttpConnection> {
public:
	typedef boost::shared_ptr<HttpConnection> Pointer;

	static Pointer Create(ba::io_service& io_service, ba::ip::tcp::socket& bsocket, char firstByte) {
		return Pointer(new HttpConnection(io_service,  bsocket, firstByte));
	}

	ba::ip::tcp::socket& Socket() {
		return bsocket_;
	}

	void Start();

//private:
public:
	HttpConnection(ba::io_service& io_service, ba::ip::tcp::socket& bsocket, char firstByte);
	void HandleBrowserWrite(const bs::error_code& err, size_t len);
	void HandleBrowserReadHeaders(const bs::error_code& err, size_t len);

	void HandleServerWrite(const bs::error_code& err, size_t len);
	void HandleServerReadHeaders(const bs::error_code& err, size_t len);
	void HandleServerReadBody(const bs::error_code& err, size_t len);
	void StartConnect();
	void StartWriteToServer();
	void Shutdown();
	void HandleResolve(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleConnect(const boost::system::error_code& err,
									ba::ip::tcp::resolver::iterator endpoint_iterator);
private:
	enum ClientRequestState
	{
		Method,
		Uri,
		Version,
		Headers
	};

	void ReadMoreFromBrowser(ClientRequestState state);

private:
	ba::io_service& io_service_;
	ba::ip::tcp::socket& bsocket_;
	ba::ip::tcp::socket ssocket_;
	char firstByte_;
	ba::ip::tcp::resolver resolver_;
	bool proxy_closed;
	bool isPersistent;
	int32_t RespLen;
	int32_t RespReaded;

	std::string fHeaders;
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


	HeadersMap reqHeaders, respHeaders;
	DataType reqHeaders_;

	
	void ParseHeaders(const std::string& h, HeadersMap& hm);

	BufferType bbuffer;
	BufferType sbuffer;
	DataType partialParsed_;
	BufferType::iterator newStart_, dataEnd_;

private:

	ClientRequestState clientRequestState_;
	network::RequestParser parser_;
	network::BasicMessage message_;


};


