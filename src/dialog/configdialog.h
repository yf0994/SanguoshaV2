#ifndef _CONFIG_DIALOG_H
#define _CONFIG_DIALOG_H

#include <QDialog>

namespace Ui {
    class ConfigDialog;
}

class QLineEdit;

class ConfigDialog: public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();

//����RoomScene::toggleFullSkin()���޸�Config::UseFullSkin��ֵ��
//Ϊʹ�Ի����fullSkinCheckBox�ؼ���ͬ���޸ĺ�Ľ����
//��Ҫ��ÿ����ʾ�Ի���ʱ����fullSkinCheckBox�ؼ�
protected:
    virtual void showEvent(QShowEvent *event);

private:
    Ui::ConfigDialog *ui;
    void showFont(QLineEdit *lineedit, const QFont &font);

    void setBackground(const QVariant &path);

    static QString m_defaultMusicPath;

private slots:
    void on_setTextEditColorButton_clicked();
    void on_setTextEditFontButton_clicked();
    void on_changeAppFontButton_clicked();
    void on_resetBgMusicButton_clicked();
    void on_browseBgMusicButton_clicked();
    void on_resetBgButton_clicked();
    void on_browseBgButton_clicked();
    void saveConfig();

	void on_resetRecordPathsButton_clicked();
    void on_browseRecordPathsButton_clicked();

    void on_resetGameBgButton_clicked();
    void on_browseGameBgButton_clicked();

signals:
    void bg_changed();
};

#endif
