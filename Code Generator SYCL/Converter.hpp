#pragma once
#include <string>
#include <fstream>
#include "Datastructures.hpp"
/*
	Namespace contains all functions that are used to generate C++ Code.
*/

namespace Converter {

	void convert_Actors(Dataflow_Network* dpn, program_options *opts, std::string native_header_include);

	void create_main(Dataflow_Network *dpn,std::string path,program_options *opt);

	void create_FIFO(std::string path,bool SYCL,bool infQ = false);

}