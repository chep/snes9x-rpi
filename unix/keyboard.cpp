#include <iostream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <fstream>

#include "keyboard.hpp"


KeyboardMapping::KeyboardMapping(std::string file):
	players(MAX_KEYBOARD_PLAYERS)
{
	std::ifstream f(file.c_str());
	if (f.is_open() && !f.bad())
	{
		boost::archive::xml_iarchive xml(f);
		try
		{
			xml >> boost::serialization::make_nvp("KeyboardMapping", *this);
		}
		catch (...)
		{
			//Error on file, keyboard will be unusable
			std::cerr<<"No keyboard configuration found"<<std::endl;
		}
	}
	else
	{
		//Error on file, keyboard will be unusable
		std::cerr<<"No keyboard configuration found"<<std::endl;
	}
}

void KeyboardMapping::save(std::string file) throw (SnesException)
{
	std::ofstream f(file.c_str());
	if (f.is_open() && !f.bad())
	{
	    boost::archive::xml_oarchive xml(f);
	    try
	    {
		    xml << boost::serialization::make_nvp("KeyboardMapping", *this);
	    }
	    catch (...)
	    {
		    throw SnesException("XML error");
	    }
	}
	else
		throw SnesException("File error");
}
