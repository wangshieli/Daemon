#include "WorkThread.h"
#include <QWaitCondition>
#include <QMutex>

#include <synchapi.h>
#include <QDebug>

QWorkThread::QWorkThread(QObject *parent) : QObject(parent)
  , m_pWaitConditon(nullptr)
  , m_pMutex(nullptr)
{

}

void QWorkThread::workThread()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    m_pWaitConditon = new QWaitCondition();
    m_pMutex = new QMutex();
    m_pMutex->lock();
    int i = 0;
    while (!m_pWaitConditon->wait(m_pMutex, 1000 * 5))
    {
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__<< "$$$" << "i = " << i++;
//        char* p = (char*)0x12345678;
//        printf("%s", p);
    }
    m_pMutex->unlock();

    emit workThreadFinished();
}

void QWorkThread::setThreadStop()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    m_pMutex->lock();
    m_pWaitConditon->wakeAll();
    m_pMutex->unlock();
}
