#include "CodeGeneratorGUI.h"
#include <QtWidgets/QApplication>
#include <iostream>
#include <memory>
#include <sstream> 
#include "Initializer.hpp"
#include "Converter.hpp"

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
				"-cpu               Target CPU without the restriction to cores.\n"
				"-c <number>        Target CPU with SYCL calls. Specify the number of cores to use.\n"
				//"-infQ				If set, a dequeue based FIFO implementation is used.\n"
				"-p         		If this flag is set, SYCL will not be used.\n";
			exit(0);
		}
		else if (strcmp(argv[i], "-w") == 0) {
			opt->target_directory = argv[++i];
		}
		else if (strcmp(argv[i], "-s") == 0) {
			opt->FIFO_size = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-d") == 0) {
			opt->source_directory = argv[++i];
		}
		else if (strcmp(argv[i], "-n") == 0) {
			opt->network_file = argv[++i];
		}
		else if (strcmp(argv[i], "-c") == 0) {
			opt->target_CPU = true;
			opt->cores = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-p") == 0) {
			opt->no_SYCL = true;
		}
		else if (strcmp(argv[i], "-infQ") == 0) {
			opt->infQ = true;
		}
		else if (strcmp(argv[i], "-cpu") == 0) {
			opt->target_CPU = true;
			opt->cores = 0;
		}
		else {
			std::cout << "Error:Unknown input" << std::endl;
			exit(0);
		}
	}
	return opt;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		QApplication a(argc, argv);
		CodeGeneratorGUI w;
		w.show();
		return a.exec();
	}
	else {
		std::unique_ptr<program_options> opts(parse_command_line_input(argc, argv));
		std::cout << "Reading the network...\n";
		std::unique_ptr<Dataflow_Network> dpn(Init_Conversion::read_network(opts.get()));
		if (!opts->native_includes.empty()) {
			std::cout << "Creating headers for the native includes...\n";
		}
		std::string	native_header_include = Init_Conversion::create_headers_for_native_code(opts.get());
		std::cout << "Creating the FIFO file and writting the FIFO code...\n";
		Converter::create_FIFO(std::string{ opts->target_directory }, !opts->no_SYCL, opts->infQ);
		std::cout << "Converting the Actors...\n";
		Converter::convert_Actors(dpn.get(), opts.get(), native_header_include);
		std::cout << "Creating the main...\n";
		Converter::create_main(dpn.get(), std::string{ opts->target_directory }+"\\main.cpp", opts.get());
		std::cout << "Conversion from RVC to C++/SYCL was successful\n";
	}
	return 0;
}
