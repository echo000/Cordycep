﻿#include "..\Parasyte\pch.h"
#include "..\Parasyte\GameHandler.h"
#include "..\Parasyte\Helper.h"
#include "..\Parasyte\Printer.h"
#include "..\Parasyte\Parasyte.h"
#include "..\Parasyte\CommandParser.h"
#include "..\Parasyte\Command.h"

// Debug
#include <dbghelp.h>
#include <Psapi.h>

// Dumping utilities
#include "..\Parasyte\ScyllaInterface.h"
#include "..\Parasyte\ProcessDumper.h"
#include "..\Parasyte\ForeignProcess.h"

// File System
#include "..\Parasyte\FileSystem.h"

#if defined _WIN64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment(lib, "ws2_32.lib")

// File Version
const char* FileVersion = "2.3.5.1";

// Our exception handler for when a fatal error occurs.
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    HMODULE hm;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(ExceptionInfo->ExceptionRecord->ExceptionAddress), &hm);
    MODULEINFO mi;
    GetModuleInformation(GetCurrentProcess(), hm, &mi, sizeof(mi));
    char fn[MAX_PATH];
    GetModuleFileNameExA(GetCurrentProcess(), hm, fn, MAX_PATH);

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId = GetCurrentThreadId();
    mei.ClientPointers = TRUE;
    mei.ExceptionPointers = ExceptionInfo;
    
    HANDLE hFile = CreateFile(
        "Dump.dmp",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MiniDumpNormal,
        &mei,
        NULL,
        NULL);

    CloseHandle(hFile);

    std::stringstream msg;

    msg << "A fatal error has occured and Cordycep must close..\n\n";
    msg << "Exception Code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::dec << "\n";
    msg << "Exception Address: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionAddress << std::dec << "\n";
   
    if (ExceptionInfo->ExceptionRecord->NumberParameters > 0)
    {
        for (size_t i = 0; i < ExceptionInfo->ExceptionRecord->NumberParameters; i++)
        {
            msg << "Exception Info: " << (char*)ExceptionInfo->ExceptionRecord->ExceptionInformation[i] << "\n";
        }
    }

    msg << "Exception Module: " << fn << "\n";
    msg << "Last Handler Set: " << ps::Parasyte::GetTelemtry("LastHandlerName") << "\n";
    msg << "Last Fast File Loaded: " << ps::Parasyte::GetTelemtry("LastFastFileName") << "\n\n";
    msg << "Before reporting this crash, please check off the following:\n\n";
    msg << "* Ensure the file is not from a previous update, lists are available in my server of valid files.\n";
    msg << "* Ensure you have followed the instructions carefully and provided the correct commands.\n";
    msg << "* Ensure you have dumped the required data correctly.\n\n";
    msg << "If you feel you have attempted to load a valid file and have followed the instructions correctly, then please provide the following:\n\n";
    msg << "* Log.txt (Generated in Cordycep's Directory)\n";
    msg << "* Dump.dmp (Generated in Cordycep's Directory)\n\n";
    msg << "If you do not provide both of this files, your bug report will be ignored. Do not send a screenshot of this message, send what it says to send.\n";

    MessageBoxA(NULL, msg.str().c_str(), "Fatal Error | Cordycep.CLI", MB_ICONERROR | MB_OK);

    return EXCEPTION_EXECUTE_HANDLER;
}

void HandleCommands(ps::CommandParser& parser)
{
    while (parser.HasCommands())
    {
        auto commandFound = false;
        auto& commandName = parser.Next();

        for (auto& command : ps::Command::GetCommands())
        {
            if (command->IsValid(commandName))
            {
                command->Execute(parser);
                commandFound = true;
                break;
            }
        }

        if (!commandFound)
        {
            throw std::exception((std::string("Unknown command provided: ") + commandName).c_str());
        }
    }
}

void InitializeConsole()
{
    if (!IsDebuggerPresent())
    {
        SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
        RECT rect;
        GetWindowRect(GetConsoleWindow(), &rect);
        MoveWindow(GetConsoleWindow(), rect.left, rect.top, 1280, 600, TRUE);
    }
}

void Print(const char* header, const char* value, bool newLine)
{
    if (newLine)
        ps::printer::WriteLineHeader(header, value);
    else
        ps::printer::WriteHeader(header, value);
}

int main_ex(int argc, const char** argv)
{
    // Initialize libtom
    register_all_ciphers();
    register_all_hashes();
    register_all_prngs();
    crypt_mp_init("ltm");

    InitializeConsole();

    // Ensure our working directory is our exe
    ps::helper::SetWorkingDirectoryToExe();

    // Initial Print
    ps::log::Init("Log.txt");
    ps::log::EnableLogType(ps::LogType::Normal);
    ps::log::EnableLogType(ps::LogType::Error);
    ps::log::SetOnPrint(Print);
    // Bleeding Edge
#if DONATORBE
    ps::printer::WriteErrorHeader("NOTE", "Bleeding Edge Build! No support provided!");
#endif
    ps::log::Print("INIT", "-----------------------");
    ps::log::Print("INIT", "  oo  Cordycep - Version: %s", FileVersion);
    ps::log::Print("INIT", "  |\"  Fast File Loader");
    ps::log::Print("INIT", "  |   Discord: https://discord.gg/RyqyThu");
    ps::log::Print("INIT", "--'   Donate: https://ko-fi.com/scobalula");
    ps::log::Print("INIT", "      Developed by the Scobalula");
    ps::log::Print("INIT", "-----------------------");
    ps::log::Print("INIT", "Cordycep is initializing, please wait...");

    // Now we can do funky
    ps::log::Print("INIT", "Loaded: %i handlers.", ps::GameHandler::GetHandlers().size());
    ps::log::Print("INIT", "Cordycep is initialized, enjoy! Always make sure to provide log and info when reporting bugs!");

    // We keep looping for input, and process it accordingly
    std::string line;
    ps::CommandParser parser;

    if (argc > 1)
    {
        try
        {
            parser.Parse(argv, 1, argc);
            HandleCommands(parser);
        }
        catch (const std::exception& ex)
        {
            ps::printer::WriteErrorHeader("ERROR", "%s", ex.what());
        }
    }

    ps::log::Print("CMD", "Need help or having trouble? Type: \"help\" to get info or ask for community support in tool support.");

    while (true)
    {
        ps::printer::WriteHeader("CMD", "Enter command: ");
        std::getline(std::cin, line);

        try
        {
            ps::log::Log(ps::LogType::Normal, "Command entered: %s", line.c_str());

            parser.Parse(line);

            if (parser.Args.size() == 0)
            {
                ps::printer::WriteErrorHeader("ERROR", "No commands were provided to Cordycep, type: \"help\" to get some usage info.");
                continue;
            }

            HandleCommands(parser);
        }
        catch (const std::exception& ex)
        {
            ps::printer::WriteErrorHeader("ERROR", "%s", ex.what());
        }
    }

    ps::log::Print("DONE", "Cordycep has completed execution, press Enter to exit.");
    std::cin.get();
    return 0;
}

int main(int argc, const char** argv)
{
    return main_ex(argc, argv);
}