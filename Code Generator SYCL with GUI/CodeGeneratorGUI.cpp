#include "CodeGeneratorGUI.h"
#include <memory>
#include <iostream>
#include <fstream>
#include "Initializer.hpp"
#include "Converter.hpp"
#include <algorithm> 
#include "Datastructures.hpp"
#include "Converter.hpp"
#include "Initializer.hpp"


CodeGeneratorGUI::CodeGeneratorGUI(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	connect(ui.networkFileSelect, SIGNAL(clicked()), this, SLOT(select_network_file()));
	connect(ui.sourceDirSelect, SIGNAL(clicked()), this, SLOT(select_source_dir()));
	connect(ui.outputDirSelect, SIGNAL(clicked()), this, SLOT(select_output_dir()));
	connect(ui.nativeFileSelect_1, SIGNAL(clicked()), this, SLOT(select_native_file1()));
	connect(ui.nativeFileSelect_2, SIGNAL(clicked()), this, SLOT(select_native_file2()));
	connect(ui.nativeFileSelect_3, SIGNAL(clicked()), this, SLOT(select_native_file3()));
	connect(ui.nativeFileSelect_4, SIGNAL(clicked()), this, SLOT(select_native_file4()));
	connect(ui.convertButton, SIGNAL(clicked()), this, SLOT(convert()));
	connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(close()));
	ui.numberOfCores->setInputMask("999");
	ui.FIFO_SIZE->setInputMask("9999999");
}


void CodeGeneratorGUI::close() {
	exit(0);
}


void CodeGeneratorGUI::select_network_file() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Network File"), "", tr("*.xdf"));
	if (!fileName.isEmpty())ui.networkFileInput->setText(fileName);
};
void CodeGeneratorGUI::select_source_dir() {
	QString fileName = QFileDialog::getExistingDirectory(this, tr("Select Source Directory"), "", QFileDialog::ShowDirsOnly);
	if (!fileName.isEmpty())ui.SourceDirInput->setText(fileName);
};
void CodeGeneratorGUI::select_output_dir() {
	QString fileName = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), "", QFileDialog::ShowDirsOnly);
	if (!fileName.isEmpty())ui.outputDirInput->setText(fileName);
};
void CodeGeneratorGUI::select_native_file1() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Native Source File"), "", tr("*.c"));
	if (!fileName.isEmpty())ui.nativeFileInput_1->setText(fileName);
};
void CodeGeneratorGUI::select_native_file2() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Native Source File"), "", tr("*.c"));
	if (!fileName.isEmpty())ui.nativeFileInput_2->setText(fileName);
};
void CodeGeneratorGUI::select_native_file3() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Native Source File"), "", tr("*.c"));
	if (!fileName.isEmpty())ui.nativeFileInput_3->setText(fileName);
};
void CodeGeneratorGUI::select_native_file4() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Native Source File"), "", tr("*.c"));
	if (!fileName.isEmpty())ui.nativeFileInput_4->setText(fileName);
};
void CodeGeneratorGUI::convert() {
	if (ui.networkFileInput->text().isEmpty() || ui.SourceDirInput->text().isEmpty() || ui.outputDirInput->text().isEmpty()) {
		write_out("Please enter network file, source and output directory!\n");
		return;
	}
	program_options *opt = new program_options;
	read_data(opt);

	convert_network(opt);

};

void CodeGeneratorGUI::read_data(program_options *opt) {
	if (ui.Target_CPU_Cores->isChecked()) {
		opt->target_CPU = true;
		opt->cores = ui.numberOfCores->text().toInt();
	}
	if (ui.Target_CPU_no_limit->isChecked()) {
		opt->target_CPU = true;
		opt->cores = 0;
	}
	opt->no_SYCL = ui.No_SYCL->isChecked();

	if (!ui.nativeFileInput_1->text().isEmpty()) {
		QString nat{ ui.nativeFileInput_1->text() };
		nat = nat.replace("/", "\\");
		std::string tmp{ nat.toStdString() };
		char * char_tmp = new char[tmp.size() + 1];
		std::copy(tmp.begin(), tmp.end(), char_tmp);
		char_tmp[tmp.size()] = '\0';
		opt->native_includes.push_back(char_tmp);
	}
	if (!ui.nativeFileInput_2->text().isEmpty()) {
		QString nat{ ui.nativeFileInput_2->text() };
		nat = nat.replace("/", "\\");
		std::string tmp{ nat.toStdString() };
		char * char_tmp = new char[tmp.size() + 1];
		std::copy(tmp.begin(), tmp.end(), char_tmp);
		char_tmp[tmp.size()] = '\0';
		opt->native_includes.push_back(char_tmp);
	}
	if (!ui.nativeFileInput_3->text().isEmpty()) {
		QString nat{ ui.nativeFileInput_3->text() };
		nat = nat.replace("/", "\\");
		std::string tmp{ nat.toStdString() };
		char * char_tmp = new char[tmp.size() + 1];
		std::copy(tmp.begin(), tmp.end(), char_tmp);
		char_tmp[tmp.size()] = '\0';
		opt->native_includes.push_back(char_tmp);
	}
	if (!ui.nativeFileInput_4->text().isEmpty()) {
		QString nat{ ui.nativeFileInput_4->text() };
		nat = nat.replace("/", "\\");
		std::string tmp{ nat.toStdString() };
		char * char_tmp = new char[tmp.size() + 1];
		std::copy(tmp.begin(), tmp.end(), char_tmp);
		char_tmp[tmp.size()] = '\0';
		opt->native_includes.push_back(char_tmp);
	}

	QString network{ ui.networkFileInput->text() };
	QString inputDir{ ui.SourceDirInput->text() };
	QString outputDir{ ui.outputDirInput->text() };
	network = network.replace("/", "\\");
	inputDir = inputDir.replace("/", "\\");
	inputDir = inputDir.append("\\");
	outputDir = outputDir.replace("/", "\\");
	outputDir = outputDir.append("\\");

	std::string tmp{ network.toStdString() };
	char * char_tmp = new char[tmp.size() + 1];
	std::copy(tmp.begin(), tmp.end(), char_tmp);
	char_tmp[tmp.size()] = '\0';
	std::string tmp2{ inputDir.toStdString() };
	char * char_tmp2 = new char[tmp2.size() + 1];
	std::copy(tmp2.begin(), tmp2.end(), char_tmp2);
	char_tmp2[tmp2.size()] = '\0';
	std::string tmp3{ outputDir.toStdString() };
	char * char_tmp3 = new char[tmp3.size() + 1];
	std::copy(tmp3.begin(), tmp3.end(), char_tmp3);
	char_tmp3[tmp3.size()] = '\0';
	opt->network_file = char_tmp;
	opt->source_directory = char_tmp2;
	opt->target_directory = char_tmp3;
	if (!ui.FIFO_SIZE->text().isEmpty()) {
		opt->FIFO_size = ui.FIFO_SIZE->text().toInt();
	}


	/*
	//LOGGING for testing
	std::ofstream output_file{ "E:\\log.txt" };
	output_file << "FIFO Size: " << opt->FIFO_size << "\nCores: " << opt->cores << "\nNetwork File: " << opt->network_file << "\nSource Directory: " << opt->source_directory << "\nOutput Directory: " << opt->target_directory << "\n";
	if (opt->target_CPU) {
		output_file << "Use CPU\n";
	}
	else {
		output_file << "Use GPU\n";
	}

	if (opt->no_SYCL) {
		output_file << "No SYCL\n";
	}
	else {
		output_file << "SYCL\n";
	}
	for (auto it = opt->native_includes.begin(); it != opt->native_includes.end(); ++it) {
		output_file << "Native Source Code File: " << *it << "\n";
	}*/
}
void CodeGeneratorGUI::convert_network(program_options *opt) {
	write_out("Reading the network...\n");
	std::unique_ptr<Dataflow_Network> dpn(Init_Conversion::read_network(opt));
	if (opt->native_includes.empty()) {
		write_out("Creating headers for the native includes...\n");
	}
	std::string	native_header_include = Init_Conversion::create_headers_for_native_code(opt);
	write_out("Creating the FIFO and Port file...\n");
	Converter::create_FIFO(std::string{ opt->target_directory }, !opt->no_SYCL, opt->infQ);
	write_out("Converting the Actors...\n");
	Converter::convert_Actors(dpn.get(), opt, native_header_include, this);
	write_out("Creating the main...\n");
	Converter::create_main(dpn.get(), std::string{ opt->target_directory }+"\\main.cpp", opt);
	write_out("Conversion from RVC to C++/SYCL was successful\n");
}
