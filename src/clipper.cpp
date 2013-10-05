#include "clipper.h"

Clipper::Clipper(QWidget *parent) :
    QWidget(parent), ui(new Ui::Clipper)
{
    ui->setupUi(this);
    appIcon.addFile(":/icon.ico");

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width()-this->width()) / 2;
    int y = (screenGeometry.height()-this->height()) / 2;
    this->move(x, y);
    this->setWindowIcon(appIcon);

    this->hide();

    QString settingsFilePath = QDir::homePath()+"/.clipper";
#ifdef Q_OS_LINUX
    settings = new QSettings(settingsFilePath, QSettings::NativeFormat);
#endif
#ifdef Q_OS_WIN
    settings = new QSettings(settingsFilePath, QSettings::IniFormat);
#endif
    multicopyEnabled = false;
    clipboard = QApplication::clipboard();
    api = new ClipperAPIs();
    tnyczOptionsWindow = new TnyczOptions;
    hotkeyEditWindow = new QHotkeyEdit;

    QMenu *trayMenu = new QMenu();
    trayMenu->addAction("Quit", qApp, SLOT(quit()));

    tray = new QSystemTrayIcon();


    tray->setIcon(appIcon);
    tray->setContextMenu(trayMenu);
    tray->show();

    shortcutButtons.addButton(ui->shortenLinkButton, 1);
    shortcutButtons.addButton(ui->publishPasteButton, 2);
    shortcutButtons.addButton(ui->makeScreenshotButton, 3);
    shortcutButtons.addButton(ui->makeQRCodeButton, 4);
    shortcutButtons.addButton(ui->toggleMulticopyButton, 5);
    shortcutButtons.addButton(ui->makePartialScreenshotButton, 6);

    reloadSettings();

    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
    connect(ui->clearButton, &QPushButton::clicked, [=]() { ui->listWidget->clear(); });
    connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(historyItemToClipboard(QListWidgetItem*)));
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
            SLOT(onTrayIconClicked(QSystemTrayIcon::ActivationReason)));
    connect(api, SIGNAL(linkReady(QString)), this, SLOT(linkToClipboard(QString)));
    connect(&shortcutButtons, SIGNAL(buttonClicked(int)), this, SLOT(onChangeHotkeyButtonClicked(int)));
    connect(ui->clearButton_2, &QPushButton::clicked, [=]() { ui->textEdit->clear(); });
    connect(ui->copyAllButton, &QPushButton::clicked, [=]()
    {
        if (!ui->textEdit->document()->isEmpty())
        {
            clipboard->setText(ui->textEdit->toPlainText());
            tray->showMessage("", "Data is copied into clipboard", QSystemTrayIcon::Information, 2000);
        }
    });
    connect(ui->changeScreenshotPath, &QPushButton::clicked, [=]()
    {
        QString path = QFileDialog::getExistingDirectory(this,
                                                         tr("Open Directory"),
                                                         "",
                                                         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
        {
            screenshotPath = path;
            ui->screenshotPathEdit->setText(screenshotPath);
        }
    });
}

Clipper::~Clipper()
{
    disconnect(this);
}

void Clipper::linkShorten()
{
#ifdef Q_OS_LINUX
    if (!clipboard->text(QClipboard::Selection).isEmpty())
        api->googleLinkShorten(clipboard->text(QClipboard::Selection));
#endif
#ifdef Q_OS_WIN
    if (!clipboard->text().isEmpty())
        api->googleLinkShorten(clipboard->text());
#endif
    else
        tray->showMessage("", "Selection is empty", QSystemTrayIcon::Information, 3000);
}

void Clipper::tnyczPublish()
{
    if (tnyczOptionsWindow->isVisible())
        return;

#ifdef Q_OS_LINUX
    if (!clipboard->text(QClipboard::Selection).isEmpty())
#endif
#ifdef Q_OS_WIN
    if (!clipboard->text().isEmpty())
#endif
    {
        tnyczOptionsWindow = new TnyczOptions();
        tnyczOptionsWindow->show();
#ifdef Q_OS_LINUX
        tnyczOptionsWindow->setPasteText(clipboard->text(QClipboard::Selection));
#endif
#ifdef Q_OS_WIN
        tnyczOptionsWindow->setPasteText(clipboard->text());
#endif
        connect(tnyczOptionsWindow, SIGNAL(optionsReady(QString,QString,QString,bool,bool,bool)), api,
                SLOT(tnyczPublish(QString,QString,QString,bool,bool,bool)));
    }
    else
        tray->showMessage("", "Selection is empty", QSystemTrayIcon::Information, 3000);
}

void Clipper::linkToClipboard(QString link)
{
    clipboard->setText(link);
    tray->showMessage("", "Link is copied to your clipboard.", QSystemTrayIcon::Information, 3000);
}

void Clipper::updateHistory()
{
    QString newText = clipboard->text();
    if (ui->listWidget->findItems(newText, Qt::MatchExactly).isEmpty())
    {
        ui->listWidget->addItem(newText);
        ui->listWidget->itemAt(ui->listWidget->count(), 0)->setToolTip(QTime::currentTime().toString());
    }
}

void Clipper::onChangeHotkeyButtonClicked(int id)
{
    if (hotkeyEditWindow->isVisible())
        return;

    hotkeyEditWindow = new QHotkeyEdit();
    hotkeyEditWindow->show();
    button_id = id;
    connect(hotkeyEditWindow, SIGNAL(hotkeyReady(QString)), this, SLOT(changeHotkey(QString)));
}

void Clipper::changeHotkey(QString hotkey)
{
    if (!shortcuts.values().contains(hotkey) || hotkey == shortcutButtons.button(button_id)->text())
    {
        shortcutButtons.button(button_id)->setText(hotkey);
        button_id = 0;
    }
    else
        QMessageBox::critical(this, "", "This hotkey is already in use. Try another.",
                              QMessageBox::Ok);
}

void Clipper::saveSettings()
{
    settings->beginGroup("Hotkeys");
    settings->setValue("ShortenLink", ui->shortenLinkButton->text());
    settings->setValue("PastePublish", ui->publishPasteButton->text());
    settings->setValue("Screenshot", ui->makeScreenshotButton->text());
    settings->setValue("PartialScreenshot", ui->makePartialScreenshotButton->text());
    settings->setValue("QRCode", ui->makeQRCodeButton->text());
    settings->setValue("Multicopy", ui->toggleMulticopyButton->text());
    settings->endGroup();

    settings->beginGroup("General");
    settings->setValue("LinkShortening", ui->linkShortening->isChecked());
    settings->setValue("PastePublishing", ui->pastePublishing->isChecked());
    settings->setValue("ScreenshotMaking", ui->screenshotMaking->isChecked());
    settings->setValue("PartialScreenshotMaking", ui->partialScreenshotMaking->isChecked());
    settings->setValue("QRCodeMaking", ui->QRCode->isChecked());
    settings->setValue("Multicopy", ui->Multicopy->isChecked());
    settings->setValue("SplitIntoLines", ui->splitMulticopy->isChecked());
    settings->setValue("KeepHistory", ui->keepHistory->isChecked());
    settings->setValue("SaveScreenshots", ui->saveScreenshots->isChecked());
    settings->setValue("ScreenshotDir", ui->screenshotPathEdit->text());
    settings->endGroup();
    reloadSettings();
}

void Clipper::onTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        if (isVisible())
        {
            hide();
        }
        else
        {
            show();
            updateSettingsGUI();
        }
    }
}

void Clipper::makeFullScreenshot()
{
    QPixmap screenshot = grabScreen();
    QByteArray screenshotData = pixmapToByteArray(screenshot);
    tray->showMessage("", "Uploading screenshot, please wait..", QSystemTrayIcon::Information, 1000);
    api->imageshackUpload(screenshotData);
    if (general["SaveScreenshots"])
        saveScreenshotToFile(screenshot);
}

void Clipper::makePartialScreenshot()
{
    QPixmap screenshot = grabScreen();
    ImageSelectWidget imageSelectDialog(&screenshot);
    imageSelectDialog.setWindowState(Qt::WindowFullScreen);
    if (!imageSelectDialog.exec())
        return;
    else
    {
        QByteArray screenshotData = pixmapToByteArray(screenshot);
        api->imageshackUpload(screenshotData);
        tray->showMessage("", "Uploading screenshot, please wait..", QSystemTrayIcon::Information, 1000);
        if (general["SaveScreenshots"])
            saveScreenshotToFile(screenshot);
    }
}

void Clipper::saveScreenshotToFile(QPixmap screenshot)
{
    QFile file(screenshotPath+"/"+getCurrentDatetime()+".png");
    if (!file.open(QIODevice::WriteOnly))
        tray->showMessage("Error saving screenshot", file.errorString(), QSystemTrayIcon::Warning, 1000);
    screenshot.save(&file, "PNG");
    file.close();
}

void Clipper::makeQRCode()
{
#ifdef Q_OS_LINUX
    if (!clipboard->text(QClipboard::Selection).isEmpty())
#endif
#ifdef Q_OS_WIN
    if (!clipboard->text().isEmpty())
#endif
    {
#ifdef Q_OS_LINUX
        api->textToQRCode(clipboard->text(QClipboard::Selection));
#endif
#ifdef Q_OS_WIN
        api->textToQRCode(clipboard->text());
#endif
        connect(api, SIGNAL(qrCodeReady(QPixmap*)), this, SLOT(QRCodeReady(QPixmap*)), Qt::UniqueConnection);
    }
    else
        tray->showMessage("", "Selection is empty", QSystemTrayIcon::Information, 3000);
}

void Clipper::QRCodeReady(QPixmap *qrCode)
{
    QLabel *qrCodeWindow = new QLabel();
    qrCodeWindow->setPixmap(*qrCode);
    qrCodeWindow->setAlignment(Qt::AlignCenter);
    qrCodeWindow->setWindowIcon(appIcon);
    qrCodeWindow->setWindowFlags(Qt::WindowCloseButtonHint);
    qrCodeWindow->setAttribute(Qt::WA_DeleteOnClose);
    qrCodeWindow->show();
}

void Clipper::reloadSettings()
{
    if (!hotkeysInit)
    {

        clipboard->disconnect(this);

        if (general["LinkShortening"])
        {
            linkShortenShortcut->disconnect(this);
            delete linkShortenShortcut;
        }
        if (general["PastePublishing"])
        {
            tnyczPublishShortcut->disconnect(this);
            delete tnyczPublishShortcut;
        }
        if (general["ScreenshotMaking"])
        {
            screenshotShortcut->disconnect(this);
            delete screenshotShortcut;
        }
        if (general["PartialScreenshotMaking"])
        {
            partialScreenshotShortcut->disconnect(this);
            delete partialScreenshotShortcut;
        }
        if (general["QRCodeMaking"])
        {
            toggleMulticopyShortcut->disconnect(this);
            delete makeQRCodeShortcut;
        }
        if (general["Multicopy"])
        {
            toggleMulticopyShortcut->disconnect(this);
            delete toggleMulticopyShortcut;
        }
    }
    else
        hotkeysInit = false;

    shortcuts["ShortenLink"] = settings->value("Hotkeys/ShortenLink", "Shift+F6").toString();
    shortcuts["PastePublish"] = settings->value("Hotkeys/PastePublish", "Shift+F7").toString();
    shortcuts["Screenshot"] = settings->value("Hotkeys/Screenshot", "Shift+F10").toString();
    shortcuts["PartialScreenshot"] = settings->value("Hotkeys/PartialScreenshot", "Shift+F11").toString();
    shortcuts["QRCode"] = settings->value("Hotkeys/QRCode", "Shift+F9").toString();
    shortcuts["Multicopy"] = settings->value("Hotkeys/Multicopy", "Ctrl+Shift+C").toString();

    general["LinkShortening"] = settings->value("General/LinkShortening", "1").toBool();
    general["PastePublishing"] = settings->value("General/PastePublishing", "1").toBool();
    general["ScreenshotMaking"] = settings->value("General/ScreenshotMaking", "1").toBool();
    general["PartialScreenshotMaking"] = settings->value("General/PartialScreenshotMaking", "1").toBool();
    general["QRCodeMaking"] = settings->value("General/QRCodeMaking", "1").toBool();
    general["Multicopy"] = settings->value("General/Multicopy", "1").toBool();
    general["SplitIntoLines"] = settings->value("General/SplitIntoLines", "1").toBool();
    general["KeepHistory"] = settings->value("General/KeepHistory", "1").toBool();
    general["SaveScreenshots"] = settings->value("General/SaveScreenshots", "1").toBool();
    screenshotPath = settings->value("General/ScreenshotDir", QDir::homePath()+"/Screenshots").toString();
    if (general["LinkShortening"])
    {
        linkShortenShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["ShortenLink"]));
        connect(linkShortenShortcut, SIGNAL(activated()), this, SLOT(linkShorten()));
    }
    if (general["PastePublishing"])
    {
        tnyczPublishShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["PastePublish"]));
        connect(tnyczPublishShortcut, SIGNAL(activated()), this, SLOT(tnyczPublish()));
    }
    if (general["ScreenshotMaking"])
    {
        screenshotShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["Screenshot"]));
        connect(screenshotShortcut, SIGNAL(activated()), this, SLOT(makeFullScreenshot()));
    }
    if (general["PartialScreenshotMaking"])
    {
        partialScreenshotShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["PartialScreenshot"]));
        connect(partialScreenshotShortcut, SIGNAL(activated()), this, SLOT(makePartialScreenshot()));
    }
    if (general["QRCodeMaking"])
    {
        makeQRCodeShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["QRCode"]));
        connect(makeQRCodeShortcut, SIGNAL(activated()), this, SLOT(makeQRCode()));
    }
    if (general["Multicopy"])
    {
        toggleMulticopyShortcut = new QxtGlobalShortcut(QKeySequence(shortcuts["Multicopy"]));
        connect(toggleMulticopyShortcut, SIGNAL(activated()), this, SLOT(toggleMulticopy()));
    }
    if (general["KeepHistory"])
    {
        connect(clipboard, SIGNAL(dataChanged()), this, SLOT(updateHistory()));
    }

    updateSettingsGUI();
}

void Clipper::toggleMulticopy()
{
    if (!multicopyEnabled)
    {
        multicopyEnabled = true;
        ui->textEdit->document()->clear();
        connect(clipboard, SIGNAL(dataChanged()), this, SLOT(updateMulticopyStore()));
        tray->showMessage("Multicopy mode: ON", "Copy/Cut to append", QSystemTrayIcon::Information, 2000);
    }
    else
    {
        multicopyEnabled = false;
        disconnect(clipboard, SIGNAL(dataChanged()), this, SLOT(updateMulticopyStore()));
        clipboard->setText(ui->textEdit->toPlainText());
        tray->showMessage("Multicopy mode: OFF", "Data is copied into clipboard and available in Main Window", QSystemTrayIcon::Information, 3000);
    }
}

void Clipper::updateMulticopyStore()
{
    QTextDocument *doc = ui->textEdit->document();
    if (general["SplitIntoLines"])
        doc->setPlainText(doc->toPlainText() + clipboard->text() + "\n");
    else
        doc->setPlainText(doc->toPlainText() + " " + clipboard->text());
    tray->showMessage("", "Data appended", QSystemTrayIcon::Information, 3000);
}

void Clipper::updateSettingsGUI()
{
    ui->shortenLinkButton->setText(shortcuts["ShortenLink"]);
    ui->publishPasteButton->setText(shortcuts["PastePublish"]);
    ui->makeScreenshotButton->setText(shortcuts["Screenshot"]);
    ui->makePartialScreenshotButton->setText(shortcuts["PartialScreenshot"]);
    ui->makeQRCodeButton->setText(shortcuts["QRCode"]);
    ui->toggleMulticopyButton->setText(shortcuts["Multicopy"]);

    ui->linkShortening->setChecked(general["LinkShortening"]);
    ui->pastePublishing->setChecked(general["PastePublishing"]);
    ui->screenshotMaking->setChecked(general["ScreenshotMaking"]);
    ui->partialScreenshotMaking->setChecked(general["PartialScreenshotMaking"]);
    ui->QRCode->setChecked(general["QRCodeMaking"]);
    ui->Multicopy->setChecked(general["Multicopy"]);
    ui->splitMulticopy->setChecked(general["SplitIntoLines"]);
    ui->keepHistory->setChecked(general["KeepHistory"]);
    ui->saveScreenshots->setChecked(general["SaveScreenshots"]);
    ui->screenshotPathEdit->setText(screenshotPath);
}

void Clipper::historyItemToClipboard(QListWidgetItem *item)
{
    clipboard->setText(item->text());
    tray->showMessage("", "History item is copied into clipboard.", QSystemTrayIcon::Information, 3000);
}

void Clipper::closeEvent(QCloseEvent *event)
{
    hotkeyEditWindow->close();
}
