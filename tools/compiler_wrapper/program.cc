#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#if defined(_WIN32)
#include <Windows.h>
#endif

#if defined(_WIN32)
typedef wchar_t char_t;
typedef std::wstring string_t;
#define _X(s) L##s
inline int strcmp(const char_t *str1, const char_t *str2)
{
    return ::wcscmp(str1, str2);
}
#else
typedef char char_t;
typedef std::string string_t;
#define _X(s) s
inline int strcmp(const char_t *str1, const char_t *str2)
{
    return ::strcmp(str1, str2);
}
#endif

#ifdef _WIN32
static bool getcwd(string_t *recv)
{
    recv->clear();

    char_t buf[MAX_PATH];
    DWORD result = GetCurrentDirectoryW(MAX_PATH, buf);
    if (result < MAX_PATH)
    {
        recv->assign(buf);
        return true;
    }
    else if (result != 0)
    {
        std::vector<char_t> str;
        str.resize(result);
        result = GetCurrentDirectoryW(static_cast<uint32_t>(str.size()), str.data());
        assert(result <= str.size());
        if (result != 0)
        {
            recv->assign(str.data());
            return true;
        }
    }
    assert(result == 0);
    return false;
}
#else
#error NYI
#endif // _WIN32

static bool endsWith(const string_t &str, const string_t &suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static string_t s_currentWorkingDirectory;

static int runOneCompilation(std::vector<string_t> args)
{
    if (args.size() < 2)
    {
        std::cerr << "Expected at least two args." << std::endl;
        return 1;
    }

    string_t pathMapArg = _X("-pathmap:");
    // Needed because unfortunately the F# compiler uses a different flag name
    if (endsWith(args[1], _X("fsc.dll")))
    {
        pathMapArg = _X("--pathmap:");
    }
    pathMapArg += s_currentWorkingDirectory;
    pathMapArg += _X("=.");
    args.push_back(pathMapArg);

    string_t program = args[0];
    string_t argStr{};
    for (int i = 0; i < args.size(); i++)
    {
        if (i != 0)
        {
            argStr += _X(" ");
        }
        // TODO: figure out better argument quoting.
        // I think there is a WinApi function for this.
        bool containsSpaces = args[i].find(_X(" ")) != string_t::npos;
        if (containsSpaces)
            argStr += _X("\"");
        argStr += args[i];
        if (containsSpaces)
            argStr += _X("\"");
    }

    auto mutArgStr = new char_t[argStr.size() + 1];
    argStr.copy(mutArgStr, argStr.size(), 0);
    mutArgStr[argStr.size()] = 0;

    STARTUPINFOW startInfo{};
    startInfo.cb = sizeof(startInfo);
    PROCESS_INFORMATION procInfo{};
    bool createdProcess = CreateProcessW(program.c_str(), mutArgStr, nullptr, nullptr, false, 0, nullptr, nullptr, &startInfo, &procInfo);

    delete[] mutArgStr;

    if (!createdProcess)
    {
        std::cerr << "Failed to create process." << std::endl;
        return 1;
    }

    if (WAIT_OBJECT_0 != WaitForSingleObject(procInfo.hProcess, INFINITE))
    {
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
        std::cerr << "Failed to wait for process to exit." << std::endl;
        return 1;
    }

    DWORD exitCode;
    if (!GetExitCodeProcess(procInfo.hProcess, &exitCode))
    {
        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);
        std::cerr << "Failed to get exit code." << std::endl;
        return 1;
    }

    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    return exitCode;
}

#if defined(_WIN32)
int __cdecl wmain(const int argc, const char_t *argv[])
#else
int main(const int argc, const char_t *argv[])
#endif
{
    if (!getcwd(&s_currentWorkingDirectory))
    {
        std::cerr << "Failed to get working directory." << std::endl;
        return 1;
    }

    bool isPersistentWorker = false;
    std::vector<string_t> args{};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], _X("--persistent_worker")) == 0)
        {
            isPersistentWorker = true;
            break;
        }
        args.push_back(argv[i]);
    }

    if (isPersistentWorker)
    {
        std::cerr << "--persistent_worker not yet implemented." << std::endl;
        return 1;
    }
    else
    {
        return runOneCompilation(args);
    }
}
