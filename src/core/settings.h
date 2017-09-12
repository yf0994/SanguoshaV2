#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "protocol.h"
#include <QSettings>
#include <QFont>
#include <QRectF>
#include <QPixmap>
#include <QBrush>

class Settings: public QSettings {
public:
    Settings();

    void init();

    static QString fromBase64(const QString &base64) {
        QByteArray data = QByteArray::fromBase64(base64.toAscii());
        return QString::fromUtf8(data);
    }

    static QString toBase64(const QString &src) {
        return src.toUtf8().toBase64();
    }

    static const QString &getQSSFileContent();

    QFont BigFont;
    QFont SmallFont;
    QFont TinyFont;
    QFont AppFont;
    QFont UIFont;
    QColor TextEditColor;

    // server side
    QString ServerName;
    int CountDownSeconds;
    int NullificationCountDown;
    bool EnableMinimizeDialog;
    QString GameMode;
    QStringList BanPackages;
    bool RandomSeat;
    bool EnableCheat;
    bool FreeChoose;
    bool ForbidSIMC;
    bool DisableChat;
    bool FreeAssignSelf;
    bool Enable2ndGeneral;
    bool EnableSame;
    bool EnableBasara;
    bool EnableHegemony;
    bool EnableSuperConvert;
    bool EnableHidden;
    int MaxHpScheme;
    int Scheme0Subtraction;
    bool PreventAwakenBelow3;
    QString Address;
    bool EnableAI;
    int AIDelay;
    int OriginAIDelay;
    bool AlterAIDelayAD;
    int AIDelayAD;
    bool SurrenderAtDeath;
    int LuckCardLimitation;
    bool OL;
    bool MUSIC;

    ushort ServerPort;
    bool DisableLua;

    QStringList BossGenerals;
    int BossLevel;
    QStringList BossEndlessSkills;
    QMap<QString, int> BossExpSkills;

    QMap<QString, QString> JianGeDefenseKingdoms;
    QMap<QString, QStringList> JianGeDefenseMachine;
    QMap<QString, QStringList> JianGeDefenseSoul;

    // client side
    QString HostAddress;
    QString UserName;
    QString UserAvatar;
    QStringList HistoryIPs;
    ushort DetectorPort;
    int MaxCards;

    bool EnableHotKey;
    bool NeverNullifyMyTrick;
    bool EnableAutoTarget;
    bool EnableIntellectualSelection;
    bool EnableDoubleClick;
    bool EnableSuperDrag;
    bool EnableAutoBackgroundChange;
	bool EnableAutoSaveRecord;
    bool NetworkOnly;

    int AutoCloseCardContainerDelaySeconds;
    bool RandomPlayBGM;
    bool ShowMsgBoxWhenExit;
    int BubbleChatBoxDelaySeconds;

    int OperationTimeout;
    bool OperationNoLimit;
    bool EnableEffects;
    bool EnableLastWord;
    bool EnableBgMusic;
    float BGMVolume;
    float EffectVolume;

    QString BackgroundImage;
    QString GameBackgroundImage;
	QString RecordSavePaths;

    // consts
    static const int S_SURRENDER_REQUEST_MIN_INTERVAL;
    static const int S_PROGRESS_BAR_UPDATE_INTERVAL;
    static const int S_SERVER_TIMEOUT_GRACIOUS_PERIOD;
    static const int S_JUDGE_ANIMATION_DURATION;
    static const int S_JUDGE_LONG_DELAY;
};

extern Settings Config;

#endif
