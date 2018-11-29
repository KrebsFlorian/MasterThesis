#pragma once
#include <string>
#include <exception>

namespace Converter_RVC_Cpp {

	class Converter_Exception : public std::exception {
	public:
		Converter_Exception(std::string _str) :str(_str) {};

		virtual const char* what() const throw()
		{
			return str.c_str();
		}
	private:
		std::string str;
	};

	class Wrong_Token_Exception : public Converter_Exception {
	public:
		Wrong_Token_Exception(std::string _str) :Converter_Exception{ _str } {};
	};

	class Failed_Main_Creation_Exception : public Converter_Exception {
	public:
		Failed_Main_Creation_Exception(std::string _str) : Converter_Exception{ _str } {};
	};

	class Tokenizer_Exception : public Converter_Exception {
	public:
		Tokenizer_Exception(std::string _str) : Converter_Exception{ _str } {};
	};
}