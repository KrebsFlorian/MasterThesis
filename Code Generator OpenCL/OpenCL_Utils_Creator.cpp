#include "Converter.hpp"
#include <fstream>
#include "Exceptions.hpp"

void create_OpenCL_arguments_struct(const std::string &path) {

	std::string header = "#pragma once\n"
		"#include <vector>\n"
		"#include <CL\\cl.hpp>\n\n"

		
		"#define OPENCL_VERSION_1_2  1.2f\n"
		"#define OPENCL_VERSION_2_0  2.0f\n\n"

		"struct opencl_arguments {\n"
		"\topencl_arguments();\n"
		"\t~opencl_arguments();\n\n"


		"\tcl_context       context;\n"
		"\tcl_device_id     device;\n"
		"\tcl_command_queue commandQueue;\n"
		"\tcl_program       program;\n"
		"\tstd::vector<cl_kernel>        kernels;\n"
		"\tfloat            platformVersion;\n"
		"\tfloat            deviceVersion;\n"
		"\tfloat            compilerVersion;\n\n"

		"\tsize_t		   globalWorkSize[1];\n"
		"\tsize_t          localWorkSize[1];\n"
		"\tcl_int          work_Dim;\n"
		"};\n";
	std::string source = "#include \"opencl_arguments.hpp\"\n"
		"#include \"Translation.hpp\"\n"
		"#include <iostream>\n\n"
					
		"opencl_arguments::opencl_arguments() :\n"
		"context(NULL),\n"
		"device(NULL),\n"
		"commandQueue(NULL),\n"
		"program(NULL),\n"
		"platformVersion(OPENCL_VERSION_1_2),\n"
		"deviceVersion(OPENCL_VERSION_1_2),\n"
		"compilerVersion(OPENCL_VERSION_1_2)\n"
	"{ }\n\n"

		"opencl_arguments::~opencl_arguments() {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"
		"\tfor(int i = 0; i < kernels.size(); ++i) {\n"
		"\t\tif (kernels[i]) {\n"
			"\t\t\terrorCode = clReleaseKernel(kernels[i]);\n"
			"\t\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\t\tstd::cout << \"Error: clReleaseKernel returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\t\t}\n"
		"\t\t}\n"
		"\t}\n"
		"\tif (program) {\n"
			"\t\terrorCode = clReleaseProgram(program);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clReleaseProgram returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\t}\n"
		"\t}\n"
		"\tif (commandQueue) {\n"
			"\t\terrorCode = clReleaseCommandQueue(commandQueue);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clReleaseCommandQueue returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\t}\n"
		"\t}\n"
		"\tif (device) {\n"
			"\t\terrorCode = clReleaseDevice(device);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clReleaseDevice returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\t}\n"
		"\t}\n"
		"\tif (context) {\n"
			"\t\terrorCode = clReleaseContext(context);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clReleaseContext returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\t}\n"
		"\t}\n"
	"}";

	std::ofstream output_header{ path + "\\opencl_arguments.hpp" };
	if (output_header.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\opencl_arguments.hpp" };
	}
	output_header << header;

	std::ofstream output_source{ path + "\\opencl_arguments.cpp" };
	if (output_source.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\opencl_arguments.cpp" };
	}
	output_source << source;
}


void create_Translation_Files(const std::string &path) {
	std::string header = "#pragma once\n"
				"#include <string>\n"
				"#include <CL\\cl.hpp>\n\n"
				"std::string TranslateErrorCode(cl_int errorCode);\n\n"
				"std::string TranslateDeviceType(cl_device_type deviceType);\n\n"
				"std::string TranslateMemFlag(cl_mem_flags flag);";
	std::string source = "#include \"Translation.hpp\"\n\n"			
		"std::string TranslateMemFlag(cl_mem_flags flag) {\n"
		"\tswitch (flag) {\n"
		"\tcase CL_MEM_ALLOC_HOST_PTR: return \"CL_MEM_ALLOC_HOST_PTR\";\n"
		"\tcase CL_MEM_COPY_HOST_PTR:  return \"CL_MEM_COPY_HOST_PTR\";\n"
		"\tcase CL_MEM_USE_HOST_PTR:   return \"CL_MEM_USE_HOST_PTR\";\n"
		"\tdefault:                    return \"UNKNOWN MEMORY FLAG\";\n"
		"\t}\n"
	"}\n\n"

	"std::string TranslateDeviceType(cl_device_type deviceType)	{\n"
		"switch (deviceType) {\n"
		"case CL_DEVICE_TYPE_DEFAULT:      return \"CL_DEVICE_TYPE_DEFAULT\";\n"
		"case CL_DEVICE_TYPE_CPU:          return \"CL_DEVICE_TYPE_CPU\";\n"
		"case CL_DEVICE_TYPE_GPU:          return \"CL_DEVICE_TYPE_GPU\";\n"
		"case CL_DEVICE_TYPE_ACCELERATOR:  return \"CL_DEVICE_TYPE_ACCELERATOR\";\n"
		"case CL_DEVICE_TYPE_ALL:          return \"CL_DEVICE_TYPE_ALL\";\n"
		"default:						   return \"UNKNOWN DEVICE TYPE\";\n"
		"}\n"
	"}\n\n"


	"std::string TranslateErrorCode(cl_int errorCode) {\n"
		"\tswitch (errorCode) {\n"
		"\tcase CL_SUCCESS:										return \"CL_SUCCESS\";\n"
		"\tcase CL_DEVICE_NOT_FOUND:							return \"CL_DEVICE_NOT_FOUND\";\n"
		"\tcase CL_DEVICE_NOT_AVAILABLE:						return \"CL_DEVICE_NOT_AVAILABLE\";\n"
		"\tcase CL_COMPILER_NOT_AVAILABLE:						return \"CL_COMPILER_NOT_AVAILABLE\";\n"
		"\tcase CL_MEM_OBJECT_ALLOCATION_FAILURE:				return \"CL_MEM_OBJECT_ALLOCATION_FAILURE\";\n"
		"\tcase CL_OUT_OF_RESOURCES:							return \"CL_OUT_OF_RESOURCES\";\n"
		"\tcase CL_OUT_OF_HOST_MEMORY:							return \"CL_OUT_OF_HOST_MEMORY\";\n"
		"\tcase CL_PROFILING_INFO_NOT_AVAILABLE:				return \"CL_PROFILING_INFO_NOT_AVAILABLE\";\n"
		"\tcase CL_MEM_COPY_OVERLAP:							return \"CL_MEM_COPY_OVERLAP\";\n"
		"\tcase CL_IMAGE_FORMAT_MISMATCH:						return \"CL_IMAGE_FORMAT_MISMATCH\";\n"
		"\tcase CL_IMAGE_FORMAT_NOT_SUPPORTED:					return \"CL_IMAGE_FORMAT_NOT_SUPPORTED\";\n"
		"\tcase CL_BUILD_PROGRAM_FAILURE:						return \"CL_BUILD_PROGRAM_FAILURE\";\n"
		"\tcase CL_MAP_FAILURE:									return \"CL_MAP_FAILURE\";\n"
		"\tcase CL_MISALIGNED_SUB_BUFFER_OFFSET:                return \"CL_MISALIGNED_SUB_BUFFER_OFFSET\";\n"                          
		"\tcase CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:   return \"CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST\";\n"   
		"\tcase CL_COMPILE_PROGRAM_FAILURE:						return \"CL_COMPILE_PROGRAM_FAILURE\";\n"                              
		"\tcase CL_LINKER_NOT_AVAILABLE:						return \"CL_LINKER_NOT_AVAILABLE\";\n"                                  
		"\tcase CL_LINK_PROGRAM_FAILURE:						return \"CL_LINK_PROGRAM_FAILURE\";\n"                                  
		"\tcase CL_DEVICE_PARTITION_FAILED:						return \"CL_DEVICE_PARTITION_FAILED\";\n"                              
		"\tcase CL_KERNEL_ARG_INFO_NOT_AVAILABLE:				return \"CL_KERNEL_ARG_INFO_NOT_AVAILABLE\";\n"                         
		"\tcase CL_INVALID_VALUE:								return \"CL_INVALID_VALUE\";\n"
		"\tcase CL_INVALID_DEVICE_TYPE:							return \"CL_INVALID_DEVICE_TYPE\";\n"
		"\tcase CL_INVALID_PLATFORM:							return \"CL_INVALID_PLATFORM\";\n"
		"\tcase CL_INVALID_DEVICE:								return \"CL_INVALID_DEVICE\";\n"
		"\tcase CL_INVALID_CONTEXT:								return \"CL_INVALID_CONTEXT\";\n"
		"\tcase CL_INVALID_QUEUE_PROPERTIES:					return \"CL_INVALID_QUEUE_PROPERTIES\";\n"
		"\tcase CL_INVALID_COMMAND_QUEUE:						return \"CL_INVALID_COMMAND_QUEUE\";\n"
		"\tcase CL_INVALID_HOST_PTR:							return \"CL_INVALID_HOST_PTR\";\n"
		"\tcase CL_INVALID_MEM_OBJECT:							return \"CL_INVALID_MEM_OBJECT\";\n"
		"\tcase CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:				return \"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\";\n"
		"\tcase CL_INVALID_IMAGE_SIZE:							return \"CL_INVALID_IMAGE_SIZE\";\n"
		"\tcase CL_INVALID_SAMPLER:								return \"CL_INVALID_SAMPLER\";\n"
		"\tcase CL_INVALID_BINARY:								return \"CL_INVALID_BINARY\";\n"
		"\tcase CL_INVALID_BUILD_OPTIONS:						return \"CL_INVALID_BUILD_OPTIONS\";\n"
		"\tcase CL_INVALID_PROGRAM:								return \"CL_INVALID_PROGRAM\";\n"
		"\tcase CL_INVALID_PROGRAM_EXECUTABLE:					return \"CL_INVALID_PROGRAM_EXECUTABLE\";\n"
		"\tcase CL_INVALID_KERNEL_NAME:							return \"CL_INVALID_KERNEL_NAME\";\n"
		"\tcase CL_INVALID_KERNEL_DEFINITION:					return \"CL_INVALID_KERNEL_DEFINITION\";\n"
		"\tcase CL_INVALID_KERNEL:								return \"CL_INVALID_KERNEL\";\n"
		"\tcase CL_INVALID_ARG_INDEX:							return \"CL_INVALID_ARG_INDEX\";\n"
		"\tcase CL_INVALID_ARG_VALUE:							return \"CL_INVALID_ARG_VALUE\";\n"
		"\tcase CL_INVALID_ARG_SIZE:							return \"CL_INVALID_ARG_SIZE\";\n"
		"\tcase CL_INVALID_KERNEL_ARGS:							return \"CL_INVALID_KERNEL_ARGS\";\n"
		"\tcase CL_INVALID_WORK_DIMENSION:						return \"CL_INVALID_WORK_DIMENSION\";\n"
		"\tcase CL_INVALID_WORK_GROUP_SIZE:						return \"CL_INVALID_WORK_GROUP_SIZE\";\n"
		"\tcase CL_INVALID_WORK_ITEM_SIZE:						return \"CL_INVALID_WORK_ITEM_SIZE\";\n"
		"\tcase CL_INVALID_GLOBAL_OFFSET:						return \"CL_INVALID_GLOBAL_OFFSET\";\n"
		"\tcase CL_INVALID_EVENT_WAIT_LIST:						return \"CL_INVALID_EVENT_WAIT_LIST\";\n"
		"\tcase CL_INVALID_EVENT:								return \"CL_INVALID_EVENT\";\n"
		"\tcase CL_INVALID_OPERATION:							return \"CL_INVALID_OPERATION\";\n"
		"\tcase CL_INVALID_GL_OBJECT:							return \"CL_INVALID_GL_OBJECT\";\n"
		"\tcase CL_INVALID_BUFFER_SIZE:							return \"CL_INVALID_BUFFER_SIZE\";\n"
		"\tcase CL_INVALID_MIP_LEVEL:							return \"CL_INVALID_MIP_LEVEL\";\n"
		"\tcase CL_INVALID_GLOBAL_WORK_SIZE:					return \"CL_INVALID_GLOBAL_WORK_SIZE\";\n"                         
		"\tcase CL_INVALID_PROPERTY:							return \"CL_INVALID_PROPERTY\";\n"                                 
		"\tcase CL_INVALID_IMAGE_DESCRIPTOR:					return \"CL_INVALID_IMAGE_DESCRIPTOR\";\n"                          
		"\tcase CL_INVALID_COMPILER_OPTIONS:					return \"CL_INVALID_COMPILER_OPTIONS\";\n"                          
		"\tcase CL_INVALID_LINKER_OPTIONS:						return \"CL_INVALID_LINKER_OPTIONS\";\n"                             
		"\tcase CL_INVALID_DEVICE_PARTITION_COUNT:				return \"CL_INVALID_DEVICE_PARTITION_COUNT\";\n"                     
		"\tcase CL_INVALID_PIPE_SIZE:							return \"CL_INVALID_PIPE_SIZE\";\n"                               
		"\tcase CL_INVALID_DEVICE_QUEUE:						return \"CL_INVALID_DEVICE_QUEUE\";\n"                              
		"\tdefault:												return \"UNKNOWN ERROR CODE\";\n"
		"\t}\n"
	"}";

	std::ofstream output_header{ path + "\\Translation.hpp" };
	if (output_header.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\Translation.hpp" };
	}
	output_header << header;

	std::ofstream output_source{ path + "\\Translation.cpp" };
	if (output_source.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\Translation.cpp" };
	}
	output_source << source;
}


void create_Utils_Files(const std::string &path) {
	std::string header = "#pragma once\n"
			"#include \"Translation.hpp\"\n"
			"#include \"opencl_arguments.hpp\"\n\n"
			
			"//Parts of the Source Code from intel template\n"
			"cl_int SetupOpenCL(opencl_arguments *ocl, cl_device_type deviceType, std::string platformName);\n"
			"//Parts of the Source Code from intel template\n"
			"cl_int CreateAndBuildProgram(opencl_arguments *ocl, std::string fileName);\n\n"

			"cl_int ExecuteKernel(opencl_arguments *ocl, int index, cl_event *event = NULL);\n\n"

			"bool printClDevices();\n\n"

			"bool printDeviceInformation(cl_device_id id);\n\n"

			"cl_int SetupOpenCLByDeviceID(opencl_arguments *ocl, cl_device_id id);\n\n"

			"cl_device_id findDeviceID(int id);";

	std::string source = "#include \"utils.hpp\"\n"
		"#include <stdio.h>\n"
		"#include <iostream>\n\n"

		"cl_int ReadSourceFromFile(const char* fileName, char** source, size_t* sourceSize) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"

		"\tFILE* fp = NULL;\n"
		"\tfopen_s(&fp, fileName, \"rb\");\n"
		"\tif (fp == NULL) {\n"
			"\t\tstd::cout << \"Error: Couldn't find program source file \" << fileName << std::endl;\n"
			"\t\terrorCode = CL_INVALID_VALUE;\n"
		"\t}\n"
		"\telse {\n"
			"\t\tfseek(fp, 0, SEEK_END);\n"
			"\t\t*sourceSize = ftell(fp);\n"
			"\t\tfseek(fp, 0, SEEK_SET);\n"

			"\t\t*source = new char[*sourceSize];\n"
			"\t\tif (*source == NULL) {\n"
				"\t\t\tstd::cout << \"Error: Couldn't allocate \" << *sourceSize << \" bytes for program source from file \" << fileName << std::endl;\n"
				"\t\t\terrorCode = CL_OUT_OF_HOST_MEMORY;\n"
			"\t\t}\n"
			"\t\telse {\n"
				"\t\t\tfread(*source, 1, *sourceSize, fp);\n"
			"\t\t}\n"
		"\t}\n"
		"\treturn errorCode;\n"
	"}\n\n"


	"cl_platform_id FindOpenCLPlatform(cl_device_type deviceType,std::string platformName) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"
		"\tcl_uint numPlatforms = 0;\n"
		"\terrorCode = clGetPlatformIDs(0, NULL, &numPlatforms);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get num platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n"

		"\tif (numPlatforms == 0) {\n"
			"\t\tstd::cout << \"Error: No platforms found!\" << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n"

		"\tstd::vector<cl_platform_id> platforms(numPlatforms);\n"

		"\terrorCode = clGetPlatformIDs(numPlatforms, &platforms[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n\n"

		"\tfor (cl_uint i = 0; i < numPlatforms; ++i) {\n"
			"\t\tbool platformFound = false;\n"
			"\t\tcl_uint numDevices = 0;\n"
			"\t\tsize_t stringLength = 0;\n"
			"\t\terrorCode = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &stringLength);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetPlatformInfo() to get CL_PLATFORM_NAME length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\tplatformFound = false;\n"
				"\t\t\terrorCode = CL_SUCCESS;\n"
			"\t\t}\n"
			"\t\telse {\n"
			"\t\t\tstd::vector<char> foundPlatformName(stringLength);\n"
			"\t\t\terrorCode = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, stringLength, &foundPlatformName[0], NULL);\n"
			"\t\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\t\tstd::cout << \"Error: clGetplatform_ids() to get CL_PLATFORM_NAME returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\t\tplatformFound = false;\n"
				"\t\t\t\terrorCode = CL_SUCCESS;\n"
			"\t\t\t}\n"
			"\t\t\telse {\n"
			"\t\t\t\tif (strstr(&foundPlatformName[0], platformName.c_str()) != 0) {\n"
				"\t\t\t\t\tplatformFound = true;\n"
				"\t\t\t\t\tstd::cout << \"Required Platform found\" << std::endl;\n"
			"\t\t\t\t}\n"
			"\t\t\t}\n"
			"\t\t}\n\n"
			"\t\tif (platformFound) {\n"
				"\t\t\terrorCode = clGetDeviceIDs(platforms[i], deviceType, 0, NULL, &numDevices);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"clGetDeviceIDs() returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\t}\n\n"

				"\t\t\tif (numDevices != 0) {\n"
					"\t\t\t\treturn platforms[i];\n"
				"\t\t\t}\n"
			"\t\t}\n"
		"\t}\n"

		"\treturn NULL;\n"
	"}\n\n"

	"cl_int GetPlatformAndDeviceVersion(cl_platform_id platformID, opencl_arguments *ocl) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"

		"\tsize_t stringLength = 0;\n"
		"\terrorCode = clGetPlatformInfo(platformID, CL_PLATFORM_VERSION, 0, NULL, &stringLength);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetPlatformInfo() to get CL_PLATFORM_VERSION length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tstd::vector<char> platformVersion(stringLength);\n"

		"\terrorCode = clGetPlatformInfo(platformID, CL_PLATFORM_VERSION, stringLength, &platformVersion[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get CL_PLATFORM_VERSION returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tif (strstr(&platformVersion[0], \"OpenCL 2.0\") != NULL) {\n"
			"\t\tocl->platformVersion = OPENCL_VERSION_2_0;\n"
		"\t}\n\n"

		"\terrorCode = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, 0, NULL, &stringLength);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get CL_DEVICE_VERSION length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tstd::vector<char> deviceVersion(stringLength);\n"

		"\terrorCode = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, stringLength, &deviceVersion[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get CL_DEVICE_VERSION returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tif (strstr(&deviceVersion[0], \"OpenCL 2.0\") != NULL) {\n"
			"\t\tocl->deviceVersion = OPENCL_VERSION_2_0;\n"
		"\t}\n\n"

		"\terrorCode = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &stringLength);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tstd::vector<char> compilerVersion(stringLength);\n"

		"\terrorCode = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, stringLength, &compilerVersion[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n"
		"\telse if (strstr(&compilerVersion[0], \"OpenCL C 2.0\") != NULL) {\n"
			"\t\tocl->compilerVersion = OPENCL_VERSION_2_0;\n"
		"\t}\n"
		"\treturn errorCode;\n"
	"}\n\n"


	"cl_int SetupOpenCL(opencl_arguments *ocl, cl_device_type deviceType, std::string platformName) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"

		"\tcl_platform_id platformID = FindOpenCLPlatform( deviceType,platformName);\n"
		"\tif (platformID == NULL) {\n"
			"\t\tstd::cout << \"Error: Failed to find OpenCL platform.\" << std::endl;\n"
			"\t\treturn CL_INVALID_VALUE;\n"
		"\t}\n\n"

		"\tcl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platformID, 0 };\n"
		"\tocl->context = clCreateContextFromType(contextProperties, deviceType, NULL, NULL, &errorCode);\n"
		"\tif ((errorCode != CL_SUCCESS) || (NULL == ocl->context)) {\n"
			"\t\tstd::cout << \"Couldn't create a context, clCreateContextFromType() returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\terrorCode = clGetContextInfo(ocl->context, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &ocl->device, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetContextInfo() to get list of devices returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tGetPlatformAndDeviceVersion(platformID, ocl);\n\n"

"#ifdef CL_VERSION_2_0\n"
		"\tif (OPENCL_VERSION_2_0 == ocl->deviceVersion) {\n"
			"\t\tconst cl_command_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };\n"
			"\t\tocl->commandQueue = clCreateCommandQueueWithProperties(ocl->context, ocl->device, properties, &errorCode);\n"
		"\t}\n"
		"\telse {\n"
			"\t\tcl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;\n"
			"\t\tocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &errorCode);\n"
		"\t}\n"
"#else\n"
		"\tcl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;\n"
		"\tocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &errorCode);\n"
"#endif\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clCreateCommandQueue() returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
		"\t}\n"
		"\treturn errorCode;\n"
	"}\n\n"

	"cl_int CreateAndBuildProgram(opencl_arguments *ocl, std::string fileName) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"

		"\tchar* source = NULL;\n"
		"\tsize_t source_size = 0;\n"
		"\terrorCode = ReadSourceFromFile(fileName.c_str(), &source, &source_size);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: ReadSourceFromFile returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\tdelete[] source;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tocl->program = clCreateProgramWithSource(ocl->context, 1, (const char**)&source, &source_size, &errorCode);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clCreateProgramWithSource returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\tdelete[] source;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\terrorCode = clBuildProgram(ocl->program, 1, &ocl->device, \"\", NULL, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clBuildProgram() for source program returned \" << TranslateErrorCode(errorCode) << std::endl;\n\n"
		"\t}\n\n"
		"\treturn errorCode;\n"
	"}\n\n"

	"cl_int ExecuteKernel(opencl_arguments *ocl, int index,cl_event *event) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"

		"\tif (ocl->localWorkSize[0] == NULL) {\n"
			"\t\terrorCode = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernels[index], ocl->work_Dim, NULL, ocl->globalWorkSize, NULL, 0, NULL, event);\n"
		"\t}\n"
		"\telse {\n"
			"\t\terrorCode = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernels[index], ocl->work_Dim, NULL, ocl->globalWorkSize, ocl->localWorkSize, 0, NULL, event);\n"
		"\t}\n\n"

		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: Failed to run the kernel, clEnqueueNDRangeKernel call returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\terrorCode = clFinish(ocl->commandQueue);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clFinish returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
		"\t}\n"
		"\treturn errorCode;\n"
	"}\n\n"

	"bool printClDevices() {\n"
		"\tcl_uint numPlatforms = 0;\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"
		"\tint devSerialNum = 0;\n\n"

		"\terrorCode = clGetPlatformIDs(0, NULL, &numPlatforms);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get num platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n\n"

		"\tif (numPlatforms == 0) {\n"
			"\t\tstd::cout << \"Error: No platforms found!\" << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n\n"

		"\tstd::vector<cl_platform_id> platforms(numPlatforms);\n"

		"\terrorCode = clGetPlatformIDs(numPlatforms, &platforms[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n\n"

		"\tfor (cl_uint i = 0; i < numPlatforms; ++i) {\n"
			"\t\tcl_uint numDevices = 0;\n"
			"\t\tsize_t stringLength = 0;\n\n"

			"\t\terrorCode = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &stringLength);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetPlatformInfo() to get CL_PLATFORM_NAME length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn false;\n"
			"\t\t}\n\n"

			"\t\tstd::vector<char> platformName(stringLength);\n"

			"\t\terrorCode = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, stringLength, &platformName[0], NULL);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetplatform_ids() to get CL_PLATFORM_NAME returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn false;\n"
			"\t\t}\n\n"

			"\t\terrorCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetplatform_ids() to get num platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn false;\n"
			"\t\t}\n\n"

			"\t\tif (numDevices == 0) {\n"
				"\t\t\tstd::cout << \"Error: No devices found!\" << std::endl;\n"
				"\t\t\treturn false;\n"
			"\t\t}\n"
			"\t\tstd::vector<cl_device_id> devices(numDevices);\n\n"

			"\t\terrorCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, &devices[0], NULL);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetDeviceIDs() to get platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn false;\n"
			"\t\t}\n\n"
			"\t\tfor (int j = 0; j < numDevices; j++) {\n"
				"\t\t\tstd::cout << \"   Device Serial Number : \" << ++devSerialNum;\n"

				"\t\t\tstd::cout << \"   DeviceID: \" << devices[j] << \", \";\n"

				"\t\t\tsize_t deviceStringLength = 0;\n\n"

				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &deviceStringLength);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device name length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::vector<char> deviceName(deviceStringLength);\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, deviceStringLength, &deviceName[0], NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device name returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n\n"
				"\t\t\tstd::cout << \"Device Name: \" <<  &deviceName[0] << \", \";\n"

				"\t\t\tcl_device_type type;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device type returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tprintf(\"Device Type:%s, \", TranslateDeviceType(type));\n\n"

				"\t\t\tcl_uint max_compute_units;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &max_compute_units, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max compute units returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"Device Max Compute Units: \" << max_compute_units << \", \";\n\n"

				"\t\t\tsize_t max_work_group_size;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_work_group_size, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max work group size returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"Device Max Work Group Size:\" <<  max_work_group_size << \", \\n\";\n\n"

				"\t\t\tcl_ulong max_global_mem;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &max_global_mem, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS)  {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max global mem returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"	  Device Max Global Memory: \" <<  max_global_mem << \" Byte, \";\n\n"

				"\t\t\tcl_ulong max_global_mem_cache;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &max_global_mem_cache, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max global mem cache size returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"Device Max Global Memory Cache Size:\" << max_global_mem_cache << \" Byte, \";\n\n"

				"\t\t\tcl_uint max_global_mem_cacheline_size;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &max_global_mem_cacheline_size, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max global mem cache size returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"Device Max Global Memory Cacheline Size:\" << max_global_mem_cacheline_size << \" Byte, \";\n\n"

				"\t\t\tcl_ulong max_local_mem;\n"
				"\t\t\terrorCode = clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &max_local_mem, NULL);\n"
				"\t\t\tif (errorCode != CL_SUCCESS) {\n"
					"\t\t\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max local mem size returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
					"\t\t\t\treturn false;\n"
				"\t\t\t}\n"
				"\t\t\tstd::cout << \"Device Max Local Memory Size:\" << max_local_mem << \" Byte, \";\n"

				"\t\t\tstd::cout << std::endl << std::endl;\n"
			"\t\t}\n"
		"\t}\n"
		"\treturn true;\n"
	"}\n\n"

	"bool printDeviceInformation(cl_device_id id) {\n"

		"\tprintf(\"DeviceID:%d, \", id);\n"

		"\tcl_int errorCode = CL_SUCCESS;\n"
		"\tsize_t deviceStringLength = 0;\n\n"

		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_NAME, 0, NULL, &deviceStringLength);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get device name length returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n"
		"\tstd::vector<char> deviceName(deviceStringLength);\n"
		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_NAME, deviceStringLength, &deviceName[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get device name returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n"
		"\tstd::cout << \"Device Name:\" << &deviceName[0] << \", \";\n\n"

		"\tcl_device_type type;\n"
		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get device type returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n"
		"\tstd::cout << \"Device Type:\" << TranslateDeviceType(type) << \", \";\n\n"

		"\tcl_uint max_compute_units;\n"
		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &max_compute_units, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max compute units returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n"
		"\tstd::cout << \"Device Max Compute Units:\" << max_compute_units << \", \";\n\n"

		"\tsize_t max_work_group_size;\n"
		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_work_group_size, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get device max work group size returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn false;\n"
		"\t}\n"
		"\tstd::cout << \"Device Max Work Group Size:\" << max_work_group_size << \",\\n\";\n"

		"\tstd::cout << std::endl << std::endl;\n"

		"\treturn true;\n"
	"}\n\n"


	"cl_int SetupOpenCLByDeviceID(opencl_arguments *ocl, cl_device_id id) {\n"
		"\tcl_int errorCode = CL_SUCCESS;\n"
		"\tcl_device_id *deviceid = (cl_device_id*)malloc(sizeof(cl_device_id));\n"
		"\t*deviceid = id;\n"

		"\tcl_platform_id platformid;\n"
		"\terrorCode = clGetDeviceInfo(id, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platformid, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetDeviceInfo() to get platform id returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tcl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platformid, 0 };\n"
		"\tocl->context = clCreateContext(contextProperties, 1, deviceid, NULL, NULL, &errorCode);\n"
		"\tif ((errorCode != CL_SUCCESS) || (NULL == ocl->context)) {\n"
			"\t\tstd::cout << \"Couldn't create a context, clCreateContext() returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\terrorCode = clGetContextInfo(ocl->context, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &ocl->device, NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetContextInfo() to get list of devices returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn errorCode;\n"
		"\t}\n\n"

		"\tGetPlatformAndDeviceVersion(platformid, ocl);\n\n"

"#ifdef CL_VERSION_2_0\n"
		"\tif (OPENCL_VERSION_2_0 == ocl->deviceVersion) {\n"
			"\t\tconst cl_command_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };\n"
			"\t\tocl->commandQueue = clCreateCommandQueueWithProperties(ocl->context, ocl->device, properties, &errorCode);\n"
		"\t}\n"
		"\telse {\n"
			"\t\tcl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;\n"
			"\t\tocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &errorCode);\n"
		"\t}\n"
"#else\n"
		"\tcl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;\n"
		"\tocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &errorCode);\n"
"#endif\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clCreateCommandQueue() returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
		"\t}\n"

		"\treturn errorCode;\n"
	"}\n\n"

	"cl_device_id findDeviceID(int id) {\n"
		"\tcl_uint numPlatforms = 0;\n"
		"\tcl_int errorCode = CL_SUCCESS;\n\n"

		"\terrorCode = clGetPlatformIDs(0, NULL, &numPlatforms);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get the number of platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n\n"

		"\tif (numPlatforms == 0) {\n"
			"\t\tstd::cout << \"Error: No platforms found!\" << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n\n"

		"\tstd::vector<cl_platform_id> platforms(numPlatforms);\n"

		"\terrorCode = clGetPlatformIDs(numPlatforms, &platforms[0], NULL);\n"
		"\tif (errorCode != CL_SUCCESS) {\n"
			"\t\tstd::cout << \"Error: clGetplatform_ids() to get platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
			"\t\treturn NULL;\n"
		"\t}\n\n"

		"\tfor (int i = 0; i < numPlatforms; i++) {\n"
			"\t\tcl_uint numDevices = 0;\n\n"

			"\t\terrorCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);\n"
			"\t\tif (errorCode != CL_SUCCESS) {\n"
				"\t\t\tstd::cout << \"Error: clGetDeviceIDs() to get the number of devices returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn NULL;\n"
			"\t\t}\n\n"

			"\t\tif (numDevices == 0) {\n"
				"\t\t\tstd::cout << \"Error: No devices found!\" << std::endl;\n"
				"\t\t\treturn NULL;\n"
			"\t\t}\n"
			"\t\tstd::vector<cl_device_id> devices(numDevices);\n\n"

			"\t\terrorCode = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, &devices[0], NULL);\n"
			"\t\tif (errorCode != CL_SUCCESS ) {\n"
				"\t\t\tstd::cout << \"Error: clGetDeviceIDs() to get platforms returned \" << TranslateErrorCode(errorCode) << std::endl;\n"
				"\t\t\treturn NULL;\n"
			"\t\t}\n"
			"\t\tfor (int j = 0; j < numDevices; ++j) {\n"
				"\t\t\tif (!--id) return devices[j];\n"
			"\t\t}\n"
		"\t}\n"

		"\treturn NULL;\n"
	"}";


	std::ofstream output_header{ path + "\\utils.hpp" };
	if (output_header.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\utils.hpp" };
	}
	output_header << header;

	std::ofstream output_source{ path + "\\utils.cpp" };
	if (output_source.bad()) {
		throw Converter_RVC_Cpp::Converter_Exception{ "Cannot open the file " + path + "\\utils.cpp" };
	}
	output_source << source;

}


void Converter::create_OpenCL_Utils(const std::string path) {

	create_OpenCL_arguments_struct(path);

	create_Translation_Files(path);

	create_Utils_Files(path);

}