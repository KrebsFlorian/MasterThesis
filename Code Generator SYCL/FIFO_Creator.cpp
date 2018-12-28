#include "Converter.hpp"
#include <fstream>
#include "Exceptions.hpp"



/*
	Function writes the FIFO code to the file path\\FIFO.hpp
	Throws an exception if it cannot open the file.
*/
void Converter::create_FIFO(std::string path,bool SYCL,bool infQ) {

	std::string FIFO_code = "#pragma once\n"
		"#include <string>\n"
		"#include <algorithm>\n"
		"#include <iostream>\n"
		"#include <CL/sycl.hpp>\n\n"

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
	"}\n\n"
	
	"template<typename T, int N>\n"
	"T* FIFO<T, N>::get_data_ptr() const {\n"
		"\treturn data_ptr;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int FIFO<T, N>::get_write_index() const {\n"
		"\treturn state.write_index;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"int FIFO<T, N>::get_read_index() const {\n"
		"\treturn state.read_index;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T, N>::notify_read_done(int read_elements) {\n"
		"\tstate.read_index = (state.read_index + read_elements) % N;\n"
		"\tif(state.full && state.read_index != state.write_index)state.full = false;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"void FIFO<T, N>::notify_write_done(int written_elements) {\n"
		"\tstate.write_index = (state.write_index + written_elements) % N;\n"
		"\tif (state.write_index == state.read_index) {\n"
			"\t\tstate.full = true;\n"
		"\t}\n"
	"}";

		std::string FIFO_code_no_sycl = "#pragma once\n"
			"#include <string>\n"
			"#include <algorithm>\n"
			"#include <iostream>\n"

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
			"\tT *data_ptr = new T[N];\n"
			"\tint write_offset{ 0 };\n\n"

			"public:\n"
			"\tFIFO();\n"
			"\tint get_size() const;\n"
			"\tT get_element();\n"
			"\tvoid put_element(T element);\n"
			"\tint get_free_space() const;\n"
			"\tvoid get_elements(T *elements, int elements_count);\n"
			"\tvoid put_elements(T *elements, int element_count);\n"
			"\tT element_preview(int offset = 0);\n"
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
			"\tstate.full = false;\n"
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
			"T FIFO<T,N>::element_preview(int offset){"
			"\tint index = (state.read_index+offset)%N;\n"
			"\treturn data_ptr[index];\n"
			"}";

	std::string inifinite_FIFO_code = "#pragma once\n"
			"#include <string>\n"
			"#include <algorithm>\n"
			"#include <iostream>\n"
			"#include <deque>\n"
			"#include <CL/sycl.hpp>\n\n"

			"template<typename T, int N>\n"
			"class FIFO {\n"
			"\tstd::deque<T> queue{};\n"
			"\tcl::sycl::buffer<T, 1>* sycl_read_buffer;\n"
			"\tcl::sycl::buffer<T, 1>* sycl_write_buffer;\n"
			"\tint write_offset{ 0 };\n\n"

			"public:\n"
			"\tFIFO();\n"
			"\tint get_size() const;\n"
			"\tT get_element();\n"
			"\tvoid put_element(T element);\n"
			"\tint get_free_space() const;\n"
			"\tvoid get_elements(T *elements, int elements_count);\n"
			"\tvoid put_elements(T *elements, int element_count);\n"
			"\tvoid sycl_write_done();\n"
			"\tvoid sycl_read_done();\n"
			"\tcl::sycl::buffer<T, 1>* get_read_buffer(const int number_elements_to_read);\n"
			"\tcl::sycl::buffer<T, 1>* get_write_buffer(const int number_elements_to_write);\n"
			"\tT element_preview(int offset = 0);\n"
			"};\n\n\n"


			"template<typename T, int N>\n"
			"FIFO<T, N>::FIFO() {queue.resize(250000);}\n\n"

			"template<typename T, int N>\n"
			"int FIFO<T, N>::get_size() const {\n"
			"\treturn queue.size();\n"
			"};\n\n"


			"template<typename T, int N>\n"
			"T FIFO<T, N>::get_element() {\n"
			"\tT element = queue.front();\n"
			"\tqueue.pop_front();\n"
			"\treturn element;\n"
			"}\n\n"


			"template<typename T, int N>\n"
			"void FIFO<T, N>::put_element(T element) {\n"
			"\tqueue.push_back(element);\n"
			"}\n\n"


			"template<typename T, int N>\n"
			"int FIFO<T, N>::get_free_space() const {\n"
			"\treturn 80000;//just a random number\n"
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
			"void FIFO<T,N>::sycl_write_done() {\n"
			"\tcl::sycl::accessor<T, 1, cl::sycl::access::mode::read, cl::sycl::access::target::host_buffer> acc(*sycl_write_buffer);\n"
			"\tfor (int i = 0; i < write_offset; i++) {\n"
			"\t\tput_element(acc[i]);\n"
			"\t}\n"
			"\t(*sycl_write_buffer).~buffer();\n"
			"\tsycl_write_buffer = nullptr;\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"void FIFO<T,N>::sycl_read_done() {\n"
			"\t(*sycl_read_buffer).~buffer();\n"
			"\tsycl_read_buffer = nullptr;\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"cl::sycl::buffer<T, 1>* FIFO<T,N>::get_read_buffer(const int number_elements_to_read) {\n"
			"\tsycl_read_buffer = new cl::sycl::buffer<T, 1>{ cl::sycl::range<1>(number_elements_to_read) };\n"
			"\tcl::sycl::accessor<T, 1, cl::sycl::access::mode::write, cl::sycl::access::target::host_buffer> acc(*sycl_read_buffer);\n"
			"\tfor (int i = 0; i < number_elements_to_read; i++) {\n"
			"\t\tacc[i] = get_element();\n"
			"\t}\n"
			"\treturn sycl_read_buffer;\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"cl::sycl::buffer<T, 1>* FIFO<T,N>::get_write_buffer(const int number_elements_to_write) {\n"
			"\twrite_offset = number_elements_to_write;\n"
			"\tsycl_write_buffer = new cl::sycl::buffer<T, 1>{ cl::sycl::range<1>(number_elements_to_write) };\n"
			"\treturn sycl_write_buffer;\n"
			"}\n\n"

			"template<typename T, int N>\n"
			"T FIFO<T,N>::element_preview(int offset){"
			"\tint index = (state.read_index+offset)%N;\n"
			"\treturn data_ptr[index];\n"
			"}";
	
	std::string port = "#pragma once\n"
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
		"\tcl::sycl::buffer<T, 1>* sycl_read_buffer;\n"
		"\tcl::sycl::buffer<T, 1>* sycl_write_buffer;\n"
		"\tint write_offset{ 0 };\n\n"

	"public:\n"
		"\tPort(std::initializer_list<std::reference_wrapper<FIFO<T, N>>> init_list);\n"
		"\tint get_size() const;\n"
		"\tT get_element();\n"
		"\tvoid put_element(T element);\n"
		"\tint get_free_space() const;\n"
		"\tvoid get_elements(T *elements, int elements_count);\n"
		"\tvoid put_elements(T *elements, int element_count);\n"
		"\tvoid sycl_write_done();\n"
		"\tvoid sycl_read_done();\n"
		"\tcl::sycl::buffer<T, 1>* get_read_buffer(const int number_elements_to_read);\n"
		"\tcl::sycl::buffer<T, 1>* get_write_buffer(const int number_elements_to_write);\n"
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
	"void Port<T, N>::sycl_read_done() {\n"
		"\t(*sycl_read_buffer).~buffer();\n"
		"\tsycl_read_buffer = nullptr;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"cl::sycl::buffer<T, 1>* Port<T, N>::get_read_buffer(const int number_elements) {\n"
		"\tsycl_read_buffer = new cl::sycl::buffer<T, 1>{ cl::sycl::range<1>(number_elements_to_read) };\n"
		"\tcl::sycl::accessor<T, 1, cl::sycl::access::mode::write, cl::sycl::access::target::host_buffer> acc(*sycl_read_buffer);\n"
		"\tFIFO<T, N>& attached_read_fifo = connected_FIFOs.front().get();\n"
		"\tif (attached_read_fifo.get_read_index() + number_elements > N) {\n"
			"\t\tint read_before_cycle = N - attached_read_fifo.get_read_index();\n"
			"\t\tint read_after_cycle = number_elements - read_before_cycle;\n"
			"\t\tmemcpy(acc, attached_read_fifo.get_data_ptr() + attached_read_fifo.get_read_index(), sizeof(T) * read_before_cycle);\n"
			"\t\tmemcpy(acc + read_before_cycle, attached_read_fifo.get_data_ptr(), sizeof(T) * read_after_cycle);\n"
		"\t}\n"
		"\telse {\n"
			"\t\tmemcpy(acc, attached_read_fifo.get_data_ptr() + attached_read_fifo.get_read_index(), sizeof(T) * number_elements);\n"
		"\t}\n"
		"\tattached_read_fifo.notify_read_done(number_elements);\n"
		"\treturn sycl_read_buffer;\n"
	"}\n\n"



	"template<typename T, int N>\n"
	"void Port<T, N>::sycl_write_done() {\n"
		"\tcl::sycl::accessor<T, 1, cl::sycl::access::mode::read, cl::sycl::access::target::host_buffer> acc(*sycl_write_buffer);\n"
		"\tfor (auto it = connected_FIFOs.begin(); it != connected_FIFOs.end(); ++it) {\n"
			"\t\tif (it->get().get_write_index() + write_offset > N) {\n"
				"\t\t\tint write_before_cycle = N - it->get().get_write_index();\n"
				"\t\t\tint write_after_cycle = write_offset - write_before_cycle;\n"
				"\t\t\tmemcpy(it->get().get_data_ptr() + it->get().get_write_index(), acc, sizeof(T) * write_before_cycle);\n"
				"\t\t\tmemcpy(it->get().get_data_ptr(), acc + write_before_cycle, sizeof(T) * write_after_cycle);\n"
			"\t\t}\n"
			"\t\telse {\n"
				"\t\t\tmemcpy(it->get().get_data_ptr() + it->get().get_write_index(), acc, sizeof(T)*write_offset);\n"
			"\t\t}\n"
			"\t\tit->get().notify_write_done(write_offset);\n"
		"\t}\n"
		"\t(*sycl_write_buffer).~buffer();\n"
		"\tsycl_write_buffer = nullptr;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"cl::sycl::buffer<T, 1>* Port<T, N>::get_write_buffer(const int number_elements_to_write) {\n"
		"\twrite_offset = number_elements_to_write;\n"
		"\tsycl_write_buffer = new cl::sycl::buffer<T, 1>{ cl::sycl::range<1>(number_elements_to_write) };\n"
		"\treturn sycl_write_buffer;\n"
	"}\n\n"

	"template<typename T, int N>\n"
	"T Port<T, N>::element_preview(int offset) {\n"
		"\treturn connected_FIFOs.front().get().element_preview(offset);\n"
	"}";

		std::string port_no_sycl = "#pragma once\n"
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
	if (!infQ) {
		if (SYCL) {
			output_file << FIFO_code;
		}
		else {
			output_file << FIFO_code_no_sycl;
		}
	}
	else {
		output_file << inifinite_FIFO_code;
	}
	std::ofstream output_port{ path + "\\Port.hpp" };
	if (output_port.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\Port.hpp" };
	}
	if (SYCL) {
		output_port << port;
	}
	else {
		output_port << port_no_sycl;
	}

}