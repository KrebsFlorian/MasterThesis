#pragma once

#include <vector>
#include "Token_Container.hpp"


class FIFO_Expression_Buffer : public Token_Container {

	std::vector<Token> tokens{};
	int index{ 0 };

	void buffer_action(Token& t, Token_Container& token_producer) {
		while (t.str != ":") {
			tokens.push_back(t);
			t = token_producer.get_next_Token();
		}
		tokens.push_back(t.str);
		t = token_producer.get_next_Token();
	}

public:
	FIFO_Expression_Buffer(Token& t, Token_Container& token_producer) {
		buffer_action(t, token_producer);
	}

	virtual Token get_next_Token() {
		if (index >= tokens.size()) {
			//throw Converter_RVC_Cpp::Tokenizer_Exception{ "FIFO Expression Buffer Index out of Bounds" };
			return Token{ "" };
		}
		Token t = tokens[index];
		index++;
		return t;
	}

};