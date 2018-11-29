#include "Initializer.hpp"
#include <fstream>
#include <regex>
#include <iostream>
#include <sstream>
#include "Exceptions.hpp"

//indicates if the ORCC Options Header was already created to prevent further creations.
static bool ORCC_Options_Header_created{ false };

/*
	Function that writes the ORCC Header to the file path\\ORCC_Header.h .
	Throws an exception if this file cannot be opened.
*/
void create_ORCC_Options_header(std::string path) {
	std::string header = "#ifndef OPTIONS_HEADER\n#define OPTIONS_HEADER\n	//Struct copied from ORCC - the native code seems to use this to read several input options from.\n"
		"struct ORCC_options {\n"
		"\t/* Video specific options */\n"
		"\tchar *input_file;\n"
		"\tchar *input_directory;             // Directory for input files.\n"
		"\t/* Video specific options */\n"
		"\tchar display_flags;              // Display flags\n"
		"\tint nbLoops;                         // (Deprecated) Number of times the input file is read\n"
		"\tint nbFrames;                       // Number of frames to display before closing application\n"
		"\tchar *yuv_file;                      // Reference YUV file\n"
		"\t/* Runtime options */\n"
		"\t//schedstrategy_et sched_strategy;     // Strategy for the actor scheduling\n"
		"\tchar *mapping_input_file;            // Predefined mapping configuration\n"
		"\tchar *mapping_output_file;           //\n"
		"\tint nb_processors;\n"
		"\tint enable_dynamic_mapping;\n"
		"\t//mappingstrategy_et mapping_strategy; // Strategy for the actor mapping\n"
		"\tint nbProfiledFrames;                // Number of frames to display before remapping application\n"
		"\tint mapping_repetition;              // Repetition of the actor remapping\n"
		"\t char *profiling_file; // profiling file\n"
		"\t char *write_file; // write file\n"
		"\t/* Debugging options */\n"
		"\tint print_firings;\n"
		"};\n"
		"typedef struct ORCC_options options_t;\n"
		"#endif";
	if (!ORCC_Options_Header_created) {
		std::ofstream output_file{ path + "\\ORCC_Header.h" };
		if (output_file.bad()) {
			throw Converter_RVC_Cpp::Converter_Exception{ "Failed to open the file " + path + "\\ORCC_Header.h" };
		}
		output_file << header;
		ORCC_Options_Header_created = true;
	}
}

/*
	The function takes the content of an native include file, the target directory where the output shall be written to and the name of native include file (e.g. linux.h -> linux) as input.
	The functions creates a header and a source file as ouput.
	It is expected that the content of the native include file is a string containing C code.
	The header file contains a header for the given input code and the source file the contains the implementation.
	The header is written to name.h and the source is written to name.c .
	The function throws an exception if it cannot open a file.
*/
void convert_native_include_to_header_source(std::string file_content, std::string target_dir, std::string name, std::string guard) {
	std::string header{ file_content };
	//regular expression to find {...}
	std::regex r_function_body("\\{[^\\{]*?\\}");
	//regular expression to find all variables except the ones tagged with extern, because they must be declared in the header
	std::regex r_variable("(^(?!extern).*;)");
	//regular expression to find the string #include options.h
	std::regex r_options_t_import("#include \"options.h\"");
	//regular expression to find more than one consecutive newlines
	std::regex r_newline("[\n]{2,}");
	std::regex r_semicolon("(\n;){1}");
	std::regex r_rest("\\{[^\\{]*?;");
	std::regex r_array_assignment_rest("(^(?!extern).*=.*;)");
	//remove all variables except extern
	header = std::regex_replace(header, r_variable, "");
	//replace options.h by ORCC_Header.h
	header = std::regex_replace(header, r_options_t_import, "#include \"ORCC_Header.h\"");
	//remove {...} blocks until all are gone. Then all function bodys have been removed.
	while(header.find("}") != std::string::npos){
		header = std::regex_replace(header, r_function_body, ";");
	}
	while (header.find("{") != std::string::npos) {
		header = std::regex_replace(header, r_rest, ";");
	}
	header = std::regex_replace(header, r_array_assignment_rest, "");
	header = std::regex_replace(header, r_newline, "\n\n");
	header = std::regex_replace(header, r_semicolon, ";");
	//add guard
	header.insert(0, "#ifndef " + guard + "\n#define " + guard + "\n\n");
	header.append("\n#endif\n");
	//write to file
	std::ofstream output_file_header{ target_dir + "\\" + name + ".h" };
	if (output_file_header.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + target_dir + "\\" + name + ".h" };
	}
	output_file_header << header;
	output_file_header.close();
	//create source file
	std::string source{file_content };
	//regular expression to find includes
	std::regex r_includes("#include (\"|<).*?(\"|>)");
	//regular expression to find all variables tagged with extern
	std::regex r_externs("(extern).*?;");
	//remove all includes
	source = std::regex_replace(source, r_includes, "");
	//include the header
	source.insert(0, "#include \""+name+".h\"\n");
	//remove all extern variables
	source = std::regex_replace(source, r_externs, "");
	source = std::regex_replace(source, r_newline, "\n\n");
	//write to a file
	std::ofstream output_file_source{ target_dir + "\\" + name + ".c" };
	if (output_file_source.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + target_dir + "\\" + name + ".c" };
	}
	output_file_source << source;
	output_file_source.close();
}

/*
	The function iterates over all native includes specified in the given program_options object and creates a header and a implementation file for them.
	Additionally it returns a string containg the include expressions for all header wrapped by a extern "C" block.
	This string can be used to include the header everywhere where @native functions are used.
	The function throws an exception if it cannot read one of the specified files.
*/
std::string Init_Conversion::create_headers_for_native_code(program_options *opt) {
	std::string output{ "extern \"C\" {\n" };
	//ORCC_Header header must be created.
	create_ORCC_Options_header(opt->target_directory);

	//counter to create unique guards for the headers
	int counter{ 1 };
	for (auto it = opt->native_includes.begin(); it != opt->native_includes.end(); ++it) {
		//extract name between the last \ and the last . in the string
		std::string path{ *it };
		int start = path.find_last_of("\\") + 1;
		int end = path.find_last_of(".") - start;
		std::string name{path.substr(start,end)};
		//add include to the extern "C" block
		output.append("#include \""+name + ".h\"\n");
		//open file and read code
		std::ifstream include_file(*it, std::ifstream::in);
		if (include_file.bad()) {
			std::string tmp{ "Cannot open the file "};
			tmp.append(*it);
			throw Converter_RVC_Cpp::Converter_Exception{  tmp };
		}
		std::stringstream buffer;
		buffer << include_file.rdbuf();
		std::string guard{ "NATIVE_INCLUDE_" + std::to_string(counter) + "_INCLUDED" };
		std::string output_path{ opt->target_directory };
		convert_native_include_to_header_source(buffer.str(), output_path , name, guard);
		++counter;
	}
	return output + "}\n";
}
