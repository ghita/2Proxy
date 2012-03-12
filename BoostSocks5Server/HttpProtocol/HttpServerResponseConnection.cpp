#include "stdafx.h"
#include "HttpServerResponseConnection.h"
#include "ResponseParser.h"
#include "../HttpConnection.h" //TODO: put all http files in one location

void HttpServerResponseConnection::Shutdown()
{
	//TODO: parentConn_ might already be deleted because of conditions in HttpConnection class. See a solution here.
	//this->parentConn_.Shutdown();
	this->lifeMng_->Shutdown();
}

void HttpServerResponseConnection::StartReadingFromServer()
{
	ba::async_read(server_, ba::buffer(buffer), ba::transfer_at_least(1),
			boost::bind(&HttpServerResponseConnection::HandleServerRead,
						shared_from_this(),
						ba::placeholders::error,
						ba::placeholders::bytes_transferred));
	newStart_ = buffer.begin();
}

void HttpServerResponseConnection::ReadMoreFromServer(ServerResponseState state)
{
	ba::async_read(server_, ba::buffer(buffer), ba::transfer_at_least(1),
			boost::bind(&HttpServerResponseConnection::HandleServerRead,
						shared_from_this(),
						ba::placeholders::error,
						ba::placeholders::bytes_transferred));
	newStart_ = buffer.begin();
}

void HttpServerResponseConnection::SendDataToClient(int len)
{
	ba::async_write(client_, ba::buffer(buffer, len),
					boost::bind(&HttpServerResponseConnection::HandleClientWrite, shared_from_this(),
								ba::placeholders::error,
								ba::placeholders::bytes_transferred));
}

void HttpServerResponseConnection::HandleServerRead(const bs::error_code& err, size_t len)
{
	if (!err && !IsStopped())
	{
		BufferType::iterator dataEnd_ = buffer.begin();
		std::advance(dataEnd_, len);
		network::ResponseParser::Result parseResult = network::ResponseParser::Indeterminate;

		std::pair<char*, char*> resultRange;
		switch (serverResponseState_)
		{
			case ReadUntilBody: // read all server response untill beginning of data
				resultRange = parser_.ParseUntil(network::ResponseParser::HttpHeadersDone, std::make_pair(newStart_, dataEnd_), parseResult);
				if (parseResult == network::ResponseParser::ParsedFailed)
				{
					//std::cout << "Error processing Method" << std::endl;
                    FILE_LOG(logERROR) << "Error processing ReadUntilBody";
					break;
				}
				else if (parseResult == network::ResponseParser::ParsedOK) // read all Method data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					//TODO: trim method
					this->newStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					this->partialParsed_.clear(); // use it only for one element

					// is there any more that in buffer that can be processed?
					if (resultRange.second == dataEnd_)
					{
						serverResponseState_ = Body;
						//ReadMoreFromServer(Body);
						SendDataToClient(len);
						break;
					}
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					serverResponseState_ = ReadUntilBody;
					//ReadMoreFromServer(ReadUntilBody);
					SendDataToClient(len);
					break;
				}
			case Body: // read all server body response
				// we asume here that body ends when server closes the connection. Have to make it work eith Content-Length and chuncked transfer
				SendDataToClient(len);
				break;
			default: // we should not get here
				break;
		}
	}
	else
	{
		Shutdown();
	}
}

void HttpServerResponseConnection::HandleClientWrite(const bs::error_code& err, size_t len)
{
	if (!err && !IsStopped())
	{
		ReadMoreFromServer(serverResponseState_);
	}
	else
	{
		Shutdown();
	}
}