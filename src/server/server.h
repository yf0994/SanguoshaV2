#ifndef _SERVER_H
#define _SERVER_H

class Room;
class QGroupBox;
class QLabel;
class QRadioButton;
class QNetworkReply;
class ServerSocket;
class ClientSocket;

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QLayoutItem>
#include <QListWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QMultiHash>
#include <QMutex>

class Package;

class Select3v3GeneralDialog: public QDialog {
    Q_OBJECT

public:
    Select3v3GeneralDialog(QDialog *parent);

private:
    QTabWidget *tab_widget;
    QSet<QString> ex_generals;

    void fillTabWidget();
    void fillListWidget(QListWidget *list, const Package *pack);

private slots:
    void save3v3Generals();
    void toggleCheck();
};

class BanlistDialog: public QDialog {
    Q_OBJECT

public:
    BanlistDialog(QWidget *parent, bool view = false);

private:
    QList<QListWidget *>lists;
    QListWidget *list;
    int item;
    QStringList ban_list;
    QPushButton *add2nd;
    QMap<QString, QStringList> banned_items;
    QLineEdit *card_to_ban;

private slots:
    void addGeneral(const QString &name);
    void add2ndGeneral(const QString &name);
    void addPair(const QString &first, const QString &second);
    void doAdd2ndButton();
    void doAddButton();
    void doRemoveButton();
    void save();
    void saveAll();
    void switchTo(int item);
};

class ServerDialog: public QDialog {
    Q_OBJECT

public:
    ServerDialog(QWidget *parent, const QString &title);

    void ensureEnableAI();
    bool config();

private:
    QWidget *createBasicTab();
    QWidget *createPackageTab();
    QWidget *createAdvancedTab();
    QWidget *createMiscTab();
    QWidget *createZYTab();
    QWidget *createMODELTab();
    QLayout *createButtonLayout();

    QGroupBox *createGameModeBox();
    QGroupBox *create1v1Box();
    QGroupBox *create3v3Box();
    QGroupBox *createXModeBox();

    QLineEdit *server_name_edit;
    QSpinBox *timeout_spinbox;
    QCheckBox *nolimit_checkbox;
    QCheckBox *random_seat_checkbox;
    QCheckBox *enable_cheat_checkbox;
    QCheckBox *free_choose_checkbox;
    QCheckBox *free_assign_checkbox;
    QCheckBox *free_assign_self_checkbox;
    QLabel *pile_swapping_label;
    QSpinBox *pile_swapping_spinbox;
    QCheckBox *without_lordskill_checkbox;
    QCheckBox *sp_convert_checkbox;
    QCheckBox *super_convert_checkbox;
    QCheckBox *hidden_checkbox;
    QSpinBox *maxchoice_spinbox;
	QSpinBox *loyalist_extra_choice_spinbox;
	QSpinBox *renegade_extra_choice_spinbox;
    QLabel *lord_maxchoice_label;
    QSpinBox *lord_maxchoice_spinbox;
    QSpinBox *nonlord_maxchoice_spinbox;
    QCheckBox *forbid_same_ip_checkbox;
    QCheckBox *disable_chat_checkbox;
    QCheckBox *second_general_checkbox;
    QCheckBox *same_checkbox;
    QCheckBox *basara_checkbox;
    QCheckBox *hegemony_checkbox;
    QLabel *hegemony_maxchoice_label;
    QSpinBox *hegemony_maxchoice_spinbox;

    QLabel *max_hp_label;
    QComboBox *max_hp_scheme_ComboBox;
    QLabel *scheme0_subtraction_label;
    QSpinBox *scheme0_subtraction_spinbox;
    QCheckBox *prevent_awaken_below3_checkbox;
    QComboBox *scenario_ComboBox;
    QComboBox *mini_scene_ComboBox;
    QPushButton *mini_scene_button;
    QPushButton *boss_mode_button;
    QLineEdit *address_edit;
    QLineEdit *port_edit;
    QSpinBox *game_start_spinbox;
    QSpinBox *nullification_spinbox;
    QCheckBox *minimize_dialog_checkbox;
    QCheckBox *ai_enable_checkbox;
	QCheckBox *ai_chat_checkbox;
    QSpinBox *ai_delay_spinbox;
    QCheckBox *ai_delay_altered_checkbox;
    QSpinBox *ai_delay_ad_spinbox;
    QCheckBox *surrender_at_death_checkbox;

    QLabel *luck_card_label;
    QSpinBox *luck_card_spinbox;

    QRadioButton *official_3v3_radiobutton;
    QComboBox *official_3v3_ComboBox;
    QComboBox *role_choose_ComboBox;
    QCheckBox *exclude_disaster_checkbox;
    QComboBox *official_1v1_ComboBox;
    QCheckBox *kof_using_extension_checkbox;
    QCheckBox *kof_card_extension_checkbox;
    QComboBox *role_choose_xmode_ComboBox;
    QCheckBox *disable_lua_checkbox;

    QButtonGroup *extension_group;
    QButtonGroup *mode_group;

    QPushButton *detect_button;
	
	
    QCheckBox *yoka;
    QCheckBox *GGGG;
    QCheckBox *ZDYJ;
    QCheckBox *fenji_down;
    QCheckBox *jiewei_down;
    QCheckBox *guixin_down;
    QCheckBox *yujin_down;
    QCheckBox *luoying_down;
    QCheckBox *xunxun_down;
    QCheckBox *shengxi_down;
    QCheckBox *heg_skill;
    QCheckBox *hidden_ai;
    QCheckBox *music;
    QCheckBox *sunce_down;
    QCheckBox *wu_down;
    QCheckBox *simayi_down;
    QCheckBox *simayi_duang;
    QCheckBox *xingxue_down;
    QCheckBox *xuanfeng_down;
    QCheckBox *nos_leiji_down;
    QCheckBox *juxiang_down;
    QCheckBox *naman_down;
    QCheckBox *yongjue_down;
    QCheckBox *paoxiao_down;
    QCheckBox *chenqing_down;
    QCheckBox *fengpo_down;
	
	
    QCheckBox *BEL;
    QCheckBox *sp_face;
    QCheckBox *white_cloth;
    QCheckBox *yummy_food;
    QCheckBox *lucky_pile;
    QLabel *yummy_food_maxhp_label;
    QSpinBox *yummy_food_maxhp_spinbox;
    QLabel *yummy_food_slash_label;
    QSpinBox *yummy_food_slash_spinbox;
    QLabel *yummy_food_analeptic_label;
    QSpinBox *yummy_food_analeptic_spinbox;
    QLabel *yummy_food_draw_label;
    QSpinBox *yummy_food_draw_spinbox;
    QLabel *lucky_pile_min_label;
    QSpinBox *lucky_pile_min_spinbox;
    QLabel *lucky_pile_max_label;
    QSpinBox *lucky_pile_max_spinbox;
    QLabel *lucky_pile_recover_label;
    QSpinBox *lucky_pile_recover_spinbox;
    QLabel *lucky_pile_draw_label;
    QSpinBox *lucky_pile_draw_spinbox;
    QLabel *lucky_pile_zhiheng_label;
    QSpinBox *lucky_pile_zhiheng_spinbox;
    QLabel *lucky_pile_jushou_label;
    QSpinBox *lucky_pile_jushou_spinbox;
    QLabel *sp_face_zhaoyun_label;
    QSpinBox *sp_face_zhaoyun_spinbox;
    QLabel *sp_face_zhouyu_label;
    QSpinBox *sp_face_zhouyu_spinbox;
    QLabel *sp_face_zhangyi_label;
    QSpinBox *sp_face_zhangyi_spinbox;
    QLabel *fuck_god_label;
    QSpinBox *fuck_god_spinbox;
    QCheckBox *exchange_god;

private slots:
    void setMaxHpSchemeBox();

    void onOkButtonClicked();
    void onDetectButtonClicked();
    void select3v3Generals();
    void edit1v1Banlist();
    void updateButtonEnablility(QAbstractButton *button);

    void doCustomAssign();
    void doBossModeCustomAssign();
    void setMiniCheckBox();

    void replyFinished(QNetworkReply *reply);
};

class BossModeCustomAssignDialog: public QDialog {
    Q_OBJECT

public:
    BossModeCustomAssignDialog(QWidget *parent);
    void config();

private:
    QCheckBox *diff_revive_checkBox;
    QCheckBox *diff_recover_checkBox;
    QCheckBox *diff_draw_checkBox;
    QCheckBox *diff_reward_checkBox;
    QCheckBox *diff_incMaxHp_checkBox;
    QCheckBox *diff_decMaxHp_checkBox;

    QCheckBox *experience_checkBox;
    QCheckBox *optional_boss_checkBox;
    QCheckBox *endless_checkBox;

    QLabel *turn_limit_label;
    QSpinBox *turn_limit_spinBox;
};

class Scenario;
class ServerPlayer;

class Server : public QObject {
    Q_OBJECT

public:
    explicit Server(QObject *parent, bool consoleStart = true);

    void broadcast(const QString &msg);
    bool listen();
    void daemonize();
    Room *createNewRoom();
    void signupPlayer(ServerPlayer *player);

    void requestDeleteSelf();

private:
    bool hasActiveRoom() const;
    void deleteSelf();
    Room *getAvailableRoom() const;

private:
    ServerSocket *server;
    QSet<Room *> rooms;
    QHash<QString, ServerPlayer *> players;
    QMultiHash<QString, ClientSocket *> addresses;
    QMultiHash<QString, QString> name2objname;

    bool m_requestDeleteSelf;
    mutable QMutex m_mutex;

private slots:
    void processNewConnection(ClientSocket *socket);
    void processRequest(const QString &request);

    void cleanup();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
