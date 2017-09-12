#ifndef BACKGROUNDRUNNER_H
#define BACKGROUNDRUNNER_H

#include <QObject>
#include <QApplication>

class BackgroundRunner : public QObject
{
    Q_OBJECT

public:
    enum DeletionPolicy {
        KeepWhenFinished = 0,
        DeleteReceiverWhenFinished,
        DeleteSelfWhenFinished,
        DeleteAllWhenFinished = DeleteReceiverWhenFinished | DeleteSelfWhenFinished
    };

    //����isUseGUI���ڱ�ʶ��̨���е�Slot�Ƿ��ʹ��GUI��Դ��
    //���ʹ����GUI��Դ�������Qt��Ҫ��(Not GUI Thread Use GUI is not safe)��
    //��Slot���������߳������У������ڲ����ȡ��ʩ��
    //ʹ�ø�Slot���е�ͬʱ�����ᵼ��������ͣ�١�
    explicit BackgroundRunner(QObject *const receiver, bool isUseGUI = false);
    ~BackgroundRunner();

    void addSlot(const char *const slotName);
    void start(BackgroundRunner::DeletionPolicy policy = DeleteAllWhenFinished);

    static void msleep(int msecs);

public slots:
    void slotFinished();

private:
    static QThread *const _getMainThread() {
        return QApplication::instance()->thread();
    }

    QObject *const m_receiver;
    QThread *const m_workerThread;

    int m_slotCount;
    int m_finishedSlotCount;

    Q_DISABLE_COPY(BackgroundRunner)
};

#endif // BACKGROUNDRUNNER_H
