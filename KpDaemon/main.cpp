#include <QCoreApplication>
#include <QDateTime>
#include "DaemonTool.h"
#include <dbghelp.h>

#include <QDebug>

LONG WINAPI CrashInfoHandler(struct _EXCEPTION_POINTERS *ExceptionInfo);

int main(int argc, char *argv[])
{
    qDebug() << argc << " " << argv[0];
    QCoreApplication a(argc, argv);
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashInfoHandler);

    QDaemonTool dt;
    dt.beginDaemon();

    return a.exec();
}

LONG CrashInfoHandler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    QDateTime CurDTime = QDateTime::currentDateTime();
    QString current_date = CurDTime.toString("yyyy_MM_dd_hh_mm_ss");

    QString dumpText = "Dump_"+current_date+".dmp";

    EXCEPTION_RECORD *record = ExceptionInfo->ExceptionRecord;
    QString errCode(QString::number(record->ExceptionCode, 16));
    QString errAddr(QString::number((uint)record->ExceptionAddress, 16));
    QString errFlag(QString::number(record->ExceptionFlags, 16));
    QString errPara(QString::number(record->NumberParameters, 16));
    qDebug() << "errCode = " << errCode;
    qDebug() << "errAddr = " << errAddr;
    qDebug() << "errFlag = " << errFlag;
    qDebug() << "errPara = " << errPara;

    HANDLE DumpHandle = CreateFile((LPCWSTR)dumpText.utf16(),
                                   GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(DumpHandle != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = ExceptionInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //将dump信息写入dmp文件
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),DumpHandle, MiniDumpNormal, &dumpInfo, NULL, NULL);
        CloseHandle(DumpHandle);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

