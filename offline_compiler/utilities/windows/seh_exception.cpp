/*
 * Copyright (c) 2018, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "seh_exception.h"
#include "runtime/os_interface/os_library.h"

#include <memory>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4091)
#include <dbghelp.h>
#pragma warning(pop)

#include <excpt.h>
#include <psapi.h>
#include <windows.h>

using namespace std;

string SehException::getExceptionDescription(unsigned int code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "Access violation";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "Datatype misalignement";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "Divide by zero";
    case EXCEPTION_STACK_OVERFLOW:
        return "Stack overflow";

    default:
        break;
    }
    return "Unknown";
}

int SehException::filter(unsigned int code, struct _EXCEPTION_POINTERS *ep) {

    printf("EXCEPTION: %s\n", SehException::getExceptionDescription(code).c_str());

    if (code != EXCEPTION_STACK_OVERFLOW) {
        string callstack;

        SehException::getCallStack(code, ep, callstack);
        printf("Callstack:\n\n%s", callstack.c_str());
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void SehException::getCallStack(unsigned int code, struct _EXCEPTION_POINTERS *ep, string &stack) {
    DWORD machine = 0;
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        machine = IMAGE_FILE_MACHINE_I386;
    } else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        machine = IMAGE_FILE_MACHINE_AMD64;
    } else {
        stack = "invalid processor arch";
        return;
    }

    stack.clear();

    BOOL result = SymInitialize(hProcess, NULL, TRUE);
    STACKFRAME64 stackFrame;
    memset(&stackFrame, 0, sizeof(STACKFRAME64));
    const int nameSize = 255;
    char buffer[sizeof(IMAGEHLP_SYMBOL64) + (nameSize + 1) * sizeof(char)];
    IMAGEHLP_SYMBOL64 *symbol = reinterpret_cast<IMAGEHLP_SYMBOL64 *>(buffer);
    symbol->MaxNameLength = nameSize;

    DWORD displacement = 0;
    DWORD64 displacement64 = 0;

    unique_ptr<OCLRT::OsLibrary> psApiLib(OCLRT::OsLibrary::load("psapi.dll"));
    auto getMappedFileName = reinterpret_cast<getMappedFileNameFunction>(psApiLib->getProcAddress("GetMappedFileNameA"));

    size_t callstackCounter = 0;
    const size_t maxCallstackDepth = 1000;

#ifdef _WIN64
    stackFrame.AddrPC.Offset = ep->ContextRecord->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = ep->ContextRecord->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = ep->ContextRecord->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
#else
    stackFrame.AddrPC.Offset = ep->ContextRecord->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = ep->ContextRecord->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = ep->ContextRecord->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

#endif

    while (callstackCounter < maxCallstackDepth) {
        symbol->Name[255] = '\0';

        if (!StackWalk64(machine, hProcess, hThread, &stackFrame, ep->ContextRecord, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, 0)) {
            break;
        }

        if (stackFrame.AddrFrame.Offset == 0) {
            break;
        }

        string lineInCode;
        string module;
        string symbolName;

        DWORD64 address = stackFrame.AddrPC.Offset;
        IMAGEHLP_LINE64 imageLine;
        imageLine.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(hProcess, address, &displacement, &imageLine)) {
            lineInCode = imageLine.FileName;
            char filename[MAX_PATH + 1];
            filename[MAX_PATH] = '\0';

            if (getMappedFileName(hProcess, reinterpret_cast<LPVOID>(imageLine.Address), filename, MAX_PATH)) {
                module = filename;
            }
        }

        if (SymGetSymFromAddr64(hProcess, address, &displacement64, symbol)) {

            symbolName = symbol->Name;
        }
        addLineToCallstack(stack, callstackCounter, module, lineInCode, symbolName);
        callstackCounter++;
    }
}

void SehException::addLineToCallstack(std::string &callstack, size_t counter, std::string &module, std::string &line, std::string &symbol) {
    callstack += "[";
    callstack += to_string(counter);
    callstack += "]: ";

    if (module.size()) {
        callstack += "Module:";
        callstack += module + "\n\t";
    }
    if (line.size()) {
        callstack += line + ":";
    }
    callstack += symbol + "\n";
}
