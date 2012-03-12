#pragma once
#include <utility>
#include <algorithm>
#include <boost/algorithm/string/classification.hpp>

namespace network
{
	class RequestParser
	{

	public:
		typedef enum StateType_
		{
			MethodStart,
			MethodChar,
            MethodDone,
            UriChar,
            UriDone,
            VersionH,
            VersionT1,
            VersionT2,
            VersionP,
            VersionSlash,
            VersionD1,
            VersionDot,
            VersionD2,
            VersionCr,
            VersionDone,
            HeaderName,
            HeaderColon,
            HeaderValue,
            HeaderCr,
            HeaderLineDone,
            HeadersCr,
            HeadersDone
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

		explicit RequestParser(StateType startState = MethodStart):
			internalState_(startState)
			{}

		void Reset(StateType startState = MethodStart)
		{
			internalState_ = startState;
		}

		StateType State() const
		{
			return internalState_;
		}

		//typedef std::pair<std::>

		template <class Iterator>
		std::pair<Iterator, Iterator> ParseUntil(StateType stopState, const std::pair<Iterator, Iterator>& range, Result& result)
		{
			bool parsedOK = true; // mesage structure is OK
			bool parsedComplete = false; // range contains data that matched completely the stopState criterion;
			                             // e.g. no partial result match

			Iterator start = range.first;
			Iterator end   = range.second;
			Iterator currentIterator = start;

			while(
				(currentIterator != end) 
					&& stopState != internalState_ &&
					parsedOK != false
				)
			{
				//currentIterator = start;
				switch(internalState_)
				{
					case MethodStart:
						if (boost::algorithm::is_upper()(*currentIterator)) internalState_ = MethodChar;
						else parsedOK = false;
						break;
					case MethodChar:
						if (boost::algorithm::is_upper()(*currentIterator)) break;
						else if (boost::algorithm::is_space()(*currentIterator)) internalState_ = MethodDone;
						else parsedOK = false;
						break;
					case MethodDone:
						if (boost::algorithm::is_cntrl()(*currentIterator)) parsedOK = false;
						else if (boost::algorithm::is_space()(*currentIterator)) parsedOK = false;
						else internalState_ = UriChar;
						break;
					case UriChar:
						if (boost::algorithm::is_cntrl()(*currentIterator)) parsedOK = false;
						else if (boost::algorithm::is_space()(*currentIterator)) internalState_ = UriDone;
						break;
					case UriDone:
						if (*currentIterator == 'H') internalState_ = VersionH;
						else parsedOK = false;
						break;
					case VersionH:
						if (*currentIterator == 'T') internalState_ = VersionT1;
						else parsedOK = false;
						break;
					case VersionT1:
						if (*currentIterator == 'T') internalState_ = VersionT2;
						else parsedOK = false;
						break;
					case VersionT2:
						if (*currentIterator == 'P') internalState_ = VersionP;
						else parsedOK = false;
						break;
					case VersionP:
						if (*currentIterator == '/') internalState_ = VersionSlash;
						else parsedOK = false;
						break;
					case VersionSlash:
						if (boost::algorithm::is_digit()(*currentIterator)) internalState_ = VersionD1;
						else parsedOK = false;
						break;
					case VersionD1:
						if(*currentIterator == '.') internalState_ = VersionDot;
						else parsedOK = false;
						break;
					case VersionDot:
						if (boost::algorithm::is_digit()(*currentIterator)) internalState_ = VersionD2;
						else parsedOK = false;
						break;
					case VersionD2:
						if (*currentIterator == '\r') internalState_ = VersionCr;
						else parsedOK = false;
						break;
					case VersionCr:
						if(*currentIterator == '\n') internalState_ = VersionDone;
						else parsedOK = false;
						break;
					case VersionDone:
						if(boost::algorithm::is_alnum()(*currentIterator)) internalState_ = HeaderName;
						else if (*currentIterator == '\r') internalState_ = HeadersCr;
						else parsedOK = false;
						break;
					case HeaderName:
						if(*currentIterator == ':') internalState_ = HeaderColon;
						else if ((boost::algorithm::is_alnum()(*currentIterator)) 
							|| boost::algorithm::is_punct()(*currentIterator)) break;
						else parsedOK = false;
						break;
					case HeaderColon:
						if (*currentIterator == ' ') internalState_ = HeaderValue;
						else parsedOK = false;
						break;
					case HeaderValue:
						if(*currentIterator == '\r') internalState_ = HeaderCr;
						else if (boost::algorithm::is_cntrl()(*currentIterator)) parsedOK = false;
                        break;
                    case HeaderCr:
                        if (*currentIterator == '\n') internalState_ = HeaderLineDone;
                        else parsedOK = false;
                        break;
                    case HeaderLineDone:
						if (*currentIterator == '\r') internalState_ = HeadersCr;
                        else if (boost::algorithm::is_alnum()(*currentIterator)) internalState_ = HeaderName;
                        else parsedOK = false;
                        break;
                    case HeadersCr:
                        if (*currentIterator == '\n') internalState_ = HeadersDone;
                        else parsedOK = false;
                        break;
                    case HeadersDone:
                        // anything that follows after headers_done is allowed.
                        break;
                    default:
                        parsedOK = false;
						break;
				}

				if (internalState_ == stopState) parsedComplete = true; // parsing completed
				++currentIterator; // pass next char
			}

			// parser state returned to the caller
			if (parsedComplete) result = ParsedOK;
			else if (parsedOK) result = Indeterminate;
			else result = ParsedFailed;

			return std::make_pair(start, currentIterator);
		};
	};
};