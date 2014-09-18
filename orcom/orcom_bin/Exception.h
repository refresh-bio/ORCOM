#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>
#include <stdexcept>

// exception class
//
class Exception : public std::exception
{
	std::string message;

public:
	Exception(const char* msg_)
		: message(msg_)
	{}

	Exception(const std::string& msg_)
		: message(msg_)
	{}

	~Exception() throw()
	{}

	const char* what() const throw()				// for std::exception interface
	{
		return message.c_str();
	}
};


#endif // EXCEPTION_H
