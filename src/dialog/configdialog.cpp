#include "configdialog.h"
#include "ui_configdialog.h"
#include "settings.h"
#include "audio.h"
#include "SkinBank.h"
#include "roomscene.h"

#include <QFileDialog>
#include <QDesktopServices>
#include <QFontDialog>
#include <QColorDialog>

QString ConfigDialog::m_defaultMusicPath = "audio/system/background.ogg";

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    // tab 1
    QString bg_path = Config.value("BackgroundImage", "image/system/backdrop/default.jpg").toString();
    if (!bg_path.startsWith(":"))
        ui->bgPathLineEdit->setText(bg_path);

    QString game_bg_path = Config.value("GameBackgroundImage", "image/system/backdrop/tableBg.jpg").toString();
    if (!game_bg_path.startsWith(":")) {
        ui->gameBgPathLineEdit->setText(game_bg_path);
    }

    ui->bgMusicPathLineEdit->setText(Config.value("BackgroundMusic", m_defaultMusicPath).toString());

    ui->enableEffectCheckBox->setChecked(Config.EnableEffects);

    ui->enableLastWordCheckBox->setEnabled(Config.EnableEffects);
    ui->enableLastWordCheckBox->setChecked(Config.EnableLastWord);
    connect(ui->enableEffectCheckBox, SIGNAL(toggled(bool)), ui->enableLastWordCheckBox, SLOT(setEnabled(bool)));

    ui->enableBgMusicCheckBox->setChecked(Config.EnableBgMusic);
    ui->randomPlayBGMCheckBox->setChecked(Config.RandomPlayBGM);

    ui->noIndicatorCheckBox->setChecked(Config.value("NoIndicator", false).toBool());
    ui->noEquipAnimCheckBox->setChecked(Config.value("NoEquipAnim", false).toBool());

    ui->bgmVolumeSlider->setValue(100 * Config.BGMVolume);
    ui->effectVolumeSlider->setValue(100 * Config.EffectVolume);

    // tab 2
    ui->neverNullifyMyTrickCheckBox->setChecked(Config.NeverNullifyMyTrick);
    ui->autoTargetCheckBox->setChecked(Config.EnableAutoTarget);
    ui->intellectualSelectionCheckBox->setChecked(Config.EnableIntellectualSelection);
    ui->doubleClickCheckBox->setChecked(Config.EnableDoubleClick);
    ui->superDragCheckBox->setChecked(Config.EnableSuperDrag);
    ui->secondsSpinBox->setValue(Config.AutoCloseCardContainerDelaySeconds);
    ui->bubbleChatBoxDelaySpinBox->setValue(Config.BubbleChatBoxDelaySeconds);
    ui->backgroundChangeCheckBox->setChecked(Config.EnableAutoBackgroundChange);
//�Զ�����¼��
	ui->enableAutoSaveCheckBox->setChecked(Config.EnableAutoSaveRecord);
    ui->networkOnlyCheckBox->setChecked(Config.NetworkOnly);

    ui->networkOnlyCheckBox->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->recordPathsSetupLabel->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->recordPathsSetupLineEdit->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->browseRecordPathsButton->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->resetRecordPathsButton->setEnabled(ui->enableAutoSaveCheckBox->isChecked());

    connect(ui->enableAutoSaveCheckBox, SIGNAL(toggled(bool)), ui->networkOnlyCheckBox, SLOT(setEnabled(bool)));
    connect(ui->enableAutoSaveCheckBox, SIGNAL(toggled(bool)), ui->recordPathsSetupLabel, SLOT(setEnabled(bool)));
    connect(ui->enableAutoSaveCheckBox, SIGNAL(toggled(bool)), ui->recordPathsSetupLineEdit, SLOT(setEnabled(bool)));
    connect(ui->enableAutoSaveCheckBox, SIGNAL(toggled(bool)), ui->browseRecordPathsButton, SLOT(setEnabled(bool)));
    connect(ui->enableAutoSaveCheckBox, SIGNAL(toggled(bool)), ui->resetRecordPathsButton, SLOT(setEnabled(bool)));

    QString record_path = Config.value("RecordSavePaths", "records/").toString();
    if (!record_path.startsWith(":"))
        ui->recordPathsSetupLineEdit->setText(record_path);

    connect(this, SIGNAL(accepted()), this, SLOT(saveConfig()));

    QFont font = Config.AppFont;
    showFont(ui->appFontLineEdit, font);

    font = Config.UIFont;
    showFont(ui->textEditFontLineEdit, font);

    QPalette palette;
    palette.setColor(QPalette::Text, Config.TextEditColor);
    QColor color = Config.TextEditColor;
    int aver = (color.red() + color.green() + color.blue()) / 3;
    palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
    ui->textEditFontLineEdit->setPalette(palette);
}

ConfigDialog::~ConfigDialog() {
    delete ui;
}

void ConfigDialog::showFont(QLineEdit *lineedit, const QFont &font) {
    lineedit->setFont(font);
    lineedit->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
}

void ConfigDialog::on_browseBgButton_clicked() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select a background image"),
                                                    "image/system/backdrop/",
                                                    tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        QString app_path = QApplication::applicationDirPath();
        app_path.replace("\\", "/");
        filename.replace("\\", "/");
        if (filename.startsWith(app_path))
            filename = filename.right(filename.length() - app_path.length() - 1);

        ui->bgPathLineEdit->setText(filename);

        Config.BackgroundImage = filename;
        Config.setValue("BackgroundImage", filename);

        emit bg_changed();
    }
}

void ConfigDialog::on_resetBgButton_clicked() {
    QString filename = "image/system/backdrop/default.jpg";
    ui->bgPathLineEdit->setText(filename);

    Config.BackgroundImage = filename;
    Config.setValue("BackgroundImage", filename);

    emit bg_changed();
}

//����¼��
void ConfigDialog::on_browseRecordPathsButton_clicked() {
    QString paths = QFileDialog::getExistingDirectory(this,
        tr("Select a Record Paths"),
        "records/");

    if (!paths.isEmpty()) {
        ui->recordPathsSetupLineEdit->setText(paths);

        Config.RecordSavePaths = paths;
        Config.setValue("RecordSavePaths", paths);
    }
}

void ConfigDialog::on_resetRecordPathsButton_clicked() {
    ui->recordPathsSetupLineEdit->clear();

    QString paths = "records/";
    ui->recordPathsSetupLineEdit->setText(paths);
    Config.RecordSavePaths = paths;
    Config.setValue("RecordSavePaths", paths);
}

void ConfigDialog::saveConfig() {
    float volume = ui->bgmVolumeSlider->value() / 100.0;

    Audio::setBackgroundMusicVolume(volume);

    Config.BGMVolume = volume;
    Config.setValue("BGMVolume", volume);
    volume = ui->effectVolumeSlider->value() / 100.0;

    Audio::setEffectVolume(volume);

    Config.EffectVolume = volume;
    Config.setValue("EffectVolume", volume);

    bool enabled = ui->enableEffectCheckBox->isChecked();
    Config.EnableEffects = enabled;
    Config.setValue("EnableEffects", enabled);

    enabled = ui->enableLastWordCheckBox->isChecked();
    Config.EnableLastWord = enabled;
    Config.setValue("EnableLastWord", enabled);

    enabled = ui->enableBgMusicCheckBox->isChecked();

    if (!enabled) {
        Audio::stopBackgroundMusic();
    }

    Config.EnableBgMusic = enabled;
    Config.setValue("EnableBgMusic", enabled);

    //����bgMusicPathLineEdit�ؼ�Ϊֻ���ģ�����newMusicPath��ֵ������Ϊ��
    QString newMusicPath = ui->bgMusicPathLineEdit->text();
    QString currentMusicPath = Config.value("BackgroundMusic", m_defaultMusicPath).toString();
    if (newMusicPath != currentMusicPath) {
        Config.setValue("BackgroundMusic", newMusicPath);
        Audio::resetCustomBackgroundMusicFileName();

        if (Config.EnableBgMusic && Audio::isBackgroundMusicPlaying()) {
            Audio::stopBackgroundMusic();
            Audio::playBackgroundMusic(newMusicPath, Config.RandomPlayBGM);
        }
    }
    else {
        if (Config.EnableBgMusic && NULL != RoomSceneInstance
            && RoomSceneInstance->isGameStarted() && !Audio::isBackgroundMusicPlaying()) {
            Audio::playBackgroundMusic(currentMusicPath, Config.RandomPlayBGM);
        }
    }

    //�Ľ��������öԻ��������û�����ȫ������󣬳�����ʵʱ��Ч������Ҫ����������Ч������
    bool useFullSkin = ui->fullSkinCheckBox->isChecked();
    if (Config.value("UseFullSkin", false).toBool() != useFullSkin) {
        if (NULL == RoomSceneInstance) {
            Config.setValue("UseFullSkin", useFullSkin);
        }
        else {
            RoomSceneInstance->toggleFullSkin();
        }
    }

    Config.setValue("NoIndicator", ui->noIndicatorCheckBox->isChecked());
    Config.setValue("NoEquipAnim", ui->noEquipAnimCheckBox->isChecked());

    Config.NeverNullifyMyTrick = ui->neverNullifyMyTrickCheckBox->isChecked();
    Config.setValue("NeverNullifyMyTrick", Config.NeverNullifyMyTrick);

    Config.EnableAutoTarget = ui->autoTargetCheckBox->isChecked();
    Config.setValue("EnableAutoTarget", Config.EnableAutoTarget);

    Config.EnableIntellectualSelection = ui->intellectualSelectionCheckBox->isChecked();
    Config.setValue("EnableIntellectualSelection", Config.EnableIntellectualSelection);

    Config.EnableDoubleClick = ui->doubleClickCheckBox->isChecked();
    Config.setValue("EnableDoubleClick", Config.EnableDoubleClick);

    Config.EnableSuperDrag = ui->superDragCheckBox->isChecked();
    Config.setValue("EnableSuperDrag", Config.EnableSuperDrag);

    Config.AutoCloseCardContainerDelaySeconds = ui->secondsSpinBox->value();
    Config.setValue("AutoCloseCardContainerDelaySeconds", Config.AutoCloseCardContainerDelaySeconds);
    Config.RandomPlayBGM = ui->randomPlayBGMCheckBox->isChecked();
    Config.setValue("RandomPlayBGM", Config.RandomPlayBGM);
    Config.ShowMsgBoxWhenExit = ui->showMsgBoxWhenExitCheckBox->isChecked();
    Config.setValue("ShowMsgBoxWhenExit", Config.ShowMsgBoxWhenExit);
    Config.BubbleChatBoxDelaySeconds = ui->bubbleChatBoxDelaySpinBox->value();
    Config.setValue("BubbleChatBoxDelaySeconds", Config.BubbleChatBoxDelaySeconds);
    Config.EnableAutoBackgroundChange = ui->backgroundChangeCheckBox->isChecked();
    Config.setValue("EnableAutoBackgroundChange", Config.EnableAutoBackgroundChange);
//¼��
	Config.EnableAutoSaveRecord = ui->enableAutoSaveCheckBox->isChecked();
    Config.setValue("EnableAutoSaveRecord", Config.EnableAutoSaveRecord);

    Config.NetworkOnly = ui->networkOnlyCheckBox->isChecked();
    Config.setValue("NetworkOnly", Config.NetworkOnly);
}

void ConfigDialog::on_browseBgMusicButton_clicked() {
    //֧�ֶ�ѡ���������ļ�
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
        tr("Select a background music"),
        "audio/system",
        tr("Audio files (*.wav *.mp3 *.ogg)"));
    QString app_path = QApplication::applicationDirPath();
    app_path.replace("\\", "/");
    int app_path_len = app_path.length();
    foreach (const QString &name, fileNames) {
        const_cast<QString &>(name).replace("\\", "/");
        if (name.startsWith(app_path)) {
            const_cast<QString &>(name) = name.right(name.length() - app_path_len - 1);
        }
    }
    QString filename = fileNames.join(";");
    if (!filename.isEmpty()) {
        ui->bgMusicPathLineEdit->setText(filename);
    }
}

void ConfigDialog::on_resetBgMusicButton_clicked() {
    ui->bgMusicPathLineEdit->setText(m_defaultMusicPath);
}

void ConfigDialog::on_changeAppFontButton_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.AppFont, this);
    if (ok) {
        Config.AppFont = font;
        showFont(ui->appFontLineEdit, font);

        Config.setValue("AppFont", font);
        QApplication::setFont(font);
    }
}

void ConfigDialog::on_setTextEditFontButton_clicked() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.UIFont, this);
    if (ok) {
        Config.UIFont = font;
        showFont(ui->textEditFontLineEdit, font);

        Config.setValue("UIFont", font);
        QApplication::setFont(font, "QTextEdit");
    }
}

void ConfigDialog::on_setTextEditColorButton_clicked() {
    QColor color = QColorDialog::getColor(Config.TextEditColor, this);
    if (color.isValid()) {
        Config.TextEditColor = color;
        Config.setValue("TextEditColor", color);
        QPalette palette;
        palette.setColor(QPalette::Text, color);
        int aver = (color.red() + color.green() + color.blue()) / 3;
        palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
        ui->textEditFontLineEdit->setPalette(palette);
    }
}

void ConfigDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        bool enabled_full = QSanSkinFactory::isEnableFullSkin();
        ui->fullSkinCheckBox->setEnabled(enabled_full);
        ui->fullSkinCheckBox->setChecked(enabled_full && Config.value("UseFullSkin", true).toBool());
        ui->showMsgBoxWhenExitCheckBox->setChecked(Config.ShowMsgBoxWhenExit);
    }
}

void ConfigDialog::on_browseGameBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Select a background image"),
        "image/system/backdrop/",
        tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        QString app_path = QApplication::applicationDirPath();
        app_path.replace("\\", "/");
        filename.replace("\\", "/");
        if (filename.startsWith(app_path)) {
            filename = filename.right(filename.length() - app_path.length() - 1);
        }

        ui->gameBgPathLineEdit->setText(filename);

        Config.GameBackgroundImage = filename;
        Config.setValue("GameBackgroundImage", filename);

        emit bg_changed();
    }
}

void ConfigDialog::on_resetGameBgButton_clicked()
{
    QString filename = "image/system/backdrop/tableBg.jpg";
    ui->gameBgPathLineEdit->setText(filename);

    Config.GameBackgroundImage = filename;
    Config.setValue("GameBackgroundImage", filename);

    emit bg_changed();
}
