#pragma once
#include <QFileDialog>
#include <QtWidgets/QMainWindow>
#include "ui_CodeGeneratorOpenCLwithGUI.h"
#include "Datastructures.hpp"

class CodeGeneratorOpenCLwithGUI : public QMainWindow
{
	Q_OBJECT

public:
	CodeGeneratorOpenCLwithGUI(QWidget *parent = Q_NULLPTR);

	void write_out(QString s) {
		ui.textBrowser->insertPlainText(s);
	}

private slots:
	void close();
	void select_network_file();
	void select_source_dir();
	void select_output_dir();
	void select_native_file1();
	void select_native_file2();
	void select_native_file3();
	void select_native_file4();
	void convert();

private:
	Ui::CodeGeneratorOpenCLwithGUIClass ui;
	void read_data(program_options *opt);
	void convert_network(program_options *opt);
};
