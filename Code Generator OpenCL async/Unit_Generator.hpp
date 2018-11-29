#pragma once
#include "Tokenizer.hpp"
#include <map>
#include "Converter_RVC_Cpp.hpp"
using namespace Converter_RVC_Cpp;

/*
	Class to generate C++ code for a given unit file.
*/
class Unit_Generator {

	Tokenizer token_producer;

	//true if a @native statement has been found
	bool &native_flag;

	std::string path;

	//map where all constants are inserted. This map is used to find constants if they are used in the calculation of the size of a type.
	//This map holds all constants in global or actor namespace
	std::map<std::string, std::string>& const_map;

	//functionality is equal to const_map, but for local namespaces, like function bodies
	std::map<std::string, std::string> local_map;

	std::pair<std::string, std::string> convert_import(Token& t) {
		if (t.str == "import") {
			std::string path_to_import_file{ path };
			std::string symbol{};
			path_to_import_file.append(token_producer.get_next_Token().str); //must be part of the path
			Token previous_token = token_producer.get_next_Token();
			Token next_token = token_producer.get_next_Token();

			for (;;) {
				if (next_token.str != ";") {
					if (previous_token.str != ".")path_to_import_file.append("\\").append(previous_token.str);
					previous_token = next_token;
					next_token = token_producer.get_next_Token();
				}
				else if (next_token.str == ";") {
					path_to_import_file.append(".cal");
					symbol = previous_token.str;
					break;
				}
			}
			Unit_Generator gen(path_to_import_file, const_map, native_flag,path);
			auto output = gen.convert_unit_file(symbol, "\t");
			return output;
		}
	}

public:
	Unit_Generator(std::string _path, std::map<std::string, std::string>&map, bool& flag, std::string source_dir) :path{ source_dir }, token_producer { _path }, const_map(map), native_flag(flag) {}

	/*
		This function takes the requested symbol to imported by an actor and the prefix for each generated LoC and returns a string containing c++ code.
		The function generates c++ code for the requested symbol. If the requested symbol is a * it returns all symbols.
		A symbol is either a constant value, a function/procedure or a @native declaration.
		For @native declarations no code is generated. Instead the native_flag is set to true to indicate that the string generated in Initialization shall be included to the output.
	*/
	std::pair<std::string,std::string> convert_unit_file(const std::string requested_symbol,const std::string prefix = "") {
		std::string converted_unit_file{};
		std::string declarations;
		Token t = token_producer.get_next_Token();
		//skip everything before unit
		while(t.str != "unit"){
			if (t.str == "import") {
				auto r = convert_import(t);
				converted_unit_file += r.second;
				declarations += r.first;
			}
			t = token_producer.get_next_Token();
		}
		Token name = token_producer.get_next_Token();
		Token doppelpunkt = token_producer.get_next_Token();
		Token next_token = token_producer.get_next_Token();
		while (next_token.str != "end") {
			//std::cout << next_token.str << std::endl;
			if (next_token.str == "@native") {
				convert_nativ_declaration(next_token,token_producer, requested_symbol, native_flag);
			}
			else if (next_token.str == "uint" || next_token.str == "int" || next_token.str == "String" || next_token.str == "bool" || next_token.str == "half" || next_token.str == "float") {
				converted_unit_file += convert_expression(next_token, token_producer, const_map, const_map, requested_symbol, false, prefix);
			}
			else if (next_token.str == "function") {
				std::string tmp{ convert_function(next_token, token_producer, const_map, prefix, requested_symbol) };
				converted_unit_file += tmp;
				//find declaration and insert it at the beginning of the source string, to avoid linker errors
				std::string dekl = tmp.substr(0, tmp.find("{")-1) + ";\n";
				declarations.insert(0, dekl);
			}
			else if (next_token.str == "procedure") {
				std::string tmp{ convert_procedure(next_token, token_producer, const_map, prefix, requested_symbol) };
				converted_unit_file += tmp; 
				//find declaration and insert it at the beginning of the source string, to avoid linker errors
				std::string dekl = tmp.substr(0, tmp.find("{")-1) + ";\n";
				declarations.insert(0, dekl);
			}
			else if (next_token.str == "List") {
 				converted_unit_file += convert_list(next_token, token_producer,const_map,const_map, requested_symbol,prefix);
			}
			//else {//skip the irrelevant tokens, because a function may return before consuming all tokens belonging to this expression due to a different requested symbol
				//next_token = token_producer.get_next_Token();
			//}
			else{
				std::cout << "Error! Something unexpected happend during the processing of import statements. \n";
				exit(-1);
			}
		}
		return std::make_pair(declarations,converted_unit_file);
	}
};