#include "stdafx.h"
#include "HttpConnection.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "HttpUtil.h"

HttpConnection::HttpConnection(ba::io_service& io_service, ba::ip::tcp::socket& bsocket, char firstByte, Connection& parrentConn) : io_service_(io_service),
													 bsocket_(bsocket),
													 ssocket_(io_service),
													 firstByte_(firstByte),
													 resolver_(io_service),
													 proxy_closed(false),
													 isPersistent(false),
													 isOpened(false),
                                                     isFirstByteAdded(false),
													 clientRequestState_(Method),
													 serverResponseState_(ReadUntilBody),
													 parrentConn_(parrentConn),
													 isShuttingDown_(false)
													 {}

void HttpConnection::Start() {

	newStart_ = bbuffer.begin();
	reqHeaders.clear();
	ba::async_read(bsocket_, ba::buffer(bbuffer), ba::transfer_at_least(1),
			   boost::bind(&HttpConnection::HandleBrowserReadHeaders,
			   shared_from_this(),
						   ba::placeholders::error,
						   ba::placeholders::bytes_transferred));
}


void HttpConnection::HandleBrowserReadHeaders(const bs::error_code& err, size_t len) 
{
	if (!err && !IsStopped())
	{
		BufferType::iterator dataEnd_ = bbuffer.begin();
		std::advance(dataEnd_, len);
		network::RequestParser::Result parseResult = network::RequestParser::Indeterminate;

		std::pair<char*, char*> resultRange;
		switch (clientRequestState_)
		{
			case Method:
				resultRange = parser_.ParseUntil(network::RequestParser::MethodDone, 
					std::make_pair(newStart_, dataEnd_), parseResult);
 				if (parseResult == network::RequestParser::ParsedFailed)
				{
					//std::cout << "Error processing Method" << std::endl;
                    FILE_LOG(logERROR) << "Error processing Method";
					break;
				}
				else if (parseResult == network::RequestParser::ParsedOK) // read all Method data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					this->message_.Method(this->partialParsed_);

					//std::cout << "\t Method = " << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend()) << std::endl;
                    FILE_LOG(logINFO) << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend());

					//TODO: trim method
					this->newStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					this->partialParsed_.clear(); // use it only for one element

					// is there any more that in buffer that can be processed?
					if (resultRange.second == dataEnd_)
					{
						clientRequestState_ = Uri;
						ReadMoreFromBrowser(Uri);
						break;
					}
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					newStart_ = bbuffer.begin();
					clientRequestState_ = Method;
					ReadMoreFromBrowser(Method);
					break;
				}

				// more data "possibly!" exists in browser buffer, no need to break here
			case Uri:
				resultRange = parser_.ParseUntil(network::RequestParser::UriDone, 
					std::make_pair(newStart_, dataEnd_), parseResult);

				if (parseResult == network::RequestParser::ParsedFailed)
				{
					FILE_LOG(logERROR) << "Error processing Uri";
					break;
				}
				else if (parseResult == network::RequestParser::ParsedOK) // read all Uri data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result

					//std::cout << "\t Uri = " << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend()) << std::endl;
                    FILE_LOG(logINFO) << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend()) << std::endl;

					this->message_.Destination(this->partialParsed_);
					//TODO: trim destination
					this->newStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					this->partialParsed_.clear(); // use it only for one element

					// is there any more that in buffer that can be processed?
					if (resultRange.second == dataEnd_)
					{
						clientRequestState_ = Version;
						ReadMoreFromBrowser(Version);
						break;
					}
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					newStart_ = bbuffer.begin();
					clientRequestState_ = Uri;
					ReadMoreFromBrowser(Uri);
					break;
				}
				// more data "possibly!" exists in browser buffer, no need to break here

			case Version:
				resultRange = parser_.ParseUntil(network::RequestParser::VersionDone, 
					std::make_pair(newStart_, dataEnd_), parseResult);

				if (parseResult == network::RequestParser::ParsedFailed)
				{
					//std::cout << "Error processing Http Version" << std::endl;
                    FILE_LOG(logERROR) << "Error processing Http Version";
					break;
				}
				else if (parseResult == network::RequestParser::ParsedOK) // read all Uri data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result

					//std::cout << "\t Version = " << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend()) << std::endl;
                    FILE_LOG(logINFO) << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend());

					// we don't have yet version information in BasicMessage class, skip this info for now
					this->newStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					this->partialParsed_.clear(); // use it only for one element

					// is there any more that in buffer that can be processed?
					if (resultRange.second == dataEnd_)
					{
						clientRequestState_ = Headers;
						ReadMoreFromBrowser(Headers);
						break;
					}
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					newStart_ = bbuffer.begin();
					clientRequestState_ = Version;
					ReadMoreFromBrowser(Version);
					break;
				}
				// more data "possibly!" exists in browser buffer, no need to break here

			case Headers:
				resultRange = parser_.ParseUntil(network::RequestParser::HeadersDone, 
					std::make_pair(newStart_, dataEnd_), parseResult);

				if (parseResult == network::RequestParser::ParsedFailed)
				{
					//std::cout << "Error processing Http Headers" << std::endl;
                    FILE_LOG(logERROR) <<  "Error processing Http Headers";
					break;
				}
				else if (parseResult == network::RequestParser::ParsedOK) // read all Uri data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					//TODO: we don't parse for now the headers and fill out our BasicMessage instance with it

					std::cout << "\t Headers = " << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend()) << std::endl;
                    FILE_LOG(logINFO) << std::string(this->partialParsed_.cbegin(), this->partialParsed_.cend());
					this->reqHeaders_ = this->partialParsed_;
					StartConnect();

					//this->newStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					//TODO: we've read all headers, it's possible that client sends also body data ??

					if (resultRange.second < dataEnd_)
					{
						//std::cout << "\t\tThere is data remaining in browser read buffer:!!!!!! " << std::string(resultRange.second, dataEnd_); 
                        FILE_LOG(logWARNING) << "There is data remaining in browser read buffer";
					}
					this->partialParsed_.clear(); // use it only for one element
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsed_)); //append to partial result
					newStart_ = bbuffer.begin();
					clientRequestState_ = Headers;
					ReadMoreFromBrowser(Headers);
					break;
				}
				// TODO: more data "possibly!" exists in browser buffer, no need to break here (e.g. request that has body!)

			default:
				break;
		}
	}
	else
	{
		Shutdown();
	}
}


void HttpConnection::ReadMoreFromBrowser(ClientRequestState state)
{
	state; // unreferenced
	ba::async_read(bsocket_, ba::buffer(bbuffer), ba::transfer_at_least(1),
			   boost::bind(&HttpConnection::HandleBrowserReadHeaders,
			   shared_from_this(),
						   ba::placeholders::error,
						   ba::placeholders::bytes_transferred));
}

// at this point all client request data is known such that connection to origin server is possible
void HttpConnection::StartConnect() 
{
	std::string uri(this->message_.Destination().begin(), this->message_.Destination().end());
	std::string host;
	std::string port;

	if (!ParseUri(uri, host, port))
        throw std::exception("Invalid URI");
        //TODO: retun an error to client;

	if(!isOpened) 
	{
		fServer=host;
		fPort=port;
		ba::ip::tcp::resolver::query query(host, port);
		resolver_.async_resolve(query,
								boost::bind(&HttpConnection::HandleResolve, shared_from_this(),
											boost::asio::placeholders::error,
											boost::asio::placeholders::iterator));
	}
	else 
	{
	    StartWriteToServer();
	}
}

// origin server resolving of ip address
void HttpConnection::HandleResolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator) {
    if (!err) 
	{
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							  boost::bind(&HttpConnection::HandleConnect, shared_from_this(),
										  boost::asio::placeholders::error,
										  ++endpoint_iterator));
    }
	else 
	{
        FILE_LOG(logERROR) << "HttpConnection::HandleResolve failed: " << err.message();
		Shutdown();
	}
}

// connection with origin server might have succeded, if not try connecting with the next endpoint available
void HttpConnection::HandleConnect(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator) 
{
    if (!err) 
	{
		isOpened=true;
		StartWriteToServer();
    } else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) 
	{
		ssocket_.close();
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							   boost::bind(&HttpConnection::HandleConnect, shared_from_this(),
										   boost::asio::placeholders::error,
										   ++endpoint_iterator));
    }
	else
	{
        FILE_LOG(logERROR) << "HttpConnection::HandleConnect failed: " << err.message();
		Shutdown();
	}
}

void HttpConnection::StartWriteToServer() 
{
	fReq=std::string(this->message_.Method().begin(), this->message_.Method().end());
	fReq+=" ";
	fReq+=std::string(this->message_.Destination().begin(), this->message_.Destination().end());
	fReq+=" HTTP/";
	fReq+="1.0";
	fReq+="\r\n";

	fReq += std::string(this->reqHeaders_.begin(), this->reqHeaders_.end());
    FILE_LOG(logDEBUG) << "HttpConnection::StartWriteToServer: " << fReq;

	ba::async_write(ssocket_, ba::buffer(fReq),
					boost::bind(&HttpConnection::HandleServerWrite, shared_from_this(),
								ba::placeholders::error,
								ba::placeholders::bytes_transferred));
}


void HttpConnection::HandleServerWrite(const bs::error_code& err, size_t len)
{
	len; // unreferenced. TODO: do we care ?
	if(!err && !IsStopped()) 
	{
		ba::async_read(ssocket_, ba::buffer(sbuffer), ba::transfer_at_least(1),
			boost::bind(&HttpConnection::HandleServerRead,
						shared_from_this(),
						ba::placeholders::error,
						ba::placeholders::bytes_transferred));
		newServerReadStart_ = sbuffer.begin();
	}
	else 
	{
        FILE_LOG(logERROR) << "HttpConnection::HandleServerWrite failed: " << err.message();
		Shutdown();
	}
}

void HttpConnection::HandleServerRead(const bs::error_code& err, size_t len)
{
	if (!err && !IsStopped())
	{
		BufferType::iterator dataEnd = sbuffer.begin();
		std::advance(dataEnd, len);
		network::ResponseParser::Result parseResult = network::ResponseParser::Indeterminate;

		std::pair<char*, char*> resultRange;
		switch (serverResponseState_)
		{
			case ReadUntilBody: // read all server response untill beginning of data
				resultRange = responseParser_.ParseUntil(network::ResponseParser::HttpHeadersDone, std::make_pair(newServerReadStart_, dataEnd), parseResult);
				if (parseResult == network::ResponseParser::ParsedFailed)
				{
					//std::cout << "Error processing Method" << std::endl;
                    FILE_LOG(logERROR) << "Error processing ReadUntilBody";
					break;
				}
				else if (parseResult == network::ResponseParser::ParsedOK) // read all ReadUntilBody data succefully
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsedServer_)); //append to partial result
					//TODO: trim method
					this->newServerReadStart_ = resultRange.second; // from here continue parsing with other data that exists in browser buffer
					this->partialParsedServer_.clear(); // use it only for one element

					// is there any more that in buffer that can be processed?
					if (resultRange.second == dataEnd)
					{
						serverResponseState_ = Body;
						//ReadMoreFromServer(Body);
						SendDataToClient(len);
						break;
					}
				}
				else //partial parsed
				{
					std::copy(resultRange.first, resultRange.second, std::back_inserter(partialParsedServer_)); //append to partial result
					serverResponseState_ = ReadUntilBody;
					//ReadMoreFromServer(ReadUntilBody);
					SendDataToClient(len);
					break;
				}
			case Body: // read all server body response
				// TODO: we asume here that body ends when server closes the connection. Have to make it work eith Content-Length and chuncked transfer
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

void HttpConnection::SendDataToClient(int len)
{
	ba::async_write(bsocket_, ba::buffer(sbuffer, len),
					boost::bind(&HttpConnection::HandleClientWrite, shared_from_this(),
								ba::placeholders::error,
								ba::placeholders::bytes_transferred));
}


void HttpConnection::HandleClientWrite(const bs::error_code& err, size_t len)
{
	len ; // unreferenced
	if (!err && !IsStopped())
	{
		ReadMoreFromServer(serverResponseState_);
	}
	else
	{
		Shutdown();
	}
}

void HttpConnection::ReadMoreFromServer(ServerResponseState state)
{
	state; // unreferenced
	ba::async_read(ssocket_, ba::buffer(sbuffer), ba::transfer_at_least(1),
			boost::bind(&HttpConnection::HandleServerRead,
						shared_from_this(),
						ba::placeholders::error,
						ba::placeholders::bytes_transferred));
	newServerReadStart_ = bbuffer.begin();
}

//TODO: below should be deleted
/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void HttpConnection::HandleServerReadHeaders(const bs::error_code& err, size_t len) 
{
	len ; // unreferenced TODO: do we care ?
	if(!err) 
	{
		//auto serverResp = HttpServerResponseConnection::Create(io_service_, ssocket_, bsocket_, this->lifeMng_);
		//serverResp->StartReadingFromServer();
	} 
	else 
	{
        FILE_LOG(logERROR) << "HttpConnection::HandleServerReadHeaders error: " << err.message();
		Shutdown();
	}
}


/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void HttpConnection::HandleServerReadBody(const bs::error_code& err, size_t len) {
	if(!err || err == ba::error::eof) {
		RespReaded+=len;
		if(err == ba::error::eof)
			proxy_closed=true;
		ba::async_write(bsocket_, ba::buffer(sbuffer,len),
						boost::bind(&HttpConnection::HandleBrowserWrite,
									shared_from_this(),
									ba::placeholders::error,
									ba::placeholders::bytes_transferred));
	} else {
		Shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void HttpConnection::HandleBrowserWrite(const bs::error_code& err, size_t len) {
	if(!err) {
		if(!proxy_closed && (RespLen == -1 || RespReaded < RespLen))
			async_read(ssocket_, ba::buffer(sbuffer,len), ba::transfer_at_least(1),
					   boost::bind(&HttpConnection::HandleServerReadBody,
								   shared_from_this(),
								   ba::placeholders::error,
								   ba::placeholders::bytes_transferred));
		else {
//			shutdown();
 			if(isPersistent && !proxy_closed) {
  				//std::cout << "Starting read headers from browser, as connection is persistent" << std::endl;
                FILE_LOG(logINFO) << "Starting read headers from browser, as connection is persistent";
  				Start();
 			}
		}
	} else {
		Shutdown();
	}
}

void HttpConnection::Shutdown() {
	if (!isShuttingDown_)
	{
		//ssocket_.close();
		//bsocket_.close();
		isShuttingDown_ = true;
		this->parrentConn_.Shutdown();
	}
}

bool HttpConnection::IsStopped()
{
	//return (!ssocket_.is_open() || !bsocket_.is_open());
	return isShuttingDown_;
}