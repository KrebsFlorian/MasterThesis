#pragma once
#include <string>
#include "Datastructures.hpp"

/*
	Namespace contains all functions that are executed before C++ code is created and are relevant to enable C++ code generation.
*/

namespace Init_Conversion {

	Dataflow_Network* read_network(program_options *opt);

	std::string create_headers_for_native_code(program_options *opt);

	//void create_cmake_build(program_options *opt);

}