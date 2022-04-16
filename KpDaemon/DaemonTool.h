#ifndef QDAEMONTOOL_H
#define QDAEMONTOOL_H

#include <QObject>
#include <windows.h>

class QThread;
class QProcess;
class QWorkThread;
class QDaemonTool : public QObject
{
    Q_OBJECT
public:
    enum DaemonExitStatus {
        NormalExit,
        CrashExit,
        InitExit
    };

public:
    explicit QDaemonTool(QObject *parent = nullptr);

public:
    void beginDaemon();

private:
    int startProcess();
    int startThread();

private:
    int getProcessListInfo();
    int getProcessModuleInfo(DWORD processId);
    int getProcessThreadInfo(DWORD ownerPId);
    int getHeapInfo(DWORD processId);

private:
    int checkTargetApp();
    void setExitCodeAndStatus(int code, int statu);
    void DTExit(int code, int statu);

signals:

private slots:
    void daemonExit();

private:
    QThread* m_pThread;
    QWorkThread* m_pWorker;
    QProcess* m_pProcess;
    QString m_strTargetPath;
    int m_iProcessExitCode;
    int m_iProcessExitState;
    HANDLE m_hJobObject;
    HANDLE m_hProcess;
};

#endif // QDAEMONTOOL_H
