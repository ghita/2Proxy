#pragma once
#include "../stdafx.h"
#include "ResponseParser.h"

//FWD
class HttpConnectionLifeManagement;

class HttpServerResponseConnection: public boost::enable_shared_from_this<HttpServerResponseConnection>
{
public:
	typedef boost::shared_ptr<HttpServerResponseConnection> Pointer;
	typedef ba::ip::tcp::socket Socket;

	static Pointer Create(ba::io_service& io_service, Socket& server, Socket& client, std::shared_ptr<HttpConnectionLifeManagement> lifeMng) {
		return Pointer(new HttpServerResponseConnection(io_service,  server, client, lifeMng));
	}

	bool IsStopped()
	{
		return (!server_.is_open() || !client_.is_open());
	}

	void Shutdown();
	//TODO: make private ?
	HttpServerResponseConnection(ba::io_service& ioService, Socket& server, Socket& client, std::shared_ptr<HttpConnectionLifeManagement> lifeMng): ioService_(ioService), 
		server_(server), client_(client), serverResponseState_(ReadUntilBody), lifeMng_(lifeMng) {}
	void StartReadingFromServer();
private:

	typedef boost::array<char, 1024uL> BufferType;
	typedef std::vector<char> DataType;
	void HandleServerRead(const bs::error_code& err, size_t len);
	void HandleClientWrite(const bs::error_code& err, size_t len);
	enum ServerResponseState
	{
		ReadUntilBody,
		Body
	};

	void ReadMoreFromServer(ServerResponseState state);
	void SendDataToClient(int len);

private:
	ba::io_service& ioService_;
	ba::ip::tcp::socket& server_;
	ba::ip::tcp::socket& client_;

	BufferType buffer;
	DataType partialParsed_;
	BufferType::iterator newStart_, dataEnd_;
	network::ResponseParser parser_;
	ServerResponseState serverResponseState_;
	std::shared_ptr<HttpConnectionLifeManagement> lifeMng_;
};