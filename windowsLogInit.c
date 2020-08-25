#include <windows.h>

static HANDLE _stdoutHandle;
static DWORD _outModeInit;

DWORD _outMode = 0;
_stdoutHandle = _GetStdHandle(STD_OUTPUT_HANDLE);

if (_stdoutHandle == INVALID_HANDLE_VALUE)
{
    exit(GetLastError());
}

if(!GetConsoleMode(_stdoutHandle, &_outMode))
{
    exit(GetLastError());
}

_outModeInit = _outMode;
_outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

if (!SetConsoleMode(_stdoutHandle, _outMode))
    exit(GetLastError());
