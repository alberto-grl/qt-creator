/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mainwindow.h"
#include "icore.h"
#include "coreicons.h"
#include "jsexpander.h"
#include "toolsettings.h"
#include "mimetypesettings.h"
#include "fancytabwidget.h"
#include "documentmanager.h"
#include "generalsettings.h"
//OPENMV-DIFF//
//#include "helpmanager.h"
//OPENMV-DIFF//
#include "idocumentfactory.h"
#include "messagemanager.h"
#include "modemanager.h"
#include "outputpanemanager.h"
#include "plugindialog.h"
#include "vcsmanager.h"
#include "versiondialog.h"
#include "statusbarmanager.h"
#include "id.h"
#include "manhattanstyle.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "editormanager/ieditorfactory.h"
#include "statusbarwidget.h"
#include "systemsettings.h"
#include "externaltoolmanager.h"
#include "editormanager/systemeditor.h"
#include "windowsupport.h"

#include <app/app_version.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actionmanager_p.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/dialogs/newdialog.h>
#include <coreplugin/dialogs/settingsdialog.h>
#include <coreplugin/dialogs/shortcutsettings.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <coreplugin/progressmanager/progressmanager_p.h>
#include <coreplugin/progressmanager/progressview.h>
#include <coreplugin/settingsdatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/historycompleter.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrinter>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

using namespace ExtensionSystem;
using namespace Utils;

namespace Core {
namespace Internal {

enum { debugMainWindow = 0 };

MainWindow::MainWindow() :
    AppMainWindow(),
    m_coreImpl(new ICore(this)),
    m_lowPrioAdditionalContexts(Constants::C_GLOBAL),
    m_settingsDatabase(new SettingsDatabase(QFileInfo(PluginManager::settings()->fileName()).path(),
                                            //OPENMV-DIFF// QLatin1String("QtCreator"),
                                            QLatin1String("OpenMVIDE"),
                                            this)),
    m_printer(0),
    m_windowSupport(0),
    m_editorManager(0),
    m_externalToolManager(0),
    m_progressManager(new ProgressManagerPrivate),
    m_jsExpander(new JsExpander),
    m_vcsManager(new VcsManager),
    m_statusBarManager(0),
    m_modeManager(0),
    //OPENMV-DIFF//
    //m_helpManager(new HelpManager),
    //OPENMV-DIFF//
    m_modeStack(new FancyTabWidget(this)),
    m_navigationWidget(0),
    m_rightPaneWidget(0),
    m_versionDialog(0),
    //OPENMV-DIFF//
    m_disableShow(0),
    //OPENMV-DIFF//
    m_generalSettings(new GeneralSettings),
    m_systemSettings(new SystemSettings),
    m_shortcutSettings(new ShortcutSettings),
    m_toolSettings(new ToolSettings),
    m_mimeTypeSettings(new MimeTypeSettings),
    m_systemEditor(new SystemEditor),
    m_focusToEditor(0),
    m_newAction(0),
    m_openAction(0),
    m_openWithAction(0),
    m_saveAllAction(0),
    m_exitAction(0),
    m_optionsAction(0),
    m_toggleSideBarAction(0),
    m_toggleSideBarButton(new QToolButton)
{
    (void) new DocumentManager(this);
    OutputPaneManager::create();

    HistoryCompleter::setSettings(PluginManager::settings());

    //OPENMV-DIFF// setWindowTitle(tr("Qt Creator"));
    setWindowTitle(tr("OpenMV IDE"));
    //OPENMV-DIFF// if (HostOsInfo::isLinuxHost())
    //OPENMV-DIFF//     QApplication::setWindowIcon(Icons::QTLOGO_128.icon());
    //OPENMV-DIFF// QCoreApplication::setApplicationName(QLatin1String("QtCreator"));
    QCoreApplication::setApplicationName(QLatin1String("OpenMVIDE"));
    //OPENMV-DIFF// QCoreApplication::setApplicationVersion(QLatin1String(Constants::IDE_VERSION_LONG));
    QCoreApplication::setApplicationVersion(QLatin1String(Constants::OMV_IDE_VERSION_LONG));
    QCoreApplication::setOrganizationName(QLatin1String(Constants::IDE_SETTINGSVARIANT_STR));
    QString baseName = QApplication::style()->objectName();
    // Sometimes we get the standard windows 95 style as a fallback
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()
            && baseName == QLatin1String("windows")) {
        baseName = QLatin1String("fusion");
    }

    // if the user has specified as base style in the theme settings,
    // prefer that
    const QStringList available = QStyleFactory::keys();
    foreach (const QString &s, Utils::creatorTheme()->preferredStyles()) {
        if (available.contains(s, Qt::CaseInsensitive)) {
            baseName = s;
            break;
        }
    }

    QApplication::setStyle(new ManhattanStyle(baseName));

    setDockNestingEnabled(true);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    m_modeManager = new ModeManager(this, m_modeStack);
    connect(m_modeStack, &FancyTabWidget::topAreaClicked, this, [](Qt::MouseButton, Qt::KeyboardModifiers modifiers) {
        if (modifiers & Qt::ShiftModifier) {
            QColor color = QColorDialog::getColor(StyleHelper::requestedBaseColor(), ICore::dialogParent());
            if (color.isValid())
                StyleHelper::setBaseColor(color);
        }
    });

    registerDefaultContainers();
    registerDefaultActions();

    m_navigationWidget = new NavigationWidget(m_toggleSideBarAction);
    m_rightPaneWidget = new RightPaneWidget();

    m_statusBarManager = new StatusBarManager(this);
    m_messageManager = new MessageManager;
    m_editorManager = new EditorManager(this);
    m_externalToolManager = new ExternalToolManager();
    setCentralWidget(m_modeStack);

    m_progressManager->progressView()->setParent(this);
    m_progressManager->progressView()->setReferenceWidget(m_modeStack->statusBar());

    connect(qApp, &QApplication::focusChanged, this, &MainWindow::updateFocusWidget);
    // Add a small Toolbutton for toggling the navigation widget
    statusBar()->insertPermanentWidget(0, m_toggleSideBarButton);
    // OPENMV-DIFF //
    m_toggleSideBarButton->hide();
    // OPENMV-DIFF //

//    setUnifiedTitleAndToolBarOnMac(true);
    //if (HostOsInfo::isAnyUnixHost())
        //signal(SIGINT, handleSigInt);

    statusBar()->setProperty("p_styled", true);

    auto dropSupport = new DropSupport(this, [](QDropEvent *event, DropSupport *) {
        return event->source() == 0; // only accept drops from the "outside" (e.g. file manager)
    });
    connect(dropSupport, &DropSupport::filesDropped,
            this, &MainWindow::openDroppedFiles);
}

void MainWindow::setSidebarVisible(bool visible)
{
    //OPENMV-DIFF//
    //if (NavigationWidgetPlaceHolder::current()) {
    //    if (m_navigationWidget->isSuppressed() && visible) {
    //        m_navigationWidget->setShown(true);
    //        m_navigationWidget->setSuppressed(false);
    //    } else {
    //        m_navigationWidget->setShown(visible);
    //    }
    //}
    //OPENMV-DIFF//
    Q_UNUSED(visible)
    //OPENMV-DIFF//
}

void MainWindow::setSuppressNavigationWidget(bool suppress)
{
    //OPENMV-DIFF//
    //if (NavigationWidgetPlaceHolder::current())
    //    m_navigationWidget->setSuppressed(suppress);
    //OPENMV-DIFF//
    Q_UNUSED(suppress)
    //OPENMV-DIFF//
}

void MainWindow::setOverrideColor(const QColor &color)
{
    m_overrideColor = color;
}

QStringList MainWindow::additionalAboutInformation() const
{
    return m_aboutInformation;
}

void MainWindow::appendAboutInformation(const QString &line)
{
    m_aboutInformation.append(line);
}

void MainWindow::addPreCloseListener(const std::function<bool ()> &listener)
{
    m_preCloseListeners.append(listener);
}

MainWindow::~MainWindow()
{
    // explicitly delete window support, because that calls methods from ICore that call methods
    // from mainwindow, so mainwindow still needs to be alive
    delete m_windowSupport;
    m_windowSupport = 0;

    //OPENMV-DIFF// PluginManager::removeObject(m_shortcutSettings);
    PluginManager::removeObject(m_generalSettings);
    //OPENMV-DIFF// PluginManager::removeObject(m_systemSettings);
    //OPENMV-DIFF// PluginManager::removeObject(m_toolSettings);
    //OPENMV-DIFF// PluginManager::removeObject(m_mimeTypeSettings);
    PluginManager::removeObject(m_systemEditor);
    delete m_externalToolManager;
    m_externalToolManager = 0;
    delete m_messageManager;
    m_messageManager = 0;
    delete m_shortcutSettings;
    m_shortcutSettings = 0;
    delete m_generalSettings;
    m_generalSettings = 0;
    delete m_systemSettings;
    m_systemSettings = 0;
    delete m_toolSettings;
    m_toolSettings = 0;
    delete m_mimeTypeSettings;
    m_mimeTypeSettings = 0;
    delete m_systemEditor;
    m_systemEditor = 0;
    delete m_printer;
    m_printer = 0;
    delete m_vcsManager;
    m_vcsManager = 0;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    // Now that the OutputPaneManager is gone, is a good time to delete the view
    PluginManager::removeObject(m_outputView);
    delete m_outputView;

    delete m_editorManager;
    m_editorManager = 0;
    delete m_progressManager;
    m_progressManager = 0;
    delete m_statusBarManager;
    m_statusBarManager = 0;
    PluginManager::removeObject(m_coreImpl);
    delete m_coreImpl;
    m_coreImpl = 0;

    delete m_rightPaneWidget;
    m_rightPaneWidget = 0;

    delete m_modeManager;
    m_modeManager = 0;

    //OPENMV-DIFF//
    //delete m_helpManager;
    //m_helpManager = 0;
    //OPENMV-DIFF//
    delete m_jsExpander;
    m_jsExpander = 0;
}

bool MainWindow::init(QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    PluginManager::addObject(m_coreImpl);
    m_statusBarManager->init();
    m_modeManager->init();
    m_progressManager->init(); // needs the status bar manager

    PluginManager::addObject(m_generalSettings);
    //OPENMV-DIFF// PluginManager::addObject(m_systemSettings);
    //OPENMV-DIFF// PluginManager::addObject(m_shortcutSettings);
    //OPENMV-DIFF// PluginManager::addObject(m_toolSettings);
    //OPENMV-DIFF// PluginManager::addObject(m_mimeTypeSettings);
    PluginManager::addObject(m_systemEditor);

    // Add widget to the bottom, we create the view here instead of inside the
    // OutputPaneManager, since the StatusBarManager needs to be initialized before
    m_outputView = new StatusBarWidget;
    m_outputView->setWidget(OutputPaneManager::instance()->buttonsWidget());
    m_outputView->setPosition(StatusBarWidget::Second);
    PluginManager::addObject(m_outputView);
    MessageManager::init();
    return true;
}

void MainWindow::extensionsInitialized()
{
    EditorManagerPrivate::extensionsInitialized();
    MimeTypeSettings::restoreSettings();
    m_windowSupport = new WindowSupport(this, Context("Core.MainWindow"));
    m_windowSupport->setCloseActionEnabled(false);
    m_statusBarManager->extensionsInitalized();
    OutputPaneManager::instance()->init();
    m_vcsManager->extensionsInitialized();
    //OPENMV-DIFF//
    //m_navigationWidget->setFactories(PluginManager::getObjects<INavigationWidgetFactory>());
    //OPENMV-DIFF//

    readSettings();
    updateContext();

    emit m_coreImpl->coreAboutToOpen();
    // Delay restoreWindowState, since it is overridden by LayoutRequest event
    QTimer::singleShot(100, this, &MainWindow::restoreWindowState);
    QTimer::singleShot(0, m_coreImpl, &ICore::coreOpened);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ICore::saveSettings();

    // Save opened files
    if (!DocumentManager::saveAllModifiedDocuments()) {
        event->ignore();
        return;
    }

    foreach (const std::function<bool()> &listener, m_preCloseListeners) {
        if (!listener()) {
            event->ignore();
            return;
        }
    }

    emit m_coreImpl->coreAboutToClose();

    writeSettings();

    //OPENMV-DIFF//
    //m_navigationWidget->closeSubWidgets();
    //OPENMV-DIFF//

    event->accept();
}

void MainWindow::openDroppedFiles(const QList<DropSupport::FileSpec> &files)
{
    raiseWindow();
    QStringList filePaths = Utils::transform(files,
                                             [](const DropSupport::FileSpec &spec) -> QString {
                                                 return spec.filePath;
                                             });
    openFiles(filePaths, ICore::SwitchMode);
}

IContext *MainWindow::currentContextObject() const
{
    return m_activeContext.isEmpty() ? 0 : m_activeContext.first();
}

QStatusBar *MainWindow::statusBar() const
{
    return m_modeStack->statusBar();
}

void MainWindow::registerDefaultContainers()
{
    ActionContainer *menubar = ActionManager::createMenuBar(Constants::MENU_BAR);

    if (!HostOsInfo::isMacHost()) // System menu bar on Mac
        setMenuBar(menubar->menuBar());
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = ActionManager::createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE);
    filemenu->menu()->setTitle(tr("&File"));
    filemenu->appendGroup(Constants::G_FILE_NEW);
    filemenu->appendGroup(Constants::G_FILE_OPEN);
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), &QMenu::aboutToShow, this, &MainWindow::aboutToShowRecentFiles);


    // Edit Menu
    ActionContainer *medit = ActionManager::createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    // Tools Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_TOOLS);
    menubar->addMenu(ac, Constants::G_TOOLS);
    ac->menu()->setTitle(tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = ActionManager::createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_VIEWS);
    mwindow->appendGroup(Constants::G_WINDOW_PANES);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_LIST);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = ActionManager::createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(tr("&Help"));
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_SUPPORT);
    ac->appendGroup(Constants::G_HELP_ABOUT);
    ac->appendGroup(Constants::G_HELP_UPDATES);
}

void MainWindow::registerDefaultActions()
{
    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = ActionManager::actionContainer(Constants::M_HELP);

    // File menu separators
    //OPENMV-DIFF//
    mfile->addSeparator(Constants::G_FILE_OPEN);
    //OPENMV-DIFF//
    mfile->addSeparator(Constants::G_FILE_SAVE);
    mfile->addSeparator(Constants::G_FILE_PRINT);
    mfile->addSeparator(Constants::G_FILE_CLOSE);
    mfile->addSeparator(Constants::G_FILE_OTHER);
    // Edit menu separators
    medit->addSeparator(Constants::G_EDIT_COPYPASTE);
    medit->addSeparator(Constants::G_EDIT_SELECTALL);
    medit->addSeparator(Constants::G_EDIT_FIND);
    medit->addSeparator(Constants::G_EDIT_ADVANCED);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    m_focusToEditor = new QAction(tr("Return to Editor"), this);
    Command *cmd = ActionManager::registerAction(m_focusToEditor, Constants::S_RETURNTOEDITOR);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    connect(m_focusToEditor, &QAction::triggered, this, &MainWindow::setFocusToEditor);

    // New File Action
    QIcon icon = QIcon::fromTheme(QLatin1String("document-new"), Icons::NEWFILE.icon());
    // OPENMV-DIFF // m_newAction = new QAction(icon, tr("&New File or Project..."), this);
    m_newAction = new QAction(icon, tr("&New File..."), this);
    cmd = ActionManager::registerAction(m_newAction, Constants::NEW);
    cmd->setDefaultKeySequence(QKeySequence::New);
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    // OPENMV-DIFF //
    //connect(m_newAction, &QAction::triggered, this, [this]() {
    //    ICore::showNewItemDialog(tr("New File or Project", "Title of dialog"),
    //                             IWizardFactory::allWizardFactories(), QString());
    //});
    // OPENMV-DIFF //
    connect(ICore::instance(), &ICore::newItemDialogRunningChanged, m_newAction, [this]() {
        m_newAction->setEnabled(!ICore::isNewItemDialogRunning());
    });

    // Open Action
    icon = QIcon::fromTheme(QLatin1String("document-open"), Icons::OPENFILE.icon());
    // OPENMV-DIFF // m_openAction = new QAction(icon, tr("&Open File or Project..."), this);
    m_openAction = new QAction(icon, tr("&Open File..."), this);
    cmd = ActionManager::registerAction(m_openAction, Constants::OPEN);
    cmd->setDefaultKeySequence(QKeySequence::Open);
    //OPENMV-DIFF//
    //mfile->addAction(cmd, Constants::G_FILE_OPEN);
    //OPENMV-DIFF//
    mfile->addAction(cmd, Constants::G_FILE_NEW);
    //OPENMV-DIFF//
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);

    //// Open With Action
    m_openWithAction = new QAction(tr("Open File &With..."), this);
    cmd = ActionManager::registerAction(m_openWithAction, Constants::OPEN_WITH);
    // OPENMV-DIFF // mfile->addAction(cmd, Constants::G_FILE_OPEN);
    connect(m_openWithAction, &QAction::triggered, this, &MainWindow::openFileWith);

    // File->Recent Files Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action
    // OPENMV-DIFF // icon = QIcon::fromTheme(QLatin1String("document-save"), Icons::SAVEFILE.icon());
    icon = QIcon::fromTheme(QLatin1String("document-save"), Icons::SAVEFILE_OLD.icon());
    QAction *tmpaction = new QAction(icon, EditorManager::tr("&Save"), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVE);
    cmd->setDefaultKeySequence(QKeySequence::Save);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // Save As Action
    icon = QIcon::fromTheme(QLatin1String("document-save-as"));
    tmpaction = new QAction(icon, EditorManager::tr("Save &As..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::SAVEAS);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Shift+S") : QString()));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Save As..."));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);

    // SaveAll Action
    m_saveAllAction = new QAction(tr("Save A&ll"), this);
    cmd = ActionManager::registerAction(m_saveAllAction, Constants::SAVEALL);
    // OPENMV-DIFF // cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString() : tr("Ctrl+Shift+S")));
    // OPENMV-DIFF // mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_saveAllAction, &QAction::triggered, this, &MainWindow::saveAll);

    // Print Action
    icon = QIcon::fromTheme(QLatin1String("document-print"));
    tmpaction = new QAction(icon, tr("&Print..."), this);
    tmpaction->setEnabled(false);
    cmd = ActionManager::registerAction(tmpaction, Constants::PRINT);
    cmd->setDefaultKeySequence(QKeySequence::Print);
    mfile->addAction(cmd, Constants::G_FILE_PRINT);

    // Exit Action
    icon = QIcon::fromTheme(QLatin1String("application-exit"));
    m_exitAction = new QAction(icon, tr("E&xit"), this);
    m_exitAction->setMenuRole(QAction::QuitRole);
    cmd = ActionManager::registerAction(m_exitAction, Constants::EXIT);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Q")));
    mfile->addAction(cmd, Constants::G_FILE_OTHER);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::exit);

    // Undo Action
    icon = QIcon::fromTheme(QLatin1String("edit-undo"), Icons::UNDO.icon());
    tmpaction = new QAction(icon, tr("&Undo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::UNDO);
    cmd->setDefaultKeySequence(QKeySequence::Undo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Undo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Redo Action
    icon = QIcon::fromTheme(QLatin1String("edit-redo"), Icons::REDO.icon());
    tmpaction = new QAction(icon, tr("&Redo"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::REDO);
    cmd->setDefaultKeySequence(QKeySequence::Redo);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Redo"));
    medit->addAction(cmd, Constants::G_EDIT_UNDOREDO);
    tmpaction->setEnabled(false);

    // Cut Action
    icon = QIcon::fromTheme(QLatin1String("edit-cut"), Icons::CUT.icon());
    tmpaction = new QAction(icon, tr("Cu&t"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::CUT);
    cmd->setDefaultKeySequence(QKeySequence::Cut);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Copy Action
    icon = QIcon::fromTheme(QLatin1String("edit-copy"), Icons::COPY.icon());
    tmpaction = new QAction(icon, tr("&Copy"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::COPY);
    cmd->setDefaultKeySequence(QKeySequence::Copy);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Paste Action
    icon = QIcon::fromTheme(QLatin1String("edit-paste"), Icons::PASTE.icon());
    tmpaction = new QAction(icon, tr("&Paste"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::PASTE);
    cmd->setDefaultKeySequence(QKeySequence::Paste);
    medit->addAction(cmd, Constants::G_EDIT_COPYPASTE);
    tmpaction->setEnabled(false);

    // Select All
    icon = QIcon::fromTheme(QLatin1String("edit-select-all"));
    tmpaction = new QAction(icon, tr("Select &All"), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::SELECTALL);
    cmd->setDefaultKeySequence(QKeySequence::SelectAll);
    medit->addAction(cmd, Constants::G_EDIT_SELECTALL);
    tmpaction->setEnabled(false);

    // Goto Action
    icon = QIcon::fromTheme(QLatin1String("go-jump"));
    tmpaction = new QAction(icon, tr("&Go to Line..."), this);
    cmd = ActionManager::registerAction(tmpaction, Constants::GOTO);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+L")));
    medit->addAction(cmd, Constants::G_EDIT_OTHER);
    tmpaction->setEnabled(false);

    // Options Action
    mtools->appendGroup(Constants::G_TOOLS_OPTIONS);
    mtools->addSeparator(Constants::G_TOOLS_OPTIONS);

    m_optionsAction = new QAction(tr("&Options..."), this);
    m_optionsAction->setMenuRole(QAction::PreferencesRole);
    cmd = ActionManager::registerAction(m_optionsAction, Constants::OPTIONS);
    cmd->setDefaultKeySequence(QKeySequence::Preferences);
    mtools->addAction(cmd, Constants::G_TOOLS_OPTIONS);
    connect(m_optionsAction, &QAction::triggered, this, [this]() { showOptionsDialog(); });

    mwindow->addSeparator(Constants::G_WINDOW_LIST);

    if (UseMacShortcuts) {
        // Minimize Action
        QAction *minimizeAction = new QAction(tr("Minimize"), this);
        minimizeAction->setEnabled(false); // actual implementation in WindowSupport
        cmd = ActionManager::registerAction(minimizeAction, Constants::MINIMIZE_WINDOW);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+M")));
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);

        // Zoom Action
        QAction *zoomAction = new QAction(tr("Zoom"), this);
        zoomAction->setEnabled(false); // actual implementation in WindowSupport
        cmd = ActionManager::registerAction(zoomAction, Constants::ZOOM_WINDOW);
        mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
    }

    // Full Screen Action
    QAction *toggleFullScreenAction = new QAction(tr("Full Screen"), this);
    toggleFullScreenAction->setCheckable(!HostOsInfo::isMacHost());
    toggleFullScreenAction->setEnabled(false); // actual implementation in WindowSupport
    cmd = ActionManager::registerAction(toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Meta+F") : tr("Ctrl+Shift+F11")));
    if (HostOsInfo::isMacHost())
        cmd->setAttribute(Command::CA_UpdateText);
    mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);

    if (UseMacShortcuts) {
        mwindow->addSeparator(Constants::G_WINDOW_SIZE);

        QAction *closeAction = new QAction(tr("Close Window"), this);
        closeAction->setEnabled(false);
        cmd = ActionManager::registerAction(closeAction, Constants::CLOSE_WINDOW);
        //OPENMV-DIFF//
        //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Meta+W")));
        //mwindow->addAction(cmd, Constants::G_WINDOW_SIZE);
        //OPENMV-DIFF//

        mwindow->addSeparator(Constants::G_WINDOW_SIZE);
    }

    // Show Sidebar Action
    m_toggleSideBarAction = new QAction(Icons::TOGGLE_SIDEBAR_TOOLBAR.icon(),
                                        QCoreApplication::translate("Core", Constants::TR_SHOW_SIDEBAR),
                                        this);
    m_toggleSideBarAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleSideBarAction, Constants::TOGGLE_SIDEBAR);
    cmd->setAttribute(Command::CA_UpdateText);
    // OPENMV-DIFFF // cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+0") : tr("Alt+0")));
    connect(m_toggleSideBarAction, &QAction::triggered, this, &MainWindow::setSidebarVisible);
    m_toggleSideBarButton->setDefaultAction(cmd->action());
    // OPENMV-DIFFF // mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);
    m_toggleSideBarAction->setEnabled(false);

    // Show Mode Selector Action
    m_toggleModeSelectorAction = new QAction(tr("Show Mode Selector"), this);
    m_toggleModeSelectorAction->setCheckable(true);
    cmd = ActionManager::registerAction(m_toggleModeSelectorAction, Constants::TOGGLE_MODE_SELECTOR);
    connect(m_toggleModeSelectorAction, &QAction::triggered,
            ModeManager::instance(), &ModeManager::setModeSelectorVisible);
    // OPENMV-DIFFF // mwindow->addAction(cmd, Constants::G_WINDOW_VIEWS);

    // Window->Views
    ActionContainer *mviews = ActionManager::createMenu(Constants::M_WINDOW_VIEWS);
    // OPENMV-DIFFF // mwindow->addMenu(mviews, Constants::G_WINDOW_VIEWS);
    mviews->menu()->setTitle(tr("&Views"));

    // "Help" separators
    mhelp->addSeparator(Constants::G_HELP_SUPPORT);
    if (!HostOsInfo::isMacHost())
        mhelp->addSeparator(Constants::G_HELP_ABOUT);

    // About IDE Action
    icon = QIcon::fromTheme(QLatin1String("help-about"));
    if (HostOsInfo::isMacHost())
        tmpaction = new QAction(icon, tr("About &Qt Creator"), this); // it's convention not to add dots to the about menu
    else
        tmpaction = new QAction(icon, tr("About &Qt Creator..."), this);
    tmpaction->setMenuRole(QAction::AboutRole);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_QTCREATOR);
    // OPENMV-DIFF // mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, &QAction::triggered, this, &MainWindow::aboutQtCreator);

    //About Plugins Action
    tmpaction = new QAction(tr("About &Plugins..."), this);
    tmpaction->setMenuRole(QAction::ApplicationSpecificRole);
    cmd = ActionManager::registerAction(tmpaction, Constants::ABOUT_PLUGINS);
    // OPENMV-DIFF // mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    tmpaction->setEnabled(true);
    connect(tmpaction, &QAction::triggered, this, &MainWindow::aboutPlugins);
    // About Qt Action
//    tmpaction = new QAction(tr("About &Qt..."), this);
//    cmd = ActionManager::registerAction(tmpaction, Constants:: ABOUT_QT);
//    mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
//    tmpaction->setEnabled(true);
//    connect(tmpaction, &QAction::triggered, qApp, &QApplication::aboutQt);
    // About sep
    if (!HostOsInfo::isMacHost()) { // doesn't have the "About" actions in the Help menu
        tmpaction = new QAction(this);
        tmpaction->setSeparator(true);
        cmd = ActionManager::registerAction(tmpaction, "QtCreator.Help.Sep.About");
        mhelp->addAction(cmd, Constants::G_HELP_ABOUT);
    }
}

void MainWindow::openFile()
{
    openFiles(EditorManager::getOpenFileNames(), ICore::SwitchMode);
}

static IDocumentFactory *findDocumentFactory(const QList<IDocumentFactory*> &fileFactories,
                                     const QFileInfo &fi)
{
    MimeDatabase mdb;
    const MimeType mt = mdb.mimeTypeForFile(fi);
    if (mt.isValid()) {
        const QString typeName = mt.name();
        foreach (IDocumentFactory *factory, fileFactories) {
            if (factory->mimeTypes().contains(typeName))
                return factory;
        }
    }
    return 0;
}

/*! Either opens \a fileNames with editors or loads a project.
 *
 *  \a flags can be used to stop on first failure, indicate that a file name
 *  might include line numbers and/or switch mode to edit mode.
 *
 *  \a workingDirectory is used when files are opened by a remote client, since
 *  the file names are relative to the client working directory.
 *
 *  \returns the first opened document. Required to support the -block flag
 *  for client mode.
 *
 *  \sa IPlugin::remoteArguments()
 */
IDocument *MainWindow::openFiles(const QStringList &fileNames,
                                 ICore::OpenFilesFlags flags,
                                 const QString &workingDirectory)
{
    QList<IDocumentFactory*> documentFactories = PluginManager::getObjects<IDocumentFactory>();
    IDocument *res = 0;

    foreach (const QString &fileName, fileNames) {
        const QDir workingDir(workingDirectory.isEmpty() ? QDir::currentPath() : workingDirectory);
        const QFileInfo fi(workingDir, fileName);
        const QString absoluteFilePath = fi.absoluteFilePath();
        if (IDocumentFactory *documentFactory = findDocumentFactory(documentFactories, fi)) {
            IDocument *document = documentFactory->open(absoluteFilePath);
            if (!document) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else {
                if (!res)
                    res = document;
                if (flags & ICore::SwitchMode)
                    ModeManager::activateMode(Id(Constants::MODE_EDIT));
            }
        } else {
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::CanContainLineAndColumnNumbers)
                emFlags |=  EditorManager::CanContainLineAndColumnNumber;
            IEditor *editor = EditorManager::openEditor(absoluteFilePath, Id(), emFlags);
            if (!editor) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else if (!res) {
                res = editor->document();
            }
        }
    }
    return res;
}

void MainWindow::setFocusToEditor()
{
    EditorManagerPrivate::doEscapeKeyFocusMoveMagic();
}

bool MainWindow::showOptionsDialog(Id page, QWidget *parent)
{
    emit m_coreImpl->optionsDialogRequested();
    if (!parent)
        parent = ICore::dialogParent();
    SettingsDialog *dialog = SettingsDialog::getSettingsDialog(parent, page);
    return dialog->execDialog();
}

void MainWindow::saveAll()
{
    DocumentManager::saveAllModifiedDocumentsSilently();
}

void MainWindow::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QTimer::singleShot(0, this,  &QWidget::close);
}

void MainWindow::openFileWith()
{
    foreach (const QString &fileName, EditorManager::getOpenFileNames()) {
        bool isExternal;
        const Id editorId = EditorManagerPrivate::getOpenWithEditorId(fileName, &isExternal);
        if (!editorId.isValid())
            continue;
        if (isExternal)
            EditorManager::openExternalEditor(fileName, editorId);
        else
            EditorManagerPrivate::openEditorWith(fileName, editorId);
    }
}

IContext *MainWindow::contextObject(QWidget *widget)
{
    return m_contextWidgets.value(widget);
}

void MainWindow::addContextObject(IContext *context)
{
    if (!context)
        return;
    QWidget *widget = context->widget();
    if (m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.insert(widget, context);
}

void MainWindow::removeContextObject(IContext *context)
{
    if (!context)
        return;

    QWidget *widget = context->widget();
    if (!m_contextWidgets.contains(widget))
        return;

    m_contextWidgets.remove(widget);
    if (m_activeContext.removeAll(context) > 0)
        updateContextObject(m_activeContext);
}

void MainWindow::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    QList<IContext *> newContext;
    if (QWidget *p = qApp->focusWidget()) {
        IContext *context = 0;
        while (p) {
            context = m_contextWidgets.value(p);
            if (context)
                newContext.append(context);
            p = p->parentWidget();
        }
    }

    // ignore toplevels that define no context, like popups without parent
    if (!newContext.isEmpty() || qApp->focusWidget() == focusWidget())
        updateContextObject(newContext);
}

void MainWindow::updateContextObject(const QList<IContext *> &context)
{
    emit m_coreImpl->contextAboutToChange(context);
    m_activeContext = context;
    updateContext();
    if (debugMainWindow) {
        qDebug() << "new context objects =" << context;
        foreach (IContext *c, context)
            qDebug() << (c ? c->widget() : 0) << (c ? c->widget()->metaObject()->className() : 0);
    }
}

void MainWindow::aboutToShutdown()
{
    disconnect(qApp, &QApplication::focusChanged, this, &MainWindow::updateFocusWidget);
    m_activeContext.clear();
    hide();
}

static const char settingsGroup[] = "MainWindow";
static const char colorKey[] = "Color";
static const char windowGeometryKey[] = "WindowGeometry";
static const char windowStateKey[] = "WindowState";
static const char modeSelectorVisibleKey[] = "ModeSelectorVisible";

void MainWindow::readSettings()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    if (m_overrideColor.isValid()) {
        StyleHelper::setBaseColor(m_overrideColor);
        // Get adapted base color.
        m_overrideColor = StyleHelper::baseColor();
    } else {
        StyleHelper::setBaseColor(settings->value(QLatin1String(colorKey),
                                  QColor(StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    bool modeSelectorVisible = settings->value(QLatin1String(modeSelectorVisibleKey), true).toBool();
    ModeManager::setModeSelectorVisible(modeSelectorVisible);
    m_toggleModeSelectorAction->setChecked(modeSelectorVisible);

    settings->endGroup();

    EditorManagerPrivate::readSettings();
    //OPENMV-DIFF//
    //m_navigationWidget->restoreSettings(settings);
    //OPENMV-DIFF//
    m_rightPaneWidget->readSettings(settings);
}

void MainWindow::writeSettings()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    if (!(m_overrideColor.isValid() && StyleHelper::baseColor() == m_overrideColor))
        settings->setValue(QLatin1String(colorKey), StyleHelper::requestedBaseColor());

    // On OS X applications usually do not restore their full screen state.
    // To be able to restore the correct non-full screen geometry, we have to put
    // the window out of full screen before saving the geometry.
    // Works around QTBUG-45241
    if (Utils::HostOsInfo::isMacHost() && isFullScreen())
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    settings->setValue(QLatin1String(windowGeometryKey), saveGeometry());
    settings->setValue(QLatin1String(windowStateKey), saveState());
    settings->setValue(QLatin1String(modeSelectorVisibleKey), ModeManager::isModeSelectorVisible());

    settings->endGroup();

    DocumentManager::saveSettings();
    ActionManager::saveSettings();
    EditorManagerPrivate::saveSettings();
    //OPENMV-DIFF//
    //m_navigationWidget->saveSettings(settings);
    //OPENMV-DIFF//
}

void MainWindow::updateAdditionalContexts(const Context &remove, const Context &add,
                                          ICore::ContextPriority priority)
{
    foreach (const Id id, remove) {
        if (!id.isValid())
            continue;
        int index = m_lowPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            m_lowPrioAdditionalContexts.removeAt(index);
        index = m_highPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            m_highPrioAdditionalContexts.removeAt(index);
    }

    foreach (const Id id, add) {
        if (!id.isValid())
            continue;
        Context &cref = (priority == ICore::ContextPriority::High ? m_highPrioAdditionalContexts
                                                                  : m_lowPrioAdditionalContexts);
        if (!cref.contains(id))
            cref.prepend(id);
    }

    updateContext();
}

void MainWindow::updateContext()
{
    Context contexts = m_highPrioAdditionalContexts;

    foreach (IContext *context, m_activeContext)
        contexts.add(context->context());

    contexts.add(m_lowPrioAdditionalContexts);

    Context uniquecontexts;
    for (int i = 0; i < contexts.size(); ++i) {
        const Id id = contexts.at(i);
        if (!uniquecontexts.contains(id))
            uniquecontexts.add(id);
    }

    ActionManager::setContext(uniquecontexts);
    emit m_coreImpl->contextChanged(uniquecontexts);
}

void MainWindow::aboutToShowRecentFiles()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_FILE_RECENTFILES);
    QMenu *menu = aci->menu();
    menu->clear();

    bool hasRecentFiles = false;
    foreach (const DocumentManager::RecentFile &file, DocumentManager::recentFiles()) {
        hasRecentFiles = true;
        QAction *action = menu->addAction(
                    QDir::toNativeSeparators(Utils::withTildeHomePath(file.first)));
        connect(action, &QAction::triggered, this, [file] {
            EditorManager::openEditor(file.first, file.second);
        });
    }
    menu->setEnabled(hasRecentFiles);

    // add the Clear Menu item
    if (hasRecentFiles) {
        menu->addSeparator();
        QAction *action = menu->addAction(QCoreApplication::translate(
                                                     "Core", Constants::TR_CLEAR_MENU));
        connect(action, &QAction::triggered,
                DocumentManager::instance(), &DocumentManager::clearRecentFiles);
    }
}

void MainWindow::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(this);
        connect(m_versionDialog, &QDialog::finished,
                this, &MainWindow::destroyVersionDialog);
    }
    m_versionDialog->show();
}

void MainWindow::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = 0;
    }
}

void MainWindow::aboutPlugins()
{
    PluginDialog dialog(this);
    dialog.exec();
}

QPrinter *MainWindow::printer() const
{
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
    return m_printer;
}

// Display a warning with an additional button to open
// the debugger settings dialog if settingsId is nonempty.

bool MainWindow::showWarningWithOptions(const QString &title,
                                        const QString &text,
                                        const QString &details,
                                        Id settingsId,
                                        QWidget *parent)
{
    if (parent == 0)
        parent = this;
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    if (!details.isEmpty())
        msgBox.setDetailedText(details);
    QAbstractButton *settingsButton = 0;
    if (settingsId.isValid())
        settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton)
        return showOptionsDialog(settingsId);
    return false;
}

void MainWindow::restoreWindowState()
{
    QSettings *settings = PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));
    if (!restoreGeometry(settings->value(QLatin1String(windowGeometryKey)).toByteArray()))
        resize(1008, 700); // size without window decoration
    restoreState(settings->value(QLatin1String(windowStateKey)).toByteArray());
    settings->endGroup();
    //OPENMV-DIFF//
    //show();
    //OPENMV-DIFF//
    if(!m_disableShow) show();
    //OPENMV-DIFF//
    m_statusBarManager->restoreSettings();
}

void MainWindow::newItemDialogFinished()
{
    m_newAction->setEnabled(true);
    // fire signal when the dialog is actually destroyed
    QTimer::singleShot(0, this, &MainWindow::newItemDialogRunningChanged);
}

} // namespace Internal
} // namespace Core
