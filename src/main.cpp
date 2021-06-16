/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QCommandLineParser>
#include <QApplication>
#include <QObject>
#include <QSettings>
#include <QLockFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <QLocalSocket>
#include <QDebug>
#include <QTranslator>
#include <QDir>
#include <QTextStream>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QDebug>
#include <locale.h>

#include "libqalculate/qalculate.h"
#include "qalculatewindow.h"
#include "qalculateqtsettings.h"

QString custom_title;
QalculateQtSettings *settings;
extern bool title_modified;

QTranslator translator, translator_qt, translator_qtbase;

int main(int argc, char **argv) {

	QApplication app(argc, argv);
	app.setApplicationName("qalculate-qt");
	app.setApplicationDisplayName("Qalculate! (Qt)");
	app.setOrganizationName("qalculate");
	app.setApplicationVersion(VERSION);

	QCommandLineParser *parser = new QCommandLineParser();
	/*QCommandLineOption fOption(QStringList() << "f" << "file", QApplication::tr("Execute expressions and commands from a file"), QApplication::tr("FILE"));
	parser->addOption(fOption);*/
	QCommandLineOption nOption(QStringList() << "n" << "new-instance", QApplication::tr("Start a new instance of the application"));
	parser->addOption(nOption);
	QCommandLineOption tOption(QStringList() << "title", QApplication::tr("Specify the window title"), QApplication::tr("TITLE"));
	parser->addOption(tOption);
	QCommandLineOption vOption(QStringList() << "v" << "verison", QApplication::tr("Display the application version"));
	parser->addOption(vOption);
	parser->addPositionalArgument("expression", QApplication::tr("Expression to calculate"), QApplication::tr("[EXPRESSION]"));
	parser->addHelpOption();
	parser->process(app);

	if(parser->isSet(vOption)) {
		printf(VERSION "\n");
		return 0;
	}
	if(!parser->isSet(nOption)) {
		std::string homedir = getLocalDir();
		recursiveMakeDir(homedir);
		QString lockpath = QString::fromStdString(buildPath(homedir, "qalculate-qt.lock"));
		QLockFile lockFile(lockpath);
		if(!lockFile.tryLock(100)) {
			if(lockFile.error() == QLockFile::LockFailedError) {
				QTextStream outStream(stdout);
				outStream << QApplication::tr("%1 is already running.").arg(app.applicationDisplayName()) << '\n';
				QLocalSocket socket;
				socket.connectToServer("qalculate-qt");
				if(socket.waitForConnected()) {
					QString command = "0";
					QStringList args = parser->positionalArguments();
					for(int i = 0; i < args.count(); i++) {
						if(i > 0) command += " ";
						command += args.at(i);
					}
					socket.write(command.toUtf8());
					socket.waitForBytesWritten(3000);
					socket.disconnectFromServer();
				}
				return 1;
			}
		}
	}

	app.setWindowIcon(LOAD_APP_ICON("qalculate-qt"));

	settings = new QalculateQtSettings();

	new Calculator(settings->ignore_locale);

	CALCULATOR->setExchangeRatesWarningEnabled(false);

	settings->loadPreferences();

	CALCULATOR->loadExchangeRates();

	if(!CALCULATOR->loadGlobalDefinitions()) {
		QTextStream outStream(stderr);
		outStream << QApplication::tr("Failed to load global definitions!\n");
		return 1;
	}

	CALCULATOR->loadLocalDefinitions();

	QalculateWindow *win = new QalculateWindow();
	if(parser->value(tOption).isEmpty()) {
		win->updateWindowTitle();
		title_modified = false;
	} else {
		win->setWindowTitle(parser->value(tOption));
		title_modified = true;
	}
	win->setCommandLineParser(parser);
	win->show();

	QStringList args = parser->positionalArguments();
	QString expression;
	for(int i = 0; i < args.count(); i++) {
		if(i > 0) expression += " ";
		expression += args.at(i);
	}
	expression = expression.trimmed();
	if(!expression.isEmpty()) win->calculate(expression);

	args.clear();

	return app.exec();

}
