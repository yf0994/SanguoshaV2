#ifndef _CLIENT_LOG_BOX_H
#define _CLIENT_LOG_BOX_H

#include <QTextEdit>

class ClientLogBox : public QTextEdit
{
    Q_OBJECT

public:
    explicit ClientLogBox(QWidget *parent = 0);

    void appendLog(const QString &type,
        const QString &from_general,
        const QStringList &to,
        const QString &card_str = QString(),
        const QString &arg = QString(),
        const QString &arg2 = QString());

    //����ʱ��֧��"�ƻ�"���ܣ��Ժ��绹Ҫ֧�ָ���ļ��ܣ�����Ҫ���¿�����֮��صĵײ����ݽṹ
    const QString &getSpecialSkillLogText() const { return m_specialSkillLogText; }
    void clearSpecialSkillLogText() { m_specialSkillLogText.clear(); }

private:
    QString bold(const QString &str, QColor color) const;

    QString m_specialSkillLogText;

public slots:
    void appendLog(const QStringList &log_str);
    QString append(const QString &text);
};

#endif
