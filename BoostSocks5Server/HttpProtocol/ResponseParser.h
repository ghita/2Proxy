#pragma once
#include <utility>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>

namespace network {

class ResponseParser
{
	public:
		typedef enum StateType_
		{
			HttpResponseBegin,
			HttpVersionH,
            HttpVersionT1,
            HttpVersionT2,
            HttpVersionP,
            HttpVersionSlash,
            HttpVersionD1,
            HttpVersionDot,
            HttpVersionD2,
			HttpVersionDone,
			HttpStatusDigit,
			HttpStatusDone,
			HttpStatusMessageChar,
			HttpStatusMessageCr,
			HttpStatusMessageDone,
			HttpHeaderNameChar,
			HttpHeaderColon,
			HttpHeaderValueChar,
			HttpHeaderCr,
			HttpHeaderLineDone,
			HttpHeadersEndCr,
			HttpHeadersDone
		} StateType;

	private:
		StateType internalState_;

	public:
		enum Result
		{
			ParsedFailed,
			Indeterminate,
			ParsedOK
		};


	public:

	explicit ResponseParser(StateType state = HttpResponseBegin):
		internalState_(state) {}

	ResponseParser(ResponseParser const& other):
		internalState_(other.internalState_) {}

	~ResponseParser () {}

	void swap (ResponseParser & other)
	{
		std::swap(other.internalState_, this->internalState_);
	}

	ResponseParser& operator= (ResponseParser rhs)
	{
		rhs.swap(*this);
		return *this;
	}

	template <class Iterator>
	std::pair<Iterator, Iterator> ParseUntil(StateType stopState, std::pair<Iterator, Iterator>& range, Result& result)
	{
		bool parsedOK = true; // mesage structure is OK
		bool parsedComplete = false; // range contains data that matched completely the stopState criterion;
			                            // e.g. no partial result match

		Iterator start = range.first;
		Iterator end   = range.second;
		Iterator current = start;

		while(
			(current != end) 
				&& stopState != internalState_ &&
				parsedOK != false
			)
		{
			//currentIterator = start;
			switch(internalState_)
			{
				case HttpResponseBegin:
					if (*current == ' ' || *current == '\r' || *current == '\n')
						// skip valid leading whitespace
						++start;
					else if (*current == 'H') 
					{
						internalState_ = HttpVersionH;
						start = current;
					}
					else parsedOK = false;
					break;
				case HttpVersionH:
					if (*current == 'T') internalState_ = HttpVersionT1;
					else parsedOK = false;
					break;
				case HttpVersionT1:
					if (*current == 'T') internalState_ = HttpVersionT2;
					else parsedOK = false;
					break;
				case HttpVersionT2:
					if (*current == 'P') internalState_ = HttpVersionP;
					else parsedOK = false;
					break;
				case HttpVersionP:
					if (*current == '/') internalState_ = HttpVersionSlash;
					else parsedOK = false;
					break;
				case HttpVersionSlash:
					if (boost::algorithm::is_digit()(*current)) internalState_ = HttpVersionD1;
					else parsedOK = false;
					break;
				case HttpVersionD1:
					if (*current == '.') internalState_ = HttpVersionDot;
					else parsedOK = false;
					break;
				case HttpVersionDot:
					if (boost::algorithm::is_digit()(*current)) internalState_ = HttpVersionD2;
					else parsedOK = false;
					break;
				case HttpVersionD2:
					if (*current == ' ') internalState_ = HttpVersionDone;
					else parsedOK = false;
					break;
				case HttpVersionDone:
					if (boost::algorithm::is_digit()(*current)) internalState_ = HttpStatusDigit;
					else parsedOK = false;
					break;
				case HttpStatusDigit:
					if (*current == ' ') internalState_ = HttpStatusDone;
					else if (!boost::algorithm::is_digit()(*current)) parsedOK = false;
					break;
				case HttpStatusDone:
					if (boost::algorithm::is_alnum()(*current)) internalState_ = HttpStatusMessageChar;
					else parsedOK = false;
					break;
				case HttpStatusMessageChar:
					if (boost::algorithm::is_alnum()(*current) || boost::algorithm::is_punct()(*current)
						|| (*current == ' ')) internalState_ = HttpStatusMessageChar;
					else if (*current == '\r') internalState_ = HttpStatusMessageCr;
					else parsedOK = false;
					break;
				case HttpStatusMessageCr:
					if (*current == '\n') internalState_ = HttpStatusMessageDone;
					else parsedOK = false;
					break;
				case HttpStatusMessageDone:
				case HttpHeaderLineDone:
					if (boost::algorithm::is_alnum()(*current)) internalState_ = HttpHeaderNameChar;
					else if (*current == '\r') internalState_ = HttpHeadersEndCr;
					else parsedOK = false;
					break;
				case HttpHeaderNameChar:
					if (*current == ':') internalState_ = HttpHeaderColon;
					else if (boost::algorithm::is_alnum()(*current) || boost::algorithm::is_space()(*current)
						|| boost::algorithm::is_punct()(*current)) internalState_ = HttpHeaderNameChar;
					else parsedOK = false;
					break;
				case HttpHeaderColon:
					if(boost::algorithm::is_space()(*current));
					else if (boost::algorithm::is_alnum()(*current) || boost::algorithm::is_punct()(*current))
						internalState_ = HttpHeaderValueChar;
					else parsedOK = false;
					break;
				case HttpHeaderValueChar:
					if (*current == '\r') internalState_ = HttpHeaderCr;
					else if (boost::algorithm::is_cntrl()(*current)) parsedOK = false;
					break;
				case HttpHeaderCr:
					if (*current == '\n') internalState_ = HttpHeaderLineDone;
					else parsedOK = false;
					break;
				case HttpHeadersEndCr:
					if (*current == '\n') internalState_ = HttpHeadersDone;
					else parsedOK = false;
					break;
				default:
					parsedOK = false;
			}

			if (internalState_ == stopState) parsedComplete = true; // parsing completed
				++current; // pass next char
		}

		// parser state returned to the caller
		if (parsedComplete) result = ParsedOK;
		else if (parsedOK) result = Indeterminate;
		else result = ParsedFailed;

		return std::make_pair(start, current);
		};
	};
}; // namespace network