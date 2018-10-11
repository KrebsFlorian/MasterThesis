#include "Converter.hpp"
#include "Generator.hpp"

/*
	Function iterates over all actor classes and for each actor class creates an Actor_Generator object and calles the convert function on this actor.
*/
void Converter::convert_Actors(Dataflow_Network* dpn, program_options *opts, std::string native_header_include){

	bool use_cpu{ false };
	if (opts->target_CPU && opts->cores > 0) {
		use_cpu = true;
	}
	
	for (auto it = dpn->id_class_map.begin(); it != dpn->id_class_map.end(); ++it) {
		std::cout << "Converting actor " << it->second<<" with id "<< it->first << "\n";
		dpn->id_constructor_map[it->first] = Actor_Generator{ dpn->actors_class_path[it->second],opts->FIFO_size,!(opts->no_SYCL),use_cpu,opts->cores,dpn,it->first }.convert_RVC_to_Cpp(opts->source_directory,opts->target_directory,native_header_include);
	}
}

