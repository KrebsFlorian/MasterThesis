#include "Initializer.hpp"
#include <sstream> 
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include "rapidxml.hpp"
#include <algorithm>
#include <set>
#include "Exceptions.hpp"
using namespace rapidxml;

//set for all network instance ids
static std::set<std::string> network_instance_ids;

static int max_specified_fifo_size{};

/*
	Function tests if a file specified by the input parameter exists. If it exists it returns true. Otherwise it returns false.
*/
bool file_exists(const char *f)
{
	std::ifstream stream(f);
	return stream.good();
}

/*
	The function tests if the file path_string.xdf exists.
	If yes it return true, otherwise it returns false.
*/
bool network_file(std::string path_string) {
	if (file_exists(path_string.append(".xdf").c_str())) {
		return true;
	}
	return false;
}

//Deklaration due to a circular dependency between start_paring and parse_network
void start_parsing(std::string& start, const char *path, Dataflow_Network *dpn, std::string prefix = "");

/*
	Function that parses a complete network. The starting node is the network_node input parameter.
	The input parameter prefix is used as a prefix for all actor ids to distinguish between the same actors used in different network instances.
	For every connection a connection object is created and stored in the given dpn.
	For each instance the function checks if it is a instance of another network (.xdf) or a actor (.cal). The actors are parsed recursively and the actors are stored in the dpn object.
	Additionally, the input parameters for the actors are stored in the dpn and inserts all network instance ids to the network_instance_ids set.
*/
void parse_network(const rapidxml::xml_node<>* network_node,const char *path,Dataflow_Network *dpn,std::string prefix) {

	std::map<std::string, std::string> decl_parameter_map;
	std::set<std::string> network_instances;

	for (const rapidxml::xml_node<>* sub_node = network_node->first_node(); sub_node; sub_node = sub_node->next_sibling()) {

		if (strcmp(sub_node->name(), "Decl") == 0) {
			//find name
			std::string decl_name;
			for (auto attributes = sub_node->first_attribute(); attributes; attributes = attributes->next_attribute()) {
				if (strcmp(attributes->name(), "name") == 0) {
					decl_name = attributes->value();
				}
			}
			//find value
			std::string value;
			for (auto sub_node_decl = sub_node->first_node(); sub_node_decl; sub_node_decl = sub_node_decl->next_sibling()) {
				if (strcmp(sub_node_decl->name(), "Expr") == 0) {
					bool string{ false };
					for (auto attributes = sub_node_decl->first_attribute(); attributes; attributes = attributes->next_attribute()) {
						if (strcmp(attributes->name(), "value") == 0) {
							if (string) {
								value.append("\"");
								value.append(attributes->value());
								value.append("\"");
							}
							else {
								value = attributes->value();
							}
						}
						else if (strcmp(attributes->name(),"literal-kind") == 0 && strcmp(attributes->value(),"String") == 0) {
							string = true;
						}
					}
				}
			}
			decl_parameter_map[decl_name] = value;
		}

		if (strcmp(sub_node->name(), "Connection") == 0) {
			Connection conec;
			for (const rapidxml::xml_attribute<>* connection_attribute = sub_node->first_attribute(); connection_attribute; connection_attribute = connection_attribute->next_attribute()) {
				if (strcmp(connection_attribute->name(), "dst") == 0) {
					std::string str{ connection_attribute->value() };
					if (str.size() == 0) {
						conec.dst_id = prefix.substr(0, prefix.size() - 1) + str;
						conec.dst_network_instance_port = true;
					}
					else {
						conec.dst_id = prefix + str;
						if (network_instances.find(str) != network_instances.end()) {
							conec.dst_network_instance_port = true;
						}
					}
				}
				if (strcmp(connection_attribute->name(), "dst-port") == 0) {
					conec.dst_port = connection_attribute->value();
				}
				if (strcmp(connection_attribute->name(), "src") == 0) {
					std::string str{ connection_attribute->value() };
					if (str.size() == 0) {
						conec.src_id = prefix.substr(0, prefix.size() - 1) + str;
						conec.src_network_instance_port = true;
					}
					else {
						conec.src_id = prefix + connection_attribute->value();
						if (network_instances.find(str) != network_instances.end()) {
							conec.src_network_instance_port = true;
						}
					}
				}
				if (strcmp(connection_attribute->name(), "src-port") == 0) {
					conec.src_port = connection_attribute->value();
				}
			}
			for (auto x = sub_node->first_node(); x; x = x->next_sibling()) {
				if (strcmp(x->name(),"Attribute")==0) {
					for (auto s = x->first_node(); s; s = s->next_sibling()) {
						if (strcmp(s->name(), "Expr") == 0) {
							for (auto a = s->first_attribute(); a; a = a->next_attribute()) {
								if (strcmp(a->name(), "value") == 0) {
									conec.specified_size = std::atoi(a->value());
								}
							}
						}
					}
				}
			}
			dpn->connections.push_back(conec);
		}
		if (strcmp(sub_node->name(), "Instance") == 0) {
			const rapidxml::xml_attribute<> *id_attribute = sub_node->first_attribute();
			for (const rapidxml::xml_node<>* instance_node = sub_node->first_node(); instance_node; instance_node = instance_node->next_sibling()) {
				if (strcmp(instance_node->name(), "Class") == 0) {
					const rapidxml::xml_attribute<>* a = instance_node->first_attribute();
					std::string path_string{ path };
					path_string.append("\\");
					std::string str{ a->value() };
					std::replace(str.begin(), str.end(), '.', '\\');
					path_string.append(str);
					if (network_file(path_string)) {
						network_instances.insert(id_attribute->value());
						path_string.append(".xdf");
						network_instance_ids.insert(prefix + id_attribute->value());
						start_parsing(path_string, path, dpn, std::string{ prefix+id_attribute->value()}+"_");
					}
					else {
						path_string.append(".cal");
						dpn->actors_class_path[a->value()] = path_string;
						dpn->id_class_map[prefix+id_attribute->value()] = a->value();
					}

				}
				else if (strcmp(instance_node->name(), "Parameter") == 0) {
					const rapidxml::xml_attribute<>* parameter_name = instance_node->first_attribute();
					const rapidxml::xml_node<>* expr_node = instance_node->first_node();
					const rapidxml::xml_attribute<>* expr_attribute;
					bool literal_type_is_string{ false };
					std::string value;
					bool variable_decl_lookup{ true }; //is set to false if a literal-kind is found, true indicates that the parameter is a variable decl
					for (expr_attribute = expr_node->first_attribute(); expr_attribute; expr_attribute = expr_attribute->next_attribute()) {
						if (strcmp(expr_attribute->name(), "value") == 0) {
							if (literal_type_is_string) {
								value.append("\"");
								value.append(expr_attribute->value());
								value.append("\"");
							}
							else {
								value = expr_attribute->value();
							}
						}
						else if (strcmp(expr_attribute->name(),"literal-kind")== 0) {
							variable_decl_lookup = false;
							if (strcmp(expr_attribute->value(), "String") == 0) {
								literal_type_is_string = true;
							}
						}
						else if (strcmp(expr_attribute->name(), "kind") == 0 && strcmp(expr_attribute->value(),"UnaryOp") == 0 ) {
							for (auto param_sub_nodes = expr_node->first_node(); param_sub_nodes; param_sub_nodes = param_sub_nodes->next_sibling()) {
								if (strcmp(param_sub_nodes->name(), "Op") == 0) {//find operator and put it at the beginning of the value string
									for (auto op_attribute = param_sub_nodes->first_attribute(); op_attribute; op_attribute = op_attribute->next_attribute()) {
										if (strcmp(op_attribute->name(),"name") == 0) {
											value.insert(0, op_attribute->value());
										}
									}
								}
								else if (strcmp(param_sub_nodes->name(), "Expr") == 0) {//find value
									for (auto op_attribute = param_sub_nodes->first_attribute(); op_attribute; op_attribute = op_attribute->next_attribute()) {
										if (strcmp(op_attribute->name(), "value") == 0) {
											if (literal_type_is_string) {
												value.append("\"");
												value.append(op_attribute->value());
												value.append("\"");
											}
											else {
												value = op_attribute->value();
											}

										}
										else if (strcmp(op_attribute->name(), "literal-kind") == 0) {
											variable_decl_lookup = false;
											if (strcmp(op_attribute->value(), "String") == 0) {
												literal_type_is_string = true;
											}
										}
									}
								}
							}
						}
					}
					if (variable_decl_lookup) {
						value = decl_parameter_map[value];
					}
					dpn->parameters.push_back(std::make_tuple(prefix+id_attribute->value(), parameter_name->value(),value ));
				}
			}
		}
	}
}
/*
	Open the file that is specified by the path given in start. The input parameter *path contains the path to the sources of the RVC network.
	Prefix contains the name of the instance, if this network is instanziated in another network.
	This function reads the content of the file and creates a xml parser object and calls parse_network to parse the network.

	This function throws an exception if it cannot open the file.
*/
void start_parsing(std::string& start, const char *path,Dataflow_Network *dpn,std::string prefix) {
	std::ifstream network_file(start, std::ifstream::in);
	if (network_file.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + start };
	}
	std::stringstream Top_network_buffer;
	Top_network_buffer << network_file.rdbuf();
	std::string str_to_parse = Top_network_buffer.str();
	char* buffer = new char[str_to_parse.size() + 1];
	strcpy(buffer, str_to_parse.c_str());
	xml_document<char> doc;
	doc.parse<0>(buffer);
	parse_network(doc.first_node(), path,dpn,prefix);
}
/*
	This function checks whether there are still ports of network instances left in the list of the connections.
*/
bool are_network_instance_ports_present(std::vector<Connection>& connections) {
	for (auto connection = connections.begin(); connection != connections.end(); ++connection) {
		if (connection->dst_network_instance_port || connection->src_network_instance_port) {
			return true;
		}
	}
	return false;
}

/*
	This function creates a vector with all connections that are between two actors or starting at an actor and going to a network port.
*/
std::vector<Connection> find_start_connections(Dataflow_Network *dpn) {
	std::vector<Connection> return_value;
	for (auto it = dpn->connections.begin(); it != dpn->connections.end(); ++it) {
		if (!it->dst_network_instance_port && !it->src_network_instance_port) {
			return_value.push_back(*it);
		}
		else if (it->dst_network_instance_port && !it->src_network_instance_port) {
			return_value.push_back(*it);
		}
	}
	return return_value;
}

/*
	This function connects connections that are going to a network port with another connection that are starting with the network port and combines these two to one connection.
	This connection is added to the output along with the connections between two actors.
*/
std::vector<Connection> connection_network_ports(Dataflow_Network *dpn, std::vector<Connection>& new_connections) {
	std::vector<Connection> return_value;
	for (auto it = new_connections.begin(); it != new_connections.end(); ++it) {
		if (it->dst_network_instance_port) {
			for (auto connec_it = dpn->connections.begin(); connec_it != dpn->connections.end(); ++connec_it) {
				if (it->dst_id == connec_it->src_id && it->dst_port == connec_it->src_port) {
					Connection connect;
					connect.dst_id = connec_it->dst_id;
					connect.dst_port = connec_it->dst_port;
					connect.dst_network_instance_port = connec_it->dst_network_instance_port;
					connect.src_id = it->src_id;
					connect.src_port = it->src_port;
					connect.src_network_instance_port = it->src_network_instance_port;
					connect.specified_size = std::max(it->specified_size, connec_it->specified_size);
					return_value.push_back(connect);
				}
			}
		}
		else {
			return_value.push_back(*it);
		}
	}
	return return_value;
}



/*
	This function parses the complete network and removes all network instances and replaces them by 
	the containing actors and connections directly between two actors without network ports between them. 
	Every remaining connection, actor id and the corresponding path the source file is inserted into a Dataflow_Network object.
*/
Dataflow_Network* Init_Conversion::read_network(program_options *opt) {


	Dataflow_Network *dpn = new Dataflow_Network;
	std::string str{ opt->network_file };
	start_parsing(str,opt->source_directory,dpn);
	std::vector<Connection> starting_point{ find_start_connections(dpn) };
	while (are_network_instance_ports_present(starting_point)) {
		starting_point = connection_network_ports(dpn, starting_point);

	}
	dpn->connections = starting_point;
	return dpn;
};

