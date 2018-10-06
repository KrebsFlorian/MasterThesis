#pragma once
#include<vector>
#include <map>
#include <string>
#include <utility> 
#include <tuple>

/*
	All datastructures that are used by different classes or functions in different namespaces/files
*/

//Contains all program options that can be specified by command line parameters
struct program_options {

	const char *source_directory;
	const char *target_directory;
	std::vector<const char*> native_includes;
	const char *network_file;
	int FIFO_size{ 100000 };
	bool target_CPU{ false };
	int cores{ 1 };
	bool no_SYCL{ false };
	bool cmake{ false };
	bool infQ{ false };
};


struct Connection {
	std::string dst_id;
	std::string dst_port;
	std::string src_id;
	std::string src_port;
	int specified_size{};
	bool dst_network_instance_port{ false };
	bool src_network_instance_port{ false };
};


struct Converted_Actor {
	std::string class_name;
	bool OpenCL;
	std::vector<std::pair<std::string, std::string>> constructor_parameter_type_order;
	std::map<std::string, std::string> default_parameter_values;
	std::string scheduling_condition; //all scheduling conditions or
};

//Stores all relevant information for and during C++ Code generation.
struct Dataflow_Network {
	std::map<std::string,std::string> actors_class_path; 
	std::vector<Connection> connections;
	std::map<std::string, std::string> id_class_map;
	std::map<std::string, Converted_Actor> id_constructor_map;
	std::vector< std::tuple<std::string, std::string, std::string> > parameters;// id name value
};

