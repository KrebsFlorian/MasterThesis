#include "Converter.hpp"
#include "Tokenizer.hpp"
#include <fstream>
#include "Exceptions.hpp"
#include <algorithm>
#include <regex>
#include <set>


void replace_str(std::string &str, std::string old_str, std::string new_str) {
	size_t pos = str.find(old_str);
	while (pos != std::string::npos) {
		str.replace(pos, old_str.size(), new_str);
		pos = str.find(old_str, pos + new_str.size()+1);
	}
}


size_t find_insert_point_before_if(std::string& str,std::string str_to_find) {
	size_t pos = str.find(str_to_find);
	while (str.find(str_to_find, pos + 1) != std::string::npos) {
		pos = str.find(str_to_find, pos + 1);
	}

	return str.rfind("\n", pos) + 1;
}

std::string read_brace(Token& t, Tokenizer& token_producer) {
	std::string output{ "{" };
	t = token_producer.get_next_Token();
	while (t.str != "}") {
		if (t.str == "{") {
			output.append(read_brace(t, token_producer));
		}
		else {
			output.append(t.str);
			t = token_producer.get_next_Token();
		}
	}
	output.append("}");
	t = token_producer.get_next_Token();
	return output;
}

void remove_duplicates_in_scope(Token &t, Tokenizer& token_producer,std::map<std::string,std::string>& symbol_expr_map, std::set<std::string>& known_symbols,std::string& output,std::string prefix = "") {
	//liste mit allen bekannten const, wenn weitere gefunden dann löschen, liste aus umgebenden scope ist konstruktorargument für dieses scope liste
	bool function_head;
	bool assignment{ false };
	std::string local_prefix{ prefix };
	std::set<std::string> known_symbols_this_scope{ known_symbols };
	if (t.str == "{") {
		function_head = false;
		output.append(" {\n");
		t = token_producer.get_next_Token();
	}
	else {
		function_head = true;
	}

	std::string line{ local_prefix };
	while (t.str != "}") {
		if (symbol_expr_map.count(t.str) > 0 && known_symbols_this_scope.find(t.str) == known_symbols_this_scope.end()) {
			//check if it is in a condition block of an else if, if yes it cannot be inserted before this line because it would be in between the if(){} and the else if(){}. This would cause an error.
			if (line.find("\telse if(") != std::string::npos) {
				output.insert(find_insert_point_before_if(output,"\n"+local_prefix+"if("),local_prefix+ symbol_expr_map[t.str] + "\n");
			}
			else {
				output.append(local_prefix + symbol_expr_map[t.str] + "\n");
			}
			known_symbols_this_scope.insert(t.str);
			line.append(" " + t.str);
			t = token_producer.get_next_Token();
		}
		else if (t.str == "{" && assignment) {
			line.append(read_brace(t, token_producer));
		}
		else if (t.str == "{" && !function_head) {
			output.append(line);
			remove_duplicates_in_scope(t, token_producer, symbol_expr_map, known_symbols_this_scope, output, local_prefix+"\t");
			line = local_prefix;
			output.append(local_prefix + "}\n");
			t = token_producer.get_next_Token();
		}
		else if (t.str == "{" && function_head) {
			function_head = false;
			local_prefix.append("\t");
			output.append(line + " {\n");
			line = local_prefix;
			t = token_producer.get_next_Token();
		}
		else if (t.str == ";") {
			if(line[line.size()-1] == ' ')line.erase(line.size() - 1);
			t = token_producer.get_next_Token();
			output.append(line + ";\n");
			line = local_prefix;
			assignment = false;
		}
		else if (t.str == "for") {
			while (t.str != ")") {
				if (t.str == "(" || t.str == "++" || t.str == "--" || t.str == "[") {
					if (line[line.size()-1] == ' ')line.erase(line.size() - 1);
					line.append(t.str);
					t = token_producer.get_next_Token();
				}
				else {
					line.append(t.str+" ");
					t = token_producer.get_next_Token();
				}
			}
			line.append(" )");
			t = token_producer.get_next_Token();
		}
		else {
			if (t.str == "(" || t.str == "[" || t.str == "++" || t.str == "--"||t.str == ")" || t.str == "]") {
				if (line[line.size()-1] == ' ')line.erase(line.size() - 1);
				line.append(t.str);
				t = token_producer.get_next_Token();
			}
			else {
				if (t.str == "=") {
					assignment = true;
				}
				line.append(t.str+" ");
				t = token_producer.get_next_Token();
			}
		}
	}

}

void Converter::convert_Cpp_to_OCL(std::string *cpp_code, std::string output_path) {

	Tokenizer token_producer{ cpp_code };
	Token t = token_producer.get_next_Token();
	std::string output{ *cpp_code };

	bool line_start{ true };
	std::map<std::string, std::string> symbol_name_to_full_expression;
	int context_count{ 0 };

	while (t.str != "") {
		if (context_count != 0) {
			if (t.str == "{") {
				++context_count;
			}
			else if (t.str == "}") {
				--context_count;
			}
			t = token_producer.get_next_Token();
			if(!line_start)line_start = true;//after the {} block there must be a new line
		}
		else if (line_start && context_count == 0 && t.str == "const") {
			std::string expr{ "const" };
			t = token_producer.get_next_Token();
			if (t.str == "unsigned") {
				expr.append(" " + t.str);
				t = token_producer.get_next_Token();
			}
			//type
			expr.append(" " + t.str);
			t = token_producer.get_next_Token();
			std::string symbol{ t.str }; 
			while (t.str != ";") {
				expr.append(" " + t.str);
				t = token_producer.get_next_Token();
			}
			expr.append(";");
			t = token_producer.get_next_Token();
			symbol_name_to_full_expression[symbol] = expr;
		}
		else {
			line_start = false;
			t = token_producer.get_next_Token();
			if (t.str == ";") {
				t = token_producer.get_next_Token();
				line_start = true;
			}
			else if (t.str == "{") {
				++context_count;
				t = token_producer.get_next_Token();
			}
		}
	}

	std::regex r_const("^\tconst .*?;\n");
	output = std::regex_replace(output, r_const, "");
	output.replace(output.find("//imports"), 9, "");
	output.replace(output.find("//imports end"), 13, "");

	Tokenizer token_producer2{ &output };
	Token token = token_producer2.get_next_Token();

	std::string final_output;
	std::set<std::string> known_symbols;
	while (token.str != "") {
		remove_duplicates_in_scope(token, token_producer2, symbol_name_to_full_expression, known_symbols, final_output);
		final_output.append(" }\n");
		token = token_producer2.get_next_Token();
	}
	
	//replace some possibilities that occur in the samples, but this should actually be done for all key words in OpenCL
	replace_str(final_output, "constant", "constant$replace");
	if (final_output.find("int min(") != std::string::npos || final_output.find("char min(") != std::string::npos || final_output.find("long min(") != std::string::npos || final_output.find("short min(") != std::string::npos || final_output.find("float min(") != std::string::npos || final_output.find("double min(") != std::string::npos) {
		//assume the min function is defined
		replace_str(final_output, " min(", " min$replace(");
		replace_str(final_output, "=min(", "= min$replace(");
	}
	if (final_output.find("int max(") != std::string::npos || final_output.find("char max(") != std::string::npos || final_output.find("long max(") != std::string::npos || final_output.find("short max(") != std::string::npos || final_output.find("float max(") != std::string::npos || final_output.find("double max(") != std::string::npos) {
		//assume the min function is defined
		replace_str(final_output, " max(", " max$replace(");
		replace_str(final_output, "=max(", "= max$replace(");
	}
	

	std::ofstream cl_output{ output_path };
	if (cl_output.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open file " + output_path };
	}
	cl_output << final_output;

}