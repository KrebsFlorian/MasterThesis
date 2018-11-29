#pragma once
#include <string>
#include "Exceptions.hpp"
/*
	Parent class to make it possible to use the same functions with a Action_Buffer or a Tokenizer as input.
*/

struct Token {
	std::string str;

	Token(std::string s) :str{ s } {
		if (str == "mod") { str = "%"; }
		if (str == "and")str = "&&";
		if (str == "or")str = "||";
		if (str == "not")str = "!";
		if (str == "abs")str = "abs_no_name_collision";//abs is defined by the c++ compiler and by the most popular ORCC include file
	};
	Token() {};
};

class Token_Container {
public:
	virtual Token get_next_Token() = 0;
};