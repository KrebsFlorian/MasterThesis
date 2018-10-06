#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Token_Container.hpp"
#include "Exceptions.hpp"


/*
	Class that produces tokens from an string. It can either read the string from an file or can directly take it as an argument.
	If the string and not a path to a file is given as an argument, additionally an flag has to be set to divide the two cases.
*/
class Tokenizer : public Token_Container {
	//no reference because i want to be able to destroy the string that is used as a parameter
	std::string str_to_tokenize;
	int index{ 0 };

	/*
		Function to skip comments, white spaces, tabs and newlines.
	*/
	void find_next_valid_character() {
		for (; index <= str_to_tokenize.size(); index++) {
		char& character = str_to_tokenize[index];
		switch (character) {
		case ' ':
			break;
		case '\n':
			break;
		case '\t':
			break;
		case '/':if (str_to_tokenize[index + 1] == '/') {
					size_t found = str_to_tokenize.find('\n', index);
					index = found;
				}
				 else if (str_to_tokenize[index + 1] == '*') {
					 size_t found = str_to_tokenize.find("*/", index);
					 index = found+1;
				 }
				 else return;
			break;
		default:
			return;
		}
		}
	}

public:
	/*
		Constructors as described above.
		The constructor taking the path to the file as an argument can throw an exception if it cannot open the file.
	*/
	Tokenizer(std::string path) {
		std::ifstream unit_file(path, std::ifstream::in);
		if (unit_file.bad()) {
			throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path };
		}
		std::stringstream buffer;
		buffer << unit_file.rdbuf();
		str_to_tokenize= buffer.str();
		//search for println, because in this case OpenCL shouldn't be used, but this cannot be found by the Generator, because it is not propagated by the Converter_RVC_Cpp lib
		//This is the easiest way to check and propagate to the code generator, if there is a println the first token will be NoOpenCLDueToPrintln
		if (str_to_tokenize.find(" println(") != std::string::npos || str_to_tokenize.find("\tprintln(") != std::string::npos) {
			str_to_tokenize.insert(0, "NoOpenCLDueToPrintln ");
		}
	};

	//bool value is irrelevant, because it is just there to distinguish the two constructors.
	Tokenizer(std::string text, bool) :str_to_tokenize{ text } {};
	Tokenizer(std::string *text) : str_to_tokenize{ *text } {};

	/*
		Function returns the next token.
		If all tokens have been read this function just returns a token with an empty string. This is the only case when an empty string can be returned.
	*/
	virtual Token get_next_Token() {
		//first set the index to the next relevant character
		find_next_valid_character();

		//no tokens to read anymore
		if (index == str_to_tokenize.size()) {
			//throw Converter_RVC_Cpp::Tokenizer_Exception{ "Tokenizer Index out of Bounds" };
			return Token{ "" };
		}
		
		std::string current_token{};

		//check for special characters
		char& first_character = str_to_tokenize[index];

		switch (first_character) {
		case '*':if (str_to_tokenize[index + 1] == '*') { index += 2; return Token{ "**" }; }
				 else { index++; return Token{ "*" }; }
			break;
		case '/':index++; return Token{ "/" };
			break;
		case ':':if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ ":=" }; }
				 else { index++; return Token{ ":" }; }
			break;
		case ';':index++; return Token{ ";"}; break;
		case '=':if (str_to_tokenize[index + 1] == '=' && str_to_tokenize[index + 2] == '>') {index += 3; return Token{ "==>" };}
				 else if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ "==" }; }
				 else { index++; return Token{ "=" }; }
			break;
		case ',':index++; return Token{ "," }; break;
		case '(':index++; return Token{ "(" }; break;
		case ')':index++; return Token{ ")" }; break;
		case '[':index++; return Token{ "[" }; break;
		case ']':index++; return Token{ "]" }; break;
		case '&':if (str_to_tokenize[index + 1] == '&' && str_to_tokenize[index + 2] == '&') { index += 3; return Token{ "&&&" }; }
				 else if (str_to_tokenize[index + 1] == '&' ) { index += 2; return Token{ "&&" }; }
				 else { index++; return Token{ "&" }; }
			break;
		case '<':if (str_to_tokenize[index + 1] == '<' && str_to_tokenize[index + 2] == '<') { index += 3; return Token{ "<<<" }; }
				 else if (str_to_tokenize[index + 1] == '<') { index += 2; return Token{ "<<" }; }
				 else if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ "<=" }; }
				 else { index++; return Token{ "<" }; }
			break;
		case '>':if (str_to_tokenize[index + 1] == '>' && str_to_tokenize[index + 2] == '>') { index += 3; return Token{ ">>>" }; }
				 else if (str_to_tokenize[index + 1] == '>') { index += 2; return Token{ ">>" }; }
				 else if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ ">=" }; }
				 else { index++; return Token{ ">" }; }
			break;
		case '|':if (str_to_tokenize[index + 1] == '|' && str_to_tokenize[index + 2] == '|') { index += 3; return Token{ "|||" }; }
				 else if (str_to_tokenize[index + 1] == '|') { index += 2; return Token{ "||" }; }
				 else { index++; return Token{ "|" }; }
			break;
		case '\"':index++; return Token{ "\"" };
			break;
		case '{':index++; return Token{ "{" };
				 break;
		case '}':index++; return Token{ "}" };
				 break;
		case '.':if (str_to_tokenize[index + 1] == '.') { index += 2; return Token{ ".." }; }
				 else { index++; return Token{ "." }; }
				 break;
		case '!':if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ "!=" }; }
				 else { index++; return Token{ "!" }; }
		case '-':if (str_to_tokenize[index + 1] == '-' && str_to_tokenize[index + 2] == '>') { index += 3; return Token{ "-->" }; }
				 else if (str_to_tokenize[index + 1] == '-'){index += 2; return Token{ "--" }; }
				 else if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ "-=" }; }
				 else { index++; return Token{ "-" }; }
			break;
		case '+':if (str_to_tokenize[index + 1] == '=') { index += 2; return Token{ "+=" }; }
				 if (str_to_tokenize[index + 1] == '+') { index += 2; return Token{ "++" }; }
				 else { index++; return Token{ "+" }; }
			break;
		case '\\':index++; return Token{ "\\" };
				  break;
		case '?':index++; return Token{ "?" };
				 break;
		default:current_token += first_character; index++;
		}

		//run until a special character is found. If one of those is found don't add it to the token and return all characters that have been added to this token yet.
		for (;;) {
			if (index == str_to_tokenize.size())return Token{ current_token };
			char& character = str_to_tokenize[index];
			switch (character) {
			case '*':return Token{ current_token }; break;
			case '/':return Token{ current_token }; break;
			case ':':return Token{ current_token }; break;
			case ';':return Token{ current_token }; break;
			case '=':return Token{ current_token }; break;
			case '(':return Token{ current_token }; break;
			case ')':return Token{ current_token }; break;
			case '[':return Token{ current_token }; break;
			case ']':return Token{ current_token }; break;
			case '&':return Token{ current_token }; break;
			case '<':return Token{ current_token }; break;
			case '>':return Token{ current_token }; break;
			case '|':return Token{ current_token }; break;
			case ' ':index++; return Token{ current_token }; break;
			case '\n':index++; return Token{ current_token }; break;
			case '\t':index++; return Token{ current_token }; break;
			case '-':return Token{ current_token }; break;
			case '+':return Token{ current_token }; break;
			case '\"':return Token{ current_token }; break;
			case '.':return Token{ current_token }; break;
			case '!':return Token{ current_token }; break;
			case '{':return Token{ current_token }; break;
			case '}':return Token{ current_token }; break;
			case ',':return Token{ current_token }; break;
			case '\\':return Token{ current_token }; break;
			case '?':return Token{ current_token }; break;
			default:current_token += character; index++;
			}
			
		}
		throw Converter_RVC_Cpp::Tokenizer_Exception{ "Tokenizer Index out of Bounds" };
	}
};
