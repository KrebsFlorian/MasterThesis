#include <iostream>
#include <memory>
#include <sstream> 
#include "Initializer.hpp"
#include "Converter.hpp"
#include "Unit_Generator.hpp"
using namespace std;


program_options* parse_command_line_input(int argc, char *argv[]) {

	program_options *opt = new program_options;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			opt->native_includes.push_back(argv[++i]);
		}
		else if (strcmp(argv[i], "-h") == 0) {
			std::cout << "\nUsage: %s [options]\n"
				"\nCommon arguments:\n"
				"-i <file>          Specify an include file for native code.\n"
				"-h                 Print this message.\n"
				"-w <directory>     Specify the directory to write the output to.\n"
				"-s <number>        Specify the size of the FIFOs.\n"
				"-d <directory>     Specify the directory of the RVC-CAL sources.\n"
				"-n <file>          Specify the top network that shall be converted.\n"
				"-p         		If this flag is set, OpenCL will not be used.\n";
			exit(0);
		}
		else if (strcmp(argv[i], "-w") == 0) {
			opt->target_directory = argv[++i];
		}else if(strcmp(argv[i],"-s")==0){
			opt->FIFO_size = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-d") == 0) {
			opt->source_directory = argv[++i];
		}
		else if (strcmp(argv[i], "-n") == 0) {
			opt->network_file = argv[++i];
		}
		else if (strcmp(argv[i], "-p") == 0) {
			opt->no_OpenCL = true;
		}
		else {
			std::cout << "Error:Unknown input" << std::endl;
			exit(0);
		}
	}
	return opt;
}


int main(int argc, char *argv[]) {
	
	unique_ptr<program_options> opts(parse_command_line_input(argc, argv));
	std::cout << "Reading the network...\n";
	unique_ptr<Dataflow_Network> dpn(Init_Conversion::read_network(opts.get()));
	if (!opts->native_includes.empty()) {
		std::cout << "Creating headers for the native includes...\n";
	}
	std::string native_header_include = Init_Conversion::create_headers_for_native_code(opts.get());
	std::cout << "Creating the FIFO file and writting the FIFO code...\n";
	Converter::create_FIFO(std::string{ opts->target_directory },!opts->no_OpenCL);
	if (!opts->no_OpenCL) {
		std::cout << "Creating the OpenCL utilities...\n";
		Converter::create_OpenCL_Utils(opts->target_directory);
	}
	std::cout << "Converting the Actors...\n";
	Converter::convert_Actors(dpn.get(),opts.get(), native_header_include);
	std::cout << "Creating the main...\n";
	Converter::create_main(dpn.get(), std::string{ opts->target_directory }+"\\main.cpp", opts.get());
	std::cout << "Conversion from RVC to C++/OpenCL was successful\n";
	system("pause");
	return 0;
}


