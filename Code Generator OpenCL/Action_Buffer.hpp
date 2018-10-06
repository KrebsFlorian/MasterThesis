#pragma once

#include <vector>
#include "Token_Container.hpp"

class Action_Buffer: public Token_Container {

	std::vector<Token> tokens{};
	int index{ 0 };
	bool &OpenCL;

	void buffer_bracket(Token& t, Tokenizer& token_producer) {
		if (t.str == "[") {
			t = token_producer.get_next_Token();
			while (t.str != "]") {
				tokens.push_back(t);
				if (t.str == "[") {
					buffer_bracket(t, token_producer);
				}
				else {
					t = token_producer.get_next_Token();
				}
			}
			tokens.push_back(t);
			t = token_producer.get_next_Token();
		}
	}

	void buffer_action(Token& t, Tokenizer& token_producer) {
		//read action head 
		while (t.str != "do" && t.str != "end") {
			tokens.push_back(t);
			if (t.str == "guard") {
				OpenCL = false;
			}
			if (t.str == "[") {//list comprehension can contain an end, this would break this loop, but it should
				buffer_bracket(t, token_producer);
			}
			else if (t.str == "if") {
				t = token_producer.get_next_Token();
				buffer_scope(t, token_producer);
			}
			else {
				t = token_producer.get_next_Token();
			}
		}
		// read rest
		if (t.str == "do") {
			tokens.push_back(t);
			t = token_producer.get_next_Token();
			buffer_scope(t, token_producer);
		}
		else if (t.str == "end") {
			tokens.push_back(t);
			t = token_producer.get_next_Token();
		}
	}

	void buffer_scope(Token& t, Tokenizer& token_producer) {
		while (t.str != "end") {
			if (t.str == "if") {
				tokens.push_back(t);
				t = token_producer.get_next_Token();
				buffer_scope(t, token_producer);
			}
			else if (t.str == "do") {
				tokens.push_back(t);
				t = token_producer.get_next_Token();
				buffer_scope(t, token_producer);
			}
			else {
				tokens.push_back(t);
				t = token_producer.get_next_Token();
			}
		}
		tokens.push_back(t);
		t = token_producer.get_next_Token();
	}
public:
	Action_Buffer(Token& t, Tokenizer& token_producer,bool& flag):OpenCL(flag) {
		buffer_action(t, token_producer);
	}

	virtual Token get_next_Token() {
		if (index >= tokens.size()) {
			throw Converter_RVC_Cpp::Tokenizer_Exception{ "Action Buffer Index out of Bounds" };
		}
		Token t = tokens[index];
		index++;
		return t;
	}

	void reset_buffer() { index = 0; }

	void decrement_index( int dec_val){
		index -= dec_val;
		if (index < 0) {
			index = 0;
		}
	}
};