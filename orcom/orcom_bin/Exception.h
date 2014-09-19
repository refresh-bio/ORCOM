/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_EXCEPTION
#define H_EXCEPTION

#include "Globals.h"

#include <string>
#include <stdexcept>


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


#endif // H_EXCEPTION
