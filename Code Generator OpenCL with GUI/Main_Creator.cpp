#include "Converter.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include "Exceptions.hpp"
using namespace Converter_RVC_Cpp;

/*
	This method takes a list of the actor ids and all connections as input and returns a sorted vector.
	The sorted vector contains the actor ids in the order of the connections. 'In the order' means  
	actors without ingoinging connections are listed first, than their successors and so on.
*/
std::vector<std::string> sort_function(std::vector<std::string>& act_ids,std::vector<Connection>& connections) {
	std::vector<std::string> sorted_actors;
	std::set<std::string> actor_ids_sorted_set;
	//find all nodes without ingoing edges
	//iterate over all actors and check if there are ingoing edges (actor is endpoint of a connection)
	for (auto it = act_ids.begin(); it != act_ids.end(); ++it) {
		bool ingoing_edge{ false };
		for (auto c = connections.begin(); c != connections.end(); ++c) {
			if (c->dst_id == *it) {
				ingoing_edge = true;
			}
		}
		//if the actor has no ingoing edges, it will be added to the list of the sorted actors next.
		if (!ingoing_edge) {
			if (actor_ids_sorted_set.find(*it) == actor_ids_sorted_set.end()) {
				actor_ids_sorted_set.insert(*it);
				sorted_actors.push_back(*it);
			}
		}
	}
	//if no element has been inserted, it must be cycle, thus a starting point will be selected 
	if (sorted_actors.size() == 0) {
		sorted_actors.push_back(act_ids.front());
		actor_ids_sorted_set.insert(act_ids.front());
	}
	
	//insert all nodes, that are directly connected to a node in the sorted set until all are in the sorted list.
	while (actor_ids_sorted_set.size() != act_ids.size()) {
		for (auto it = connections.begin(); it != connections.end(); ++it) {
			if (actor_ids_sorted_set.find(it->src_id) != actor_ids_sorted_set.end()) {
				if (actor_ids_sorted_set.find(it->dst_id) == actor_ids_sorted_set.end()) {
					actor_ids_sorted_set.insert(it->dst_id);
					sorted_actors.push_back(it->dst_id);
				}
			}
		}
	}
	return sorted_actors;
}

/*
	This function returns a string contain the corresponding type of the FIFO connected to the given port of the given actor id.
	Throws an exception if it cannot find the type of the FIFO connected to the given port of the given actor (id).
*/
std::string find_fifo_type(std::string id, std::string port, Dataflow_Network *dpn) {
	//find actor information object related to the id
	auto actor_inf = dpn->id_constructor_map[id];
	//check all constructor parameters of the actor until the one fitting to the given id and port are found.
	for (auto it = actor_inf.constructor_parameter_type_order.begin(); it != actor_inf.constructor_parameter_type_order.end(); ++it) {
		if (it->second == port+"$FIFO") {
			std::string tmp{ it->first };
			tmp.erase(tmp.find("&"));//remove & because the constructor_parameter_type_order_map contains the types of the constructors. But for the definition in the main no reference is required.
			tmp.replace(tmp.find("Port"), 4, "FIFO");
			return tmp;
		}
	}
	//Type not found
	throw Failed_Main_Creation_Exception{ "Cannot find the type of the FIFO connected to port " + port +" of the actor "+id };
}

/*
	Function returns a string containg all fifo names (srcid_srcport_dstid_dstport) identified by the given actor id and port separated by a comma.
*/
std::string find_all_FIFOs_at_this_port(std::string id, std::string port,Dataflow_Network *dpn) {
	std::string output;
	for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
		if ((it->dst_id == id && it->dst_port == port) || (it->src_id == id && it->src_port == port)) {
			if (!output.empty()) {
				output.append(",");
			}
			output.append(it->src_id + "_" + it->src_port + "_" + it->dst_id + "_" + it->dst_port);
		}
	}
	return output;
}

/*
	Function creates c++ expressions to create all ports and returns this in a string.
	Ports are named id of the actor+_+name of the constructor parameter(most likelty port name+$FIFO.
	Every port is created even when no FIFO is attached to it.
	A Ports gets every FIFO as a constructor parameter.
*/
std::string create_ports_string(Dataflow_Network *dpn) {
	std::string output;
	for (auto it = dpn->id_constructor_map.begin(); it != dpn->id_constructor_map.end(); ++it) {
		std::string id{ it->first };
		for (auto it2 = it->second.constructor_parameter_type_order.begin(); it2 != it->second.constructor_parameter_type_order.end(); ++it2) {
			std::string port_dekl_name{ id+"_"+it2->second };
			std::string port_dekl_type{ it2->first };
			std::string port{ it2->second };
			port.erase(port.find("$"));
			port_dekl_type.erase(port_dekl_type.find("&"));
			output.append(port_dekl_type + " " + port_dekl_name + "{" + find_all_FIFOs_at_this_port(id,port,dpn) + "};\n");
		}
	}
	return output;
}

/*
	The function return a string containing the value of a given parameter of a given actor (id).
	A parameter is a input parameter of the constructor of the actor. 
	The function checks if the type is a FIFO. If yes, the corresponding function above is called.
	Otherwise in the list of parameters the corresponding value to the given id,parameter pair is searched.
	Throws an exception if it cannot find the corresponding parameter value.
*/
std::string find_parameter_value(std::string id, std::string parameter, std::string parameter_type, Dataflow_Network *dpn) {
	return id + "_" + parameter;
}   

/*
	The function returns a vector of strings containg the name of ingoing FIFOs of an actor (id).
*/
std::set<std::string> find_ingoing_ports(std::string id, Dataflow_Network *dpn) {
	std::set<std::string> output;
	for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
		if (it->dst_id == id) {
			output.insert( it->dst_id + "_" + it->dst_port+"$FIFO");
		}
	}
	return output;
}

/*
	The function returns a vector of strings containg the name of outgoing FIFOs of an actor (id).
*/
std::set<std::string> find_outgoing_ports(std::string id, Dataflow_Network *dpn) {
	std::set<std::string> output;
	for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
		if (it->src_id == id) {
			output.insert(it->src_id + "_" + it->src_port + "$FIFO" );
		}
	}
	return output;
}

/*
	The function returns a string containg &FIFO name, &FIFO name for all FIFOs that are in or outgoing of the actor with the given id.
*/
std::string insert_Port_names(std::string id, Dataflow_Network *dpn) {
	std::string output;
	auto ingoing = find_ingoing_ports(id,dpn);
	auto outgoing = find_outgoing_ports(id,dpn);
	for (auto it = ingoing.begin(); it != ingoing.end(); ++it) {
		if (output.size() == 0) {
			output.append("&" + *it);
		}
		else {
			output.append(" , &" + *it);
		}
	}

	for (auto it = outgoing.begin(); it != outgoing.end(); ++it) {
		if (output.size() == 0) {
			output.append("&" + *it);
		}
		else {
			output.append(" , &" + *it);
		}
	}
	return output;

}

/*
	Core function that created the main.cpp file. The code in this file creates all FIFO and Actor objects and contains the central scheduler.
	First all include statements are added, next the OpenCL queue is created, then the expressions to create the FIFO and actor objects and some relevant scheduler variables.
	Last the infinite loop of the scheduler is created. In this loop the scheduler checks if the actor is running. If not it restarts it.
	The actors shall be called in the order of the dataflow through the network.

	This function throws an exception if it cannot open the file to write the source code to.
*/
void Converter::create_main(Dataflow_Network *dpn, std::string path, program_options *opt) {
	//add header include 
	std::string second_part;
	std::string first_part{ };
	first_part.append("#include <iostream>\n");
	std::set<std::string> includes;
	for (auto it = dpn->id_constructor_map.begin(); it != dpn->id_constructor_map.end(); ++it) {
		includes.insert("#include \"" + it->second.class_name + ".hpp\"\n");
	}
	for (auto it = includes.begin(); it != includes.end(); ++it) {
		first_part.append(*it);
	}
	first_part.append("#include \"ORCC_Header.h\"\n");
	first_part.append("#include <atomic> \n");
	first_part.append("#include <future>\n\n");
	first_part.append("options_t *opt;\n");

	//add the function to parse command line inputs
	std::string parse_command_line_input_function = "void parse_command_line_input(int argc, char *argv[]) {\n"
		"\topt = new options_t;\n"
		"\t//set default\n"
		"\topt->display_flags = 1;\n"
		"\topt->nbLoops = -1;\n"
		"\topt->nbFrames = -1;\n"
		"\topt->nb_processors = 1;\n"
		"\topt->enable_dynamic_mapping = 0;\n"
		"\topt->nbProfiledFrames = 10;\n"
		"\topt->mapping_repetition = 1;\n"
		"\topt->print_firings = 0;\n"
		"\topt->yuv_file = NULL;\n"
		"\topt->input_directory = NULL;\n"
		"\topt->input_file = NULL;\n"
		"\topt->write_file = NULL;\n"
		"\topt->mapping_input_file = NULL;\n"
		"\topt->mapping_output_file = NULL;\n"
		"//read command line parameters\n"
		"\tfor (int i = 1; i < argc; i++) {\n"
			"\t\tif (strcmp(argv[i], \"-i\") == 0) {\n"
				"\t\t\topt->input_file = argv[++i];\n"
			"\t\t}\n"
			"\t\telse if (strcmp(argv[i], \"-h\") == 0) {\n"
				"\t\t\tstd::cout << \"\\nUsage: %s [options]\\n\"\n"
		"\t\t\t\"\\nCommon arguments:\\n\"\n"
		"\t\t\t\"-i <file>          Specify an input file.\\n\"\n"
		"\t\t\t\"-h                 Print this message.\\n\"\n"
		"\t\t\t\"\\nVideo-specific arguments:\\n\"\n"
		"\t\t\t\"-f <nb frames>     Set the number of frames to decode before exiting.\\n\"\n"
		"\t\t\t\"-n                 Ensure that the display will not be initialized (useful on non-graphic terminals).\\n\"\n"
		"\t\t\t\"\\nRuntime arguments:\\n\"\n"
		"\t\t\t\"-p <file>          Filename to write the profiling information.\\n\"\n"
		"\t\t\t\"-r <nb frames>     Specify the number of frames before mapping or between each mapping {Default : 10}.\\n\"\n"
		"\t\t\t\"-a                 Do a new mapping every <nb frames> setted by previous option.\\n\"\n"
		"\t\t\t\"\\nOther specific arguments:\\n\"\n"
		"\t\t\t\"Depending on how the application has been designed, one of these arguments can be used.\\n\"\n"
		"\t\t\t\"-l <nb loops>      Set the number of readings of the input file before exiting.\\n\"\n"
		"\t\t\t\"-d <directory>     Set the path when multiple input files are required.\\n\"\n"
		"\t\t\t\"-w <file>          Specify a file to write the output stream.\\n\";\n"
		"\t\t\texit(0);\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-f\") == 0) {\n"
		"\t\t\topt->nbFrames = atoi(argv[++i]);\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-n\") == 0) {\n"
		"\t\t\topt->display_flags = 0;\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-p\") == 0) {\n"
		"\t\t\topt->profiling_file = argv[++i];\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-r\") == 0) {\n"
		"\t\t\topt->nbProfiledFrames = atoi(argv[++i]);\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-a\") == 0) {\n"
		"\t\t\topt->mapping_repetition = -1;\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-l\") == 0) {\n"
		"\t\t\topt->nbLoops = atoi(argv[++i]);\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-d\") == 0) {\n"
		"\t\t\topt->input_directory = argv[++i];\n"
		"\t\t}\n"
		"\t\telse if (strcmp(argv[i], \"-w\") == 0) {\n"
		"\t\t\topt->write_file = argv[++i];\n"
		"\t\t}\n"
		"\t\telse {\n"
		"\t\t\tstd::cout << \"Error:Unknown input\" << std::endl;\n"
		"\t\t\texit(0);\n"
		"\t\t}\n"
		"\t}\n"
		"}\n";

	second_part.append(parse_command_line_input_function + "\n\n");

	//create main function head
	second_part.append("int main(int argc, char *argv[]) {\n");
	second_part.append("\tparse_command_line_input(argc, argv);\n");
	//second_part.append("\tconstexpr int max_fifo_size = "+std::to_string(opt->FIFO_size)+";\n");
	//create FIFOs
	for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
		std::string id{ it->dst_id };
		std::string port{ it->dst_port };
		first_part.append(find_fifo_type(it->dst_id, it->dst_port,dpn) + " ");
		first_part.append(it->src_id + "_" + it->src_port + "_" + it->dst_id + "_" + it->dst_port + "{};\n");
	}
	
	first_part.append("\n");
	//create ports
	first_part.append(create_ports_string(dpn));

	first_part.append("\n");

	//create actors
	for (auto it = dpn->id_constructor_map.begin(); it != dpn->id_constructor_map.end(); ++it) {
		Converted_Actor actor_inf = it->second;
		if (actor_inf.OpenCL) {
			first_part.append(actor_inf.class_name + " *" + it->first+"_instance;\n");
			second_part.append("\t"+it->first+"_instance = new "+ actor_inf.class_name+"{");
		}
		else {
			first_part.append(actor_inf.class_name + " *" + it->first + "_instance;\n");
			second_part.append("\t" + it->first + "_instance = new " + actor_inf.class_name + "{");
		}
		for (auto param = actor_inf.constructor_parameter_type_order.begin(); param != actor_inf.constructor_parameter_type_order.end(); ++param) {
			if (param == actor_inf.constructor_parameter_type_order.begin()) {
				second_part.append(find_parameter_value(it->first, param->second, param->first,dpn));
			}
			else {
				second_part.append("," + find_parameter_value(it->first, param->second, param->first, dpn));
			}
		}
		second_part.append("};\n");
		//variables for the scheduler
		first_part.append("std::atomic_flag " + it->first + "_running = ATOMIC_FLAG_INIT;\n");

	}
	//create the global scheduler
	first_part.append("void global_scheduler() {\n");
	first_part.append("\tfor(;;){\n");
	//create a list of all actors
	std::vector<std::string> actor_ids;
	for (auto it = dpn->id_class_map.begin(); it != dpn->id_class_map.end(); ++it) {
		actor_ids.push_back(it->first);
	}
	//sort the actors in the way they have to be called by the scheduler
	std::vector<std::string> actor_ids_sorted = sort_function(actor_ids, dpn->connections);
	//create a scheduler check for each actor
	for (auto it = actor_ids_sorted.begin(); it != actor_ids_sorted.end(); ++it) {
		Converted_Actor actor_inf = dpn->id_constructor_map[*it];

		//check if it is running, if not than start an async thread with the actor
		first_part.append("\t\tif(!" + *it + "_running.test_and_set()){\n");
		first_part.append("\t\t\t" + *it + "_instance->schedule();\n");
		first_part.append("\t\t\t"+*it+"_running.clear();\n");
		first_part.append("\t\t}\n");

	}
	first_part.append("\t}\n");//close for
	first_part.append("}\n\n");//close global_scheduler function
	//create threads and close main
	second_part.append("\n\n");
	second_part.append("\tstd::async(std::launch::async, global_scheduler);\n");
	second_part.append("\tstd::async(std::launch::async, global_scheduler);\n");
	second_part.append("\tstd::async(std::launch::async, global_scheduler);\n");
	second_part.append("\tstd::async(std::launch::async, global_scheduler);\n");
	second_part.append("\tstd::async(std::launch::async, global_scheduler);\n");
	second_part.append("\tglobal_scheduler();\n");
	second_part.append("\treturn 0;\n");
	second_part.append("}\n");//close main

	std::ofstream output_file{ path };
	if (output_file.bad()) {
		throw Failed_Main_Creation_Exception{ "Cannot open the file " + path };
	}
	output_file << first_part+second_part;
}
