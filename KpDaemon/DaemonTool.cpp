#include "DaemonTool.h"
#include <QThread>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <jobapi.h>
#include <tlhelp32.h>
#include "WorkThread.h"
#include <QDebug>

QDaemonTool::QDaemonTool(QObject *parent) : QObject(parent)
  , m_pThread(nullptr)
  , m_pWorker(nullptr)
  , m_pProcess(nullptr)
  , m_strTargetPath("")
  , m_iProcessExitCode(0)
  , m_iProcessExitState(0)
  , m_hJobObject(NULL)
  , m_hProcess(NULL)
{

}

void QDaemonTool::beginDaemon()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    int errCode = 0;
    errCode = startThread();
    if (0 != errCode)
        DTExit(-1, InitExit);

    errCode = startProcess();
    if (0 != errCode)
    {
        if (NULL != m_hProcess)
            TerminateProcess(m_hProcess, 0);

        DTExit(-1, InitExit);
    }
}

int QDaemonTool::startProcess()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    if (0 != checkTargetApp())
    {
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "&&&" << "未找到notpad++";
        return -1;
    }

    m_pProcess = new QProcess();
    connect(m_pProcess, &QProcess::stateChanged, this, [=](QProcess::ProcessState state){
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "&&&" << state;
        if (QProcess::NotRunning == state)
        {
            setExitCodeAndStatus(m_pProcess->exitCode(), m_pProcess->exitStatus());
            m_pWorker->setThreadStop();
        }
    });

    m_hJobObject = CreateJobObjectA(NULL, "Daemon_Tool");
    if (NULL == m_hJobObject)
    {
        qDebug() << "CreateJobObjectA failed: " << GetLastError();
        return -2;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (0 == SetInformationJobObject(m_hJobObject, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo)))
    {
        qDebug() << "SetInformationJobObject failed: " << GetLastError();
        return -2;
    }

    m_pProcess->start(m_strTargetPath);
    m_pProcess->waitForStarted();

    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_pProcess->processId());
    if (NULL == m_hProcess)
    {
        qDebug() << "OpenProcess failed: " << GetLastError();
        return -2;
    }

    if (0 == AssignProcessToJobObject(m_hJobObject, m_hProcess))
    {
        qDebug() << "AssignProcessToJobObject failed: " << GetLastError();
        return -3;
    }

    return 0;
}

int QDaemonTool::startThread()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    m_pThread = new QThread();
    m_pWorker = new QWorkThread();
    m_pWorker->moveToThread(m_pThread);
    connect(m_pThread, &QThread::started, m_pWorker, &QWorkThread::workThread);

    connect(m_pWorker, &QWorkThread::workThreadFinished, m_pThread, [=](){
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
        m_pThread->quit();
        m_pThread->wait();
    });

    connect(m_pThread, &QThread::finished, m_pWorker, [=](){
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
        m_pWorker->deleteLater();
    });

    connect(m_pWorker, &QObject::destroyed, m_pThread, [=]()
    {
       qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
       m_pThread->deleteLater();
    });

    connect(m_pThread, &QObject::destroyed, this, [=](){
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
        daemonExit();
    });

    m_pThread->start();
    return 0;
}

int QDaemonTool::getProcessListInfo()
{
    HANDLE hProcessSnap = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 proc32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcessSnap)
        return -1;

    proc32.dwSize = sizeof (PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &proc32))
    {
        CloseHandle(hProcessSnap);
        return -1;
    }

    do
    {

    } while (Process32Next(hProcessSnap, &proc32));

    CloseHandle(hProcessSnap);

    return 0;
}

int QDaemonTool::getProcessModuleInfo(DWORD processId)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 module32;

    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
    if (INVALID_HANDLE_VALUE == hModuleSnap)
    {
        return -1;
    }

    module32.dwSize = sizeof (MODULEENTRY32);

    if (!Module32First(hModuleSnap, &module32))
    {
        CloseHandle(hModuleSnap);
        return -1;
    }

    do
    {

    } while (Module32Next(hModuleSnap, &module32));

    CloseHandle(hModuleSnap);
    return 0;
}

int QDaemonTool::getProcessThreadInfo(DWORD ownerPId)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 thread32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);// 系统中所有线程
    if (INVALID_HANDLE_VALUE == hThreadSnap)
    {
        return -1;
    }

    thread32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hThreadSnap, &thread32))
    {
        CloseHandle(hThreadSnap);
        return -1;
    }

    do
    {
        if (thread32.th32OwnerProcessID == ownerPId)
        {

        }
    }while(Thread32Next(hThreadSnap, &thread32));

    CloseHandle(hThreadSnap);
    return 0;
}

int QDaemonTool::getHeapInfo(DWORD processId)
{
    HANDLE hHeapSnap = INVALID_HANDLE_VALUE;
    HEAPLIST32 hlist32;

    hHeapSnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, processId);
    if (INVALID_HANDLE_VALUE == hHeapSnap)
    {
        return -1;
    }

    hlist32.dwSize = sizeof(HEAPLIST32);

    if (!Heap32ListFirst(hHeapSnap, &hlist32))
    {
        CloseHandle(hHeapSnap);
        return -1;
    }

    do
    {
       HEAPENTRY32 heap32;
       memset(&heap32, 0, sizeof(HEAPENTRY32));
       heap32.dwSize = sizeof(HEAPENTRY32);

       if (!Heap32First(&heap32, processId, hlist32.th32HeapID))
       {
           hlist32.dwSize = sizeof(HEAPLIST32);
           continue;
       }

       do
       {
           heap32.dwSize = sizeof(HEAPENTRY32);
       }while(Heap32Next(&heap32));
       hlist32.dwSize = sizeof(HEAPLIST32);
    }while(Heap32ListNext(hHeapSnap, &hlist32));

    CloseHandle(hHeapSnap);
    return 0;
}

int QDaemonTool::checkTargetApp()
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    QString deamonDirPath = QDir::currentPath();

    QDir deamonDir(deamonDirPath);
    if (!deamonDir.exists())
        return -1; // 文件夹不存在

    QStringList filters;
    filters << QString("*.dll") << QString("*.exe");
    QDirIterator dir_iterator(deamonDirPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo file_info = dir_iterator.fileInfo();
        if (file_info.fileName().compare("notepad++.exe", Qt::CaseInsensitive) == 0)
        {
            m_strTargetPath = file_info.absoluteFilePath();
            return 0;
        }
    }

    return -2;// 文件不存在
}

void QDaemonTool::setExitCodeAndStatus(int code, int statu)
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    m_iProcessExitCode = code;
    m_iProcessExitState = statu;
}

void QDaemonTool::daemonExit()
{
    if (0 == m_iProcessExitState)
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "&&&" << "结束状态：正常结束";
    else if (1 == m_iProcessExitState)
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "&&&" << "结束状态：奔溃退出";
    else if (2 == m_iProcessExitState)
        qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "&&&" << "结束状态：异常退出";
    exit(m_iProcessExitCode);
}

void QDaemonTool::DTExit(int code, int statu)
{
    qDebug() << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__;
    setExitCodeAndStatus(code, statu);
    daemonExit();
}
