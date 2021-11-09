/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2018 Simon Wells <simonrwells@gmail.com>
** Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <clocale>
#include "main.h"

#include <QDebug>
#include <QApplication>
#include <QSplashScreen>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QSplashScreen>
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "qg_dlginitial.h"

#include "lc_application.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"

#include "console_dxf2pdf.h"

#if defined(qApp)
#undef qApp
#endif
#define qApp (dynamic_cast<QApplication *>(QCoreApplication::instance()))

// Check first two arguments in order to decide if we want to run librecad
// as console dxf2pdf tool. On Linux we can create a link to librecad
// executable and  name it dxf2pdf. So, we can run either:
//
//     librecad dxf2pdf [options] ...
//
// or just:
//
//     dxf2pdf [options] ...
//
bool runAsConsoleApp(int argc, char** argv) {
    for (int i = 0; i < qMin(argc, 2); i++) {
        QString arg(argv[i]);
        if (i == 0) {
            arg = QFileInfo(QFile::decodeName(argv[i])).baseName();
        }
        if (arg.compare("dxf2pdf") == 0) {
            return true;
        }
    }
   return false;
}

bool justNeedHelpAndExit(const QString &argument) {
    const QString help0("-h"), help1("--help");
    const QString exit_arg = "--exit";

    if (argument == exit_arg) {
        return true;
    }
    if (help0.compare(argument, Qt::CaseInsensitive) == 0 || help1.compare(argument, Qt::CaseInsensitive) == 0) {
        qDebug() << "Usage: librecad [command] <options> <dxf file>";
        qDebug() << "";
        qDebug() << "Commands:";
        qDebug() << "";
        qDebug() << "  dxf2pdf\tRun librecad as console dxf2pdf tool. Use -h for help.";
        qDebug() << "";
        qDebug() << "Options:";
        qDebug() << "";
        qDebug() << "  -h, --help\tdisplay this message";
        qDebug() << "  -d, --debug <level>";
        qDebug() << "";
        RS_DEBUG->print(RS_Debug::D_NOTHING, "possible debug levels:");
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
        RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
        return true;
    }
    return false;
}

bool setDebugSwitch(int argc, char **argv, int &i) {

    const QString lpDebugSwitch0("-d"), lpDebugSwitch1("--debug");

    QString argument = argv[i];

    if (argument.startsWith(lpDebugSwitch0, Qt::CaseInsensitive) ||
        argument.startsWith(lpDebugSwitch1, Qt::CaseInsensitive)) {

        // to control the level of debugging output use --debug with level 0-6, e.g. --debug3
        // for a list of debug levels use --debug?
        // if no level follows, the debugging level is set
        argument.remove(QRegExp("^" + lpDebugSwitch0));
        argument.remove(QRegExp("^" + lpDebugSwitch1));
        char level;
        if (argument.size() == 0) {
            if (i + 1 < argc) {
                if (QRegExp("\\d*").exactMatch(argv[i + 1])) {
                    i++;
                    qDebug() << "reading " << argv[i] << " as debugging level";
                    level = argv[i][0];
                } else
                    level = '3';
            } else
                level = '3'; //default to D_WARNING
        } else
            level = argument.toStdString()[0];

        switch (level) {
            case '?' :
                RS_DEBUG->print(RS_Debug::D_NOTHING, "possible debug levels:");
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
                RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
                return 0;

            case '0' + RS_Debug::D_NOTHING :
                RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
                break;

            case '0' + RS_Debug::D_CRITICAL :
                RS_DEBUG->setLevel(RS_Debug::D_CRITICAL);
                break;

            case '0' + RS_Debug::D_ERROR :
                RS_DEBUG->setLevel(RS_Debug::D_ERROR);
                break;

            case '0' + RS_Debug::D_WARNING :
                RS_DEBUG->setLevel(RS_Debug::D_WARNING);
                break;

            case '0' + RS_Debug::D_NOTICE :
                RS_DEBUG->setLevel(RS_Debug::D_NOTICE);
                break;

            case '0' + RS_Debug::D_INFORMATIONAL :
                RS_DEBUG->setLevel(RS_Debug::D_INFORMATIONAL);
                break;

            default :
                RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
                break;
        }
        return true;
    }
    return false;
}

void setInitialUnit() {
    // show initial config dialog:

    RS_DEBUG->print("main: show initial config dialog..");
    QG_DlgInitial di(nullptr);
    QPixmap pxm(":/main/intro_librecad.png");
    di.setPixmap(pxm);
    if (di.exec()) {
        RS_SETTINGS->beginGroup("/Defaults");
        RS_SETTINGS->endGroup();
    }
    RS_DEBUG->print("main: show initial config dialog: OK");
}

QStringList handleArguments(int argc, char **argv) {
    QStringList not_handled_arguments;

    for (int i = 1; i < argc; i++) {
        QString argstr(argv[i]);
        if(justNeedHelpAndExit(argstr)) {
            exit(0);
        }
        if(setDebugSwitch(argc, argv, i)) {
            continue;
        }

        not_handled_arguments.push_back(argv[i]);
    }

    return not_handled_arguments;
}

/**
 * Main. Creates Application window.
 */
int main(int argc, char** argv)
{
    QT_REQUIRE_VERSION(argc, argv, "5.2.1")

    if(runAsConsoleApp(argc, argv)) {
        return console_dxf2pdf(argc, argv);
    }

    RS_DEBUG->setLevel(RS_Debug::D_WARNING);

    LC_Application app(argc, argv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    QGuiApplication::setDesktopFileName("librecad.desktop");
#endif

    QStringList not_handled_arguments = handleArguments(argc, argv);
    RS_DEBUG->print("param 0: %s", argv[0]);

    QSettings settings;
    bool first_load = settings.value("Startup/FirstLoad", 1).toBool();

    RS_SETTINGS->init(LC_Application::organizationName(), LC_Application::applicationName());
    RS_SYSTEM->init(LC_Application::applicationName(), LC_Application::applicationVersion(), XSTR(QC_APPDIR),
                    QFileInfo(QFile::decodeName(argv[0]) ).absolutePath());

    // parse command line arguments that might not need a launched program:
    QStringList fileList = handleFileArguments(not_handled_arguments);


    if(first_load) {
        setInitialUnit();
    }

    auto splash = (settings.value("Startup/ShowSplash", 1).toBool())
            ?std::make_unique<QSplashScreen>()
                    :std::unique_ptr<QSplashScreen>(nullptr);

    QC_ApplicationWindow::initSplashWindow(splash.get());
    QC_ApplicationWindow::showMessageInSplashWindow(splash.get(), QObject::tr("Loading.."));

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    settings.beginGroup("Appearance");
    QString lang = settings.value("Language", "en").toString();
    QString langCmd = settings.value("LanguageCmd", "en").toString();
    settings.endGroup();

    RS_SYSTEM->loadTranslation(lang, langCmd);

    QC_ApplicationWindow appWin;
#ifdef Q_OS_MAC
    app.installEventFilter(&appWin);
#endif

    appWin.arrangeWindow(first_load, &settings);

    QC_ApplicationWindow::showMessageInSplashWindow(splash.get(), QObject::tr("Loading..."));

    // Set LC_NUMERIC so that entering numeric values uses . as the decimal separator
    setlocale(LC_NUMERIC, "C");

    if (!appWin.loadFiles(fileList, splash.get())) { appWin.slotFileNewNew(); }

    if (first_load) { settings.setValue("Startup/FirstLoad", 0); }
    if(splash) {splash->finish(&appWin);}
    RS_DEBUG->print("main: entering Qt event loop");

    int return_code = LC_Application::exec();

    RS_DEBUG->print("main: exited Qt event loop");

    return return_code;
}


/**
 * Handles command line arguments that might not require a GUI.
 *
 * @return list of files to load on startup.
 */
QStringList handleFileArguments(const QStringList& arguments)
{
    RS_DEBUG->print("main: handling args..");
    QStringList ret;

    for(const auto& argument: arguments) {
        if (!argument.startsWith("-"))
        {
            QString fname = QDir::toNativeSeparators(
            QFileInfo(QFile::decodeName(argument.toLocal8Bit())).absoluteFilePath());
            ret.append(fname);
        }
    }
    RS_DEBUG->print("main: handling args: OK");
    return ret;
}

