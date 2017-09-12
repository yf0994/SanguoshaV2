#include "client.h"
#include "settings.h"
#include "engine.h"
#include "standard.h"
#include "choosegeneraldialog.h"
#include "clientsocket.h"
#include "replayer.h"
#include "recorder.h"
#include "jsonutils.h"
#include "SkinBank.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextDocument>
#include <QTextCursor>

using namespace std;
using namespace QSanProtocol;
using namespace QSanProtocol::Utils;

Client *ClientInstance = NULL;

Client::Client(QObject *parent, Replayer *replayerPtr/* = 0*/)
    : QObject(parent), m_isDiscardActionRefusable(true),
      m_status(NotActive), alive_count(1), swap_pile(0), m_bossLevel(0),
      _m_roomState(true), replayer(replayerPtr)
{
    ClientInstance = this;
    m_isGameOver = false;

    m_callbacks[S_COMMAND_CHECK_VERSION] = &Client::checkVersion;
    m_callbacks[S_COMMAND_SETUP] = &Client::setup;
    m_callbacks[S_COMMAND_NETWORK_DELAY_TEST] = &Client::networkDelayTest;
    m_callbacks[S_COMMAND_ADD_PLAYER] = &Client::addPlayer;
    m_callbacks[S_COMMAND_REMOVE_PLAYER] = &Client::removePlayer;
    m_callbacks[S_COMMAND_START_IN_X_SECONDS] = &Client::startInXs;
    m_callbacks[S_COMMAND_ARRANGE_SEATS] = &Client::arrangeSeats;
    m_callbacks[S_COMMAND_WARN] = &Client::warn;
    m_callbacks[S_COMMAND_SPEAK] = &Client::speak;

    m_callbacks[S_COMMAND_GAME_START] = &Client::startGame;
    m_callbacks[S_COMMAND_GAME_OVER] = &Client::gameOver;

    m_callbacks[S_COMMAND_CHANGE_HP] = &Client::hpChange;
    m_callbacks[S_COMMAND_CHANGE_MAXHP] = &Client::maxhpChange;
    m_callbacks[S_COMMAND_KILL_PLAYER] = &Client::killPlayer;
    m_callbacks[S_COMMAND_REVIVE_PLAYER] = &Client::revivePlayer;
    m_callbacks[S_COMMAND_SHOW_CARD] = &Client::showCard;
    m_callbacks[S_COMMAND_UPDATE_CARD] = &Client::updateCard;
    m_callbacks[S_COMMAND_SET_MARK] = &Client::setMark;
    m_callbacks[S_COMMAND_LOG_SKILL] = &Client::log;
    m_callbacks[S_COMMAND_ATTACH_SKILL] = &Client::attachSkill;
    m_callbacks[S_COMMAND_MOVE_FOCUS] = &Client::moveFocus;
    m_callbacks[S_COMMAND_SET_EMOTION] = &Client::setEmotion;
    m_callbacks[S_COMMAND_INVOKE_SKILL] = &Client::skillInvoked;
    m_callbacks[S_COMMAND_SHOW_ALL_CARDS] = &Client::showAllCards;
    m_callbacks[S_COMMAND_SKILL_GONGXIN] = &Client::askForGongxin;
    m_callbacks[S_COMMAND_LOG_EVENT] = &Client::handleGameEvent;
    m_callbacks[S_COMMAND_ADD_HISTORY] = &Client::addHistory;
    m_callbacks[S_COMMAND_ANIMATE] = &Client::animate;
    m_callbacks[S_COMMAND_FIXED_DISTANCE] = &Client::setFixedDistance;
    m_callbacks[S_COMMAND_ATTACK_RANGE] = &Client::setAttackRangePair;
    m_callbacks[S_COMMAND_CARD_LIMITATION] = &Client::cardLimitation;
    m_callbacks[S_COMMAND_NULLIFICATION_ASKED] = &Client::setNullification;
    m_callbacks[S_COMMAND_ENABLE_SURRENDER] = &Client::enableSurrender;
    m_callbacks[S_COMMAND_EXCHANGE_KNOWN_CARDS] = &Client::exchangeKnownCards;
    m_callbacks[S_COMMAND_SET_KNOWN_CARDS] = &Client::setKnownCards;
    m_callbacks[S_COMMAND_VIEW_GENERALS] = &Client::viewGenerals;

    m_callbacks[S_COMMAND_UPDATE_BOSS_LEVEL] = &Client::updateBossLevel;
    m_callbacks[S_COMMAND_UPDATE_STATE_ITEM] = &Client::updateStateItem;
    m_callbacks[S_COMMAND_AVAILABLE_CARDS] = &Client::setAvailableCards;

    m_callbacks[S_COMMAND_GET_CARD] = &Client::getCards;
    m_callbacks[S_COMMAND_LOSE_CARD] = &Client::loseCards;
    m_callbacks[S_COMMAND_SET_PROPERTY] = &Client::updateProperty;

    m_callbacks[S_COMMAND_REMOVE_ANJIANG_NAMES] = &Client::removeAnjiangNames;

    m_callbacks[S_COMMAND_RESET_PILE] = &Client::resetPiles;
    m_callbacks[S_COMMAND_UPDATE_PILE] = &Client::setPileNumber;
    m_callbacks[S_COMMAND_SYNCHRONIZE_DISCARD_PILE] = &Client::synchronizeDiscardPile;
    m_callbacks[S_COMMAND_CARD_FLAG] = &Client::setCardFlag;

    // interactive methods
    m_interactions[S_COMMAND_CHOOSE_GENERAL] = &Client::askForGeneral;
    m_interactions[S_COMMAND_CHOOSE_PLAYER] = &Client::askForPlayerChosen;
    m_interactions[S_COMMAND_CHOOSE_ROLE] = &Client::askForAssign;
    m_interactions[S_COMMAND_CHOOSE_DIRECTION] = &Client::askForDirection;
    m_interactions[S_COMMAND_EXCHANGE_CARD] = &Client::askForExchange;
    m_interactions[S_COMMAND_ASK_PEACH] = &Client::askForSinglePeach;
    m_interactions[S_COMMAND_SKILL_GUANXING] = &Client::askForGuanxing;
    m_interactions[S_COMMAND_SKILL_GONGXIN] = &Client::askForGongxin;
    m_interactions[S_COMMAND_SKILL_YIJI] = &Client::askForYiji;
    m_interactions[S_COMMAND_PLAY_CARD] = &Client::activate;
    m_interactions[S_COMMAND_DISCARD_CARD] = &Client::askForDiscard;
    m_interactions[S_COMMAND_CHOOSE_SUIT] = &Client::askForSuit;
    m_interactions[S_COMMAND_CHOOSE_KINGDOM] = &Client::askForKingdom;
    m_interactions[S_COMMAND_RESPONSE_CARD] = &Client::askForCardOrUseCard;
    m_interactions[S_COMMAND_INVOKE_SKILL] = &Client::askForSkillInvoke;
    m_interactions[S_COMMAND_MULTIPLE_CHOICE] = &Client::askForChoice;
    m_interactions[S_COMMAND_NULLIFICATION] = &Client::askForNullification;
    m_interactions[S_COMMAND_SHOW_CARD] = &Client::askForCardShow;
    m_interactions[S_COMMAND_AMAZING_GRACE] = &Client::askForAG;
    m_interactions[S_COMMAND_PINDIAN] = &Client::askForPindian;
    m_interactions[S_COMMAND_CHOOSE_CARD] = &Client::askForCardChosen;
    m_interactions[S_COMMAND_CHOOSE_ORDER] = &Client::askForOrder;
    m_interactions[S_COMMAND_CHOOSE_ROLE_3V3] = &Client::askForRole3v3;
    m_interactions[S_COMMAND_SURRENDER] = &Client::askForSurrender;
    m_interactions[S_COMMAND_LUCK_CARD] = &Client::askForLuckCard;

    m_callbacks[S_COMMAND_FILL_AMAZING_GRACE] = &Client::fillAG;
    m_callbacks[S_COMMAND_TAKE_AMAZING_GRACE] = &Client::takeAG;
    m_callbacks[S_COMMAND_CLEAR_AMAZING_GRACE] = &Client::clearAG;

    // 3v3 mode & 1v1 mode
    m_interactions[S_COMMAND_ASK_GENERAL] = &Client::askForGeneral3v3;
    m_interactions[S_COMMAND_ARRANGE_GENERAL] = &Client::startArrange;

    m_callbacks[S_COMMAND_FILL_GENERAL] = &Client::fillGenerals;
    m_callbacks[S_COMMAND_TAKE_GENERAL] = &Client::takeGeneral;
    m_callbacks[S_COMMAND_RECOVER_GENERAL] = &Client::recoverGeneral;
    m_callbacks[S_COMMAND_REVEAL_GENERAL] = &Client::revealGeneral;
    m_callbacks[S_COMMAND_UPDATE_SKILL] = &Client::updateSkill;

    m_noNullificationThisTime = false;
    m_noNullificationTrickName = ".";
    m_respondingUseFixedTarget = NULL;

    Self = new ClientPlayer(this);
    Self->setScreenName(Config.UserName);
    Self->setProperty("avatar", Config.UserAvatar);
    connect(Self, SIGNAL(phase_changed()), this, SLOT(alertFocus()));

    m_players << Self;

    if (NULL != replayer) {
        socket = NULL;
        recorder = NULL;

        replayer->setParent(this);
        connect(replayer, SIGNAL(command_parsed(const QString &)), this, SLOT(processServerPacket(const QString &)));
    }
    else {
        socket = new ClientSocket(this);
        recorder = new Recorder(this);

        connect(socket, SIGNAL(message_received(const QString &)), recorder, SLOT(recordLine(const QString &)));
        connect(socket, SIGNAL(message_received(const QString &)), this, SLOT(processServerPacket(const QString &)));

        connect(socket, SIGNAL(error_occurred(int, const QString &)),
            this, SLOT(emitErrorMessage(int, const QString &)));

        QString address;
        ushort port;
        getHostAddressAndPort(address, port);
        socket->connectToHost(address, port);
    }

    lines_doc = new QTextDocument(this);

    prompt_doc = new QTextDocument(this);
    prompt_doc->setTextWidth(350);
    prompt_doc->setDefaultFont(QFont("SimHei"));
}

void Client::updateCard(const Json::Value &val)
{
    if (val.isInt()) {
        // reset card
        int cardId = val.asInt();
        Card *card = _m_roomState.getCard(cardId);
        if (card->isModified()) {
            _m_roomState.resetCard(cardId);
        }
    }
    else {
        // update card
        Q_ASSERT(val.size() >= 5);
        int cardId = val[0].asInt();
        Card::Suit suit = (Card::Suit)val[1].asInt();
        int number = val[2].asInt();
        QString cardName = val[3].asCString();
        QString skillName = val[4].asCString();
        QString objectName = val[5].asCString();
        QStringList flags;
        tryParse(val[6], flags);

        Card *card = Sanguosha->cloneCard(cardName, suit, number, flags);
        card->setId(cardId);
        card->setSkillName(skillName);
        card->setObjectName(objectName);
        WrappedCard *wrapped = Sanguosha->getWrappedCard(cardId);
        if (NULL != wrapped) {
            wrapped->copyEverythingFrom(card);
        }
    }
}

void Client::signup()
{
    if (replayer) {
        replayer->start();
    }
    else {
        QString base64 = Settings::toBase64(Config.UserName);

        Json::Value arg(Json::arrayValue);
        arg[0] = Config.value("EnableReconnection", false).toBool();
        arg[1] = toJsonString(base64);
        arg[2] = toJsonString(Config.UserAvatar);

        notifyServer(S_COMMAND_SIGN_UP, arg);
    }
}

void Client::networkDelayTest(const Json::Value &)
{
    notifyServer(S_COMMAND_NETWORK_DELAY_TEST);
}

void Client::replyToServer(CommandType command, const Json::Value &arg)
{
    if (socket) {
        QSanGeneralPacket packet(S_SRC_CLIENT | S_TYPE_REPLY | S_DEST_ROOM, command);
        packet.m_localSerial = _m_lastServerSerial;
        packet.setMessageBody(arg);
        socket->send(toQString(packet.toString()));
    }
}

void Client::handleGameEvent(const Json::Value &arg)
{
    emit event_received(arg);
}

void Client::requestToServer(CommandType command, const Json::Value &arg)
{
    if (socket) {
        QSanGeneralPacket packet(S_SRC_CLIENT | S_TYPE_REQUEST | S_DEST_ROOM, command);
        packet.setMessageBody(arg);
        socket->send(toQString(packet.toString()));
    }
}

void Client::notifyServer(CommandType command, const Json::Value &arg)
{
    if (socket) {
        QSanGeneralPacket packet(S_SRC_CLIENT | S_TYPE_NOTIFICATION | S_DEST_ROOM, command);
        packet.setMessageBody(arg);
        socket->send(toQString(packet.toString()));
    }
}

void Client::checkVersion(const Json::Value &server_version)
{
    QString version = toQString(server_version);
    QString version_number, mod_name;
    if (version.contains(QChar(':'))) {
        QStringList texts = version.split(QChar(':'));
        version_number = texts.value(0);
        mod_name = texts.value(1);
    }
    else {
        version_number = version;
        mod_name = "official";
    }

    emit version_checked(version_number, mod_name);
}

void Client::setup(const Json::Value &setup_str)
{
    if (socket && !socket->isConnected()) {
        return;
    }

    QStringList setup_info;
    if (!setup_str.isArray() || !tryParse(setup_str, setup_info)) {
        return;
    }

    ServerInfo.clear();
    if (ServerInfo.parse(setup_info)) {
        emit server_connected();
        notifyServer(S_COMMAND_TOGGLE_READY);
    }
    else {
        MainWindow *mainWnd = qobject_cast<MainWindow *>(parent());
        if (NULL != mainWnd) {
            mainWnd->deleteClient();
        }

        QMessageBox::warning(NULL, tr("Warning"), tr("Setup string can not be parsed: %1").arg(setup_info.join(":")));
    }
}

void Client::disconnectFromHost()
{
    if (socket) {
        socket->disconnectFromHost();
        socket = NULL;
    }
}

void Client::processServerPacket(const QString &cmd)
{
    if (m_isGameOver) {
        return;
    }

    QSanGeneralPacket packet;
    if (packet.parse(cmd.toStdString())) {
        if (packet.getPacketType() == S_TYPE_NOTIFICATION) {
            CallBack callback = m_callbacks[packet.getCommandType()];
            if (callback) {
                (this->*callback)(packet.getMessageBody());
            }
        }
        else if (packet.getPacketType() == S_TYPE_REQUEST) {
            if (!replayer) {
                processServerRequest(packet);
            }
        }
    }
}

bool Client::processServerRequest(const QSanGeneralPacket &packet)
{
    setStatus(NotActive);

    _m_lastServerSerial = packet.m_globalSerial;
    CommandType command = packet.getCommandType();
    Json::Value msg = packet.getMessageBody();

    Countdown countdown;
    countdown.m_current = 0;
    if (!msg.isArray() || msg.size() <= 1
        || !countdown.tryParse(msg[msg.size() - 1])) {
        countdown.m_type = Countdown::S_COUNTDOWN_USE_DEFAULT;
        countdown.m_max = ServerInfo.getCommandTimeout(command, S_CLIENT_INSTANCE);
    }
    setCountdown(countdown);

    CallBack callback = m_interactions[command];
    if (!callback) {
        return false;
    }

    (this->*callback)(msg);
    return true;
}

void Client::addPlayer(const Json::Value &player_info)
{
    if (!player_info.isArray() || player_info.size() != 4
        || !player_info[0].isString() || !player_info[1].isString() || !player_info[2].isString()
        || !player_info[3].isBool()) {
        return;
    }

    QString name = toQString(player_info[0]);
    QString base64 = toQString(player_info[1]);
    QString screen_name = Settings::fromBase64(base64);
    QString avatar = toQString(player_info[2]);
    bool owner = player_info[3].asBool();

    ClientPlayer *player = new ClientPlayer(this);
    player->setObjectName(name);
    player->setScreenName(screen_name);
    player->setProperty("avatar", avatar);
    player->setOwner(owner);

    m_players << player;
    ++alive_count;

    emit player_added(player);
}

void Client::updateProperty(const Json::Value &arg)
{
    if (!isStringArray(arg, 0, 2)) {
        return;
    }

    QString object_name = toQString(arg[0]);
    ClientPlayer *player = getPlayer(object_name);
    if (!player) {
        return;
    }

    player->setProperty(arg[1].asCString(), toQString(arg[2]));
}

void Client::removePlayer(const Json::Value &player_name)
{
    if (!player_name.isString()) {
        return;
    }

    QString name = toQString(player_name);
    ClientPlayer *player = findChild<ClientPlayer *>(name);
    if (player) {
        m_players.removeOne(player);
        --alive_count;

        emit player_removed(name);
    }
}

bool Client::_loseSingleCard(int card_id, CardsMoveStruct move)
{
    const Card *card = Sanguosha->getCard(card_id);
    if (move.from) {
        move.from->removeCard(card, move.from_place);
    }
    else {
        if (move.from_place == Player::DiscardPile) {
            discarded_list.removeOne(card);
        }
        else if (move.from_place == Player::DrawPile && !Self->hasFlag("marshalling")) {
            --pile_num;
        }
    }
    return true;
}

bool Client::_getSingleCard(int card_id, CardsMoveStruct move)
{
    const Card *card = Sanguosha->getCard(card_id);
    if (move.to) {
        move.to->addCard(card, move.to_place);
    }
    else {
        if (move.to_place == Player::DrawPile) {
            ++pile_num;
        }
        else if (move.to_place == Player::DiscardPile) {
            discarded_list.prepend(card);
        }
    }
    return true;
}

void Client::getCards(const Json::Value &arg)
{
    Q_ASSERT(arg.isArray() && arg.size() >= 1);

    int moveId = arg[0].asInt();
    QList<CardsMoveStruct> moves;
    for (unsigned int i = 1; i < arg.size(); ++i) {
        CardsMoveStruct move;
        if (!move.tryParse(arg[i])) return;
        move.from = getPlayer(move.from_player_name);
        move.to = getPlayer(move.to_player_name);
        Player::Place dstPlace = move.to_place;

        if (dstPlace == Player::PlaceSpecial) {
            ((ClientPlayer *)move.to)->changePile(move.to_pile_name, true, move.card_ids);
        }
        else {
            foreach (int card_id, move.card_ids) {
                _getSingleCard(card_id, move);
            }
        }
        moves.append(move);
    }
    updatePileNum();
    emit move_cards_got(moveId, moves);
}

void Client::loseCards(const Json::Value &arg)
{
    Q_ASSERT(arg.isArray() && arg.size() >= 1);

    int moveId = arg[0].asInt();
    QList<CardsMoveStruct> moves;
    for (unsigned int i = 1; i < arg.size(); ++i) {
        CardsMoveStruct move;
        if (!move.tryParse(arg[i])) {
            return;
        }
        move.from = getPlayer(move.from_player_name);
        move.to = getPlayer(move.to_player_name);
        Player::Place srcPlace = move.from_place;

        if (srcPlace == Player::PlaceSpecial) {
            ((ClientPlayer *)move.from)->changePile(move.from_pile_name, false, move.card_ids);
        }
        else {
            foreach (int card_id, move.card_ids) {
                _loseSingleCard(card_id, move);
            }
        }
        moves.append(move);
    }
    updatePileNum();
    emit move_cards_lost(moveId, moves);
}

void Client::onPlayerChooseGeneral(const QString &item_name)
{
    setStatus(NotActive);

    if (!item_name.isEmpty()) {
        replyToServer(S_COMMAND_CHOOSE_GENERAL, toJsonString(item_name));

        Sanguosha->playSystemAudioEffect("choose-item");

        if (ServerInfo.EnableHegemony) {
            emit general_choosed(item_name);
        }
    }
}

void Client::requestCheatRunScript(const QString &script)
{
    Json::Value cheatReq(Json::arrayValue);
    cheatReq[0] = (int)S_CHEAT_RUN_SCRIPT;
    cheatReq[1] = toJsonString(script);
    requestToServer(S_COMMAND_CHEAT, cheatReq);
}

void Client::requestCheatRevive(const QString &name)
{
    Json::Value cheatReq(Json::arrayValue);
    cheatReq[0] = (int)S_CHEAT_REVIVE_PLAYER;
    cheatReq[1] = toJsonString(name);
    requestToServer(S_COMMAND_CHEAT, cheatReq);
}

void Client::requestCheatDamage(const QString &source, const QString &target, DamageStruct::Nature nature, int points)
{
    Json::Value cheatReq(Json::arrayValue), cheatArg(Json::arrayValue);
    cheatArg[0] = toJsonString(source);
    cheatArg[1] = toJsonString(target);
    cheatArg[2] = (int)nature;
    cheatArg[3] = points;

    cheatReq[0] = (int)S_CHEAT_MAKE_DAMAGE;
    cheatReq[1] = cheatArg;
    requestToServer(S_COMMAND_CHEAT, cheatReq);
}

void Client::requestCheatKill(const QString &killer, const QString &victim)
{
    Json::Value cheatArg;
    cheatArg[0] = (int)S_CHEAT_KILL_PLAYER;
    cheatArg[1] = toJsonArray(killer, victim);
    requestToServer(S_COMMAND_CHEAT, cheatArg);
}

void Client::requestCheatGetOneCard(int card_id)
{
    Json::Value cheatArg;
    cheatArg[0] = (int)S_CHEAT_GET_ONE_CARD;
    cheatArg[1] = card_id;
    requestToServer(S_COMMAND_CHEAT, cheatArg);
}

void Client::requestCheatChangeGeneral(const QString &name, bool isSecondaryHero)
{
    Json::Value cheatArg;
    cheatArg[0] = (int)S_CHEAT_CHANGE_GENERAL;
    cheatArg[1] = toJsonString(name);
    cheatArg[2] = isSecondaryHero;
    requestToServer(S_COMMAND_CHEAT, cheatArg);
}

void Client::addRobot()
{
    notifyServer(S_COMMAND_ADD_ROBOT);
}

void Client::fillRobots()
{
    notifyServer(S_COMMAND_FILL_ROBOTS);
}

void Client::onPlayerResponseCard(const Card *card, const QList<const Player *> &targets)
{
    if (Self->hasFlag("Client_PreventPeach")) {
        Self->setFlags("-Client_PreventPeach");
        Self->removeCardLimitation("use", "Peach$0");
    }

    if ((m_status & ClientStatusBasicMask) == Responding) {
        _m_roomState.setCurrentCardUsePattern(QString());
    }

    if (card == NULL) {
        replyToServer(S_COMMAND_RESPONSE_CARD, Json::Value::null);
    }
    else {
        Json::Value targetNames(Json::arrayValue);
        if (!card->targetFixed()) {
            foreach (const Player *target, targets) {
                targetNames.append(toJsonString(target->objectName()));
            }
        }

        replyToServer(S_COMMAND_RESPONSE_CARD, toJsonArray(card->toString(), targetNames));

        if (card->isVirtualCard() && !card->parent()) {
            delete card;
        }
    }

    setStatus(NotActive);
}

void Client::startInXs(const Json::Value &left_seconds)
{
    if (!left_seconds.isInt()) {
        return;
    }

    int seconds = left_seconds.asInt();
    if (seconds > 0) {
        lines_doc->setHtml(tr("<p align = \"center\">Game will start in <b>%1</b> seconds...</p>").arg(seconds));
    }
    else {
        lines_doc->setHtml(QString());
    }

    emit start_in_xs();
}

void Client::arrangeSeats(const Json::Value &seats_str)
{
    if (!seats_str.isArray()) {
        return;
    }

    QStringList player_names;
    tryParse(seats_str, player_names);
    if (player_names.isEmpty()) {
        return;
    }

    int seatCount = player_names.length();
    int playerCount = m_players.length();
    if (seatCount != playerCount) {
        return;
    }

    m_players.clear();
    for (int i = 0; i < seatCount; ++i) {
        ClientPlayer *player = findChild<ClientPlayer *>(player_names.at(i));
        player->setSeat(i + 1);
        m_players << player;
    }

    QList<const ClientPlayer *> seats;
    int self_index = m_players.indexOf(Self);
    Q_ASSERT(self_index != -1);

    for (int i = self_index + 1; i < playerCount; ++i) {
        seats.append(m_players.at(i));
    }
    for (int i = 0; i < self_index; ++i) {
        seats.append(m_players.at(i));
    }
    Q_ASSERT(seats.length() == playerCount - 1);

    emit seats_arranged(seats);
}

void Client::activate(const Json::Value &playerId)
{
    setStatus(toQString(playerId) == Self->objectName() ? Playing : NotActive);
}

void Client::startGame(const Json::Value &)
{
    Sanguosha->registerRoom(this);
    _m_roomState.reset();

    emit game_started();
}

void Client::hpChange(const Json::Value &change_str)
{
    if (!change_str.isArray() || change_str.size() != 3) {
        return;
    }

    if (!change_str[0].isString() || !change_str[1].isInt() || !change_str[2].isInt()) {
        return;
    }

    QString who = toQString(change_str[0]);
    int delta = change_str[1].asInt();

    int nature_index = change_str[2].asInt();
    DamageStruct::Nature nature = DamageStruct::Normal;
    if (nature_index > 0) {
        nature = (DamageStruct::Nature)nature_index;
    }

    emit hp_changed(who, delta, nature, nature_index == -1);
}

void Client::maxhpChange(const Json::Value &change_str)
{
    if (!change_str.isArray() || change_str.size() != 2) {
        return;
    }

    if (!change_str[0].isString() || !change_str[1].isInt()) {
        return;
    }

    QString who = toQString(change_str[0]);
    int delta = change_str[1].asInt();
    emit maxhp_changed(who, delta);
}

void Client::setStatus(Status status)
{
    Status old_status = m_status;
    m_status = status;

    switch (status) {
    case Playing:
        _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY);
        break;

    case Responding:
        _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE);
        break;

    case RespondingUse:
        _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
        break;

    default:
        _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_UNKNOWN);
        break;
    }

    emit status_changed(old_status, status);
}

void Client::cardLimitation(const Json::Value &limit)
{
    if (!limit.isArray() || limit.size() != 4) {
        return;
    }

    bool set = limit[0].asBool();
    bool single_turn = limit[3].asBool();
    if (limit[1].isNull() && limit[2].isNull()) {
        Self->clearCardLimitation(single_turn);
    }
    else {
        if (!limit[1].isString() || !limit[2].isString()) {
            return;
        }
        QString limit_list = toQString(limit[1]);
        QString pattern = toQString(limit[2]);
        if (set) {
            Self->setCardLimitation(limit_list, pattern, single_turn);
        }
        else {
            Self->removeCardLimitation(limit_list, pattern);
        }
    }
}

void Client::setNullification(const Json::Value &str)
{
    if (!str.isString()) {
        return;
    }

    QString astr = toQString(str);
    if (astr != ".") {
        if (m_noNullificationTrickName == ".") {
            m_noNullificationThisTime = false;
            m_noNullificationTrickName = astr;
            emit nullification_asked(true);
        }
    }
    else {
        m_noNullificationThisTime = false;
        m_noNullificationTrickName = ".";
        emit nullification_asked(false);
    }
}

void Client::enableSurrender(const Json::Value &enabled)
{
    if (!enabled.isBool()) {
        return;
    }

    bool en = enabled.asBool();
    emit surrender_enabled(en);
}

void Client::exchangeKnownCards(const Json::Value &players)
{
    if (!players.isArray() || players.size() != 2 || !players[0].isString() || !players[1].isString()) {
        return;
    }

    ClientPlayer *a = getPlayer(toQString(players[0]));
    ClientPlayer *b = getPlayer(toQString(players[1]));
    QList<int> a_known, b_known;
    foreach (const Card *card, a->getHandcards()) {
        a_known << card->getId();
    }
    foreach (const Card *card, b->getHandcards()) {
        b_known << card->getId();
    }
    a->setCards(b_known);
    b->setCards(a_known);
}

void Client::setKnownCards(const Json::Value &set_str)
{
    if (!set_str.isArray() || set_str.size() != 2) {
        return;
    }

    QString name = toQString(set_str[0]);
    ClientPlayer *player = getPlayer(name);
    if (player == NULL) {
        return;
    }

    QList<int> ids;
    tryParse(set_str[1], ids);
    player->setCards(ids);
}

void Client::viewGenerals(const Json::Value &str)
{
    if (str.size() != 2 || !str[0].isString()) {
        return;
    }

    QStringList names;
    if (!tryParse(str[1], names)) {
        return;
    }

    QString reason = toQString(str[0]);
    emit generals_viewed(reason, names);
}

QString Client::getPlayerName(const QString &str)
{
    QRegExp rx("sgs\\d+");
    if (rx.exactMatch(str)) {
        ClientPlayer *player = getPlayer(str);
        if (!player) {
            return QString();
        }

        QString general_name = player->getGeneralName();
        general_name = Sanguosha->translate(general_name);
        if (player->getGeneral2()) {
            general_name.append("/" + Sanguosha->translate(player->getGeneral2Name()));
        }
        if (ServerInfo.EnableSame || player->getGeneralName() == "anjiang") {
            general_name = QString("%1[%2]").arg(general_name).arg(player->getSeat());
        }
        return general_name;
    }
    else {
        return Sanguosha->translate(str);
    }
}

QString Client::getSkillNameToInvokeData() const{
    return skill_to_invoke_data;
}

void Client::onPlayerInvokeSkill(bool invoke)
{
    if (skill_name == "surrender") {
        replyToServer(S_COMMAND_SURRENDER, invoke);
    }
    else {
        replyToServer(S_COMMAND_INVOKE_SKILL, invoke);
    }

    setStatus(NotActive);
}

QString Client::setPromptList(const QStringList &texts)
{
    QString prompt = Sanguosha->translate(texts.at(0));
    if (texts.length() >= 2) {
        prompt.replace("%src", getPlayerName(texts.at(1)));
    }

    if (texts.length() >= 3) {
        prompt.replace("%dest", getPlayerName(texts.at(2)));
    }

    if (texts.length() >= 5) {
        QString arg2 = Sanguosha->translate(texts.at(4));
        prompt.replace("%arg2", arg2);
    }

    if (texts.length() >= 4) {
        QString arg = Sanguosha->translate(texts.at(3));
        prompt.replace("%arg", arg);
    }

    prompt_doc->setHtml(prompt);
    return prompt;
}

void Client::commandFormatWarning(const QString &str, const QRegExp &rx, const char *command)
{
    QString text = tr("The argument (%1) of command %2 does not conform the format %3")
                      .arg(str).arg(command).arg(rx.pattern());
    QMessageBox::warning(NULL, tr("Command format warning"), text);
}

QString Client::_processCardPattern(const QString &pattern)
{
    const QChar c = pattern.at(pattern.length() - 1);
    if (c == '!' || c.isNumber()) {
        return pattern.left(pattern.length() - 1);
    }

    return pattern;
}

void Client::askForCardOrUseCard(const Json::Value &cardUsage)
{
    if (!cardUsage.isArray() || !cardUsage[0].isString() || !cardUsage[1].isString()) {
        return;
    }

    QStringList texts = toQString(cardUsage[1]).split(":");
    if (texts.isEmpty()) {
        return;
    }

    setPromptList(texts);

    QString card_pattern = toQString(cardUsage[0]);
    _m_roomState.setCurrentCardUsePattern(card_pattern);

    if (card_pattern.endsWith("!")) {
        m_isDiscardActionRefusable = false;
    }
    else {
        m_isDiscardActionRefusable = true;
    }

    QString temp_pattern = _processCardPattern(card_pattern);
    QRegExp rx("^@@?(\\w+)(-card)?$");
    if (rx.exactMatch(temp_pattern)) {
        QString skill_name = rx.capturedTexts().at(1);
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill) {
            int index = -1;
            if (cardUsage[3].isInt() && cardUsage[3].asInt() > 0) {
                index = cardUsage[3].asInt();
            }

            QString text = prompt_doc->toHtml();
            text.append(tr("<br/> <b>Notice</b>: %1<br/>").arg(skill->getNotice(index)));
            prompt_doc->setHtml(text);
        }
    }

    Status status = Responding;
    m_respondingUseFixedTarget = NULL;
    if (cardUsage[2].isInt()) {
        Card::HandlingMethod method = (Card::HandlingMethod)(cardUsage[2].asInt());
        switch (method) {
        case Card::MethodDiscard:
            status = RespondingForDiscard;
            break;

        case Card::MethodUse:
            status = RespondingUse;
            break;

        case Card::MethodResponse:
            status = Responding;
            break;

        default:
            status = RespondingNonTrigger;
            break;
        }
    }
    setStatus(status);
}

void Client::askForSkillInvoke(const Json::Value &arg)
{
    if (!isStringArray(arg, 0, 1)) {
        return;
    }

    QString skill_name = toQString(arg[0]);
    QString data = toQString(arg[1]);

    //�����������õ�������־ֵ��ò�Ʋ���Ҫ��ʽɾ����
    //������ϴ�ƺ���Щ��־ֵ��ȫ��ɾ��(�μ�Engine::getRandomCards)
    if (skill_name == "weapon_recast") {
        int cardId = toQString(arg[2]).toInt();
        Card *card = Sanguosha->getCard(cardId);
        if (NULL != card) {
            card = const_cast<Card *>(card->getRealCard());
            card->setFlags("weapon_recast");
        }
    }

    skill_to_invoke = skill_name;
	skill_to_invoke_data = data;

    QString text;
    if (data.isEmpty()) {
        text = tr("Do you want to invoke skill [%1] ?").arg(Sanguosha->translate(skill_name));
        prompt_doc->setHtml(text);
    }
    else if (data.startsWith("playerdata:")) {
        QString name = getPlayerName(data.split(":").last());
        text = tr("Do you want to invoke skill [%1] to %2 ?").arg(Sanguosha->translate(skill_name)).arg(name);
        prompt_doc->setHtml(text);
    }
    else if (skill_name.startsWith("cv_")) {
        setPromptList(QStringList() << "@sp_convert" << QString() << QString() << data);
    }
    else {
        QStringList texts = data.split(":");
        text = QString("%1:%2").arg(skill_name).arg(texts.first());
        texts.replace(0, text);
        setPromptList(texts);
    }

    setStatus(AskForSkillInvoke);
}

void Client::onPlayerMakeChoice()
{
    QString option = sender()->objectName();
    replyToServer(S_COMMAND_MULTIPLE_CHOICE, toJsonString(option));
    setStatus(NotActive);
}

void Client::askForSurrender(const Json::Value &initiator)
{
    if (!initiator.isString()) {
        return;
    }

    QString text = tr("%1 initiated a vote for disadvataged side to claim "
                      "capitulation. Click \"OK\" to surrender or \"Cancel\" to resist.")
                      .arg(Sanguosha->translate(toQString(initiator)));
    text.append(tr("<br/> <b>Notice</b>: if all people on your side decides to surrender. "
                   "You'll lose this game."));
    skill_name = "surrender";

    prompt_doc->setHtml(text);
    setStatus(AskForSkillInvoke);
}

void Client::askForLuckCard(const Json::Value &)
{
    skill_to_invoke = "luck_card";
	skill_to_invoke_data = QString();
    prompt_doc->setHtml(tr("Do you want to use the luck card?"));
    setStatus(AskForSkillInvoke);
}

void Client::askForNullification(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 3 || !arg[0].isString()
        || !(arg[1].isNull() || arg[1].isString())
        || !arg[2].isString()) {
        return;
    }

    QString trick_name = toQString(arg[0]);
    Json::Value source_name = arg[1];
    ClientPlayer *target_player = getPlayer(toQString(arg[2]));

    if (!target_player || !target_player->getGeneral()) {
        return;
    }

    ClientPlayer *source = NULL;
    if (source_name != Json::Value::null) {
        source = getPlayer(source_name.asCString());
    }

    const Card *trick_card = Sanguosha->findChild<const Card *>(trick_name);
    if (Config.NeverNullifyMyTrick && source == Self) {
        if (trick_card->isKindOf("SingleTargetTrick") || trick_card->isKindOf("IronChain")) {
            onPlayerResponseCard(NULL);
            return;
        }
    }
    if (m_noNullificationThisTime && m_noNullificationTrickName == trick_name) {
        if (trick_card->isKindOf("AOE") || trick_card->isKindOf("GlobalEffect")) {
            onPlayerResponseCard(NULL);
            return;
        }
    }

    if (source == NULL) {
        prompt_doc->setHtml(tr("Do you want to use nullification to trick card %1 from %2?")
                               .arg(Sanguosha->translate(trick_card->objectName()))
                               .arg(getPlayerName(target_player->objectName())));
    }
    else {
        prompt_doc->setHtml(tr("%1 used trick card %2 to %3 <br>Do you want to use nullification?")
                               .arg(getPlayerName(source->objectName()))
                               .arg(Sanguosha->translate(trick_name))
                               .arg(getPlayerName(target_player->objectName())));
    }

    _m_roomState.setCurrentCardUsePattern("nullification");
    m_isDiscardActionRefusable = true;
    m_respondingUseFixedTarget = NULL;
    setStatus(RespondingUse);
}

void Client::onPlayerChooseCard(int card_id)
{
    Json::Value reply = Json::Value::null;
    if (card_id != -2) {
        reply = card_id;
    }
    replyToServer(S_COMMAND_CHOOSE_CARD, reply);
    setStatus(NotActive);
}

void Client::onPlayerChoosePlayer(const Player *player)
{
    if (player == NULL && !m_isDiscardActionRefusable) {
        player = findChild<const Player *>(players_to_choose.first());
    }

    replyToServer(S_COMMAND_CHOOSE_PLAYER, (player == NULL) ? Json::Value::null : toJsonString(player->objectName()));
    setStatus(NotActive);
}

void Client::trust()
{
    notifyServer(S_COMMAND_TRUST);

    if (Self->getState() == "trust") {
        Sanguosha->playSystemAudioEffect("untrust");
    }
    else {
        Sanguosha->playSystemAudioEffect("trust");
    }

    setStatus(NotActive);
}

void Client::requestSurrender() {
    requestToServer(S_COMMAND_SURRENDER);
    setStatus(NotActive);
}

void Client::speakToServer(const QString &text) {
    if (text.isEmpty()) {
        return;
    }

    notifyServer(S_COMMAND_SPEAK, toJsonString(Settings::toBase64(text)));
}

void Client::addHistory(const Json::Value &history)
{
    if (!history.isArray() || history.size() != 2) {
        return;
    }
    if (!history[0].isString() || !history[1].isInt()) {
        return;
    }

    QString add_str = toQString(history[0]);
    int times = history[1].asInt();
    if (add_str == "pushPile") {
        emit card_used();
        return;
    }
    else if (add_str == ".") {
        Self->clearHistory();
        return;
    }

    if (times == 0) {
        Self->clearHistory(add_str);
    }
    else {
        Self->addHistory(add_str, times);
    }
}

ClientPlayer *Client::getPlayer(const QString &name)
{
    if ((Self && Self->objectName() == name)
        || name == QSanProtocol::S_PLAYER_SELF_REFERENCE_ID) {
        return Self;
    }
    else {
        return findChild<ClientPlayer *>(name);
    }
}

bool Client::save(const QString &filename) const
{
    if (recorder) {
        return recorder->save(filename);
    }
    else {
        return false;
    }
}

QList<QString> Client::getRecords() const
{
    if (recorder) {
        return recorder->getRecords();
    }
    else {
        return QList<QString>();
    }
}

void Client::resetPiles(const Json::Value &)
{
    discarded_list.clear();
    ++swap_pile;
    updatePileNum();
}

void Client::setPileNumber(const Json::Value &pile_str)
{
    if (!pile_str.isInt()) {
        return;
    }

    pile_num = pile_str.asInt();
    updatePileNum();
}

void Client::synchronizeDiscardPile(const Json::Value &discard_pile)
{
    if (!discard_pile.isArray()) {
        return;
    }

    QList<int> discard;
    if (tryParse(discard_pile, discard)) {
        foreach (int id, discard) {
            const Card *card = Sanguosha->getCard(id);
            discarded_list.append(card);
        }
        updatePileNum();
    }
}

void Client::setCardFlag(const Json::Value &pattern_str)
{
    if (!pattern_str.isArray() || pattern_str.size() != 2) {
        return;
    }
    if (!pattern_str[0].isInt() || !pattern_str[1].isString()) {
        return;
    }

    int id = pattern_str[0].asInt();
    Card *card = Sanguosha->getCard(id);
    if (NULL != card) {
        QString flag = toQString(pattern_str[1]);
        card->setFlags(flag);
    }
}

void Client::updatePileNum()
{
    QString pile_str = tr("Draw pile: <b>%1</b>, discard pile: <b>%2</b>, swap times: <b>%3</b>")
                       .arg(pile_num).arg(discarded_list.length()).arg(swap_pile);
    if (ServerInfo.GameMode == "04_boss") {
        pile_str.prepend(tr("Level: <b>%1</b>,").arg(m_bossLevel + 1));
    }
    lines_doc->setHtml(QString("<font color='%1'><p align = \"center\">" + pile_str + "</p></font>").arg(Config.TextEditColor.name()));
}

void Client::askForDiscard(const Json::Value &req)
{
    if (!req.isArray() || !req[0].isInt() || !req[1].isInt() || !req[2].isBool() || !req[3].isBool()
        || !req[4].isString() || !req[5].isString()) {
        QMessageBox::warning(NULL, tr("Warning"), tr("Discard string is not well formatted!"));
        return;
    }

    discard_num = req[0].asInt();
    min_num = req[1].asInt();
    m_isDiscardActionRefusable = req[2].asBool();
    m_canDiscardEquip = req[3].asBool();
    QString prompt = req[4].asCString();
    QString pattern = req[5].asCString();
    if (pattern.isEmpty()) {
        pattern = ".";
    }
    m_cardDiscardPattern = pattern;

    if (prompt.isEmpty()) {
        if (m_canDiscardEquip) {
            prompt = tr("Please discard %1 card(s), include equip").arg(discard_num);
        }
        else {
            prompt = tr("Please discard %1 card(s), only hand cards is allowed").arg(discard_num);
        }
        prompt_doc->setHtml(prompt);
    }
    else {
        QStringList texts = prompt.split(":");
        if (texts.length() < 4) {
            while (texts.length() < 3) {
                texts.append(QString());
            }
            texts.append(QString::number(discard_num));
        }
        setPromptList(texts);
    }

    setStatus(Discarding);
}

void Client::askForExchange(const Json::Value &exchange_str)
{
    if (!exchange_str.isArray() || !exchange_str[0].isInt() || !exchange_str[1].isInt()
        || !exchange_str[2].isBool() || !exchange_str[3].isString() || !exchange_str[4].isBool()
        || !exchange_str[5].isString()) {
        QMessageBox::warning(NULL, tr("Warning"), tr("Exchange string is not well formatted!"));
        return;
    }

    discard_num = exchange_str[0].asInt();
    min_num = exchange_str[1].asInt();
    m_canDiscardEquip = exchange_str[2].asBool();
    QString prompt = exchange_str[3].asCString();
    m_isDiscardActionRefusable = exchange_str[4].asBool();

    QString pattern = exchange_str[5].asCString();
    if (pattern.isEmpty()) {
        pattern = ".";
    }
    m_cardDiscardPattern = pattern;

    if (prompt.isEmpty()) {
        prompt = tr("Please give %1 cards to exchange").arg(discard_num);
        prompt_doc->setHtml(prompt);
    }
    else {
        QStringList texts = prompt.split(":");
        if (texts.length() < 4) {
            while (texts.length() < 3) {
                texts.append(QString());
            }
            texts.append(QString::number(discard_num));
        }
        setPromptList(texts);
    }

    setStatus(Exchanging);
}

void Client::gameOver(const Json::Value &arg)
{
    disconnectFromHost();
    m_isGameOver = true;
    setStatus(Client::NotActive);

    //�������Ͷ��ʱ����뵽�ú���������������Ұ�ļ�������
    //�����ڹ�սģʽ�£����޷�׼ȷ��ʾ��δ������ɫ��������
    if (!ServerInfo.EnableHegemony) {
        QStringList roles;
        tryParse(arg[1], roles);

        for (int i = 0; i < roles.length(); ++i) {
            QString name = m_players.at(i)->objectName();
            getPlayer(name)->setRole(roles.at(i));
        }
    }

    QString winner = toQString(arg[0]);
    if (winner == ".") {
        emit standoff();
		Sanguosha->unregisterRoom();
        return;
    }

    QSet<QString> winners = winner.split("+").toSet();
    foreach (const ClientPlayer *player, m_players) {
        QString role = player->getRole();
        bool win = winners.contains(player->objectName()) || winners.contains(role);

        ClientPlayer *p = const_cast<ClientPlayer *>(player);
        p->setProperty("win", win);
    }

    Sanguosha->unregisterRoom();
    emit game_over();
}

void Client::killPlayer(const Json::Value &player_arg)
{
    if (!player_arg.isString()) {
        return;
    }

    --alive_count;

    QString player_name = toQString(player_arg);
    ClientPlayer *player = getPlayer(player_name);
    if (player == Self) {
        emit skill_all_detached();
    }
    player->detachAllSkills();

    if (!Self->hasFlag("marshalling")) {
        updatePileNum();
    }

    emit player_killed(player_name);
}

void Client::revivePlayer(const Json::Value &player_arg)
{
    if (!player_arg.isString()) {
        return;
    }

    ++alive_count;
    updatePileNum();

    QString player_name = toQString(player_arg);
    emit player_revived(player_name);
}

void Client::warn(const Json::Value &reason_json)
{
    disconnectFromHost();

    QString msg;
    QString reason = toQString(reason_json);
    if (reason == "GAME_OVER") {
        msg = tr("Game is over now");
    }
    else if (reason == "INVALID_FORMAT") {
        msg = tr("Invalid signup string");
    }
    else if (reason == "LEVEL_LIMITATION") {
        msg = tr("Your level is not enough");
    }
    else {
        msg = tr("Unknown warning: %1").arg(reason);
    }
    QMessageBox::warning(NULL, tr("Warning"), msg);
}

void Client::askForGeneral(const Json::Value &arg)
{
    QStringList generals;
    if (!tryParse(arg, generals)) {
        return;
    }

    emit generals_got(generals);
    setStatus(ExecDialog);
}

void Client::askForSuit(const Json::Value &)
{
    static QStringList suits;
    if (suits.isEmpty()) {
        suits << "spade" << "club" << "heart" << "diamond";
    }

    emit suits_got(suits);
    setStatus(ExecDialog);
}

void Client::askForKingdom(const Json::Value &)
{
    QStringList kingdoms = Sanguosha->getKingdoms();
    kingdoms.removeOne("god"); // god kingdom does not really exist
    emit kingdoms_got(kingdoms);

    setStatus(ExecDialog);
}

void Client::askForChoice(const Json::Value &ask_str)
{
    if (!isStringArray(ask_str, 0, 1)) {
        return;
    }

    QString skill_name = toQString(ask_str[0]);
	QString split_string = "|";
	if (skill_name == "BossModeExpStore")
	    split_string = "+";
    QStringList options = toQString(ask_str[1]).split(split_string);
    emit options_got(skill_name, options);

    setStatus(ExecDialog);
}

void Client::askForCardChosen(const Json::Value &ask_str)
{
    if (!ask_str.isArray() || ask_str.size() != 6 || !isStringArray(ask_str, 0, 2)
        || !ask_str[3].isBool() || !ask_str[4].isInt()) {
        return;
    }

    QString player_name = toQString(ask_str[0]);
    ClientPlayer *player = getPlayer(player_name);
    if (player == NULL) {
        return;
    }

    QString flags = toQString(ask_str[1]);
    QString reason = toQString(ask_str[2]);
    bool handcard_visible = ask_str[3].asBool();
    Card::HandlingMethod method = (Card::HandlingMethod)ask_str[4].asInt();
    QList<int> disabled_ids;
    tryParse(ask_str[5], disabled_ids);

    emit cards_got(player, flags, reason, handcard_visible, method, disabled_ids);
    setStatus(ExecDialog);
}

void Client::askForOrder(const Json::Value &arg)
{
    if (!arg.isInt()) {
        return;
    }

    Game3v3ChooseOrderCommand reason = (Game3v3ChooseOrderCommand)arg.asInt();
    emit orders_got(reason);

    setStatus(ExecDialog);
}

void Client::askForRole3v3(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 2
        || !arg[0].isString() || !arg[1].isArray()) {
        return;
    }

    QStringList roles;
    if (!tryParse(arg[1], roles)) {
        return;
    }

    QString scheme = toQString(arg[0]);
    emit roles_got(scheme, roles);

    setStatus(ExecDialog);
}

void Client::askForDirection(const Json::Value &)
{
    emit directions_got();
    setStatus(ExecDialog);
}

void Client::setMark(const Json::Value &mark_str)
{
    if (!mark_str.isArray() || mark_str.size() != 3) {
        return;
    }
    if (!mark_str[0].isString() || !mark_str[1].isString() || !mark_str[2].isInt()) {
        return;
    }

    QString who = toQString(mark_str[0]);
    QString mark = toQString(mark_str[1]);
    int value = mark_str[2].asInt();

    ClientPlayer *player = getPlayer(who);
    player->setMark(mark, value);
}

void Client::onPlayerChooseSuit()
{
    replyToServer(S_COMMAND_CHOOSE_SUIT, toJsonString(sender()->objectName()));
    setStatus(NotActive);
}

void Client::onPlayerChooseKingdom()
{
    replyToServer(S_COMMAND_CHOOSE_KINGDOM, toJsonString(sender()->objectName()));
    setStatus(NotActive);
}

void Client::onPlayerDiscardCards(const Card *cards)
{
    Json::Value val;
    if (!cards) {
        val = Json::Value::null;
    }
    else {
        foreach (int card_id, cards->getSubcards()) {
            val.append(card_id);
        }
        if (cards->isVirtualCard() && !cards->parent()) {
            delete cards;
        }
    }
    replyToServer(S_COMMAND_DISCARD_CARD, val);

    setStatus(NotActive);
}

void Client::fillAG(const Json::Value &cards_str)
{
    if (!cards_str.isArray() || cards_str.size() != 2) {
        return;
    }

    QList<int> card_ids, disabled_ids;
    tryParse(cards_str[0], card_ids);
    tryParse(cards_str[1], disabled_ids);
    emit ag_filled(card_ids, disabled_ids);
}

void Client::takeAG(const Json::Value &take_str)
{
    if (!take_str.isArray() || take_str.size() != 3) {
        return;
    }
    if (!take_str[1].isInt() || !take_str[2].isBool()) {
        return;
    }

    int card_id = take_str[1].asInt();
    bool move_cards = take_str[2].asBool();
    const Card *card = Sanguosha->getCard(card_id);

    if (take_str[0].isNull()) {
        if (move_cards) {
            discarded_list.prepend(card);
            updatePileNum();
        }
        emit ag_taken(NULL, card_id, move_cards);
    }
    else {
        ClientPlayer *taker = getPlayer(toQString(take_str[0]));
        if (move_cards) {
            taker->addCard(card, Player::PlaceHand);
        }
        emit ag_taken(taker, card_id, move_cards);
    }
}

void Client::clearAG(const Json::Value &)
{
    emit ag_cleared();
}

void Client::askForSinglePeach(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 2 || !arg[0].isString() || !arg[1].isInt()) {
        return;
    }

    ClientPlayer *dying = getPlayer(toQString(arg[0]));
    int peaches = arg[1].asInt();

    // @todo: anti-cheating of askForSinglePeach is not done yet!!!
    QStringList pattern;

	pattern << "peach";
    if (dying == Self) {
        prompt_doc->setHtml(tr("You are dying, please provide %1 peach(es)(or analeptic) to save yourself").arg(peaches));
        pattern << "analeptic";
    }
    else {
        QString dying_general = getPlayerName(dying->objectName());
        prompt_doc->setHtml(tr("%1 is dying, please provide %2 peach(es) to save him").arg(dying_general).arg(peaches));
    }

    if (Self->getMark("Global_PreventPeach") > 0) {
        bool has_skill = false;
        foreach (const Skill *skill, Self->getVisibleSkillList(true)) {
            const ViewAsSkill *view_as_skill = ViewAsSkill::parseViewAsSkill(skill);
            if (view_as_skill && view_as_skill->isAvailable(Self, CardUseStruct::CARD_USE_REASON_RESPONSE_USE, pattern.join("+"))) {
                has_skill = true;
                break;
            }
        }
	
		bool invoke = Self->hasSkill("futai") && Self->getPhase() == Player::NotActive;
		foreach (const Player *p, Self->getAliveSiblings()) {
			if (!invoke)
				invoke = p->hasSkill("futai") && p->getPhase() == Player::NotActive;
		}

        if (!has_skill || invoke) {
            pattern.removeOne("peach");
            if (pattern.isEmpty()) {
                onPlayerResponseCard(NULL);
                return;
            }
        } else {
            Self->setFlags("Client_PreventPeach");
            Self->setCardLimitation("use", "Peach");
        }
    }
    _m_roomState.setCurrentCardUsePattern(pattern.join("+"));
    m_isDiscardActionRefusable = true;
    m_respondingUseFixedTarget = dying;

    setStatus(RespondingUse);
}

void Client::askForCardShow(const Json::Value &requestor)
{
    if (!requestor.isString()) {
        return;
    }

    QString name = Sanguosha->translate(toQString(requestor));
    prompt_doc->setHtml(tr("%1 request you to show one hand card").arg(name));

    _m_roomState.setCurrentCardUsePattern(".");
    setStatus(AskForShowOrPindian);
}

void Client::askForAG(const Json::Value &arg)
{
    if (!arg.isBool()) {
        return;
    }

    m_isDiscardActionRefusable = arg.asBool();
    setStatus(AskForAG);
}

void Client::onPlayerChooseAG(int card_id)
{
    replyToServer(S_COMMAND_AMAZING_GRACE, card_id);
    setStatus(NotActive);
}

void Client::alertFocus()
{
    if (Self->getPhase() == Player::Play) {
        Sanguosha->playSystemAudioEffect("your-turn");
        QApplication::alert(QApplication::focusWidget());
    }
}

void Client::showCard(const Json::Value &show_str)
{
    if (!show_str.isArray() || show_str.size() != 2
        || !show_str[0].isString() || !show_str[1].isInt()) {
        return;
    }

    QString player_name = toQString(show_str[0]);
    int card_id = show_str[1].asInt();

    ClientPlayer *player = getPlayer(player_name);
    if (player != Self) {
        player->addKnownHandCard(Sanguosha->getCard(card_id));
    }

    emit card_shown(player_name, card_id);
}

void Client::attachSkill(const Json::Value &skill)
{
    if (!skill.isString()) {
        return;
    }

    QString skill_name = toQString(skill);
    Self->acquireSkill(skill_name);
    emit skill_attached(skill_name);
}

void Client::askForAssign(const Json::Value &)
{
    emit assign_asked();
}

void Client::onPlayerAssignRole(const QList<QString> &names, const QList<QString> &roles)
{
    Q_ASSERT(names.size() == roles.size());

    Json::Value reply(Json::arrayValue);
    reply[0] = toJsonArray(names);
    reply[1] = toJsonArray(roles);
    replyToServer(S_COMMAND_CHOOSE_ROLE, reply);
}

void Client::askForGuanxing(const Json::Value &arg)
{
    Json::Value deck = arg[0];
    int guanxing_type = arg[1].asInt();

    QList<int> card_ids;
    tryParse(deck, card_ids);

    emit guanxing(card_ids, guanxing_type);

    setStatus(AskForGuanxing);
}

void Client::showAllCards(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 3
        || !arg[0].isString() || ! arg[1].isBool()) {
        return;
    }

    QList<int> card_ids;
    if (!tryParse(arg[2], card_ids)) {
        return;
    }

    ClientPlayer *who = getPlayer(toQString(arg[0]));
    if (who) {
        who->setCards(card_ids);
    }

    emit gongxin(card_ids, false, QList<int>());
}

void Client::askForGongxin(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 4
        || !arg[0].isString() || ! arg[1].isBool()) {
        return;
    }

    QList<int> card_ids;
    if (!tryParse(arg[2], card_ids)) {
        return;
    }
    QList<int> enabled_ids;
    if (!tryParse(arg[3], enabled_ids)) {
        return;
    }

    ClientPlayer *who = getPlayer(toQString(arg[0]));
    if (who) {
        who->setCards(card_ids);
    }

    bool enable_heart = arg[1].asBool();
    emit gongxin(card_ids, enable_heart, enabled_ids);

    setStatus(AskForGongxin);
}

void Client::onPlayerReplyGongxin(int card_id)
{
    Json::Value reply = Json::Value::null;
    if (card_id != -1) {
        reply = card_id;
    }
    replyToServer(S_COMMAND_SKILL_GONGXIN, reply);
    setStatus(NotActive);
}

void Client::askForPindian(const Json::Value &ask_str)
{
    if (!isStringArray(ask_str, 0, 1)) {
        return;
    }

    QString from = toQString(ask_str[0]);
    if (from == Self->objectName()) {
        prompt_doc->setHtml(tr("Please play a card for pindian"));
    }
    else {
        QString requestor = getPlayerName(from);
        prompt_doc->setHtml(tr("%1 ask for you to play a card to pindian").arg(requestor));
    }

    _m_roomState.setCurrentCardUsePattern(".");
    setStatus(AskForShowOrPindian);
}

void Client::askForYiji(const Json::Value &ask_str)
{
    if (!ask_str.isArray() || (ask_str.size() != 4 && ask_str.size() != 5)) {
        return;
    }

    Json::Value card_list = ask_str[0];
    int count = ask_str[2].asInt();
    m_isDiscardActionRefusable = ask_str[1].asBool();

    if (ask_str.size() == 5) {
        QString prompt = toQString(ask_str[4]);
        QStringList texts = prompt.split(":");
        if (texts.length() < 4) {
            while (texts.length() < 3) {
                texts.append(QString());
            }
            texts.append(QString::number(count));
        }
        setPromptList(texts);
    }
    else {
        prompt_doc->setHtml(tr("Please distribute %1 cards %2 as you wish")
                               .arg(count)
                               .arg(m_isDiscardActionRefusable ? QString() : tr("to another player")));
    }

    //@todo: use cards directly rather than the QString
    QStringList card_str;
    for (unsigned int i = 0; i < card_list.size(); ++i) {
        card_str << QString::number(card_list[i].asInt());
    }

    Json::Value players = ask_str[3];
    QStringList names;
    tryParse(players, names);
    _m_roomState.setCurrentCardUsePattern(QString("%1=%2=%3").arg(count).arg(card_str.join("+")).arg(names.join("+")));

    setStatus(AskForYiji);
}

void Client::askForPlayerChosen(const Json::Value &players)
{
    if (!players.isArray() || players.size() != 4) {
        return;
    }
    if (!players[1].isString() || !players[0].isArray() || !players[3].isBool()) {
        return;
    }
    if (players[0].size() == 0) {
        return;
    }

    skill_name = toQString(players[1]);
    players_to_choose.clear();
    for (unsigned int i = 0; i < players[0].size(); ++i) {
        players_to_choose.push_back(toQString(players[0][i]));
    }
    m_isDiscardActionRefusable = players[3].asBool();

    QString prompt = toQString(players[2]);
    if (!prompt.isEmpty()) {
        QStringList texts = prompt.split(":");
        setPromptList(texts);
    }
    else {
        prompt_doc->setHtml(tr("Please choose a player"));
    }

    setStatus(AskForPlayerChoose);
}

void Client::onPlayerReplyYiji(const Card *card, const Player *to)
{
    Json::Value req;
    if (!card) {
        req = Json::Value::null;
    }
    else {
        req = Json::Value(Json::arrayValue);
        req[0] = toJsonArray(card->getSubcards());
        req[1] = toJsonString(to->objectName());
    }
    replyToServer(S_COMMAND_SKILL_YIJI, req);

    setStatus(NotActive);
}

void Client::onPlayerReplyGuanxing(const QList<int> &up_cards, const QList<int> &down_cards)
{
    Json::Value decks(Json::arrayValue);
    decks[0] = toJsonArray(up_cards);
    decks[1] = toJsonArray(down_cards);

    replyToServer(S_COMMAND_SKILL_GUANXING, decks);

    setStatus(NotActive);
}

void Client::log(const Json::Value &log_str)
{
    if (!log_str.isArray() || log_str.size() != 6) {
        emit log_received(QStringList() << QString());
    }
    else {
        QStringList log;
        tryParse(log_str, log);
        if (log.first().contains("#BasaraReveal")) {
            Sanguosha->playSystemAudioEffect("choose-item");
        }
        else if (log.first() == "#UseLuckCard") {
            ClientPlayer *from = getPlayer(log.at(1));
            if (from && from != Self) {
                from->setHandcardNum(0);
            }
        }
        emit log_received(log);
    }
}

void Client::speak(const Json::Value &speak_data)
{
    if (!speak_data.isArray() || speak_data.size() != 2
        || !speak_data[0].isString() || !speak_data[1].isString()) {
        return;
    }

    QString who = toQString(speak_data[0]);
    QString base64 = toQString(speak_data[1]);
    QString text = Settings::fromBase64(base64);

    static const QString prefix("<img width=50 height=50 src='image/system/chatface/");
    static const QString suffix(".png'></img>");
    text = text.replace("<#", prefix).replace("#>", suffix);

    if (who == ".") {
        QString line = tr("<font color='red'>System: %1</font>").arg(text);
        emit line_spoken(QString("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
        return;
    }

    emit player_spoken(who, QString("<p style=\"margin:3px 2px;\">%1</p>").arg(text));

    const ClientPlayer *from = getPlayer(who);

    QString title;
    if (from) {
        title = from->getGeneralName();
        title = Sanguosha->translate(title);
        title.append(QString("(%1)").arg(from->screenName()));
    }

    title = QString("<b>%1</b>").arg(title);

    QString line = tr("<font color='%1'>[%2] said: %3 </font>")
                   .arg(Config.TextEditColor.name()).arg(title).arg(text);

    emit line_spoken(QString("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
}

void Client::moveFocus(const Json::Value &focus)
{
    if (focus.isArray() && focus.size() == 3) {
        QStringList players;
        tryParse(focus[0], players);

        // focus[1] is the moveFocus reason, which is unused for now.

        Countdown countdown;
        countdown.tryParse(focus[2]);

        emit focus_moved(players, countdown);
    }
}

void Client::setEmotion(const Json::Value &set_str)
{
    if (!set_str.isArray() || set_str.size() != 2) {
        return;
    }
    if (!set_str[0].isString() || !set_str[1].isString()) {
        return;
    }

    QString target_name = toQString(set_str[0]);
    QString emotion = toQString(set_str[1]);
    emit emotion_set(target_name, emotion);
}

void Client::skillInvoked(const Json::Value &arg)
{
    if (!isStringArray(arg, 0, 1)) {
        return;
    }

    emit skill_invoked(QString(arg[1].asCString()), QString(arg[0].asCString()));
}

void Client::animate(const Json::Value &animate_str)
{
    if (!animate_str.isArray() || !animate_str[0].isInt()
        || !animate_str[1].isString() ||  !animate_str[2].isString()) {
        return;
    }

    QStringList args;
    args << toQString(animate_str[1]) << toQString(animate_str[2]);
    int name = animate_str[0].asInt();
    emit animated(name, args);
}

void Client::setFixedDistance(const Json::Value &set_str)
{
    if (!set_str.isArray() || set_str.size() != 4) {
        return;
    }
    if (!set_str[0].isString() || !set_str[1].isString()
        || !set_str[2].isInt() || !set_str[3].isBool()) {
        return;
    }

    ClientPlayer *from = getPlayer(toQString(set_str[0]));
    ClientPlayer *to = getPlayer(toQString(set_str[1]));
    if (from && to) {
        bool isSet = set_str[3].asBool();
        int distance = set_str[2].asInt();
        if (isSet) {
            from->setFixedDistance(to, distance);
        }
        else {
            from->removeFixedDistance(to, distance);
        }
    }
}

void Client::setAttackRangePair(const Json::Value &set_arg)
{
    if (!set_arg.isArray() || set_arg.size() != 3) {
        return;
    }
    if (!set_arg[0].isString() || !set_arg[1].isString()
        || !set_arg[2].isBool()) {
        return;
    }

    ClientPlayer *from = getPlayer(toQString(set_arg[0]));
    ClientPlayer *to = getPlayer(toQString(set_arg[1]));
    if (from && to) {
        bool isSet = set_arg[2].asBool();
        if (isSet) {
            from->insertAttackRangePair(to);
        }
        else {
            from->removeAttackRangePair(to);
        }
    }
}

void Client::fillGenerals(const Json::Value &generals)
{
    if (!generals.isArray()) {
        return;
    }

    QStringList filled;
    tryParse(generals, filled);
    emit generals_filled(filled);
}

void Client::askForGeneral3v3(const Json::Value &)
{
    emit general_asked();
    setStatus(AskForGeneralTaken);
}

void Client::takeGeneral(const Json::Value &take_str)
{
    if (!isStringArray(take_str, 0, 2)) {
        return;
    }

    QString who = toQString(take_str[0]);
    QString name = toQString(take_str[1]);
    QString rule = toQString(take_str[2]);
    emit general_taken(who, name, rule);
}

void Client::startArrange(const Json::Value &to_arrange)
{
    if (to_arrange.isNull()) {
        emit arrange_started(QString());
    }
    else {
        if (!to_arrange.isArray()) {
            return;
        }

        QStringList arrangelist;
        tryParse(to_arrange, arrangelist);
        emit arrange_started(arrangelist.join("+"));
    }

    setStatus(AskForArrangement);
}

void Client::onPlayerChooseRole3v3()
{
    replyToServer(S_COMMAND_CHOOSE_ROLE_3V3, toJsonString(sender()->objectName()));
    setStatus(NotActive);
}

void Client::recoverGeneral(const Json::Value &recover_str)
{
    if (!recover_str.isArray() || recover_str.size() != 2 || !recover_str[0].isInt() || !recover_str[1].isString()) {
        return;
    }

    int index = recover_str[0].asInt();
    QString name = toQString(recover_str[1]);
    emit general_recovered(index, name);
}

void Client::revealGeneral(const Json::Value &reveal_str)
{
    if (!reveal_str.isArray() || reveal_str.size() != 2 || !reveal_str[0].isString() || !reveal_str[1].isString()) {
        return;
    }

    bool self = (toQString(reveal_str[0]) == Self->objectName());
    QString general = toQString(reveal_str[1]);
    emit general_revealed(self, general);
}

void Client::onPlayerChooseOrder()
{
    OptionButton *button = qobject_cast<OptionButton *>(sender());
    QString order;
    if (button) {
        order = button->objectName();
    }
    else {
        if (qrand() % 2 == 0) {
            order = "warm";
        }
        else {
            order = "cool";
        }
    }

    int req;
    if (order == "warm") {
        req = (int)S_CAMP_WARM;
    }
    else {
        req = (int)S_CAMP_COOL;
    }

    replyToServer(S_COMMAND_CHOOSE_ORDER, req);
    setStatus(NotActive);
}

void Client::updateStateItem(const Json::Value &state_str)
{
    if (!state_str.isString()) {
        return;
    }

    emit role_state_changed(toQString(state_str));
}

void Client::updateBossLevel(const Json::Value &arg)
{
    if (!arg.isInt()) {
        return;
    }

    m_bossLevel = arg.asInt();
}

void Client::setAvailableCards(const Json::Value &pile)
{
    if (!pile.isArray()) {
        return;
    }

    QList<int> drawPile;
    tryParse(pile, drawPile);
    available_cards = drawPile;
}


void Client::updateSkill(const Json::Value &skill_name)
{
    if (!skill_name.isString())
        return;

    emit skill_updated(toQString(skill_name));
}

void Client::removeAnjiangNames(const Json::Value &arg)
{
    ClientPlayer *player = ClientInstance->getPlayer(arg.asCString());
    player->tag.remove("anjiang_generals");
}

void Client::emitErrorMessage(int errorCode, const QString &errorString)
{
    QString reason = errorString;
    if (errorCode == QAbstractSocket::RemoteHostClosedError
        && Self && Self->isKicked()) {
        reason = tr("You have been kicked out of the room by owner");
    }

    emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(errorCode).arg(reason));
}

void Client::getHostAddressAndPort(QString &address, ushort &port)
{
    if (Config.HostAddress.contains(QChar(':'))) {
        QStringList texts = Config.HostAddress.split(QChar(':'));
        address = texts.value(0);
        port = texts.value(1).toUShort();
    }
    else {
        address = Config.HostAddress;
        if (address == "127.0.0.1") {
            port = Config.ServerPort;
        }
        else {
            port = Config.value("ServerPort", "9527").toString().toUShort();
        }
    }
}
