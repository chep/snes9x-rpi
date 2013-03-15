#ifndef _EXCEPTIONS_HPP_
#define _EXCEPTIONS_HPP_

#include <string>

class SnesException
{
public:
	SnesException(const std::string name): name(name) {}
	virtual ~SnesException() {}

public:
	std::string getName() const {return name;}

private:
	std::string name;
};

inline std::ostream& operator<< (std::ostream& os, const SnesException &e)
{
	os << e.getName();
	return os;
}

class ExitException: public SnesException
{
public:
	ExitException() : SnesException("Exit") {}
	virtual ~ExitException() {}

private:
};

#endif
