#include "Converter.hpp"
#include <fstream>
#include "Exceptions.hpp"



/*
	Function writes the FIFO code to the file path\\FIFO.hpp
	Throws an exception if it cannot open the file.
*/
void Converter::create_FIFO(std::string path,bool OpenCL) {

	std::string FIFO_code = "#pragma once\n"
		"#include <string>\n"
		"#include <algorithm>\n"
		"#include <iostream>\n\n"

	"template<int N>\n"
	"struct FIFO_State {\n"
		"\tint read_index{ 0 };\n"
		"\tint write_index{ 0 };\n"
		"\tint max_size{ N };\n"
		"\tbool full{ false };\n"
	"};\n\n\n"


	"template<typename T, int N>\n"
	"class FIFO {\n"
		"\tFIFO_State<N> state;\n"
		"\tT *data_ptr = new T[N];\n\n"

	"public:\n"
		"\tFIFO();\n"
		"\tint get_size() const;\n"
		"\tT get_element();\n"
		"\tvoid put_element(T element);\n"
		"\tint get_free_space() const;\n"
		"\tvoid get_elements(T *elements, int elements_count);\n"
		"\tvoid put_elements(T *elements, int element_count);\n"
		"\tT element_preview(int offset = 0);\n"
		"\tT* get_data_ptr() const;\n" 
		"\tint get_write_index() const;\n" 
		"\tint get_read_index() const;\n"
		"\tvoid notify_read_done(int read_elements);\n"
		"\tvoid notify_write_done(int written_elements);\n" 
	"};\n\n\n"


	"template<typename T, int N>\n"
	"FIFO<T, N>::FIFO() {}\n\n"

	"template<typename T, int N>\n"
	"int FIFO<T, N>::get_size() const {\n"
		"\tif (state.full) return N;\n"
		"\tint s = (N + (state.write_index - state.read_index) % N) % N;\n"
		"\treturn s;\n"
	"};\n\n"


	"template<typename T, int N>\n"
	"T FIFO<T, N>::get_element() {\n"
		"\tT element = data_ptr[state.read_index++];\n"
		"\tif (state.read_index == N)state.read_index = 0;\n"
		"\tif(state.full && state.read_index != state.write_index)state.full = false;\n"
		"\treturn element;\n"
	"}\n\n"


	"template<typename T, int N>\n"
	"void FIFO<T, N>::put_element(T element) {\n"
		"\tdata_ptr[state.write_index++] = element;\n"
		"\tif (state.write_index == N)state.write_index = 0;\n"
		"\tif (state.read_index == state.write_index) state.full = true;\n"
	"}\n\n"


	"template<typename T, int N>\n"
	"int FIFO<T, N>::get_free_space() const {\n"
		"\treturn N - get_size();\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T, N>::get_elements(T *elements, int elements_count) {\n"
		"\tfor (int i = 0; i < elements_count; i++) {\n"
			"\t\telements[i] = get_element();\n"
		"\t}\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T, N>::put_elements(T *elements, int elements_count) {\n"
		"\tfor (int i = 0; i < elements_count; i++) {\n"
			"\t\tput_element(elements[i]);\n"
		"\t}\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"T FIFO<T,N>::element_preview(int offset){\n"
		"\tint index = (state.read_index+offset)%N;\n"
		"\treturn data_ptr[index];\n"
	"}"

	"template<typename T, int N>\n"
	"T* FIFO<T,N>::get_data_ptr() const {\n"
		"\treturn data_ptr;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int FIFO<T,N>::get_write_index() const {\n"
		"\treturn state.write_index;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int FIFO<T,N>::get_read_index() const {\n"
		"\treturn state.read_index;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T,N>::notify_read_done(int read_elements) {\n"
		"\tstate.read_index = (state.read_index + read_elements) % N;\n"
		"\tif(state.full && state.read_index != state.write_index)state.full = false;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T,N>::notify_write_done(int written_elements) {\n"
		"\tstate.write_index = (state.write_index + written_elements) % N;\n"
		"\tif (state.write_index == state.read_index) {\n"
			"\t\tstate.full = true;\n"
		"\t}\n"
	"}\n";	

	
	std::string port = "#pragma once\n"
		"#include <vector>\n"
		"#include <algorithm>\n"
		"#include <initializer_list>\n"
		"#include <functional>\n"
		"#include \"FIFO.hpp\"\n"
		"#include <CL\\cl.hpp>\n"
		"#include \"utils.hpp\"\n"
		"using namespace std;\n\n"


	"template<typename T, int N>\n"
	"class Port {\n"
	"private:\n"
		"\tstd::vector<std::reference_wrapper<FIFO<T, N>>> connected_FIFOs;\n"
		"\tcl_mem read_buffer;\n"
		"\tcl_mem write_buffer;\n"
		"\tint write_offset{ 0 };\n\n"

	"public:\n"
		"\tPort(std::initializer_list<std::reference_wrapper<FIFO<T, N>>> init_list);\n"
		"\tint get_size() const;\n"
		"\tT get_element();\n"
		"\tvoid put_element(T element);\n"
		"\tint get_free_space() const;\n"
		"\tvoid get_elements(T *elements, int elements_count);\n"
		"\tvoid put_elements(T *elements, int element_count);\n"
		"\tT element_preview(int offset = 0);\n"
		"\tcl_mem* get_read_buffer(const int number_elements, opencl_arguments &ocl);\n"
		"\tcl_mem* get_write_buffer(const int number_elements, opencl_arguments &ocl);\n"
		"\tvoid opencl_read_done();\n"
		"\tvoid opencl_write_done(opencl_arguments &ocl);\n"
	"};\n\n"

	"template<typename T, int N>\n"
	"Port<T, N>::Port(std::initializer_list<std::reference_wrapper<FIFO<T, N>>> init_list) : connected_FIFOs(init_list) {\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int Port<T, N>::get_size() const {\n"
		"\treturn connected_FIFOs.front().get().get_size();\n"
	"};\n\n"

	"template<typename T, int N>\n"
	"T Port<T, N>::get_element() {\n"
		"\treturn connected_FIFOs.front().get().get_element();\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void Port<T, N>::put_element(T element) {\n"
		"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
		"\t\tit->get().put_element(element);\n"
		"\t}\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int Port<T, N>::get_free_space() const {\n"
		"\tint size{ 500000000 };//dummy number\n"
		"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
		"\t\tsize = size < it->get().get_free_space()?size: it->get().get_free_space();\n"
		"\t}\n"
		"\treturn size;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void Port<T, N>::get_elements(T *elements, int elements_count) {\n"
		"\tconnected_FIFOs.front().get().get_elements(elements, elements_count);\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void Port<T, N>::put_elements(T *elements, int elements_count) {\n"
		"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
		"\t\tit->get().put_elements(elements, elements_count);\n"
		"\t}\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"T Port<T, N>::element_preview(int offset) {\n"
		"\treturn connected_FIFOs.front().get().element_preview(offset);\n"
	"}\n\n"
		
	"template<typename T, int N>\n"
	"cl_mem* Port<T, N>::get_read_buffer(const int number_elements, opencl_arguments &ocl) {\n"
		"\tcl_int err = CL_SUCCESS;\n"
		"\tread_buffer = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE /*| CL_MEM_ALLOC_HOST_PTR*/, sizeof(T) * number_elements, NULL, &err);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clCreateBuffer for inputBuffer returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\tT *resultPtr = (T *)clEnqueueMapBuffer(ocl.commandQueue, read_buffer, true, CL_MAP_WRITE, 0, sizeof(T) * number_elements, 0, NULL, NULL, &err);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error:(get_read_buffer) clEnqueueMapBuffer returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\terr = clFinish(ocl.commandQueue);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clFinish returned %s during the creation of the initial buffer\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\tFIFO<T, N>& attached_read_fifo = connected_FIFOs.front().get();\n"
		"\tif (attached_read_fifo.get_read_index() + number_elements > N) {\n"
			"\t\tint read_before_cycle = N - attached_read_fifo.get_read_index();\n"
			"\t\tint read_after_cycle = number_elements - read_before_cycle;\n"
			"\t\tmemcpy(resultPtr, attached_read_fifo.get_data_ptr() + attached_read_fifo.get_read_index(), sizeof(T) * read_before_cycle);\n"
			"\t\tmemcpy(resultPtr + read_before_cycle, attached_read_fifo.get_data_ptr(), sizeof(T) * read_after_cycle);\n"
		"\t}\n"
		"\telse {\n"
			"\t\tmemcpy(resultPtr, attached_read_fifo.get_data_ptr() + attached_read_fifo.get_read_index(), sizeof(T) * number_elements);\n"
		"\t}\n"
		"\tattached_read_fifo.notify_read_done(number_elements);\n"
		"\terr = clEnqueueUnmapMemObject(ocl.commandQueue, read_buffer, resultPtr, 0, NULL, NULL);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clEnqueueUnmapMemObject returned %s during the creation of the initial input buffer\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\treturn &read_buffer;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"cl_mem* Port<T, N>::get_write_buffer(const int number_elements, opencl_arguments &ocl) {\n"
		"\tcl_int err = CL_SUCCESS;\n"
		"\twrite_buffer = clCreateBuffer(ocl.context, CL_MEM_READ_WRITE /*| CL_MEM_ALLOC_HOST_PTR*/, sizeof(T) * number_elements, NULL, &err);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clCreateBuffer for outputBuffer returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\twrite_offset = number_elements;\n"
		"\treturn &write_buffer;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void Port<T, N>::opencl_read_done() {\n"
		"\tif(read_buffer == NULL)return;\n"
		"\tcl_int err = CL_SUCCESS;\n"
		"\terr = clReleaseMemObject(read_buffer);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clReleaseMemObject returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\tread_buffer = NULL;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void Port<T, N>::opencl_write_done(opencl_arguments &ocl) {\n"
		"\tif (write_buffer == NULL)return;\n"
		"\tcl_int err = CL_SUCCESS;\n"
		"\tT* resultPtr2 = (T *)clEnqueueMapBuffer(ocl.commandQueue, write_buffer, true, CL_MAP_READ, 0, sizeof(T) * write_offset, 0, NULL, NULL, &err);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error:(opencl_write_done) clEnqueueMapBuffer returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\terr = clFinish(ocl.commandQueue);\n"
		"\tif (CL_SUCCESS != err)\n"
		"\t{\n"
			"\t\tprintf(\"Error: clFinish returned %s during reading results\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
			"\t\tif (it->get().get_write_index() + write_offset > N) {\n"
				"\t\t\tint write_before_cycle = N - it->get().get_write_index();\n"
				"\t\t\tint write_after_cycle = write_offset - write_before_cycle;\n"
				"\t\t\tmemcpy(it->get().get_data_ptr() + it->get().get_write_index(), resultPtr2, sizeof(T) * write_before_cycle);\n"
				"\t\t\tmemcpy(it->get().get_data_ptr(), resultPtr2 + write_before_cycle, sizeof(T) * write_after_cycle);\n"
			"\t\t}\n"
			"\t\telse {\n"
				"\t\t\tmemcpy(it->get().get_data_ptr() + it->get().get_write_index(), resultPtr2, sizeof(T)*write_offset);\n"
			"\t\t}\n"
			"\t\tit->get().notify_write_done(write_offset);\n"
		"\t}\n"
		"\tif (CL_SUCCESS != clReleaseMemObject(write_buffer))\n"
		"\t{\n"
			"\t\tprintf(\"Error: clReleaseMemObject returned %s\\n\", TranslateErrorCode(err));\n"
			"\t\texit(err);\n"
		"\t}\n"
		"\twrite_buffer = NULL;\n"
	"}\n";

		std::string port_without_opencl = "#pragma once\n"
			"#include <vector>\n"
			"#include <algorithm>\n"
			"#include <initializer_list>\n"
			"#include <functional>\n"
			"#include \"FIFO.hpp\"\n"
			"using namespace std;\n\n"


			"template<typename T, int N>\n"
			"class Port {\n"
			"private:\n"
			"\tstd::vector<std::reference_wrapper<FIFO<T, N>>> connected_FIFOs;\n"
			"\tint write_offset{ 0 };\n\n"

			"public:\n"
			"\tPort(std::initializer_list<std::reference_wrapper<FIFO<T, N>>> init_list);\n"
			"\tint get_size() const;\n"
			"\tT get_element();\n"
			"\tvoid put_element(T element);\n"
			"\tint get_free_space() const;\n"
			"\tvoid get_elements(T *elements, int elements_count);\n"
			"\tvoid put_elements(T *elements, int element_count);\n"
			"\tT element_preview(int offset = 0);\n"
			"};\n\n"

			"template<typename T, int N>\n"
			"Port<T, N>::Port(std::initializer_list<std::reference_wrapper<FIFO<T, N>>> init_list) : connected_FIFOs(init_list) {\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"int Port<T, N>::get_size() const {\n"
			"\treturn connected_FIFOs.front().get().get_size();\n"
			"};\n\n"

			"template<typename T, int N>\n"
			"T Port<T, N>::get_element() {\n"
			"\treturn connected_FIFOs.front().get().get_element();\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"void Port<T, N>::put_element(T element) {\n"
			"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
			"\t\tit->get().put_element(element);\n"
			"\t}\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"int Port<T, N>::get_free_space() const {\n"
			"\tint size{ 500000000 };//dummy number\n"
			"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
			"\t\tsize = size < it->get().get_free_space()?size: it->get().get_free_space();\n"
			"\t}\n"
			"\treturn size;\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"void Port<T, N>::get_elements(T *elements, int elements_count) {\n"
			"\tconnected_FIFOs.front().get().get_elements(elements, elements_count);\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"void Port<T, N>::put_elements(T *elements, int elements_count) {\n"
			"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
			"\t\tit->get().put_elements(elements, elements_count);\n"
			"\t}\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"T Port<T, N>::element_preview(int offset) {\n"
			"\treturn connected_FIFOs.front().get().element_preview(offset);\n"
			"}";

	std::ofstream output_file{ path+"\\FIFO.hpp" };
	if (output_file.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\FIFO.hpp" };
	}
	output_file << FIFO_code;
	
	std::ofstream output_port{ path + "\\Port.hpp" };
	if (output_port.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\Port.hpp" };
	}
	if (OpenCL) {
		output_port << port;
	}
	else {
		output_port << port_without_opencl;
	}

}