printf("\x1b[0m");

if (!SetConsoleMode(_stdoutHandle, _outModeInit))
{
    exit(GetLastError());
}
