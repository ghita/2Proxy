#pragma once
#include <string>

// Parses an input uri and returns host and port part from it
// @retunrs: true in case it was a valid uri 
bool ParseUri(const std::string uri, std::string& host, std::string& port)
{
    std::string::size_type posNoProtoUri = std::string::npos;
    std::string::size_type pos = uri.find("http");

    if (pos != std::string::npos)
    {
        posNoProtoUri = pos + sizeof("http://") - 1;
    }
    else if ((pos = uri.find("https")) != std::string::npos)
    {
        posNoProtoUri = pos + sizeof("https://") - 1;
    }
    else
    {
        FILE_LOG(logERROR) << "Invalid Uri format: " << uri;
        return false;
    }

    // find in string whichever if first from ':' or '/'. This is delimiter for the host part of uri
    // TODO: is this right ? see RFC
    std::string::size_type hostEndPos = uri.find("/", posNoProtoUri);
    if (hostEndPos != std::string::npos)
    {
        host = uri.substr(posNoProtoUri, hostEndPos - posNoProtoUri);

        if ((pos = host.find(':')) != std::string::npos) // have port information also here
        {
            host = uri.substr(posNoProtoUri, pos - posNoProtoUri);
            port = uri.substr(pos + 1, uri.length()); // substr does no go beyong length();
        }
        else
        {
            port = "80";
        }
        return true;
    }

    FILE_LOG(logERROR) << "Invalid Uri format: " << uri;
    return false;
}