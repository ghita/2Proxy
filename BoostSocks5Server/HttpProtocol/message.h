#pragma once

#include <vector>
#include <string>
#include <algorithm>

namespace network {

    /** The common message type.
     */
class BasicMessage 
{
	public:

        typedef std::vector<std::pair<std::string, std::string>> HeadersContainerType;
        typedef std::pair<std::string, std::string> HeaderType;
		typedef std::string HeaderKey;
		typedef std::vector<char> DataType;

        BasicMessage()
            : headers_(), method_(), body_(), source_(), destination_()
        { }

        BasicMessage(const BasicMessage & other)
            : headers_(other.headers_), method_(), body_(other.body_), source_(other.source_), destination_(other.destination_)
        { }

        BasicMessage & operator= (BasicMessage rhs) {
            rhs.swap(*this);
            return *this;
        }

        void swap(BasicMessage & other) {
            std::swap(other.headers_, headers_);
			std::swap(other.method_, method_);
            std::swap(other.body_, body_);
            std::swap(other.source_, source_);
            std::swap(other.destination_, destination_);
        }

        HeadersContainerType & Headers() {
            return headers_;
        }

        void Headers(HeadersContainerType const & headers) const {
            headers_ = headers;
        }

		void AddHeader(HeaderType const & pair) const {
			headers_.push_back(pair);
        }

		void RemoveHeader(HeaderKey const & key) const {
			std::remove_if(headers_.begin(), headers_.end(), [&key](HeaderType& header) -> bool{
				if (header.first == key)
					return true;
				else
					return false;
			});
        }

		HeadersContainerType const & Headers() const {
            return headers_;
        }

		DataType & Method() {
            return method_;
        }

        void Method(DataType const & method) const {
            method_ = method;
        }


        DataType const & Method() const {
			return method_;
        }

        void Body(DataType const & body) const {
            body_ = body;
        }

        DataType const & Body() const {
            return body_;
        }
        
        DataType & Source() {
            return source_;
        }

        void Source(DataType const & source) const {
            source_ = source;
        }

        DataType const & Source() const {
            return source_;
        }

        DataType & Destination() {
            return destination_;
        }

        void Destination(DataType const & destination) const {
            destination_ = destination;
        }

        DataType const & Destination() const {
            return destination_;
        }

	private:

		mutable HeadersContainerType headers_;
		mutable DataType method_;
        mutable DataType body_;
        mutable DataType source_;
        mutable DataType destination_;
};

#if 0
void swap(BasicMessage & left, BasicMessage & right) 
{
    // swap for ADL
    left.swap(right);
}
#endif
    
}; // namespace network
