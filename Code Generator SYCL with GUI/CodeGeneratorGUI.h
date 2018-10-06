#pragma once
#include <QtWidgets/QMainWindow>
#include "ui_CodeGeneratorGUI.h"
#include "Datastructures.hpp"
#include <QFileDialog>

class CodeGeneratorGUI : public QMainWindow
{
	Q_OBJECT

public:
	CodeGeneratorGUI(QWidget *parent = Q_NULLPTR);
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
	Ui::CodeGeneratorGUIClass ui;
	void read_data(program_options *opt);
	void convert_network(program_options *opt);
};
