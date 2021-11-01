/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

// Changes: https://github.com/LibreCAD/LibreCAD/commits/master/librecad/src/main/qc_applicationwindow.h

#ifndef QC_APPLICATIONWINDOW_H
#define QC_APPLICATIONWINDOW_H

#include "mainwindowx.h"

#include "rs_pen.h"
#include "rs_snapper.h"
#include <QMap>
#include <QSettings>

class QSplashScreen;
class QMdiArea;
class QMdiSubWindow;
class QC_MDIWindow;
class QG_LibraryWidget;
class QG_SnapToolBar;
class QC_DialogFactory;
class QG_LayerWidget;
class QG_BlockWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class QG_MouseWidget;
class QG_SelectionWidget;
class QG_RecentFiles;
class QG_PenToolBar;
class QC_PluginInterface;
class QG_ActiveLayerName;
class LC_SimpleTests;
class QG_ActionHandler;
class RS_GraphicView;
class RS_Document;
class TwoStackedLabels;
class LC_ActionGroupManager;
class LC_PenWizard;

struct DockAreas
{
    QAction* left {nullptr};
    QAction* right {nullptr};
    QAction* top {nullptr};
    QAction* bottom {nullptr};
    QAction* floating {nullptr};
};

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow: public MainWindowX
{
    Q_OBJECT

public:
    QC_ApplicationWindow();
    ~QC_ApplicationWindow() override;

    void arrangeWindow(bool first_load, QSettings* settings);
    bool loadFiles(const QStringList &file_list, QSplashScreen* splash);

    void initSettings();
    void storeSettings();

    bool queryExit(bool force);

    /** Catch hotkey for giving focus to command line. */
    void keyPressEvent(QKeyEvent* e) override;
    void setRedoEnable(bool enable);
    void setUndoEnable(bool enable);

    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * opens the given file.
     */
    void fileOpen(const QString& fileName, RS2::FormatType type);
    void fileOpen(const QString& fileName); // Assume Unknown type
    bool fileExport(const QString& name,
                    const QString& format,
                    QSize size,
                    QSize borders,
                    bool black,
                    bool bw=true);

    QMap<QString, QAction*> a_map;
    LC_ActionGroupManager* ag_manager {nullptr};

public slots:
    void relayAction(QAction* q_action);
    void slotFocus();

    void slotKillAllActions();
    void slotEnter();
    void slotFocusCommandLine();

    void slotWindowActivated(QMdiSubWindow* w);
    void slotWindowsMenuAboutToShow();
    void slotWindowsMenuActivated(bool);
    void slotCascade();
    void slotTile();
    void slotTileHorizontal();
    void slotTileVertical();
	void slotSetMaximized();

	void slotTabShapeRounded();
	void slotTabShapeTriangular();
	void slotTabPositionNorth();
	void slotTabPositionSouth();
	void slotTabPositionEast();
	void slotTabPositionWest();

    void slotToggleTab();
    void slotZoomAuto();

    void slotPenChanged(const RS_Pen& p);

    /** generates a new document for a graphic. */
	QC_MDIWindow* slotFileNew(RS_Document* doc=nullptr);
    /** generates a new document based in predefined template */
    void slotFileNewNew();
    /** generates a new document based in selected template */
    void slotFileNewTemplate();
    /** opens a document */
    void slotFileOpen();

    void slotFileOpenRecent(QAction* action);
    /** saves a document */
    void slotFileSave();
    /** saves a document under a different filename*/
    void slotFileSaveAs();
	/** saves all open documents; return false == operation cancelled **/
	bool slotFileSaveAll();
    /** auto-save document */
    void slotFileAutoSave();
    /** exports the document as bitmap */
    void slotFileExport();
    /** closing the current file */
    void slotFileClosing(QC_MDIWindow *win, QCloseEvent *event = nullptr);
	/** close all files; return false == operation cancelled */
	bool slotFileCloseAll();
    /** prints the current file */
    void slotFilePrint(bool printPDF=false);
    void slotFilePrintPDF();
    /** shows print preview of the current file */
    void slotFilePrintPreview(bool on);
    /** exits the application */
    void slotFileQuit();

    /** toggle the grid */
    void slotViewGrid(bool toggle);
    /** toggle the draft mode */
    void slotViewDraft(bool toggle);
    /** toggle the statusbar */
    void slotViewStatusBar(bool toggle);

    void slotOptionsGeneral();

    void slotImportBlock();

    /** shows an about dlg*/
    static void showAboutWindow();

    /**
     * @brief slotUpdateActiveLayer
     * update layer name when active layer changed
     */
    void slotUpdateActiveLayer();
	void execPlug();

    static void invokeLinkList();

    void toggleFullscreen(bool checked);

    void setPreviousZoomEnable(bool enable);

    static void hideOptions(QC_MDIWindow*);

    void widgetOptionsDialog();

    void modifyCommandTitleBar(Qt::DockWidgetArea area);
    void reloadStyleSheet();

    void updateGridStatus(const QString&);

    void showDeviceOptions() const;

    void updateDevice(const QString&);

    void invokeMenuCreator();
    void invokeToolbarCreator();
    void createToolbar(const QString& toolbar_name);
    void destroyToolbar(const QString& toolbar_name);
    void destroyMenu(const QString& activator);
    void unassignMenu(const QString& activator, const QString& menu_name);
    void assignMenu(const QString& activator, const QString& menu_name);
    void invokeMenuAssigner(const QString& menu_name);
    void updateMenu(const QString& menu_name);

    static void invokeLicenseWindow();
    static void initSplashWindow(QSplashScreen* splash);
    static void showMessageInSplashWindow(QSplashScreen *splash, const QString &message);


signals:
    void gridChanged(bool on);
    void draftChanged(bool on);
    void printPreviewChanged(bool on);
    void windowsChanged(bool windowsLeft);

private:
    static bool loadStyleSheet(const QString& path);
public:
    /**
     * @return Pointer to application window.
     */
    static QC_ApplicationWindow* getAppWindow() {
        return appWindow;
    }

    /**
     * @return Pointer to MdiArea.
     */
	QMdiArea* getMdiArea();

    /**
	 * @return Pointer to the currently active MDI Window or nullptr if no
     * MDI Window is active.
     */
	[[nodiscard]] const QC_MDIWindow* getMDIWindow() const;
	QC_MDIWindow* getMDIWindow();

    RS_GraphicView* getGraphicView();

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic document of the currently active document
	 * window or nullptr if no window is available.
     */
	[[nodiscard]] const RS_Document* getDocument() const;
	RS_Document* getDocument();

    void redrawAll();
    void updateGrids();

    QG_BlockWidget* getBlockWidget();

    /**
     * Find opened window for specified document.
     */
    QC_MDIWindow* getWindowWithDoc(const RS_Document* doc);

protected:
    void closeEvent(QCloseEvent*) override;
    //! \{ accept drop files to open
    void dropEvent(QDropEvent* e) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void changeEvent(QEvent* event) override;
    //! \}

private:
    QMenu* createPopupMenu() override;

    static QString format_filename_caption(const QString &qstring_in);
    /** Helper function for Menu file -> New & New.... */
	bool fileNewHelper(const QString &fileName, QC_MDIWindow* w = nullptr);
	// more helpers
	void doArrangeWindows(RS2::SubWindowMode mode, bool actuallyDont = false);
	void setTabLayout(RS2::TabShape s, RS2::TabPosition p);
	bool doSave(QC_MDIWindow* w, bool forceSaveAs = false);
	void doClose(QC_MDIWindow* w, bool activateNext = true);
	void doActivate(QMdiSubWindow* w);
	int showCloseDialog(QC_MDIWindow* w, bool showSaveAll = false);
	void enableFileActions(QC_MDIWindow* w);

    //Plugin support
    void loadPlugins();
    QMenu *findMenu(const QString &searchMenu, const QObjectList &thisMenuList, const QString& currentEntry);

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* appWindow;
    QTimer *autosaveTimer {nullptr};

    QG_ActionHandler* actionHandler {nullptr};

    /** MdiArea for MDI */
    QMdiArea* mdiAreaCAD {nullptr};
    QMdiSubWindow* activedMdiSubWindow {nullptr};
    QMdiSubWindow* current_subwindow {nullptr};


    /** Dialog factory */
    QC_DialogFactory* dialogFactory {nullptr};

    /** Recent files list */
    QG_RecentFiles* recentFiles {nullptr};

    // --- Dockwidgets ---
    //! toggle actions for the dock areas
    DockAreas dock_areas;

    /** Layer list widget */
    QG_LayerWidget* layerWidget {nullptr};
    /** Block list widget */
    QG_BlockWidget* blockWidget {nullptr};
    /** Command line */
    QG_CommandWidget* commandWidget {nullptr};

    LC_PenWizard* pen_wiz {nullptr};

    // --- Statusbar ---
    /** Coordinate widget */
    QG_CoordinateWidget* coordinateWidget {nullptr};
    /** Mouse widget */
    QG_MouseWidget* mouseWidget {nullptr};
    /** Selection Status */
    QG_SelectionWidget* selectionWidget {nullptr};
    QG_ActiveLayerName* m_pActiveLayerName {nullptr};
    TwoStackedLabels* grid_status {nullptr};

    // --- Menus ---
    QMenu* windowsMenu {nullptr};
    QMenu* file_menu {nullptr};

    // --- Toolbars ---
    QG_SnapToolBar* snapToolBar {nullptr};
    QG_PenToolBar* penToolBar {nullptr}; //!< for selecting the current pen
    QToolBar* optionWidget {nullptr}; //!< for individual tool options

    // --- Actions ---
    QAction* previousZoom {nullptr};
    QAction* undoButton {nullptr};
    QAction* redoButton {nullptr};

    // --- Lists ---
    QList<QC_PluginInterface*> loadedPlugins;
    QList<QC_MDIWindow*> window_list;

    QStringList openedFiles;

    // --- Strings ---
    QString style_sheet_path;
    static const unsigned int MAX_DROPPED_FILES = 32;
};

#ifdef _WINDOWS
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

#endif

