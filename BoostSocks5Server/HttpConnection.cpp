#include "stdafx.h"
#include "HttpConnection.h"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

HttpConnection::HttpConnection(ba::io_service& io_service, socket_ptr bsocket, char firstByte) : io_service_(io_service),
													 bsocket_(bsocket),
													 ssocket_(io_service),
													 firstByte_(firstByte),
													 resolver_(io_service),
													 proxy_closed(false),
													 isPersistent(false),
													 isOpened(false),
                                                     isFirstByteAdded(false) {
}

/** 
 * 
 * 
 */
void HttpConnection::Start() {
	fHeaders.clear();
	reqHeaders.clear();
	respHeaders.clear();
	
	ba::async_read(*bsocket_, ba::buffer(bbuffer), ba::transfer_at_least(1),
			   boost::bind(&HttpConnection::HandleBrowserReadHeaders,
			   shared_from_this(),
						   ba::placeholders::error,
						   ba::placeholders::bytes_transferred));
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void HttpConnection::HandleBrowserReadHeaders(const bs::error_code& err, size_t len) {
	if(!err) {
		if(fHeaders.empty())
        {
            if ((firstByte_ != 0) && (!isFirstByteAdded))
            {
                fHeaders= firstByte_ + std::string(bbuffer.data(),len);
                isFirstByteAdded = true;
            }
            else
                fHeaders= std::string(bbuffer.data(),len);
        }
		else
			fHeaders+=std::string(bbuffer.data(),len);
		if(fHeaders.find("\r\n\r\n") == std::string::npos) { // going to read rest of headers
			ba::async_read(*bsocket_, ba::buffer(bbuffer), ba::transfer_at_least(1),
					   boost::bind(&HttpConnection::HandleBrowserReadHeaders,
								   shared_from_this(),
								   ba::placeholders::error,
								   ba::placeholders::bytes_transferred));
		} else { // analyze headers
			std::string::size_type idx=fHeaders.find("\r\n");
			std::string reqString=fHeaders.substr(0,idx);
			fHeaders.erase(0,idx+2);

			idx=reqString.find(" ");
			if(idx == std::string::npos) {
				std::cout << "Bad first line: " << reqString << std::endl;
				return;
			}
			
			fMethod=reqString.substr(0,idx);
			reqString=reqString.substr(idx+1);
			idx=reqString.find(" ");
			if(idx == std::string::npos) {
				std::cout << "Bad first line of request: " << reqString << std::endl;
				return;
			}
			fURL=reqString.substr(0,idx);
			fReqVersion=reqString.substr(idx+1);
			idx=fReqVersion.find("/");
			if(idx == std::string::npos) {
				std::cout << "Bad first line of request: " << reqString << std::endl;
				return;
			}
			fReqVersion=fReqVersion.substr(idx+1);
			
			// analyze headers, etc
			ParseHeaders(fHeaders,reqHeaders);
			// pass control
			StartConnect();
		}
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
void HttpConnection::HandleServerWrite(const bs::error_code& err, size_t len) {
	if(!err) {
		ba::async_read(ssocket_, ba::buffer(sbuffer), ba::transfer_at_least(1),
				   boost::bind(&HttpConnection::HandleServerReadHeaders,
							   /*shared_from_this()*/ shared_from_this(),
							   ba::placeholders::error,
							   ba::placeholders::bytes_transferred));
	}else {
		Shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param len 
 */
void HttpConnection::HandleServerReadHeaders(const bs::error_code& err, size_t len) {
	if(!err) {
		std::string::size_type idx;
		if(fHeaders.empty())
			fHeaders=std::string(sbuffer.data(),len);
		else
			fHeaders+=std::string(sbuffer.data(),len);
		idx=fHeaders.find("\r\n\r\n");
		if(idx == std::string::npos) { // going to read rest of headers
			ba::async_read(ssocket_, ba::buffer(sbuffer), ba::transfer_at_least(1),
					   boost::bind(&HttpConnection::HandleBrowserReadHeaders,
								   shared_from_this(),
								   ba::placeholders::error,
								   ba::placeholders::bytes_transferred));
		} else { // analyze headers
			RespReaded=len-idx-4;
			idx=fHeaders.find("\r\n");
 			std::string respString=fHeaders.substr(0,idx);
			RespLen = -1;
			ParseHeaders(fHeaders.substr(idx+2),respHeaders);
			std::string reqConnString="",respConnString="";

			std::string respVersion=respString.substr(respString.find("HTTP/")+5,3);
			
			HeadersMap::iterator it=respHeaders.find("Content-Length");
			if(it != respHeaders.end())
				RespLen=boost::lexical_cast<int>(it->second);
			it=respHeaders.find("Connection");
			if(it != respHeaders.end())
				respConnString=it->second;
			it=reqHeaders.find("Connection");
			if(it != reqHeaders.end())
				reqConnString=it->second;
			
			isPersistent=(
				((fReqVersion == "1.1" && reqConnString != "close") ||
				 (fReqVersion == "1.0" && reqConnString == "keep-alive")) &&
				((respVersion == "1.1" && respConnString != "close") ||
				 (respVersion == "1.0" && respConnString == "keep-alive")) &&
				RespLen != -1);
			// send data
			ba::async_write(*bsocket_, ba::buffer(fHeaders),
							boost::bind(&HttpConnection::HandleBrowserWrite,
										shared_from_this(),
										ba::placeholders::error,
										ba::placeholders::bytes_transferred));
		}
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
void HttpConnection::HandleServerReadBody(const bs::error_code& err, size_t len) {
	if(!err || err == ba::error::eof) {
		RespReaded+=len;
		if(err == ba::error::eof)
			proxy_closed=true;
		ba::async_write(*bsocket_, ba::buffer(sbuffer,len),
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
  				std::cout << "Starting read headers from browser, as connection is persistent" << std::endl;
  				Start();
 			}
		}
	} else {
		Shutdown();
	}
}

void HttpConnection::Shutdown() {
	ssocket_.close();
	bsocket_->close();
}

/** 
 * 
 * 
 */
void HttpConnection::StartConnect() {
	std::string server="";
	std::string port="80";
	boost::regex rHTTP("http://(.*?)(:(\\d+))?(/.*)");
	boost::smatch m;
	
	if(boost::regex_search(fURL, m, rHTTP, boost::match_extra)) {
		server=m[1].str();
		if(m[2].str() != "") {
			port=m[3].str();
		}
		fNewURL=m[4].str();
	}
	if(server.empty()) {
		std::cout << "Can't parse URL "<< std::endl;
		return;
	}
	if(!isOpened || server != fServer || port != fPort) {
		fServer=server;
		fPort=port;
		ba::ip::tcp::resolver::query query(server, port);
		resolver_.async_resolve(query,
								boost::bind(&HttpConnection::HandleResolve, shared_from_this(),
											boost::asio::placeholders::error,
											boost::asio::placeholders::iterator));
	} else {
	    StartWriteToServer();
	}
}

/** 
 * 
 * 
 */
void HttpConnection::StartWriteToServer() {
	fReq=fMethod;
	fReq+=" ";
	fReq+=fNewURL;
	fReq+=" HTTP/";
	fReq+="1.0";
	fReq+="\r\n";
	fReq+=fHeaders;
	ba::async_write(ssocket_, ba::buffer(fReq),
					boost::bind(&HttpConnection::HandleServerWrite, shared_from_this(),
								ba::placeholders::error,
								ba::placeholders::bytes_transferred));

	fHeaders.clear();
}


/** 
 * 
 * 
 * @param err 
 * @param endpoint_iterator 
 */
void HttpConnection::HandleResolve(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator) {
    if (!err) {
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							  boost::bind(&HttpConnection::HandleConnect, shared_from_this(),
										  boost::asio::placeholders::error,
										  ++endpoint_iterator));
    }else {
		Shutdown();
	}
}

/** 
 * 
 * 
 * @param err 
 * @param endpoint_iterator 
 */
void HttpConnection::HandleConnect(const boost::system::error_code& err,
								ba::ip::tcp::resolver::iterator endpoint_iterator) {
    if (!err) {
		isOpened=true;
		StartWriteToServer();
    } else if (endpoint_iterator != ba::ip::tcp::resolver::iterator()) {
		ssocket_.close();
		ba::ip::tcp::endpoint endpoint = *endpoint_iterator;
		ssocket_.async_connect(endpoint,
							   boost::bind(&HttpConnection::HandleConnect, shared_from_this(),
										   boost::asio::placeholders::error,
										   ++endpoint_iterator));
    } else {
		Shutdown();
	}
}

void HttpConnection::ParseHeaders(const std::string& h, HeadersMap& hm) {
	std::string str(h);
	std::string::size_type idx;
	std::string t;
    bool badHeaderFormat = false;

	while((idx=str.find("\r\n")) != std::string::npos) {
        badHeaderFormat = false;
		t=str.substr(0,idx);
		str.erase(0,idx+2);
		if(t == "")
			break;
		idx=t.find(": ");
		if(idx == std::string::npos) {
			std::cout << "Bad header line, trying with other format now: " << t << std::endl;
			//break;
            badHeaderFormat = true;
            idx=t.find(":");
            if (idx == std::string::npos)
            {
                std::cout << "Bad header line, giving up now: " << t << std::endl;
                break;
            }
		}

        if (!badHeaderFormat)
		    hm.insert(std::make_pair(t.substr(0,idx),t.substr(idx+2)));
        else
            hm.insert(std::make_pair(t.substr(0,idx),t.substr(idx+1)));
	}
}
