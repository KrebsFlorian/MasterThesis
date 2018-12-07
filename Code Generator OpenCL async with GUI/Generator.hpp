#pragma once
#include <string>
#include "Unit_Generator.hpp"
#include <map>
#include <utility> 
#include <vector>
#include <utility>  
#include <tuple>
#include <set>
#include <algorithm>
#include <functional>
#include "Action_Buffer.hpp"
#include "FIFO_Expression_Buffer.hpp"
#include "Converter_RVC_Cpp.hpp"
#include <Windows.h>
#include "Converter.hpp"
using namespace Converter_RVC_Cpp;

/*
	Function object to sort actions according to their priority defined in the priority block of the actor.
*/
class comparison_object {
	//(Tag,Name) > (Tag,Name) pairs 
	std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>>& priority_greater_lesser;

public:
	comparison_object(std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>>& _priority_greater_lesser) :priority_greater_lesser(_priority_greater_lesser) {}

	//strings are method names
	bool operator()(std::string& str1, std::string& str2) const {
		for (auto it = priority_greater_lesser.begin(); it != priority_greater_lesser.end(); ++it) {
			//str1 > str2
			if (str1 == it->first.first+"$"+it->first.second && str2 == it->second.first+"$"+it->second.second) { 
				return true;
			}
			//str2 > str1
			else if (str2 == it->first.first+"$"+it->first.second && str1 == it->second.first+"$"+it->second.second ) { 
				return false;
			}
			else if (it->first.second.empty() && it->second.second.empty()) {
				//str1 > str2
				if (str1.find(it->first.first + "$") == 0 && str2.find(it->second.first + "$")==0) {
					return true;
				}
				//str2 > str1
				else if (str2.find(it->first.first + "$")==0 && str1.find(it->second.first + "$")==0) {
					return false;
				}
			}
			else if (!it->first.second.empty() && it->second.second.empty()) {
				//str1 > str2
				if (str1.find(it->first.first + "$" + it->first.second+"$") == 0 && str2.find(it->second.first + "$" ) == 0) {
					return true;
				}
				//str2 > str1
				else if (str2.find(it->first.first + "$" + it->first.second+"$") == 0 && str1.find(it->second.first + "$") == 0) {
					return false;
				}
			}
			else if (it->first.second.empty() && !it->second.second.empty()) {
				//str1 > str2
				if (str1.find(it->first.first + "$") == 0 && str2.find(it->second.first + "$" + it->second.second+"$") == 0) {
					return true;
				}
				//str2 > str1
				else if (str2.find(it->first.first + "$" ) == 0 && str1.find(it->second.first + "$" + it->second.second+"$") == 0) {
					return false;
				}
			}
		}
		return true;
	}
};

/*
	Object to store information about an action
*/
struct Action_Information {
	std::string tag;
	std::string name;
	//method name = tag$name
	std::string method_name;
	//(FIFO Name,required number of tokens to fire once) pairs
	std::vector<std::pair<std::string, int>> input_fifos;
	std::vector<std::pair<std::string, int>> output_fifos;
	std::string guards;
};

/*
	Class to convert RVC code to C++ Code
*/
class Actor_Generator {

	//OpenCL relevant variables
	int actor_count{ 0 };

	std::map<int, std::string> actor_index_to_kernel_name_map;

	std::string cl_file;
	//end

	std::string id;

	std::string actor_name;

	std::string actor_scheduling_condition;

	//needed to insert the values for constant inputs, to minimize the constructor and avoid this reference in OpenCL environments
	Dataflow_Network *dpn;

	int FIFO_size;

	Tokenizer token_producer;

	//true: OpenCL can be used to accelerate the execution of the actor, false: OpenCL cannot be used
	bool OpenCL;

	//true if there are no guard conditions for the actions
	bool no_guards{ true };

	//true:actor contains a FSM with exactly one cycle, false: no FSM or not exactly one cycle in this FSM
	bool cycleFSM{ false };

	//true:actor has an initialization action that has to be called in the constructor. false: no initialization action
	bool call_initialize{ false };

	//string to be added to the constructor body - for e.g. string comprehension
	std::string add_to_constructor{};

	//true: @native methods are used and the native code has to be included, false: no native code is used
	bool include_native{ false };

	//true: fsm defined, false: no fsm defined
	bool FSM{ false };

	//true: Priorities defined, false: no priorities defined
	bool priorities{ false };

	//name for actions without a name: "action_"+default_number_action++
	static int default_number_action; //must be static to avoid kernel name collisions

	//map for all symbols defined in the global and the actor scope, consts are mapped to their value, functions/procedures are mapped to function and the rest is mapped to an empty string
	std::map<std::string, std::string> symbol_definition_map_for_actor{};

	//(type,name) pairs in the order of the constructor parameters
	std::vector<std::pair<std::string, std::string>> constructor_parameters_type_name;

	//maps the variable name to the default parameter
	std::map<std::string, std::string> default_parameter_value_map;

	//maps the fifo name to the type of the elements in the fifo, NOT to the type of the FIFO, because this would be FIFO<...,...>. But the type is needed to use the correct type when elements are read from or written to the fifo
	std::map<std::string, std::string> fifo_name_to_rawType_map;

	//(state,action tag, action name, next state) tuples extracted from the FSM block
	std::vector<std::tuple<std::string, std::string, std::string, std::string>> fsm_state_actionTag_actionName_nextstate;

	// (tag,name) > (tag,name) extracted from the priority block
	//must also contain the transitive closure in case of a>b>c to make sorting possible!!!!
	std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>> priority_greater_tag_name_lesser_tag_name;

	//set of all states defined in the FSM - used for scheduling
	std::set<std::string> states;

	//maps method names to their scheduling condition: guards and size and free space in the FIFOs
	std::map<std::string, std::string> actionMethodName_schedulingCondition_mapping;
	std::map<std::string, std::string> actionMethodName_freeSpaceCondition_mapping;

	//list of all buffered actions, because actions are converted after all other blocks have been converted.
	//Convertion is delayed until the final decision is made if OpenCL can be used or not!!
	std::vector<Action_Buffer> buffered_actions{};

	//if a cyclic fsm is executed in OpenCL, the starting state of the cycle is stored here - the state where the OpenCL execution begins
	std::string start_state;

	//maps variable names containing values read from the fifos to expressions to read them from a OpenCL buffer
	std::map<std::string, std::string> FIFO_access_replace_map_for_OpenCL;

	//set of input FIFOs
	std::set<std::string> input_FIFOs_set;
	//set of output FIFOs
	std::set<std::string> output_FIFOs_set;
	/*
		Function takes a parameter_name as input and searchs the parameter name,value list in dpn object for the corresponding value to the name and returns it.
	*/
	std::string find_value_for_const_input(const std::string parameter_name) {
		for (auto it = dpn->parameters.begin(); it != dpn->parameters.end(); ++it) {
			if (std::get<0>(*it) == id && std::get<1>(*it) == parameter_name) {
				std::string tmp{ std::get<2>(*it) };
				//if it is a list, it cannot be added to the const map, because replacement doesnt work properly
				bool add_to_map{ true };
				//replace [ and ] by { and } 
				while (tmp.find("[") != std::string::npos) {
					tmp = tmp.replace(tmp.find("["), 1, "{");
					add_to_map = false;
				}
				while (tmp.find("]") != std::string::npos) {
					tmp = tmp.replace(tmp.find("]"), 1, "}");
					add_to_map = false;
				}
				//add to const map
				if (add_to_map) { symbol_definition_map_for_actor[parameter_name] = tmp; }
				return tmp;
			}
		}
		//add to const map
		if (default_parameter_value_map[parameter_name].find("[") == std::string::npos) {
			symbol_definition_map_for_actor[parameter_name] = default_parameter_value_map[parameter_name];
		}
		return default_parameter_value_map[parameter_name];
	}

	/*
		Input token contains the string actor
		Input string imports contains c++ code generated from all imported files

		The function generates C++ for a complete actor and returns the code as a string.		
	*/
	std::string convert_actor(Token& t, std::string& imports, std::string& imports_function_declarations) {
		std::string output{ "#pragma once\n#include \"Port.hpp\"\n\n\nclass " };
		t = token_producer.get_next_Token(); //name of the actor
		actor_name = t.str;

		t = token_producer.get_next_Token(); // (
		t = token_producer.get_next_Token(); 
		//process constant actor parameters
		std::string const_parameters;
		bool append_id_to_name{ false };
		while (t.str != ")") {
			append_id_to_name = true;
			if (t.str == ",") {
				t = token_producer.get_next_Token();
			}
			std::string type{ convert_type(t, token_producer,symbol_definition_map_for_actor) };
			std::string name{ t.str };
			symbol_definition_map_for_actor[name] = "";
			const_parameters.append("\tconst " + type + name);
			t = token_producer.get_next_Token();//skip name
			if (t.str == "=") {//default value found!
				t = token_producer.get_next_Token(); //default value
				std::string default_value{ " " };
				while (t.str != ")" && t.str != ",") {
					if (t.str == "[") {//replace [ and ] by { and } because of c++ synthax
						default_value.append("{");
					}
					else if (t.str == "]") {
						default_value.append("}");
					}
					else {
						default_value.append(t.str + " ");
					}
					t = token_producer.get_next_Token();
				}
				default_parameter_value_map[name] = default_value;
			}
			const_parameters.append(" = " + find_value_for_const_input(name) + ";\n");
		}
		t = token_producer.get_next_Token();
		if (append_id_to_name) {
			actor_name.append("$" + id);
		}
		//start building the strings:
		output.append(actor_name + "{\nprivate:\n");
		//source_output.append("#include \"" + actor_name + ".hpp\"\n\n");
		output.append("//imports\n");
		output.append(imports);
		output.append("//imports end\n\n");
		cl_file.append("//imports\n"+imports_function_declarations + imports + "//imports end\n\n");
		output.append("//Const inputs:\n");
		output.append(const_parameters);
		cl_file.append(const_parameters);
		output.append("//Functions and Variables:\n");
		//buffer port or fifo declaration part, to convert it later, because consts defined in the actor can be used here, but this cannot be evaluated because they are not converted by now.
		FIFO_Expression_Buffer fifo_buffer{ t,token_producer };
		//convert variables, constants, function, procedures and decide if OpenCL can be used. Therefore actions are buffered until the rest is processed.
		while (t.str != "end") {
			if (t.str == "int" || t.str == "uint" || t.str == "String" || t.str == "bool" || t.str == "half" || t.str == "float") {
				std::string tmp{ convert_expression(t, token_producer, symbol_definition_map_for_actor,"\t") };
				bool const_expr{ true };
				if (tmp.find("\tconst ") == std::string::npos) {//variable 
					OpenCL = false; //non const state variable -> hard to parallize
					const_expr = false;
				}
				if (tmp.find("{\n") != std::string::npos) { //{\n in the string indicates list comprehension! this must go to the constructor
					std::string list_comprehension = tmp.substr(tmp.find("\t{\n"));
					std::string declaration = tmp.substr(0, tmp.find(";") + 1);
					declaration.append("\n");
					output.append(declaration);
					add_to_constructor.append(list_comprehension);
					OpenCL = false;
				}
				else {
					cl_file.append(tmp);
					output.append(tmp);
				}
			}
			else if (t.str == "List") {
				std::string tmp{ convert_list(t, token_producer,symbol_definition_map_for_actor,"\t") };
				bool const_expr{ true };
				if (tmp.find("\tconst ") == std::string::npos) {//variable 
					OpenCL = false; //non const state variable -> hard to parallize
					const_expr = false;
				}
				if (tmp.find("{\n") != std::string::npos) { //{\n in the string indicates list comprehension! this must go to the constructor
					std::string list_comprehension = tmp.substr(tmp.find("{\n"));
					std::string declaration = tmp.substr(0, tmp.find(";") + 1);
					declaration.append("\n");
					output.append(declaration);
					add_to_constructor.append(list_comprehension);
					OpenCL = false;
				}
				else {
					cl_file.append(tmp);
					output.append(tmp);
				}
			}
			else if (t.str == "@native") {
				convert_nativ_declaration(t, token_producer, "*", include_native, symbol_definition_map_for_actor);
				OpenCL = false; //native methods cannot be parallized
			}
			else if (t.str == "function") {
				std::string tmp{ convert_function(t, token_producer, symbol_definition_map_for_actor, "\t") };
				output.append(tmp);
				cl_file.append(tmp);
				//find declaration and insert it at the beginning of the source string, to avoid linker errors
				int end_of_dekl = tmp.find("{");
				std::string dekl = tmp.substr(0, end_of_dekl-1) + ";\n";
				cl_file.insert(0, dekl);
			}
			else if (t.str == "procedure") {
				std::string tmp{ convert_procedure(t, token_producer, symbol_definition_map_for_actor, "\t") };
				output.append(tmp);
				cl_file.append(tmp);
				//find declaration and insert it at the beginning of the source string, to avoid linker errors
				int end_of_dekl = tmp.find("{");
				std::string dekl = tmp.substr(0, end_of_dekl-1) + ";\n";
				cl_file.insert(0, dekl);
			}
			else if (t.str == "schedule") {//FSM
				std::string tmp{ convert_FSM(t, "\t") };
				output.append(tmp);
			}
			else if (t.str == "priority") {
				convert_priority(t);
			}
			else {//action
				  //actions are buffered because it is not clear at this point if there is a fsm or state variables
				buffered_actions.push_back(Action_Buffer{ t,token_producer,no_guards });
			}
		}
		t = token_producer.get_next_Token();
		//now the fifos declarations can be converted
		output.append(convert_Port_declarations(fifo_buffer));
		//std::cout << output << std::endl << "--------------------------------\n";
		//system("pause");
		//now the actions can be converted
		output.append("//Actions:\n");
		output.append(convert_actions());
		output.append("public:\n");
		output.append(generate_constructor() + "\n");
		output.append(generate_scheduler() + "\n");
		output.append("};\n");

		return output;
	}

	/*
	This function returns a string containg the name of the FIFO connected to the given port of a given actor (id).
	The name of the FIFO is build the following way: source actor id_source actor port_destination actor id_destination actor port.
	Throws an exception if it cannot find the fifo name that is connected to the port of the actor (id).
	*/
	std::string find_fifo_name(std::string port) {
		for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
			if ((it->dst_id == id && it->dst_port + "$FIFO" == port) || (it->src_id == id && it->src_port + "$FIFO" == port)) {
				return it->src_id + "_" + it->src_port + "_" + it->dst_id + "_" + it->dst_port;
			}
		}
		return "";
	}

	/*
		Function to convert the fifo or port declaration stored in a given FIFO_Expression_Buffer.
		Returns the generated code as a string.
	*/
	std::string convert_Port_declarations(FIFO_Expression_Buffer& token_producer) {
		Token t = token_producer.get_next_Token();
		std::string output;
		//start of the FIFO block
		output.append("//Input FIFOs:\n");
		//convert input fifos
		while (t.str != "==>") {
			std::string type{ convert_type(t, token_producer,symbol_definition_map_for_actor) };
			std::string name{ t.str + "$FIFO" };
			input_FIFOs_set.insert(name);
			fifo_name_to_rawType_map[name] = type;
			symbol_definition_map_for_actor[name] = "";
			t = token_producer.get_next_Token();
			if (t.str == ",") {
				t = token_producer.get_next_Token();
			}
			std::string fifo_template_type{ "Port< " + type + ", " + determine_final_fifo_size(name,true) + ">&" };
			output.append("\t" + fifo_template_type + " " + name + ";\n");
			constructor_parameters_type_name.push_back(std::make_pair(fifo_template_type, name));
		}

		t = token_producer.get_next_Token(); //skip ==>
		output.append("//Output FIFOs:\n");
		//convert output fifos
		while (t.str != ":") {
			std::string type{ convert_type(t, token_producer,symbol_definition_map_for_actor) };
			std::string name{ t.str + "$FIFO" };
			output_FIFOs_set.insert(name);
			fifo_name_to_rawType_map[name] = type;
			symbol_definition_map_for_actor[name] = "";
			t = token_producer.get_next_Token();
			if (t.str == ",") {
				t = token_producer.get_next_Token();
			}
			std::string fifo_template_type{ "Port< " + type + ", " + determine_final_fifo_size(name,false) + ">& " };
			output.append("\t" + fifo_template_type + " " + name + ";\n");
			constructor_parameters_type_name.push_back(std::make_pair(fifo_template_type, name));
		}
		return output;
	}

	/*
		Function to determine the FIFO size based on the given fifo size and information that is specified in the network file.
		If the specified fifo size in the network file is bigger than the given fifo size, the number from the network file is taken.
		Otherwise the given fifo size is used.
		The input flag indicates if the fifo is a input (true) or a output (false) fifo.
	*/
	std::string determine_final_fifo_size(std::string port,bool input) {
		//find a possible if that matches the current class string, any id is sufficient that matches these criterias, because is assume in all instances 
		//the fifos have the same size
		//the id is needed to search the connections list for the corresponding entry
		std::string possible_id{};
		for (auto it = dpn->id_class_map.begin(); it != dpn->id_class_map.end(); ++it) {
			if (it->second.find(actor_name) != std::string::npos) {
				possible_id = it->first;
				break;
			}
		}

		//find the corresponding entry in the connections list.
		//if it is a input fifo, the dst of each entry will be checked, if it is a output, the src parts will be checked
		for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
			if (it->dst_id == possible_id && port == it->dst_port + "$FIFO" && input) {
				if (it->specified_size != 0 && it->specified_size*10 > FIFO_size) {
					return std::to_string(it->specified_size*10);
				}
			}
			else if (it->src_id == possible_id && port == it->src_port + "$FIFO" && !input) {
				if (it->specified_size != 0 && it->specified_size*10 > FIFO_size) {
					return std::to_string(it->specified_size*10);
				}
			}
		}
		//if nothing was found, return the given FIFO_size
		return std::to_string(FIFO_size);
	}

	/*
		Convert the buffered actions either with the use of OpenCL if the OpenCL flag is true or without OpenCL if OpenCL is false.
		If OpenCL and cycleFSM are true, every action is converted to pure C++ and additionally one big action contain a complete cycle is generated.
		The function returns a string containing the generated code for all actions.
	*/
	std::string convert_actions() {
		std::string output;
		//map method name to (Action Information, OpenCL code for this action) - OpenCL code is need to generate an action for the whole FSM cycle if possible
		std::map<std::string, std::pair<Action_Information, std::string>> name_to_info_map; 


		bool guards_parallelizable{ false };
		if (OpenCL && !no_guards) {
			guards_parallelizable = actions_with_guards_parallelizable();
		}

		if ((OpenCL && no_guards) || (OpenCL && guards_parallelizable)) {//if opencl is used, add struct instance to store all relevant OpenCL objects
			output.append("\n\topencl_arguments ocl;\n\n");
		}

		if (OpenCL && !no_guards && guards_parallelizable) {
			output.append(generate_OpenCL_code_with_guards("\t"));
			return output;
		}
		for (auto it = buffered_actions.begin(); it != buffered_actions.end(); ++it) {
			it->reset_buffer();
			Action_Information act_inf;
			if (cycleFSM && OpenCL && no_guards) {
				output += convert_action(*it, act_inf, "\t");
				if (act_inf.method_name != "initialize$") {//initialize = no OpenCL, because only fired once
					it->reset_buffer(); //rest the buffer to the start to convert it to OpenCL to generate an action for the whole FSM cycle
					std::string OpenCL_string;
					OpenCL_string = convert_action_cyclic_fsm_OpenCL(*it, "\t");
					name_to_info_map[act_inf.method_name] = std::make_pair(act_inf, OpenCL_string);
				}
			}
			else if (OpenCL && no_guards) { //pure OpenCL without cyclic fsm
				output += convert_action_opencl(*it, act_inf, "\t");
			}
			else {// not OpenCL or OpenCL!
				output += convert_action(*it, act_inf, "\t");
			}
		}

		if (cycleFSM && OpenCL && no_guards) {
			output.append(build_FSM_Cycle_function(name_to_info_map, "\t"));
		}
		else if (OpenCL && !no_guards && !guards_parallelizable) {//in this case no OpenCL code has been generated despite the OpenCL flag is true, therefore the flag is adjusted to this here
			OpenCL = false;
		}

		return output;
	}
	//--------------------------generate code for parallel action execution with guards-------------------------

	/*
	This function checks if all actions except the initialization function consume and produce the same amount of tokens for each FIFO.
	If this is the case the function returns true, otherwise it returns false.

	*/
	bool actions_with_guards_parallelizable() {
		std::vector<Action_Information> action_informations;
		//test if the actions can be parallelized despite guard conditions are present
		for (auto it = buffered_actions.begin(); it != buffered_actions.end(); ++it) {
			auto act_inf = get_fifo_information(*it);
			if (act_inf.name != "initialize") {
				action_informations.push_back(act_inf);
			}
		}
		bool is_parallelizable{ true };
		for (auto act_inf_it = action_informations.begin(); act_inf_it != action_informations.end(); ++act_inf_it) {
			for (auto act_inf_it_inner = action_informations.begin(); act_inf_it_inner != action_informations.end(); ++act_inf_it_inner) {
				if (act_inf_it->input_fifos.size() != act_inf_it_inner->input_fifos.size() || act_inf_it->output_fifos.size() != act_inf_it_inner->output_fifos.size()) {
					is_parallelizable = false;
					break;
				}
				for (auto input_it = act_inf_it->input_fifos.begin(); input_it != act_inf_it->input_fifos.end(); ++input_it) {
					bool found{ false };
					for (auto input_it_inner = act_inf_it_inner->input_fifos.begin(); input_it_inner != act_inf_it_inner->input_fifos.end(); ++input_it_inner) {
						if (input_it->first == input_it_inner->first && input_it->second != input_it_inner->second) {
							is_parallelizable = false;
							found = true;
							break;
						}
						else if (input_it->first == input_it_inner->first && input_it->second == input_it_inner->second) {
							found = true;
						}

					}
					if (!found) {
						is_parallelizable = false;
						break;
					}
				}
				for (auto output_it = act_inf_it->output_fifos.begin(); output_it != act_inf_it->output_fifos.end(); ++output_it) {
					bool found{ false };
					for (auto output_it_inner = act_inf_it_inner->output_fifos.begin(); output_it_inner != act_inf_it_inner->output_fifos.end(); ++output_it_inner) {
						if (output_it->first == output_it_inner->first && output_it->second != output_it_inner->second) {
							is_parallelizable = false;
							found = true;
							break;
						}
						else if (output_it->first == output_it_inner->first && output_it->second == output_it_inner->second) {
							found = true;
						}
					}
					if (!found) {
						is_parallelizable = false;
						break;
					}
				}
			}
		}
		return is_parallelizable;
	}

	/*
	This function reads how many tokens are produced and consumed for each FIFO and stores this information in an Action_Information Object, what is returned.
	*/
	Action_Information get_fifo_information(Action_Buffer& token_producer) {
		Action_Information act_inf;
		std::map<std::string, std::string> local_map;
		//part before the FIFOs
		Token t = token_producer.get_next_Token();
		if (t.str == "initialize") {
			act_inf.name = "initialize";
			return act_inf;
		}
		while (t.str != "action" && t.str != "initialize") {
			t = token_producer.get_next_Token();
		}
		if (t.str == "initialize") {
			act_inf.name = "initialize";
			return act_inf;
		}
		t = token_producer.get_next_Token();
		//Input FIFOs
		while (t.str != "==>") {
			int preview_index{ 0 };
			std::string name{ t.str + "$FIFO" };
			t = token_producer.get_next_Token();// :
			t = token_producer.get_next_Token(); // [
			std::vector<std::pair<std::string, bool>> consumed_element_names; // store elements first, because there can be a repeat
			t = token_producer.get_next_Token(); //start of token name part
			while (t.str != "]") {
				std::string expr{ " " };
				bool first{ true };
				bool already_known_variable{ false };
				while (t.str != "," && t.str != "]") {
					if (first) {//check if variable name is already known and therefore there is no need to define it again!
						first = false;
						if (symbol_definition_map_for_actor.find(t.str) != symbol_definition_map_for_actor.end()) {
							already_known_variable = true;
						}
					}
					if (t.str == "[") {
						while (t.str != "]") {
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						expr.append(t.str);
						t = token_producer.get_next_Token();
					}
					else {
						expr.append(t.str);
						t = token_producer.get_next_Token();
					}
				}
				if (t.str == ",") {
					t = token_producer.get_next_Token();
				}
				consumed_element_names.push_back(std::make_pair(expr, already_known_variable));
			}
			t = token_producer.get_next_Token(); // drop ]
			if (t.str == "repeat") {
				t = token_producer.get_next_Token(); //repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "==>") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);
				act_inf.input_fifos.push_back(std::make_pair(name, repeat_count*consumed_element_names.size()));
			}
			else {//no repeat, every element in the fifo brackets consumes one element
				act_inf.input_fifos.push_back(std::make_pair(name, consumed_element_names.size()));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be ==> and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		//part between the FIFOs
		t = token_producer.get_next_Token(); //get next token, that should contain first output fifo name, if any are accessed
											 //part of the output FIFO
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {
			std::string name{ t.str + "$FIFO" };
			std::vector<std::string> output_fifo_expr;
			t = token_producer.get_next_Token(); //:
			t = token_producer.get_next_Token(); //[
			t = token_producer.get_next_Token();
			if (t.str == "[") {
				while (t.str != "]") {
					t = token_producer.get_next_Token();
				}
				t = token_producer.get_next_Token();//drop last ] of the list comprehension
			}
			else {
				while (t.str != "]") {
					std::string expr{ " " };
					while (t.str != "," && t.str != "]") {
						if (t.str == "[") {
							while (t.str != "]") {
								expr.append(t.str);
								t = token_producer.get_next_Token();
							}
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						else {
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
					}
					if (t.str == ",") {
						t = token_producer.get_next_Token();
					}
					output_fifo_expr.push_back(expr);
				}
			}
			t = token_producer.get_next_Token();
			if (t.str == "repeat") {
				t = token_producer.get_next_Token();//contains repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "do" && t.str != "var" && t.str != "end" && t.str != "guard") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);
				act_inf.output_fifos.push_back(std::make_pair(name, repeat_count*output_fifo_expr.size()));
			}
			else {
				act_inf.output_fifos.push_back(std::make_pair(name, output_fifo_expr.size()));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be var or do  and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		token_producer.reset_buffer();
		return act_inf;
	}

	/*
	This function generates a function that uses OpenCL to execute actions in parallel.
	But it can only be used if all actions consume and produce the same amount of tokens. (Use function actions_with_guards_parallelizable first)
	This functions creates one sycl kernel for all actions. This kernel can fire exactly one action due to the scheduling conditions that are checked before execution.
	The created code is returned as a string.

	*/
	std::string generate_OpenCL_code_with_guards(std::string prefix = "") {
		std::map<std::string, std::string> name_to_code_map;
		std::vector<std::string> actions;
		Action_Information act_inf;
		std::string output;
		std::string prefix_for_actions = "\t";
		std::string cl_function_head;
		std::string end_of_output;
		for (auto it = buffered_actions.begin(); it != buffered_actions.end(); ++it) {
			act_inf = Action_Information{};
			(*it).reset_buffer();

			auto r = convert_action_to_OpenCL_Code(*it, act_inf, prefix_for_actions);
			if (r.first != "initialize") {
				name_to_code_map[r.first] = r.second;
				actions.push_back(r.first);
			}
		}
		//add to actor_scheduling_condition string
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name + ".get_size() >= " + std::to_string(it->second));
			}
		}
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name + ".get_free_space() >= " + std::to_string(it->second));
			}
		}
		//sort according to priorities!
		if (priorities) {
			std::sort(actions.begin(), actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
		}
		//create function declaration 
		output.append(prefix+"void action_" + actor_name + "(cl_event *event) {\n");
		cl_function_head.append("__kernel action_" + actor_name + "(");
		actor_index_to_kernel_name_map[actor_count] = "action_"+actor_name;
		//insert action into the actionName Scheduling Condition map, otherwise the scheduler doesn't recognize it
		std::string condition;
		bool something_inserted_to_condition{ false };
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (!something_inserted_to_condition) {
				something_inserted_to_condition = true;
				condition.append(it->first + "$FIFO.get_size() >= " + std::to_string(it->second));
			}
			else {
				condition.append(" && ");
				condition.append(it->first + "$FIFO.get_size() >= " + std::to_string(it->second));
			}
		}
		std::string condition_freeSpace;
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (condition_freeSpace.empty()) {
				condition_freeSpace.append(it->first + "$FIFO.get_free_space() >= " + std::to_string(it->second));
			}
			else {
				condition_freeSpace.append(" && ");
				condition_freeSpace.append(it->first + "$FIFO.get_free_space() >= " + std::to_string(it->second));
			}
		}
		actionMethodName_schedulingCondition_mapping["action_" + actor_name] = condition;
		actionMethodName_freeSpaceCondition_mapping["action_" + actor_name] = condition_freeSpace;
		//create sycl environment
		output.append(prefix + "\tint number$instances = std::min({" );
		bool first_calc_add{ true };
		int kernel_arg_counter{ 0 };
		std::string buffer_access{};
		//create indices to access the buffers/accessors
		std::string indices{};
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (!first_calc_add) {
				output.append(" , ");
			}
			else {
				first_calc_add = false;
			}
			output.append(it->first + ".get_size()/ " + std::to_string(it->second));

			cl_function_head.append("__global " + fifo_name_to_rawType_map[it->first] + " *" + it->first + "$ptr ,");
			buffer_access.append(prefix + "\tclSetKernelArg(ocl.kernels[" + std::to_string(actor_count) + "] , " + std::to_string(kernel_arg_counter++) + ", sizeof(cl_mem), (void *)" + it->first + ".get_read_buffer(number$instances * " + std::to_string(it->second) + ",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_read_done();\n");
		}
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (!first_calc_add) {
				output.append(" , ");
			}
			else {
				first_calc_add = false;
			}
			output.append(it->first + ".get_free_space()/ " + std::to_string(it->second));

			cl_function_head.append("__global " + fifo_name_to_rawType_map[it->first] + " *" + it->first + "$ptr ,");
			buffer_access.append(prefix + "\tclSetKernelArg(ocl.kernels[" + std::to_string(actor_count) + "] , " + std::to_string(kernel_arg_counter++) + ", sizeof(cl_mem), (void *)" + it->first + ".get_write_buffer(number$instances * " + std::to_string(it->second) + ",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_write_done(ocl);\n");

		}
		//finish number of instances calc expression
		output.append("});\n");
		//remove last komma and insert closing bracket of the ocl function
		cl_function_head.erase(cl_function_head.size() - 1);
		cl_function_head.append("){\n");
		cl_file.append(cl_function_head);
		cl_file.append(indices);
		//set up OpenCL environment
		output.append(prefix + "\tocl.globalWorkSize[0] = number$instances;\n");
		output.append(prefix + "\tocl.work_Dim = 1;\n");
		output.append(prefix + "\tocl.localWorkSize[0] = NULL;\n");
		output.append(buffer_access);
		output.append(prefix + "\tExecuteKernel(&ocl," + std::to_string(actor_count) + ", event);\n");
		output.append(end_of_output);
		output.append(prefix + "}\n\n");
		//place the opencl code in there
		for (auto it = actions.begin(); it != actions.end(); ++it) {
			std::string code = name_to_code_map[*it];
			if (it != actions.begin()) {
				code.insert(code.find("if"), "else ");
			}
			cl_file.append(code);
		}
		cl_file.append("}\n\n");
		++actor_count;

		return output;
	}

	/*
	The function converts a RVC action to C++. The guards are checked with a if statement and in the if body the rest of the action is placed.
	As a result the method name (Tag$Name) and the produced code is returned as a pair.

	*/
	std::pair<std::string, std::string> convert_action_to_OpenCL_Code(Action_Buffer &token_producer, Action_Information& act_inf, std::string prefix = "") {
		Token t = token_producer.get_next_Token();
		std::map<std::string, std::string> local_map;
		std::string output{};
		std::string end_of_output;
		std::string action_tag;
		std::string action_name{ "" };
		std::string method_name;
		std::string condition;
		//Find action name and tag and generate method name, fill action information object
		if (t.str == "action") {//action has no name
			action_tag = "action";
			action_name = std::to_string(default_number_action++); //give the action a default name to use it as function name
		}
		else if (t.str == "initialize") {
			token_producer.reset_buffer();
			return std::make_pair("initialize", convert_action(token_producer, act_inf, "\t")); //no need to convert it to a sycl action because it will be executed once anyhow
		}
		else {
			action_tag = t.str;
			t = token_producer.get_next_Token(); // : or .
			if (t.str == ".") {
				t = token_producer.get_next_Token();//name
				while (t.str != ":") {
					if (t.str == ".") {
						action_name.append("$");
					}
					else {
						action_name.append(t.str);
					}
					t = token_producer.get_next_Token();
				}
			}
			t = token_producer.get_next_Token(); // action
			if (t.str == "initialize") {// init action can have a name, dont know why
				return std::make_pair("initialize", convert_action(token_producer, act_inf, "\t")); //no need to convert it to a sycl action because it will be executed once anyhow
			}
		}
		method_name = action_tag + "$" + action_name;
		symbol_definition_map_for_actor[method_name] = "";
		act_inf.tag = action_tag;
		act_inf.name = action_name;
		act_inf.method_name = method_name;
		//read input FIFOs
		t = token_producer.get_next_Token();//skip action
		while (t.str != "==>") {
			output += convert_input_FIFO_access_opencl(t, token_producer, act_inf, local_map, prefix + "\t");
		}
		t = token_producer.get_next_Token();
		//read output FIFOs
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {//output fifos
			end_of_output.append(convert_output_FIFO_access_opencl(t, token_producer, act_inf, local_map, prefix + "\t"));
		}
		//read guards
		if (t.str == "guard") {
			t = token_producer.get_next_Token();
			while (t.str != "var" && t.str != "do" && t.str != "end") {
				if (t.str == ",") {
					condition.append(" && ");
					t = token_producer.get_next_Token();
				}
				else if (t.str == "=") {
					condition.append(" == ");
					t = token_producer.get_next_Token();
				}
				else {
					if (FIFO_access_replace_map_for_OpenCL.find(t.str) != FIFO_access_replace_map_for_OpenCL.end()) {
						condition.append(FIFO_access_replace_map_for_OpenCL[t.str]);
						t = token_producer.get_next_Token();
						if (t.str == "[") {//must be part of a repeat list/array 
							t = token_producer.get_next_Token();
							std::string bracket_content;
							while (t.str != "]") {
								bracket_content.append(t.str);
								t = token_producer.get_next_Token();
							}
							condition.append(bracket_content + "]");
							t = token_producer.get_next_Token();
						}
						condition.append(" ");
					}
					else {
						condition.append(t.str);
						t = token_producer.get_next_Token();
					}
				}
			}
			act_inf.guards = condition;
		}
		else {
			condition = "true";
		}
		//convert action body
		output.append(convert_action_body(t, token_producer, local_map, prefix + "\t"));
		output.insert(0, prefix + "if(" + condition + ") {\n");
		end_of_output.append(prefix + "}\n");
		return std::make_pair(method_name, output + end_of_output);
	}
	//----------------------------- generate Cpp and OpenCL code for cyclic FSMs ---------------------------------
	/*
	This function builds the action containing a complete FSM cycle accelerated by OpenCL.
	The map where the method name is mapped to the (action information, OpenCL code for the action) pair is used here.
	Prefix is the prefix that is inserted before each new line of code to achieve a correct format.
	The function returns the code to execute a complete FSM cycle.
	*/
	std::string build_FSM_Cycle_function(std::map<std::string, std::pair<Action_Information, std::string>>& map, std::string prefix) {
		std::map< std::string, int > input_fifo_to_number_tokens;
		std::map< std::string, int > output_fifo_to_number_tokens;
		//aggregate the number of tokens consumed or produced per fifo
		for (auto it = states.begin(); it != states.end(); ++it) {
			std::vector<std::string> schedulable_actions = find_schedulable_actions(*it);
			if (priorities) {
				std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
			}
			Action_Information act_inf = map[schedulable_actions.front()].first;
			for (auto act_inf_it = act_inf.input_fifos.begin(); act_inf_it != act_inf.input_fifos.end(); ++act_inf_it) {
				input_fifo_to_number_tokens[act_inf_it->first] = input_fifo_to_number_tokens[act_inf_it->first] + act_inf_it->second;
			}
			for (auto act_inf_it = act_inf.output_fifos.begin(); act_inf_it != act_inf.output_fifos.end(); ++act_inf_it) {
				output_fifo_to_number_tokens[act_inf_it->first] = output_fifo_to_number_tokens[act_inf_it->first] + act_inf_it->second;
			}

		}
		//create method code
		std::string output{ prefix + "void FSM$Cycle$"+actor_name+"(cl_event *event){\n" };
		output.append(prefix + "\tint number$instances = std::min({");
		std::string number_inst_condition;
		std::string scheduling_condtion;
		//build expression to find the number of parallel instances and build buffer_access calls and the index calculations
		//each FIFO sycl accessor is created with a get_buffer call to the fifo and get_access to this buffer.
		//the number of instanes is calculated by taking the min of all FIFOs/consumed/produced elements in one complete cycle (calculated above)
		//additionally, each fifo accessors gets an index = sycl id * aggregated number of tokens that is used to access the accessors
		std::string buffer_access;
		std::string indices;
		std::string cl_function_head{ "__kernel void FSMCycle(" };
		actor_index_to_kernel_name_map[actor_count] = "FSMCycle";
		std::string end_of_output;
		int kernel_arg_counter{ 0 };
		for (auto it = input_fifo_to_number_tokens.begin(); it != input_fifo_to_number_tokens.end(); ++it) {//pair<fifo_name,#tokens>
			if (number_inst_condition.size() == 0) {
				number_inst_condition.append(it->first + ".get_size()/ " + std::to_string(it->second));
				scheduling_condtion.append(it->first + "get.size() >= " + std::to_string(it->second));
			}
			else {
				number_inst_condition.append(", " + it->first + ".get_size()/ " + std::to_string(it->second));
				scheduling_condtion.append(" && " + it->first + "get.size() >= " + std::to_string(it->second));
			}

			cl_function_head.append("__global " + fifo_name_to_rawType_map[it->first] + " *" + it->first + "$ptr ,");
			buffer_access.append(prefix + "\tclSetKernelArg(ocl.kernels[" + std::to_string(actor_count) + "] , " + std::to_string(kernel_arg_counter++) + ", sizeof(cl_mem), (void *)" + it->first + ".get_read_buffer(number$instances * " + std::to_string(it->second) + ",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_read_done();\n");
		}
		std::string scheduling_condition_freeSpace;
		for (auto it = output_fifo_to_number_tokens.begin(); it != output_fifo_to_number_tokens.end(); ++it) {
			if (number_inst_condition.size() == 0) {
				number_inst_condition.append(it->first + ".get_free_space()/ " + std::to_string(it->second));
			}
			else {
				number_inst_condition.append(", " + it->first + ".get_free_space()/ " + std::to_string(it->second));
			}
			if (scheduling_condition_freeSpace.empty()) {
				scheduling_condition_freeSpace.append(it->first + "get_free_space() >= " + std::to_string(it->second));
			}
			else {
				scheduling_condition_freeSpace.append(" && " + it->first + "get_free_space() >= " + std::to_string(it->second));
			}
			
			cl_function_head.append("__global " + fifo_name_to_rawType_map[it->first] + " *" + it->first + "$ptr ,");
			buffer_access.append(prefix + "\tclSetKernelArg(ocl.kernels[" + std::to_string(actor_count) + "] , " + std::to_string(kernel_arg_counter++) + ", sizeof(cl_mem), (void *)" + it->first + ".get_write_buffer(number$instances * " + std::to_string(it->second) + ",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_write_done(ocl);\n");

		}
		output.append(number_inst_condition + "});\n");
		//create OpenCL call - go through one complete cycle and read the previously generated sycl code from the map and append it to the code of the previously read actions.
		std::string OpenCL_Code;
		for (auto state_it = states.begin(); state_it != states.end(); ++state_it) {
			if (state_it == states.begin()) {
				start_state = *state_it;
			}
			std::vector<std::string> schedulable_actions = find_schedulable_actions(*state_it);
			if (priorities) {
				std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
			}
			OpenCL_Code.append(map[schedulable_actions.front()].second);
		}
		//remove last komma and insert closing bracket of the ocl function
		cl_function_head.erase(cl_function_head.size() - 1);
		cl_function_head.append("){\n");
		cl_file.append(cl_function_head);
		cl_file.append(indices);
		//set up OpenCL environment
		output.append(prefix + "\tocl.globalWorkSize[0] = number$instances;\n");
		output.append(prefix + "\tocl.work_Dim = 1;\n");
		output.append(prefix + "\tocl.localWorkSize[0] = NULL;\n");
		output.append(buffer_access);
		output.append(prefix + "\tExecuteKernel(&ocl," + std::to_string(actor_count) + ", event);\n");
		output.append(end_of_output);
		output.append(prefix + "}\n\n");
		//convert the body of the action
		cl_file.append(OpenCL_Code);
		cl_file.append("}\n\n");
		//insert scheduling condition into the maps
		actionMethodName_schedulingCondition_mapping["FSM$Cycle$" + actor_name] = scheduling_condtion;
		actionMethodName_freeSpaceCondition_mapping["FSM$Cycle$" + actor_name] = scheduling_condition_freeSpace;
		++actor_count;
		return output;
	}

	/*
	This function takes an action buffer (index at 0) and a prefix for each line of code and generates code to be used in a sycl environment.
	This code will be placed later on in the body of the FSM Cycle function.
	Therefore, no function head is created. Only read from the fifos, perform some actions, write to fifos.
	The function returns the generated code as a string. The action buffer is pointing to the end of the action after this function.
	*/
	std::string convert_action_cyclic_fsm_OpenCL(Action_Buffer& token_producer, std::string prefix = "") { //returns the body of the opencl kernel
		std::map<std::string, std::string> local_map;
		Token t = token_producer.get_next_Token();
		Action_Information act_inf;
		std::string output;
		output.append(prefix+"{\n");
		std::string end_of_output;
		while (t.str != "action") {
			t = token_producer.get_next_Token();
		}
		t = token_producer.get_next_Token(); // action -> skip, not relevant because no function is created!
		while (t.str != "==>") {
			output += convert_input_FIFO_access_opencl(t, token_producer, act_inf, local_map, prefix+"\t");
		}
		t = token_producer.get_next_Token();
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {//output fifos
			end_of_output.append(convert_output_FIFO_access_opencl(t, token_producer, act_inf, local_map, prefix+"\t"));
		}
		if (t.str == "guard") {//this case shouldn't happen, because the opencl flag should be false in this case!!!
			while (t.str != "end" && t.str != "do") {
				t = token_producer.get_next_Token();
			}
		}
		output.append(convert_action_body(t, token_producer, local_map, prefix+"\t"));
		end_of_output.append(prefix+"}\n");
		return output + end_of_output;
	}

	//-------------------------Generate Cpp and OpenCL code ------------------------------------------

	/*
		This function takes a action buffer with its index pointing to the start of the buffer, a action information object for this action
		and the prefix for each line of code as arguments.
		It converts the RVC code to C++ + OpenCL framework code.
		The action name, tag and the method name of the generated method and the accessed fifos along with there consumed/produced number of tokens in one run of the action
		is inserted into the action information object. 
		It is assumed that no guard for this action exists because guards would refer to actor variables, but they cannot exists if the OpenCL flag is true.
		The function returns the generated code as a string.
		Additionally, the scheduling condition for this action is inserted into the (method name, sched.cond.) map of the class.
	*/
	std::string convert_action_opencl(Action_Buffer& token_producer, Action_Information& act_inf, std::string prefix = "") {
		std::map<std::string, std::string> local_symbol_map;
		std::string output{  };
		std::string end_of_output;
		std::string end_of_cl_file;
		std::string cl_code;
		std::string cl_code_input_fifo;
		std::string cl_function_head{ "__kernel void " };
		Token t = token_producer.get_next_Token();
		std::string action_tag;
		std::string action_name{ "" };
		std::string method_name;
		std::string condition;
		//Find action name and tag and generate method name, fill action information object
		if (t.str == "action") {//action has no name
			action_tag = "action";
			action_name = std::to_string(default_number_action++); //give the action a default name to use it as function name
		}
		else if (t.str == "initialize") {
			token_producer.reset_buffer();
			return convert_action(token_producer, act_inf, prefix); //no need to convert it to a OpenCL action because it will be executed once anyhow
		}
		else {
			action_tag = t.str;
			t = token_producer.get_next_Token(); // : or .
			if (t.str == ".") {
				t = token_producer.get_next_Token();//name
				while (t.str != ":") {
					if (t.str == ".") {
						action_name.append("$");
					}
					else {
						action_name.append(t.str);
					}
					t = token_producer.get_next_Token();
				}
			}
			t = token_producer.get_next_Token(); // action
			if (t.str == "initialize") {// init action can have a name, dont know why
				return convert_action(token_producer, act_inf, prefix); //no need to convert it to a OpenCL action because it will be executed once anyhow
			}
		}
		method_name = action_tag + "$" + action_name;
		symbol_definition_map_for_actor[method_name] = "";
		act_inf.tag = action_tag;
		act_inf.name = action_name;
		act_inf.method_name = method_name;
		//create function head
		output.append(prefix+"void " + method_name + "(cl_event *event){\n");
		cl_function_head.append(method_name + "(");
		actor_index_to_kernel_name_map[actor_count] = method_name;
		//continue conversion
		t = token_producer.get_next_Token();//start of the fifo part
		//input fifos
		while (t.str != "==>") {
			cl_code_input_fifo.append(convert_input_FIFO_access_opencl(t, token_producer, act_inf, local_symbol_map, prefix + ""));
		}
		t = token_producer.get_next_Token();//skip ==>
		//output fifos
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {
				end_of_cl_file.append(convert_output_FIFO_access_opencl(t, token_producer, act_inf, local_symbol_map, prefix + ""));
		}
		//add to actor_scheduling_condition string
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name+".get_size() >= " + std::to_string(it->second));
			}
		}
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name+".get_free_space() >= " + std::to_string(it->second));
			}
		}
		//add scheduling conditions regarding the space of the fifos to conditions and create expression to calculate the number of instances and create the expression to get the OpenCL buffers from the FIFOs
		//the number of instances is the minimum of FIFO.size/consumed elements or FIFO.free_space/produces elements over all FIFOs
		//The FIFO accessors in OpenCL are generated with fifo name.get_buffer.get_access
		//Additionally, every FIFO gets a index = OpenCL id * consumed or produced number of tokens per iteration
		std::string calc_instances_expr{ prefix + "\tint number$instances = std::min({" };
		bool first_calc_add{ true };
		std::string buffer_access{};
		//create indices to access the buffers/accessors
		std::string indices{};
		int kernel_arg_counter{ 0 };
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (condition.size() != 0) {
				condition.append(" && ");
			}
			if (!first_calc_add) {
				calc_instances_expr.append(" , ");
			}
			else {
				first_calc_add = false;
			}
			condition.append(it->first + ".get_size() >= " + std::to_string(it->second));
			calc_instances_expr.append(it->first + ".get_size()/ " + std::to_string(it->second));
			//
			cl_function_head.append("__global "+fifo_name_to_rawType_map[it->first]+" *"+it->first+"$ptr ,");
			buffer_access.append(prefix+"\tclSetKernelArg(ocl.kernels["+std::to_string(actor_count)+"] , "+ std::to_string(kernel_arg_counter++)+ ", sizeof(cl_mem), (void *)"+ it->first+".get_read_buffer(number$instances * "+ std::to_string(it->second)+",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_read_done();\n");
		}
		std::string scheduling_condition_freeSpace;
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (!first_calc_add) {
				calc_instances_expr.append(" , ");
			}
			else {
				first_calc_add = false;
			}
			if (scheduling_condition_freeSpace.empty()) {
				scheduling_condition_freeSpace.append(it->first + ".get_free_space() >= " + std::to_string(it->second));
			}
			else {
				scheduling_condition_freeSpace.append(" && "+it->first + ".get_free_space() >= " + std::to_string(it->second));
			}
			calc_instances_expr.append(it->first + ".get_free_space()/ " + std::to_string(it->second));

			cl_function_head.append("__global " + fifo_name_to_rawType_map[it->first] + " *" + it->first + "$ptr ,");
			buffer_access.append(prefix + "\tclSetKernelArg(ocl.kernels[" + std::to_string(actor_count) + "] , " + std::to_string(kernel_arg_counter++) + ", sizeof(cl_mem), (void *)" + it->first + ".get_write_buffer(number$instances * " + std::to_string(it->second) + ",ocl));\n");
			indices.append("\tint " + it->first + "$index = get_global_id(0) * " + std::to_string(it->second) + ";\n");
			//end_of_output.append(prefix + "\t" + it->first + ".opencl_write_done(ocl);\n");
		}
		//remove last komma and insert closing bracket of the ocl function
		cl_function_head.erase(cl_function_head.size() - 1);
		cl_function_head.append("){\n");
		cl_file.append(cl_function_head);
		cl_file.append(indices);
		cl_file.append(cl_code_input_fifo);
		//calculate number of instances
		output.append(calc_instances_expr + "});\n");
		//set up OpenCL environment
		output.append(prefix + "\tocl.globalWorkSize[0] = number$instances;\n");
		output.append(prefix + "\tocl.work_Dim = 1;\n");
		output.append(prefix + "\tocl.localWorkSize[0] = NULL;\n");
		output.append(buffer_access);
		output.append(prefix + "\tExecuteKernel(&ocl," + std::to_string(actor_count) + ", event);\n");
		output.append(end_of_output);
		//convert the body of the action
		cl_file.append(convert_action_body(t, token_producer, local_symbol_map, "\t"));
		cl_file.append(end_of_cl_file);
		cl_file.append("}\n\n");

		if (condition.size() == 0) {
			condition = "true";
		}
		actionMethodName_schedulingCondition_mapping[method_name] = condition;
		actionMethodName_freeSpaceCondition_mapping[method_name] = scheduling_condition_freeSpace;
		//increment action counter
		++actor_count;
		return output + prefix + "}\n";
	}

	/*
		This function has to be used if something like: int a[2] = b<3?[5,b]:[0,0] is used in the part of the action where the ports/FIFOs are accessed.
		C++ doesn't allow list assignments without initialization. Thus, this must be converted to a for loop surrounded by condition checking (if...else...).
		The result is returned as a string and is meant to be used only in OpenCL kernels. Otherwise the way the FIFOs/Ports are accessed won't work.
	*/
	std::string convert_inline_if_with_list_assignment_opencl(Token& t, Token_Container& token_producer, std::map<std::string, std::string>& global_map, std::map<std::string, std::string>& local_map, std::string prefix, std::string fifo_name) {
		std::string condition;
		std::string expression1;
		std::string expression2;
		std::string output;
		//condition
		while (t.str != "?") {
			condition.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		t = token_producer.get_next_Token(); // skip "?"
		//if body
		int count{ 1 };
		bool nested{ false };
		while (count != 0) {
			if (t.str == "?") {
				++count;
				nested = true;
			}
			else if (t.str == ":") {
				--count;
			}
			expression1.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		expression1.erase(expression1.size() - 2, 2);//remove last :
		if (nested) {//found another ...?...:... in the if body -> recursive function call of this part 
			Tokenizer tok{ expression1,true };
			Token tok_token = tok.get_next_Token();
			expression1 = convert_inline_if_with_list_assignment_opencl(tok_token,tok, global_map, local_map, prefix + "\t", fifo_name);
		}
		else {
			//auf listenzuweiseung prüfen, wenn ja mit convert_list_comprehension in C++ Code umwandeln
			if (expression1[0] == '[') {
				Tokenizer tok{ expression1,true };
				Token tok_token = tok.get_next_Token();
				expression1 = convert_list_comprehension_opencl(tok_token,tok, fifo_name, prefix + "\t",global_map,local_map);
			}
			else {
				expression1 = prefix + "\t" + fifo_name + "$ptr[" + fifo_name + "$index++] =" + expression1 + ";\n";
			}
		}
		//ELSE Part

		count = 1;
		nested = false;
		while (count != 0) {
			if (t.str == "?") {
				++count;
				nested = true;

			}
			else if (t.str == ":") {
				--count;
			}
			else if (t.str == "") {
				break;
			}
			expression2.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		if (nested) { //found another ...?...:... in the else part -> recursive call
			Tokenizer tok{ expression2,true };
			Token tok_token = tok.get_next_Token();
			expression2 = convert_inline_if_with_list_assignment_opencl(tok_token,tok, global_map, local_map, prefix + "\t", fifo_name);
		}
		else {
			//auf listenzuweiseung prüfen, wenn ja mit convert_list_comprehension in C++ Code umwandeln
			if (expression2[0] == '[') {
				Tokenizer tok{ expression2,true };
				Token tok_token = tok.get_next_Token();
				expression2 = convert_list_comprehension_opencl(tok_token,tok, fifo_name, prefix + "\t", global_map,local_map);
			}
			else {
				expression2 = prefix + "\t" + fifo_name + "$ptr[" + fifo_name + "$index++] =" + expression2 + ";\n";
			}
		}

		//Build expression
		output.append(prefix + "if(" + condition + ") {\n");
		output.append(expression1);
		output.append(prefix + "}\n");
		output.append(prefix + "else {\n");
		output.append(expression2);
		output.append(prefix + "}\n");
		return output;
	}

	/*
	This function takes a token pointing at the start of the list comprehension, the corresponding Action_Buffer,
	the name of the FIFO the list comprehension is used for and the prefix for each line of code as arguments.
	This function is only used for list comprehensions that occur in FIFOs, all other list comprehensions are handled in the Converter_RVC_Cpp.
	The list comprehension is converted to a loop or multiple loops in C++ with OpenCL.
	The assumption is, list comprehension can only occur in output FIFOs, because in input FIFOs it wouldn't make any sense.
	The function returns the generated C++ as a string.
	After the function the Token reference should point at the closing ] of the list comprehension.
	*/
	std::string convert_list_comprehension_opencl(Token& t, Token_Container& token_producer, std::string fifo_name, std::string prefix, std::map<std::string, std::string>& global_map, std::map<std::string, std::string>& local_map) {
		std::string output;
		t = token_producer.get_next_Token();
		std::string command{ "\t" + fifo_name + "$ptr[" + fifo_name + "$index++] =" };//wenn das hier in actor verwendet wird: flag ob in FIFO, dann FIFO.insert(...) statt dem hier
		//everything before the : is part of the loop body
		std::string tmp;
		while (t.str != ":" && t.str != "]") {
			if (t.str == "if") {
				auto buffer = convert_inline_if(t, token_producer);
				if (buffer.second) {
					tmp.append(buffer.first);
				}
				else {
					tmp.append(buffer.first);
				}
			}
			else if (t.str == "[") {
				tmp.append(convert_brackets(t, token_producer, false, global_map, local_map, prefix).first);
			}
			else {
				tmp.append(t.str);
				t = token_producer.get_next_Token();
			}
		}
		if (t.str == ":") {
			command.append(tmp+";\n");
			t = token_producer.get_next_Token();// drop :
			//loop head(s)
			std::string head{};
			std::string tail{};
			while (t.str != "]") {
				if (t.str == "for" || t.str == "foreach") {
					head.append(prefix + "for(");
					t = token_producer.get_next_Token();
					if (t.str == "uint" || t.str == "int" || t.str == "String" || t.str == "bool" || t.str == "half" || t.str == "float") {
						head.append(convert_type(t, token_producer, symbol_definition_map_for_actor));
						head.append(t.str + " = ");
						std::string var_name{ t.str };
						t = token_producer.get_next_Token(); //in
						t = token_producer.get_next_Token(); //start value
						head.append(t.str + ";");
						t = token_producer.get_next_Token(); //..
						t = token_producer.get_next_Token(); //end value
						head.append(var_name + " <= " + t.str + ";" + var_name + "++){\n");
						t = token_producer.get_next_Token();
					}
					else {//no type, directly variable name, indicates foreach loop
						head.append(fifo_name_to_rawType_map[fifo_name]+" " + t.str);
						t = token_producer.get_next_Token();//in
						head.append(":");
						t = token_producer.get_next_Token();
						if (t.str == "[" || t.str == "{") {
							t = token_producer.get_next_Token();
							head.append("{");
							while (t.str != "]") {
								head.append(t.str);
							}
							head.append("}");
							t = token_producer.get_next_Token();
						}
					}
					tail.append(prefix + "}\n");
					if (t.str == ",") {//a komma indicates a further loop head, thus the next token has to be inspected
						t = token_producer.get_next_Token();
					}
				}
			}
			output.append(head);
			output.append(prefix + command);
			output.append(tail);
		}
		else if (t.str == "]") {
			//count elements of the list
			int element_count_list = tmp.empty() ? 0 : 1;
			size_t pos = tmp.find(",");
			while (pos != std::string::npos) {
				++element_count_list;
				pos = tmp.find(",", pos + 1);
			}
			tmp.insert(0, "{");
			tmp.append("}");
			std::string unused_var_name = find_unused_name(global_map, local_map);
			command.append(unused_var_name + "[i];\n");
			output.append(prefix + fifo_name_to_rawType_map[fifo_name] + " " + unused_var_name + "[" + std::to_string(element_count_list) + "] = " + tmp + ";\n");
			output.append(prefix + "for( int i = 0; i < "+std::to_string(element_count_list)+"; i++) {\n");
			output.append(prefix+command);
			output.append(prefix + "}\n");
		}
		return output;
	}

	/*
	This function converts the access part to input fifos.
	The token points at the start of this part and it has to contain the first fifo name or ==> if there is not input fifo access,action buffer has to be the
	corresponding buffer to the token. Also the function takes the action information object for this action, the local map of this action and the prefix for each
	line of code as arguments.
	The FIFO access is converted to C++ + OpenCL framework code.
	Every FIFO name with the number of consumed tokens is inserted into the action information object and every symbol declaration is inserted into the local map.
	The function returns the generated code in a string object.
	*/
	std::string convert_input_FIFO_access_opencl(Token& t, Action_Buffer& token_producer, Action_Information& act_inf, std::map<std::string, std::string>& local_map, std::string prefix = "") {
		//token contains start of the fifo part
		std::string output;
		while (t.str != "==>") {
			std::string name{ t.str + "$FIFO" };
			t = token_producer.get_next_Token();// :
			t = token_producer.get_next_Token(); // [
			t = token_producer.get_next_Token(); // start of token read part
			std::vector<std::string> consumed_element_names; // store elements first, because there can be a repeat
			while (t.str != "]") {
				if (t.str == ",") {
					t = token_producer.get_next_Token();
				}
				else {
					consumed_element_names.push_back(t.str);
					local_map[t.str] = "";
					t = token_producer.get_next_Token();
				}
			}
			t = token_producer.get_next_Token(); // drop ]
			if (t.str == "repeat") {
				t = token_producer.get_next_Token(); //repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "==>") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);
				
				//create expressions to read the tokens from the fifo
				int preview_index{ 0 };
				for (auto it = consumed_element_names.begin(); it != consumed_element_names.end(); ++it) {
					output.append(prefix + fifo_name_to_rawType_map[name] + " " + *it + "[");
					output.append(std::to_string(repeat_count) + "];\n");
					//i cannot be a name collision because every fifo name is: name+"$FIFO"
					output.append(prefix+"for(int i = 0;i < " + std::to_string(repeat_count) + ";i++){\n");
					output.append(prefix + "\t" + *it + "[i] = " + name + "$ptr[" + name + "$index++];\n");
					output.append(prefix + "}\n");
					FIFO_access_replace_map_for_OpenCL[*it] = name+"$ptr["+name+"$index + " + std::to_string(preview_index)+"+";
					preview_index += repeat_count;
				}
				act_inf.input_fifos.push_back(std::make_pair(name, repeat_count*consumed_element_names.size()));
			}
			else {
				int preview_index{ 0 };
				for (auto it = consumed_element_names.begin(); it != consumed_element_names.end(); ++it) {
					output.append(prefix + fifo_name_to_rawType_map[name]+" " + *it + " = " + name + "$ptr[" + name + "$index++];\n");
					FIFO_access_replace_map_for_OpenCL[*it] = name + "$ptr[" + name + "$index + " + std::to_string(preview_index) + "]";
					++preview_index;
				}
				act_inf.input_fifos.push_back(std::make_pair(name, consumed_element_names.size()));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be ==> and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		return output;
	}

	/*
		The functions takes a token pointing at the first fifo name of the output part and the corresponding action buffer, the action information object
		for this action, the local map of this action and the prefix for each line of code as input parameters.
		The fifo accesses are converted to C++ + OpenCL framework code and this code is returned as a string.
		Additionally, each fifo name and number of consumed/produces tokens in this access are inserted into the output fifo list of the action information list.
		All defined symbols are inserted into the local map.
	*/
	std::string convert_output_FIFO_access_opencl(Token& t, Action_Buffer& token_producer, Action_Information& act_inf, std::map<std::string, std::string>& local_map, std::string prefix = "") {
		//token contains start of output fifo part
		std::string output;
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {
			std::string name{ t.str + "$FIFO" };
			std::vector<std::string> output_fifo_expr;
			t = token_producer.get_next_Token(); //:
			t = token_producer.get_next_Token(); //[
			t = token_producer.get_next_Token();
			bool list_comprehension_found{ false };
			int expr_counter{ 0 };
			if (t.str == "[") {
				list_comprehension_found = true;
				//FOUND LIST COMPREHENSION!!!!!!!
				output.append(convert_list_comprehension_opencl(t, token_producer, name, prefix,symbol_definition_map_for_actor,local_map));
				t = token_producer.get_next_Token(); // drop last ]
				++expr_counter;
			}
			else {//no list comprehension
				while (t.str != "]") {
					std::string expr{ " " };
					bool add_to_expr{ true };
					while (t.str != "," && t.str != "]") {
						if (t.str == "[") {
							while (t.str != "]") {
								expr.append(t.str);
								t = token_producer.get_next_Token();
							}
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						else if (t.str == "(") {
							while (t.str != ")") {
								expr.append(t.str);
								t = token_producer.get_next_Token();
							}
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						else if (t.str == "if") {
							auto tmp = convert_inline_if(t, token_producer);
							if (tmp.second) {
								add_to_expr = false;
								Tokenizer tok{ tmp.first,true };
								Token tok_token = tok.get_next_Token();
								output.append(convert_inline_if_with_list_assignment_opencl(tok_token, tok, symbol_definition_map_for_actor, local_map, prefix, name));
								++expr_counter;
							}
							else {
								expr.append(tmp.first);
							}
						}
						else {
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
					}
					if (t.str == ",") {
						t = token_producer.get_next_Token();
					}
					if (add_to_expr) {
						output_fifo_expr.push_back(expr);
					}
					++expr_counter;
				}
			}
			t = token_producer.get_next_Token();
			if (t.str == "repeat") {
				t = token_producer.get_next_Token(); //repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "do" && t.str != "var" && t.str != "end" && t.str != "guard") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);

				act_inf.output_fifos.push_back(std::make_pair(name, repeat_count * expr_counter));
				//list comprehensions are not in the output_fifo_expr list -> not inserted twice!s
				for (auto it = output_fifo_expr.begin(); it != output_fifo_expr.end(); ++it) {
					output.append(prefix + "for(int i = 0;i < " + std::to_string(repeat_count) + ";i++){\n");
					output.append(prefix + "\t" + name + "$ptr[" + name + "$index++] = " + *it + "[i];\n");
					output.append(prefix + "}\n");
				}
			}
			else {
				for (auto it = output_fifo_expr.begin(); it != output_fifo_expr.end(); ++it) {
					output.append(prefix + name + "$ptr[" + name + "$index++] = " + *it + ";\n");
				}
				act_inf.output_fifos.push_back(std::make_pair(name, expr_counter));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be var or do or...  and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		return output;
	}


	//---------------------------- Pure Cpp without OpenCL generation -----------------------------------

	/*
		This function takes an action buffer (index at 0), a action information object and the prefix for each line of code as arguments.
		The tokens in the action buffer will be converted to pure C++ code.
		Additionally, the name, tag, the name of the generated method and the fifo names and the consumed/produced tokens are inserted into the action information object.
		The generated C++ code is return in a string.
		The scheduling conditions (guards, fifo sizes) are inserted into the scheduling condition map of the class.
	*/
	std::string convert_action(Action_Buffer& token_producer, Action_Information& act_inf, std::string prefix = "") {
		std::map<std::string, std::string> local_symbol_map;
		std::string output{ };
		std::string end_of_output;
		Token t = token_producer.get_next_Token();
		//determine action name, tag and the method name for the generated method and insert them into the action information object
		std::string action_tag;
		std::string action_name{ "" };
		std::string method_name;
		std::string condition;
		std::string condition_freeSpace;
		bool add_to_actionlist{ true };
		if (t.str == "action") {//action has no name
			action_tag = "action";
			action_name = std::to_string(default_number_action++); //give the action a default name to use it as function name
		}
		else if (t.str == "initialize") {

			action_tag = "initialize";
			add_to_actionlist = false;
			call_initialize = true;
		}
		else {
			action_tag = t.str;
			t = token_producer.get_next_Token(); // : or .
			if (t.str == ".") {
				t = token_producer.get_next_Token();
				while (t.str != ":") {
					if (t.str == ".") {//dots cannot be used in c++ 
						action_name.append("$");
					}
					else {
						action_name.append(t.str);
					}
					t = token_producer.get_next_Token();
				}
			}
			t = token_producer.get_next_Token(); // action
			if (t.str == "initialize") {// init action can have a name, dont know why
				action_tag = "initialize";
				add_to_actionlist = false;
				call_initialize = true;
			}
		}
		method_name = action_tag + "$" + action_name;
		act_inf.tag = action_tag;
		act_inf.name = action_name;
		act_inf.method_name = method_name;
		symbol_definition_map_for_actor[method_name] = "";
		//create function head
		output.append(prefix+"void " + method_name + "(){\n");
		//convert fifo access
		t = token_producer.get_next_Token();//start of the fifo part - fifo name
		std::map<std::string, std::string> fifo_var_preview_expression_map;
		while (t.str != "==>") {//input fifos
			output.append(convert_input_FIFO_access(t, token_producer, act_inf, local_symbol_map, fifo_var_preview_expression_map, prefix + "\t"));
		}
		t = token_producer.get_next_Token(); //get next token, that should contain first output fifo name, if any are accessed
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {//output fifos
			end_of_output.append(convert_output_FIFO_access(t, token_producer, act_inf, local_symbol_map, prefix + "\t"));
		}
		//add scheduling conditions regarding the space of the fifos to conditions
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (condition.size() != 0) {
				condition.append(" && ");
			}
			condition.append(it->first + ".get_size() >= " + std::to_string(it->second));
		}
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (condition_freeSpace.size() != 0) {
				condition_freeSpace.append(" && ");
			}
			condition_freeSpace.append(it->first + ".get_free_space() >= " + std::to_string(it->second));
		}
		//add to actor_scheduling_condition string
		for (auto it = act_inf.input_fifos.begin(); it != act_inf.input_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name+".get_size() >= " + std::to_string(it->second));
			}
		}
		for (auto it = act_inf.output_fifos.begin(); it != act_inf.output_fifos.end(); ++it) {
			if (!actor_scheduling_condition.empty()) {
				actor_scheduling_condition.append(" && ");
			}
			std::string fifo_name{ find_fifo_name(it->first) };
			if (!fifo_name.empty()) {
				actor_scheduling_condition.append(fifo_name+".get_free_space() >= " + std::to_string(it->second));
			}
		}
		//read guards
		if (t.str == "guard") {
			std::string guard;
			t = token_producer.get_next_Token();
			while (t.str != "var" && t.str != "do" && t.str != "end") {
				if (t.str == ",") {
					guard.append(" && ");
					t = token_producer.get_next_Token();
				}
				else if (t.str == "=") {
					guard.append(" == ");
					t = token_producer.get_next_Token();
				}
				else {
					if (fifo_var_preview_expression_map.find(t.str) != fifo_var_preview_expression_map.end()) {
						guard.append(fifo_var_preview_expression_map[t.str]);
						t = token_producer.get_next_Token();
						if (t.str == "[") {//must be part of a repeat list/array 
							t = token_producer.get_next_Token();
							std::string bracket_content;
							while (t.str != "]") {
								bracket_content.append(t.str);
								t = token_producer.get_next_Token();
							}
							guard.append(bracket_content + ")");
							t = token_producer.get_next_Token();
						}
						guard.append(" ");
					}
					else {
						guard.append(t.str);
						t = token_producer.get_next_Token();
					}
				}
			}
			act_inf.guards = guard;
			if (condition.size() != 0) {
				condition.append(" && ");
			}
			condition.append(guard);
		}
		//convert the body of the action
		if (t.str != "end") {
			output.append(convert_action_body(t, token_producer, local_symbol_map, prefix + "\t"));
		}
		if (add_to_actionlist) {
			if (condition.size() == 0) {
				condition = "true";
			}
			actionMethodName_schedulingCondition_mapping[method_name] = condition;
			actionMethodName_freeSpaceCondition_mapping[method_name] = condition_freeSpace;
		}
		return output + end_of_output + prefix + "}\n";
	}

	/*
		This function has to be used if something like: int a[2] = b<3?[5,b]:[0,0] is used in the part of the action where the ports/FIFOs are accessed.
		C++ doesn't allow list assignments without initialization. Thus, this must be converted to a for loop surrounded by condition checking (if...else...).
		The result is returned as a string. This function is meant to be used if OpenCL is NOT used. Otherwise the elements won't be stored in the FIFOs and undefined variables will be used!
	*/
	std::string convert_inline_if_with_list_assignment(Token& t, Token_Container& token_producer, std::map<std::string, std::string>& global_map, std::map<std::string, std::string>& local_map, std::string prefix, std::string fifo_name) {
		std::string condition;
		std::string expression1;
		std::string expression2;
		std::string output;
		//Condition
		while (t.str != "?") {
			condition.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		t = token_producer.get_next_Token(); // skip "?"
		//if body
		int count{ 1 };
		bool nested{ false };
		while (count != 0) {
			if (t.str == "?") {
				++count;
				nested = true;
			}
			else if (t.str == ":") {
				--count;
			}
			expression1.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		expression1.erase(expression1.size() - 2, 2);//remove last :
		if (nested) {
			Tokenizer tok{ expression1,true };
			Token tok_token = tok.get_next_Token();
			expression1 = convert_inline_if_with_list_assignment(tok_token, tok, global_map, local_map, prefix + "\t", fifo_name);
		}
		else {
			//auf listenzuweiseung prüfen, wenn ja mit convert_list_comprehension in C++ Code umwandeln
			if (expression1[0] == '[') {
				Tokenizer tok{ expression1,true };
				Token tok_token = tok.get_next_Token();
				expression1 = convert_list_comprehension(tok_token, tok, fifo_name, prefix + "\t", global_map, local_map);
			}
			else {
				expression1 = prefix + "\t" + fifo_name + ".put_element(" + expression1 + ");\n";
			}
		}
		//ELSE Part

		count = 1;
		nested = false;
		while (count != 0) {
			if (t.str == "?") {
				++count;
				nested = true;

			}
			else if (t.str == ":") {
				--count;
			}
			else if (t.str == "") {
				break;
			}
			expression2.append(t.str + " ");
			t = token_producer.get_next_Token();
		}
		if (nested) {
			Tokenizer tok{ expression2,true };
			Token t = tok.get_next_Token();
			expression2 = convert_inline_if_with_list_assignment(t, tok, global_map, local_map, prefix + "\t", fifo_name);
		}
		else {
			//auf listenzuweiseung prüfen, wenn ja mit convert_list_comprehension in C++ Code umwandeln
			if (expression2[0] == '[') {
				Tokenizer tok{ expression2,true };
				Token t = tok.get_next_Token();
				expression2 = convert_list_comprehension(t, tok, fifo_name, prefix + "\t", global_map, local_map);
			}
			else {
				expression2 = prefix + "\t" + fifo_name + ".put_element(" + expression2 + ");\n";
			}
		}

		//Build expression
		output.append(prefix + "if(" + condition + ") {\n");
		output.append(expression1);
		output.append(prefix + "}\n");
		output.append(prefix + "else {\n");
		output.append(expression2);
		output.append(prefix + "}\n");
		return output;
	}


	/*
		This function takes a token that points to part of the action where operations are performed (after the fifo access part),
		the corresponding Action_Buffer, the local map for this action and the prefix for each line of code as inputs.
		The function converts these statements to C++ and returns this code as a string.
		All symbol declarations/definitions are inserted into the local map.
	*/
	std::string convert_action_body(Token& t, Action_Buffer& token_producer, std::map<std::string, std::string> local_symbol_map, std::string prefix = "") {
		std::string output;
		while (t.str != "end") {
			if (t.str == "var" || t.str == "do") {
				t = token_producer.get_next_Token();
			}
			else if (t.str == "for" || t.str == "foreach") {
				output.append(convert_for(t, token_producer, symbol_definition_map_for_actor, local_symbol_map, false, prefix));
			}
			else if (t.str == "while") {
				output.append(convert_while(t, token_producer, symbol_definition_map_for_actor, local_symbol_map, false, prefix));
			}
			else if (t.str == "if") {
				output.append(convert_if(t, token_producer, symbol_definition_map_for_actor, local_symbol_map, false, prefix));
			}
			else {
				output.append(convert_expression(t, token_producer, symbol_definition_map_for_actor, local_symbol_map, "*", false, prefix));
			}
		}
		return output;
	}

	/*
		This function converts the access part to input fifos.
		The token points at the start of this part and it has to contain the first fifo name or ==> if there is not input fifo access,action buffer has to be the
		corresponding buffer to the token. Also the function takes the action information object for this action, the local map of this action and the prefix for each 
		line of code as arguments.
		The FIFO access is converted to pure C++ code, for OpenCL environments there is another function.
		Every FIFO name with the number of consumed tokens is inserted into the action information object and every symbol declaration is inserted into the local map.
		The function returns the generated code in a string object.
	*/
	std::string convert_input_FIFO_access(Token& t, Action_Buffer& token_producer, Action_Information& act_inf, std::map<std::string, std::string>& local_map, std::map<std::string, std::string>& fifo_var_preview_expression_map, std::string prefix = "") {
		//token contains start of the fifo part - first fifo name
		std::string output;
		while (t.str != "==>") {
			int preview_index{ 0 };
			std::string name{ t.str + "$FIFO" };
			t = token_producer.get_next_Token();// :
			t = token_producer.get_next_Token(); // [
			std::vector<std::pair<std::string,bool>> consumed_element_names; // store elements first, because there can be a repeat
			t = token_producer.get_next_Token(); //start of token name part
			while (t.str != "]") {
				std::string expr{};
				bool first{ true };
				bool already_known_variable{ false };
				while (t.str != "," && t.str != "]") {
					if (first) {//check if variable name is already known and therefore there is no need to define it again!
						first = false;
						if (symbol_definition_map_for_actor.count(t.str) == 1) {
							already_known_variable = true;
							//std::cout << "Variable known:" << t.str << " Value:"<<symbol_definition_map_for_actor[t.str] << std::endl;
						}
					}
					if (t.str == "[") {
						while (t.str != "]") {
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						expr.append(t.str);
						t = token_producer.get_next_Token();
					}
					else {
						expr.append(t.str);
						t = token_producer.get_next_Token();
					}
				}
				if (t.str == ",") {
					t = token_producer.get_next_Token();
				}
				consumed_element_names.push_back(std::make_pair(expr,already_known_variable));
			}
			t = token_producer.get_next_Token(); // drop ]
			if (t.str == "repeat") {
				t = token_producer.get_next_Token(); //repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "==>") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);
				//create expressions to read the tokens from the fifo
				for (auto it = consumed_element_names.begin(); it != consumed_element_names.end(); ++it) {
					if (std::get<1>(*it)) {

					}
					else {
						output.append(prefix + fifo_name_to_rawType_map[name] + " " + std::get<0>(*it) + "[" + std::to_string(repeat_count) + "];\n");
					}
					output.append(prefix + name + ".get_elements(" + std::get<0>(*it) + "," + std::to_string(repeat_count) + ");\n");
					fifo_var_preview_expression_map[std::get<0>(*it)] = name+".element_preview("+std::to_string(preview_index)+"+";
					preview_index+=repeat_count;
				}
				act_inf.input_fifos.push_back(std::make_pair(name, repeat_count*consumed_element_names.size()));
			}
			else {//no repeat, every element in the fifo brackets consumes one element
				for (auto it = consumed_element_names.begin(); it != consumed_element_names.end(); ++it) {
					if (std::get<1>(*it)) {
						output.append(prefix + std::get<0>(*it) + " = " + name + ".get_element();\n");
					}
					else {
						output.append(prefix + "auto " + std::get<0>(*it) + " = " + name + ".get_element();\n");
					}
					fifo_var_preview_expression_map[std::get<0>(*it)] = name + ".element_preview("+std::to_string(preview_index)+")";
					preview_index++;
				}
				act_inf.input_fifos.push_back(std::make_pair(name, consumed_element_names.size()));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be ==> and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		return output;
	}

	/*
		The functions takes a token pointing at the first fifo name of the output part and the corresponding action buffer, the action information object
		for this action, the local map of this action and the prefix for each line of code as input parameters.
		The fifo accesses are converted to pure C++ code and this code is returned as a string. For the use in a OpenCL environment, there is another function to convert output fifo accesses.
		Additionally, each fifo name and number of consumed/produces tokens in this access are inserted into the output fifo list of the action information list.
		All defined symbols are inserted into the local map.
	*/
	std::string convert_output_FIFO_access(Token& t, Action_Buffer& token_producer, Action_Information& act_inf, std::map<std::string, std::string>& local_map, std::string prefix = "") {
		//token contains start of output fifo part - first fifo name
		std::string output;
		while (t.str != "do" && t.str != "guard" && t.str != "var" && t.str != "end") {
			std::string name{ t.str + "$FIFO" };
			std::vector<std::string> output_fifo_expr;
			t = token_producer.get_next_Token(); //:
			t = token_producer.get_next_Token(); //[
			t = token_producer.get_next_Token();
			bool list_comprehension_found{ false };
			int expr_counter{ 0 };
			if (t.str == "[") {
				list_comprehension_found = true;
				//FOUND LIST COMPREHENSION!!!!!!!
				output.append(convert_list_comprehension(t, token_producer, name, prefix,symbol_definition_map_for_actor,local_map));
				t = token_producer.get_next_Token(); //drop last ] of the list comprehension
				++expr_counter;
			}
			else {
				while (t.str != "]") {
					std::string expr{ " " };
					bool add_to_expr{ true };
					while (t.str != "," && t.str != "]") {
						if (t.str == "[") {
							while (t.str != "]") {
								expr.append(t.str);
								t = token_producer.get_next_Token();
							}
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						else if (t.str == "(") {
							while (t.str != ")") {
								expr.append(t.str);
								t = token_producer.get_next_Token();
							}
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
						else if (t.str == "if") {
							auto tmp = convert_inline_if(t, token_producer);
							if (tmp.second) {
								add_to_expr = false;
								Tokenizer tok{ tmp.first,true };
								Token tok_token = tok.get_next_Token();
								output.append(convert_inline_if_with_list_assignment(tok_token,tok,symbol_definition_map_for_actor,local_map,prefix,name));
								++expr_counter;
							}
							else {
								expr.append(tmp.first);
							}
						}
						else {
							expr.append(t.str);
							t = token_producer.get_next_Token();
						}
					}
					if (t.str == ",") {
						t = token_producer.get_next_Token();
					}
					if (add_to_expr) {
						output_fifo_expr.push_back(expr);
					}
					++expr_counter;
				}
			}
			t = token_producer.get_next_Token();
			if (t.str == "repeat") {
				t = token_producer.get_next_Token();//contains repeat count
				std::string repeat_count_expression;
				while (t.str != "," && t.str != "do" && t.str != "var" && t.str != "end" && t.str != "guard") {
					repeat_count_expression.append(t.str + " ");
					t = token_producer.get_next_Token();
				}
				int repeat_count = evaluate_constant_expression(repeat_count_expression, symbol_definition_map_for_actor, local_map);
				act_inf.output_fifos.push_back(std::make_pair(name, repeat_count * expr_counter ));
				for (auto it = output_fifo_expr.begin(); it != output_fifo_expr.end(); ++it) {
					output.append(prefix + name + ".put_elements(" + *it + " , " + std::to_string(repeat_count) + ");\n");
				}
				
			}
			else {
				for (auto it = output_fifo_expr.begin(); it != output_fifo_expr.end(); ++it) {
					output.append(prefix + name + ".put_element(" + *it + " );\n");
				}
				act_inf.output_fifos.push_back(std::make_pair(name, expr_counter ));
			}
			if (t.str == ",") {// another fifo access, otherwise it has to be var or do  and the loop terminates
				t = token_producer.get_next_Token();
			}
		}
		return output;
	}

	/*
		This function takes a token pointing at the start of the list comprehension, the corresponding Action_Buffer, 
		the name of the FIFO the list comprehension is used for and the prefix for each line of code as arguments.
		This function is only used for list comprehensions that occur in FIFOs, all other list comprehensions are handled in the Converter_RVC_Cpp.
		The list comprehension is converted to a loop or multiple loops in pure C++ without OpenCL. For list comprehension in relation with OpenCL there is another function.
		The assumption is, list comprehension can only occur in output FIFOs, because in input FIFOs it wouldn't make any sense.
		The function returns the generated C++ as a string.
		After the function the Token reference should point at the closing ] of the list comprehension.
	*/
	std::string convert_list_comprehension(Token& t, Token_Container& token_producer, std::string fifo_name, std::string prefix, std::map<std::string, std::string>& global_map, std::map<std::string, std::string>& local_map ) {
		std::string output;
		t = token_producer.get_next_Token();//drop [
		std::string command{ "\t" + fifo_name + ".put_element(" };
		//everything before the : is the body of the loop(s)
		std::string tmp;
		while (t.str != ":" && t.str != "]") {
			if (t.str == "if") {
				auto buf = convert_inline_if(t, token_producer);
				tmp.append(buf.first);
			}
			else if (t.str == "[") {
				tmp.append(convert_brackets(t, token_producer, false, global_map, local_map, prefix).first);
			}
			else {
				tmp.append(t.str);
				t = token_producer.get_next_Token();
			}
		}
		if(t.str == ":"){
			command.append(tmp + ");\n");
			t = token_producer.get_next_Token();// drop :
			//loop head(s)
			std::string head{};
			std::string tail{};
			while (t.str != "]") {
				if (t.str == "for" || t.str == "foreach") {
					head.append(prefix + "for(");
					t = token_producer.get_next_Token();
					if (t.str == "uint" || t.str == "int" || t.str == "String" || t.str == "bool" || t.str == "half" || t.str == "float") {
						head.append(convert_type(t, token_producer, symbol_definition_map_for_actor));
						head.append(t.str + " = ");
						std::string var_name{ t.str };
						t = token_producer.get_next_Token(); //in
						t = token_producer.get_next_Token(); //start value
						head.append(t.str + ";");
						t = token_producer.get_next_Token(); //..
						t = token_producer.get_next_Token(); //end value
						head.append(var_name + " <= " + t.str + ";" + var_name + "++){\n");
						t = token_producer.get_next_Token();
					}
					else {//no type, directly variable name, indicates foreach loop
						head.append("auto " + t.str);
						t = token_producer.get_next_Token();//in
						head.append(":");
						t = token_producer.get_next_Token();
						if (t.str == "[" || t.str == "{") {
							t = token_producer.get_next_Token();
							head.append("{");
							while (t.str != "]") {
								head.append(t.str);
							}
							head.append("}");
							t = token_producer.get_next_Token();
						}
					}
					tail.append(prefix + "}\n");
					if (t.str == ",") {//a komma indicates a further loop head, thus the next token has to be inspected
						t = token_producer.get_next_Token();
					}
				}
			}
			output.append(head);
			output.append(prefix + command);
			output.append(tail);
		}
		else if (t.str == "]") {
			tmp.insert(0, "{");
			tmp.append("}");
			std::string unused_var_name = find_unused_name(global_map, local_map);
			command.append(unused_var_name + ");\n");
			output.append(prefix + "for( auto " + unused_var_name + ":" + tmp + ") {\n");
			output.append(prefix + command);
			output.append(prefix + "}\n");
		}
		return output;
	}

	//------------------------------------ Generate Scheduler, FSM, ... ---------------------------------------

	/*
		The functions converts an import to C++ code, that can be inserted at the beginning of the actor.
		The function take the path to the RVC sources to find the imported file and the token that points at the beginning of the import expression.
		The function returns the generated C++ code as a pair of strings. First:Function declarations, Second: consts, var, functions,...
		Additionally, this function sets the OpenCL flag to false and the include native flag to true if a native function is imported.
	*/
	std::pair<std::string,std::string> convert_import(Token& t, const std::string path) {
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
			bool native_flag{ false };
			Unit_Generator gen(path_to_import_file, symbol_definition_map_for_actor, native_flag, path);
			auto output = gen.convert_unit_file(symbol, "\t");
			if (native_flag) {
				OpenCL = false;
				include_native = true;
			}
			return output;
		}
	}

	/*
		This function reads the priority block and stores all priority relations in a class datastructure.
		These relations are later used to sort the schedulable actions according to their priority.
		This function shall also insert the transitive hull into the datastructure to make sorting easier.
		The Token argument must contain the start of the priority block! And has to contain one token past the priority block after the function is done!
	*/
	void convert_priority(Token& t) {
		priorities = true;
		t = token_producer.get_next_Token();
		std::vector<std::pair<std::string, std::string>> greater_actions;
		while (t.str != "end") {
			if (t.str == ";") {
				t = token_producer.get_next_Token();
			}
			else if (t.str == ">") {
				t = token_producer.get_next_Token();
				std::string lesser_tag{ t.str };
				std::string lesser_name{ "" };
				t = token_producer.get_next_Token();
				if (t.str == ".") {
					t = token_producer.get_next_Token();
					while (t.str != ">" && t.str != ";") {
						if (t.str == ".") {
							lesser_name.append("$");
						}
						else {
							lesser_name.append(t.str);
						}
						t = token_producer.get_next_Token();
					}
				}
				//insert transitive closure for sorting
				for (auto greater_it = greater_actions.begin(); greater_it != greater_actions.end(); ++greater_it) {
					priority_greater_tag_name_lesser_tag_name.push_back(std::make_pair(*greater_it, std::make_pair(lesser_tag, lesser_name)));
				}
				greater_actions.push_back(std::make_pair(lesser_tag, lesser_name));
			}
			else {//start of one row of priority relations
				std::string greater_tag = t.str;
				std::string greater_name = "";
				t = token_producer.get_next_Token();
				if (t.str == ".") {
					t = token_producer.get_next_Token();
					while (t.str != ">" && t.str != ";") {
						if (t.str == ".") {
							greater_name.append("$");
						}
						else {
							greater_name.append(t.str);
						}
						t = token_producer.get_next_Token();
					}
				}
				greater_actions.clear();
				greater_actions.push_back(std::make_pair(greater_tag,greater_name));
			}
		}
		//move one token past the end of the block
		t = token_producer.get_next_Token();
	}


	/*
		This function generates a enum class with all states, and a class variable for the current state initialized with the starting state.
		The input tokens contains the start of the FSM part and the prefix is the prefix for each line of code.
		Additionally, all state transitions are stores in a  datastructure to be used to generate the scheduler.
		This function returns the generated code in a string.
	*/
	std::string convert_FSM(Token& t, const std::string prefix = "") {
		FSM = true;
		//token contains schedule 
		std::set<std::string> check_for_cycle; //all outgoing nodes are inserted here, if one is already in the list, there is more than exactly one cycle
		t = token_producer.get_next_Token(); //contains fsm
		t = token_producer.get_next_Token(); //contains starting state
		//state variable MUST be defined AFTER the enum is defined!!!
		std::string end_of_output{ prefix + "states state = states::" + t.str + ";\n" };
		std::string output{ prefix + "enum class states{\n" };
		states.insert(t.str);
		t = token_producer.get_next_Token(); //contains :
		t = token_producer.get_next_Token(); //start of the fsm body
		while (t.str != "end") {
			std::string state{ t.str };
			t = token_producer.get_next_Token(); // ( 
			//there can be multiple actions to be fired to trigger this transition!
			std::vector<std::pair<std::string, std::string>> vector_tag_name_action_to_fire;
			while (t.str != ")") {
				std::string action_tag;
				std::string action_name{ "" };
				t = token_producer.get_next_Token();
				action_tag = t.str;
				t = token_producer.get_next_Token();
				if (t.str == ".") {
					t = token_producer.get_next_Token();
					while (t.str != "," && t.str != ")") {
						if (t.str == ".") {
							action_name.append("$");
						}
						else {
							action_name.append(t.str);
						}
						t = token_producer.get_next_Token();
					}
				}
				vector_tag_name_action_to_fire.push_back(std::make_pair(action_tag, action_name));
			}
			t = token_producer.get_next_Token(); // -->
			t = token_producer.get_next_Token();
			std::string next_state{ t.str };
			t = token_producer.get_next_Token(); // ;
			t = token_producer.get_next_Token();
			//add all transitions to corresponding class members
			for (auto it = vector_tag_name_action_to_fire.begin(); it != vector_tag_name_action_to_fire.end(); ++it) {
				fsm_state_actionTag_actionName_nextstate.push_back(std::make_tuple(state, it->first, it->second, next_state));
			}
			if (vector_tag_name_action_to_fire.size() > 1) {
				OpenCL = false; //cannot be a cycle if multiple transitions between these two states are possible!
			}
			states.insert(state);
			states.insert(next_state);
			//check for cycle, if a state has already been visited, there must be a cycle
			if (check_for_cycle.find(next_state) == check_for_cycle.end()) {//not found -> no cycle 
				check_for_cycle.insert(next_state);
			}
			else {//already in the set => cycle
				OpenCL = false;
			}
		}
		if (OpenCL) {
			cycleFSM = true;
		}//it must contain a cycle, because otherwise OpenCl would be false already due to the previous loop

		//move one token past the end of the FSM block
		t = token_producer.get_next_Token();
		//insert all states into the enum
		for (auto it = states.begin(); it != states.end(); ++it) {
			output.append(prefix + "\t" + *it + ",\n");
		}
		output.erase(output.find_last_of(","), 1);//remove last , 
		output.append(prefix + "};\n");
		output.append(end_of_output + "\n");
		return output;
	}

	/*
		Finds all actions that are schedulable in the given state and returns their methodnames
	*/
	std::vector<std::string> find_schedulable_actions(std::string state) {
		std::vector<std::string> output;
		std::vector<std::pair<std::string, std::string>> schedulable_actionTag_actionName;
		//go through all fsm entries and find the ones for the given state, store actionTag and actionName
		for (auto it = fsm_state_actionTag_actionName_nextstate.begin(); it != fsm_state_actionTag_actionName_nextstate.end(); ++it) {
			if (std::get<0>(*it) == state) {
				schedulable_actionTag_actionName.push_back(std::make_pair(std::get<1>(*it),std::get<2>(*it)));
			}
		}
		//go through all action method names and compare with the previously created list, to check if this action can be scheduled in the corresponding state
		for (auto it = actionMethodName_schedulingCondition_mapping.begin(); it != actionMethodName_schedulingCondition_mapping.end(); ++it) {
			if (state == "") {
				output.push_back(it->first);
			}
			else {
				std::string method_name{ it->first };
				for (auto fsm_entry_it = schedulable_actionTag_actionName.begin(); fsm_entry_it != schedulable_actionTag_actionName.end(); ++fsm_entry_it) {
					if (fsm_entry_it->second.empty()) {//if second part if empty, than only the first part has to be used
						if (method_name.find(fsm_entry_it->first + "$") == 0) {//schedulable if the concat. fsm entry is the starting part (or the complete) of the method name 
							output.push_back(method_name);
						}
					}
					else {
						if (method_name.find(fsm_entry_it->first + "$" + fsm_entry_it->second + "$") == 0) {//schedulable if the concat. fsm entry is the starting part (or the complete) of the method name 
							output.push_back(method_name);
						}
						else if (method_name == fsm_entry_it->first + "$" + fsm_entry_it->second) {//schedulable if the concat. fsm entry is the method name
							output.push_back(method_name);
						}
					}
				}
			}
		}
		return output;
	}

	/*
		Finds for a given state and the action that has been fired in this state the corresponding next state according to the transitions of the FSM.
		Throws an exception if it cannot find the next state.
	*/
	std::string find_next_state(std::string current_state, std::string fired_action_methodName) {
		for (auto fsm_it = fsm_state_actionTag_actionName_nextstate.begin(); fsm_it != fsm_state_actionTag_actionName_nextstate.end(); ++fsm_it) {
			if (std::get<0>(*fsm_it) == current_state) {
				if (fired_action_methodName == std::get<1>(*fsm_it) + "$" + std::get<2>(*fsm_it)) {
					return std::get<3>(*fsm_it);
				}
				else if (fired_action_methodName.find(std::get<1>(*fsm_it) + "$" + std::get<2>(*fsm_it) + "$") == 0) {
					return std::get<3>(*fsm_it);
				}
				else if (std::get<2>(*fsm_it).empty() && fired_action_methodName.find(std::get<1>(*fsm_it) + "$") == 0) {
					return std::get<3>(*fsm_it);
				}
			}
		}
		//cannot find the next state
		throw Converter_Exception{ "Cannot find the next state. Current state:" + current_state + ", Fired Action:" + fired_action_methodName };
	}

	/*
		This function generates the scheduler for this actor.
		It distinguishes between four cases: OpenCL with exactly one cycle in the FSM, OpenCL with actions with guards that are parallelizable, OpenCL and no OpenCL.
		If a FSM is defined, for each state a if statement is created and in the body for each schedulable action in this state it is again checked if the scheduling condition ( guards, FIFOs) 
		is fulfilled. This checking is done in the order of the priority, if defined.
		In case of OpenCL/OpenCL with exactly one cycle in the FSM, at the end the function that executes complete cycles is scheduled.

		This function returns the generated scheduler as a string.
	*/
	std::string generate_scheduler() {
		std::string output{ "\t" };
		if (OpenCL) {
			output.append("void schedule(cl_event *event){\n");
			//TODO FIFOs
			for (auto it = input_FIFOs_set.begin(); it != input_FIFOs_set.end();++it) {
				output.append("\t\t" + *it + ".opencl_read_done();\n");
			}
			for (auto it = output_FIFOs_set.begin(); it != output_FIFOs_set.end(); ++it) {
				output.append("\t\t" + *it + ".opencl_write_done(ocl);\n");
			}
			if (cycleFSM) {
				//insert every action once into the scheduler
				for (auto it = states.begin(); it != states.end(); ++it) {
					output.append("\t\tif(state == states::" + *it + "){\n"); 
					//find actions that could be scheduled in this state
					std::vector<std::string> schedulable_actions = find_schedulable_actions(*it);
					//sort the list of schedulable actions with the comparsion function defined above if a priority block is defined
					if (priorities) {
						std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
					}
					//create condition test and scheduling for each schedulable action
					for (auto action_it = schedulable_actions.begin(); action_it != schedulable_actions.end(); ++action_it) {
						if (action_it == schedulable_actions.begin()) {
							output.append("\t\t\tif(" + actionMethodName_schedulingCondition_mapping[*action_it] + "){\n");
						}
						else {
							output.append("\t\t\telse if(" + actionMethodName_schedulingCondition_mapping[*action_it] + "){\n");
						}
						if (actionMethodName_freeSpaceCondition_mapping[*action_it].empty()) {
							output.append("\t\t\t\t" + *action_it + "();\n");
							output.append("\t\t\t\tstate = states::" + find_next_state(*it, *action_it) + ";\n");
							output.append("\t\t\t}\n");
						}
						else {
							output.append("\t\t\t\tif( " + actionMethodName_freeSpaceCondition_mapping[*action_it] + " ) {\n");
							output.append("\t\t\t\t\t" + *action_it + "();\n");
							output.append("\t\t\t\t\tstate = states::" + find_next_state(*it, *action_it) + ";\n");
							output.append("\t\t\t\t}\n");
							//output.append("\t\t\t\telse {\n");
							//output.append("\t\t\t\t\tbreak;\n");
							//output.append("\t\t\t\t}\n");
							output.append("\t\t\t}\n");
						}
					}
				}
				//append the FSM_Cycle method to the scheduler
				output.append("\t\tif(");
				//condition
				std::string cond{ "state = states::" + start_state };
				if (actionMethodName_schedulingCondition_mapping.find("FSM$Cycle$"+actor_name) != actionMethodName_schedulingCondition_mapping.end()) {
					cond.append(" && " + actionMethodName_schedulingCondition_mapping["FSM$Cycle$"+actor_name]);
				}
				if (!actionMethodName_freeSpaceCondition_mapping["FSM$Cycle$" + actor_name].empty()) {
					cond.append(" && " + actionMethodName_freeSpaceCondition_mapping["FSM$Cycle$" + actor_name]);
				}
				output.append(cond + " ) {\n");
				output.append("\t\t\treturn FSM$Cycle$"+actor_name+"(event);\n");
				output.append("\t\t}\n");
				//no state change here because a complete cycle is executed!!!
			}
			else {//OpenCL/OpenCL but not cycleFSM
				std::vector<std::string> schedulable_actions = find_schedulable_actions("");
				if (priorities) {//sort according to the priorities
					std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
				}
				//create scheduling test for each action
				for (auto it = schedulable_actions.begin(); it != schedulable_actions.end(); ++it) {
					if (it == schedulable_actions.begin()) {
						output.append("\t\tif(" + actionMethodName_schedulingCondition_mapping[*it] + "){\n");
					}
					else {
						output.append("\t\telse if(" + actionMethodName_schedulingCondition_mapping[*it] + "){\n");
					}
					if (actionMethodName_freeSpaceCondition_mapping[*it].empty()) {
						output.append("\t\t\treturn " + *it + "(event);\n");
					}
					else {
						output.append("\t\t\tif(" + actionMethodName_freeSpaceCondition_mapping[*it] + ") {\n");
						output.append("\t\t\t\treturn " + *it + "(event);\n");
						output.append("\t\t\t}\n");
						//output.append("\t\t\telse {\n");
						//output.append("\t\t\t\tbreak;\n");
						//output.append("\t\t\t}\n");
					}
					output.append("\t\t}\n");
				}
			}
			output.append("\t\treturn;\n");
		}
		else {//Pure C++
			output.append("void schedule(){\n\t\tfor(;;){\n");
			if (FSM) {
				for (auto it = states.begin(); it != states.end(); ++it) {
					if (it == states.begin()) { output.append("\t\t\tif(state == states::" + *it + "){\n"); }
					else { output.append("\t\t\telse if(state == states::" + *it + "){\n"); }
					//find actions that could be scheduled in this state
					std::vector<std::string> schedulable_actions = find_schedulable_actions(*it);
					//sort the list of schedulable actions with the comparsion function defined above if a priority block is defined
					if (priorities) {
						std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
					}
					//create condition test and scheduling for each schedulable action
					for (auto action_it = schedulable_actions.begin(); action_it != schedulable_actions.end(); ++action_it) {
						if (action_it == schedulable_actions.begin()) {
							output.append("\t\t\t\tif(" + actionMethodName_schedulingCondition_mapping[*action_it] + "){\n");
						}
						else {
							output.append("\t\t\t\telse if(" + actionMethodName_schedulingCondition_mapping[*action_it] + "){\n");
						}
						if (actionMethodName_freeSpaceCondition_mapping[*action_it].empty()) {
							output.append("\t\t\t\t\t" + *action_it + "();\n");
							output.append("\t\t\t\t\tstate = states::" + find_next_state(*it, *action_it) + ";\n");
						}
						else {
							output.append("\t\t\t\t\tif(" + actionMethodName_freeSpaceCondition_mapping[*action_it] + ") {\n");
							output.append("\t\t\t\t\t\t" + *action_it + "();\n");
							output.append("\t\t\t\t\t\tstate = states::" + find_next_state(*it, *action_it) + ";\n");
							output.append("\t\t\t\t\t}\n");
							output.append("\t\t\t\t\telse {\n");
							output.append("\t\t\t\t\t\tbreak;\n");
							output.append("\t\t\t\t\t}\n");
						}
						output.append("\t\t\t\t}\n");
					}
					output.append("\t\t\t\telse {\n");
					output.append("\t\t\t\t\tbreak;\n");
					output.append("\t\t\t\t}\n");
					output.append("\t\t\t}\n");//close state checking if
				}
			}
			else {
				std::vector<std::string> schedulable_actions = find_schedulable_actions("");
				if (priorities) {
					std::sort(schedulable_actions.begin(), schedulable_actions.end(), comparison_object{ priority_greater_tag_name_lesser_tag_name });
				}
				for (auto it = schedulable_actions.begin(); it != schedulable_actions.end(); ++it) {
					if (it == schedulable_actions.begin()) {
						output.append("\t\t\tif(" + actionMethodName_schedulingCondition_mapping[*it] + "){\n");
					}
					else {
						output.append("\t\t\telse if(" + actionMethodName_schedulingCondition_mapping[*it] + "){\n");
					}
					if (actionMethodName_freeSpaceCondition_mapping[*it].empty()) {
						output.append("\t\t\t\t" + *it + "();\n");
					}
					else {
						output.append("\t\t\t\tif(" + actionMethodName_freeSpaceCondition_mapping[*it] + ") {\n");
						output.append("\t\t\t\t\t" + *it + "();\n");
						output.append("\t\t\t\t}\n");
						output.append("\t\t\t\telse {\n");
						output.append("\t\t\t\t\tbreak;\n");
						output.append("\t\t\t\t}\n");
					}
					output.append("\t\t\t}\n");
				}
				output.append("\t\t\telse {\n");
				output.append("\t\t\t\tbreak;\n");
				output.append("\t\t\t}\n");
			}
			output.append("\t\t}\n");//close for(;;) loop
		}
		output.append("\t}\n");// close scheduler method
		return output;
	}

	/*
		Function to generate the constructor.
		It iterates over all const inputs and FIFOs and inserts them as a parameter into the constructor parameter list and assigns the input parameters to their corresponding class member.
		Additionally, if a initialization function is present, the constructor calls this function.
		This function returns the generated constructor in a string.
	*/
	std::string generate_constructor() {
		if (OpenCL) {
			//add OpenCL init to add_to_constructor string 
			add_to_constructor.append("\t\tSetupOpenCL(&ocl, CL_DEVICE_TYPE_GPU, \"NVIDIA\");\n");
			add_to_constructor.append("\t\tCreateAndBuildProgram(&ocl, \""+actor_name+".cl\");\n");
			add_to_constructor.append("\t\tcl_int err = CL_SUCCESS;\n");
			for (auto it = actor_index_to_kernel_name_map.begin(); it != actor_index_to_kernel_name_map.end(); ++it) {
				add_to_constructor.append("\t\tocl.kernels.push_back(clCreateKernel(ocl.program, \"" + it->second + "\", &err));\n");
				add_to_constructor.append("\t\tif(err != CL_SUCCESS) {\n");
				add_to_constructor.append("\t\t\tstd::cout << \"Kernel Creation failed. clCreateKernel for " + it->second + "  in  " + actor_name + " returned \" << TranslateErrorCode(err);\n");
				add_to_constructor.append("\t\t\texit(err);\n");
				add_to_constructor.append("\t\t}\n");
			}
		}
		std::string output{ "\t" + actor_name + "(" };
		std::string initializer_list{ ") : " };
		for (auto it = constructor_parameters_type_name.begin(); it != constructor_parameters_type_name.end(); ++it) {
			output.append(it->first + " _" + it->second + ",");
			initializer_list.append(it->second + "(_" + it->second + "),");
		}
		//remove the last , of both strings
		output.erase(output.find_last_of(","), 1);
		initializer_list.erase(initializer_list.find_last_of(","), 1);
		output.append(initializer_list);
		//add the list comprehensions to the constructor and the init method
		if (call_initialize) {
			output.append(" {\n\t\tinitialize$();\n"+add_to_constructor+"\t}\n");
		}
		else {
			add_to_constructor.insert(0, "\t");
			int index = 0;
			while (true) {
				index = add_to_constructor.find("\n", index);
				if (index == std::string::npos) {
					break;
				}
				index += 2;
				if (index >= add_to_constructor.size()) {
					break;
				}
				add_to_constructor.insert(index, "\t");

			}
			output.append(" {\n"+add_to_constructor+"\t}\n");
		}
		return output;
	}

public:
	/*
		Constructor:
			1. parameter: path to the .cal file of the actor.
			2. parameter: size of the FIFOs, required as a template argument for the FIFO Types
			3. parameter: boolean if OpenCL shall be used where possible, if true OpenCL shall be used, if false not.
			4. parameter: a pointer to the Dataflow_Network object containg all the relevant data about the network
			5. parameter: the id of the actor in the network
	*/
	Actor_Generator(std::string path_to_actor, int fifo_size, const bool realize_with_OpenCL, Dataflow_Network *_dpn, std::string _id) :token_producer{ path_to_actor }, FIFO_size{ fifo_size }, OpenCL{ realize_with_OpenCL }, dpn{ _dpn }, id{ _id } {
		//declare build in functions in the map
		symbol_definition_map_for_actor["print"] = "function";
		symbol_definition_map_for_actor["println"] = "function";
		symbol_definition_map_for_actor["min"] = "function";
		symbol_definition_map_for_actor["max"] = "function";
	};

	/*
		Function to convert RVC code to C++ code.
		This function takes the path to the directory where the RVC source files are located, the directory the output should be stored in and the include expression for native calls as arugments.
		This function converts import expressions and actors to C++ code and stores the code in a file in the given output dir in a file with the name of the actor .hpp.
		If during the convertion a deklaration of a native function has been detected, this function inserts at the beginning of the C++ code the native include statement given as an argument.
		If it cannot open this file, it throws an exception.

	*/
	Converted_Actor convert_RVC_to_Cpp(std::string path_to_src_directory, std::string path_storage, std::string native_include_statement) {
		Token t = token_producer.get_next_Token();
		std::string converted_actor{};
		std::string import_function_declarations;
		while (t.str != "") {
			if (t.str == "import") {
				auto tmp =  convert_import(t, path_to_src_directory);
				converted_actor += tmp.second;
				import_function_declarations += tmp.first;
			}
			else if (t.str == "actor") {
				converted_actor = convert_actor(t, converted_actor,import_function_declarations);
			}
			else if (t.str == "NoOpenCLDueToPrintln") {
				OpenCL = false;
			}
			t = token_producer.get_next_Token();
		}
		if (include_native) {
			converted_actor.insert(0, native_include_statement + "\n");
		}
		if (actor_name.size() != 0) {
			if (OpenCL) {
				std::string p{ path_storage + "\\" + actor_name + ".cl" };
				Converter::convert_Cpp_to_OCL(&cl_file, p);
			}
			std::ofstream output_file{ path_storage + "\\" + actor_name + ".hpp" };
			if (output_file.bad()) {
				throw Converter_Exception{ "Cannot open file " + path_storage + "\\" + actor_name + ".hpp" };
			}
			output_file << converted_actor;
			return Converted_Actor{ actor_name, OpenCL,constructor_parameters_type_name,default_parameter_value_map,actor_scheduling_condition };
		}
		else {
			throw Converter_Exception{ "Could not convert the actor " + id };
		}
	}
};
//initialize static variable
int Actor_Generator::default_number_action{ 1 };






