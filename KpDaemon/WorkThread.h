#ifndef QWORKTHREAD_H
#define QWORKTHREAD_H

#include <QObject>

class QWaitCondition;
class QMutex;
class QWorkThread : public QObject
{
    Q_OBJECT
public:
    explicit QWorkThread(QObject *parent = nullptr);

public:
    void setThreadStop();

signals:
    void workThreadFinished();

public slots:
    void workThread();

private:
    QWaitCondition *m_pWaitConditon;
    QMutex *m_pMutex;
};

#endif // QWORKTHREAD_H
