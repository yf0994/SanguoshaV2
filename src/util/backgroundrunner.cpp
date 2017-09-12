#include "backgroundrunner.h"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTime>

BackgroundRunner::BackgroundRunner(QObject *const receiver, bool isUseGUI)
    : m_receiver(receiver), m_workerThread(new QThread),
    m_slotCount(0), m_finishedSlotCount(0)
{
    //���ݲ���isUseGUI��ֵ���������������ĸ��߳���
    //��receiverʹ�����̣߳�m_workerThread�������ڹ����ۺ�������
    QThread *const targetThread = isUseGUI ? _getMainThread() : m_workerThread;
    receiver->moveToThread(targetThread);

    connect(receiver, SIGNAL(slot_finished()), this, SLOT(slotFinished()));
}

BackgroundRunner::~BackgroundRunner()
{
    m_workerThread->deleteLater();
}

void BackgroundRunner::addSlot(const char *const slotName)
{
    if (!m_workerThread->isRunning()) {
        connect(m_workerThread, SIGNAL(started()), m_receiver, slotName);
        ++m_slotCount;
    }
}

void BackgroundRunner::start(DeletionPolicy policy/* = DeleteAllWhenFinished*/)
{
    if (policy & DeleteReceiverWhenFinished) {
        connect(m_workerThread, SIGNAL(finished()), m_receiver, SLOT(deleteLater()));
    }
    if (policy & DeleteSelfWhenFinished) {
        connect(m_workerThread, SIGNAL(finished()), this, SLOT(deleteLater()));
    }

    //����m_workerThread��start�󣬻�����ִ����started�źŹ��������в�
    m_workerThread->start();

    //@todo: �������m_workerThread�����У����½��۳�����
    //@todo: Ϊ��ͳһ������ȫ����������ô˷�����
    //����Щ��δִ�����ǰ����ʹ������m_workerThread��quit()��
    //Ҳ���������˳��̣߳�����Ҫ�ȵ����в۶�ִ������ˣ��Żᷢ��finished�źţ�
    //�Ӷ�ȷ��m_workerThread��m_receiver��BackgroundRunner�����Լ����������ͷš�
    //m_workerThread->quit();
}

//������������ԭ��������Ҫ�Լ�ʵ��msleep��
//1��QThread::msleep�����ķ��ʼ�����protected���ڷ�QThread�������޷����ã�
//2��QThread::yieldCurrentThread�𲻵���ͣ�̵߳�����(����Դ�뷢�����ڲ�ֻ��::Sleep(0))��
//3��Ϊ���ִ���Ķ�ƽ̨����ֲ�ԣ�Ҳ������ֱ��ʹ��Windows API������Sleep��
void BackgroundRunner::msleep(int msecs)
{
    if (QThread::currentThread() != _getMainThread()) {
        QMutex mutex;
        QMutexLocker locker(&mutex);

        QWaitCondition waitCondition;
        waitCondition.wait(locker.mutex(), msecs);
    }
    else {
        //���m_receiver����Ĳ��������߳���ִ�У�
        //���ܲ���QWaitCondition���ƣ������������ͣ�١�
        QTime idleTime = QTime::currentTime().addMSecs(msecs);
        while (QTime::currentTime() < idleTime) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
    }
}

void BackgroundRunner::slotFinished()
{
    ++m_finishedSlotCount;
    if (m_finishedSlotCount == m_slotCount) {
        m_workerThread->quit();
    }
}
