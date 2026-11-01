#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "loader.h"

#include "gpio.h"
#include "host.h"
#include "log.h"
#include "sim_api.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

HMODULE g_dll = nullptr;
std::wstring g_dll_path;
void (*g_setup)() = nullptr;
void (*g_loop)() = nullptr;
volatile bool g_loaded = false;
volatile bool g_setup_done = false;
volatile bool g_busy = false;
volatile bool g_has_pending = false;
volatile bool g_armed = false;
volatile int g_jump = 0;
jmp_buf* g_jmp = nullptr;
std::vector<std::wstring> g_pending;
std::wstring g_gxx;
std::wstring g_include;
std::wstring g_forward;
std::wstring g_work;
int g_generation = 0;
char g_status[240] = "no sketch loaded";

void set_status(const std::string& text) {
    std::snprintf(g_status, sizeof g_status, "%s", text.c_str());
}

std::wstring exe_dir() {
    wchar_t buf[4096];
    DWORD n = GetModuleFileNameW(nullptr, buf, 4096);
    if (n == 0 || n >= 4096) {
        return L".";
    }
    std::wstring path(buf, n);
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, slash);
}

std::wstring full_path(const std::wstring& path) {
    wchar_t buf[4096];
    DWORD n = GetFullPathNameW(path.c_str(), 4096, buf, nullptr);
    if (n == 0 || n >= 4096) {
        return path;
    }
    return std::wstring(buf, n);
}

std::wstring filename_of(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

std::wstring parent_of(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, slash);
}

std::wstring extension_of(const std::wstring& path) {
    std::wstring name = filename_of(path);
    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) {
        return L"";
    }
    std::wstring ext = name.substr(dot);
    for (wchar_t& c : ext) {
        c = (wchar_t)towlower(c);
    }
    return ext;
}

std::string narrow(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring widen_acp(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    int n = MultiByteToWideChar(CP_ACP, 0, text.c_str(), (int)text.size(), nullptr, 0);
    if (n <= 0) {
        return {};
    }
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), (int)text.size(), out.data(), n);
    return out;
}

bool file_exists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool dir_exists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring find_gxx() {
    wchar_t env[4096];
    DWORD n = GetEnvironmentVariableW(L"PIXXCRAFT_CXX", env, 4096);
    if (n > 0 && n < 4096 && file_exists(env)) {
        return env;
    }
    n = GetEnvironmentVariableW(L"LOCALAPPDATA", env, 4096);
    if (n > 0 && n < 4096) {
        std::wstring candidate = std::wstring(env) + L"\\pixxcraft-sim-tools\\w64devkit\\bin\\g++.exe";
        if (file_exists(candidate)) {
            return candidate;
        }
    }
    wchar_t found[4096];
    DWORD got = SearchPathW(nullptr, L"g++.exe", nullptr, 4096, found, nullptr);
    if (got > 0 && got < 4096) {
        return found;
    }
    return L"";
}

void unload_sketch() {
    g_setup = nullptr;
    g_loop = nullptr;
    g_loaded = false;
    g_setup_done = false;
    if (g_dll != nullptr) {
        gpio_init();
        FreeLibrary(g_dll);
        g_dll = nullptr;
    }
    if (!g_dll_path.empty()) {
        DeleteFileW(g_dll_path.c_str());
        g_dll_path.clear();
    }
    sim_reset_board();
}

bool write_bytes(const std::wstring& path, const std::string& bytes) {
    FILE* file = _wfopen(path.c_str(), L"wb");
    if (file == nullptr) {
        return false;
    }
    size_t n = std::fwrite(bytes.data(), 1, bytes.size(), file);
    std::fclose(file);
    return n == bytes.size();
}

std::string read_file(const std::wstring& path) {
    FILE* file = _wfopen(path.c_str(), L"rb");
    if (file == nullptr) {
        return {};
    }
    std::string data;
    char buf[4096];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof buf, file)) > 0) {
        data.append(buf, n);
    }
    std::fclose(file);
    return data;
}

void log_lines(const std::string& text, int limit) {
    int shown = 0;
    size_t i = 0;
    while (i < text.size() && shown < limit) {
        size_t end = text.find('\n', i);
        if (end == std::string::npos) {
            end = text.size();
        }
        std::string line = text.substr(i, end - i);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            sim_log_warn("%s", line.c_str());
            shown++;
        }
        i = end + 1;
    }
    if (i < text.size()) {
        sim_log_warn("compiler output truncated");
    }
}

std::vector<std::string> warning_lines(const std::string& text) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < text.size() && out.size() < 12) {
        size_t end = text.find('\n', i);
        if (end == std::string::npos) {
            end = text.size();
        }
        std::string line = text.substr(i, end - i);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.find("warning:") != std::string::npos) {
            out.push_back(line);
        }
        i = end + 1;
    }
    return out;
}

bool run_compiler(const std::wstring& command, const std::wstring& gxx_dir, std::string* output, DWORD* exit_code) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        return false;
    }
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, 0, nullptr);

    wchar_t old_path[32768];
    DWORD old_n = GetEnvironmentVariableW(L"PATH", old_path, 32768);
    std::wstring joined = gxx_dir + L";" + (old_n > 0 && old_n < 32768 ? old_path : L"");
    SetEnvironmentVariableW(L"PATH", joined.c_str());

    STARTUPINFOW si{};
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = nul;
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutable_cmd(command.begin(), command.end());
    mutable_cmd.push_back(0);
    BOOL started = CreateProcessW(
        nullptr,
        mutable_cmd.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        g_work.c_str(),
        &si,
        &pi);
    if (nul != INVALID_HANDLE_VALUE) {
        CloseHandle(nul);
    }
    CloseHandle(write_pipe);
    if (!started) {
        CloseHandle(read_pipe);
        if (old_n > 0 && old_n < 32768) {
            SetEnvironmentVariableW(L"PATH", old_path);
        }
        return false;
    }

    std::string collected;
    for (;;) {
        DWORD avail = 0;
        if (PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
            char buf[1024];
            DWORD got = 0;
            if (ReadFile(read_pipe, buf, sizeof buf, &got, nullptr) && got > 0) {
                collected.append(buf, got);
            }
        }
        DWORD wait = WaitForSingleObject(pi.hProcess, 30);
        if (wait == WAIT_OBJECT_0) {
            for (;;) {
                DWORD left = 0;
                if (!PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &left, nullptr) || left == 0) {
                    break;
                }
                char buf[1024];
                DWORD got = 0;
                if (!ReadFile(read_pipe, buf, sizeof buf, &got, nullptr) || got == 0) {
                    break;
                }
                collected.append(buf, got);
            }
            break;
        }
    }
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(read_pipe);
    if (old_n > 0 && old_n < 32768) {
        SetEnvironmentVariableW(L"PATH", old_path);
    }
    *output = std::move(collected);
    *exit_code = code;
    return true;
}

bool activate_dll(const std::wstring& dll, const std::string& label, const std::vector<std::string>& notes) {
    unload_sketch();
    HMODULE mod = LoadLibraryW(dll.c_str());
    if (mod == nullptr) {
        sim_log_warn("could not load the compiled sketch (Win32 error %lu)", GetLastError());
        DeleteFileW(dll.c_str());
        set_status("build failed");
        return false;
    }
    auto setup = reinterpret_cast<void (*)()>(GetProcAddress(mod, "sketch_setup"));
    auto loop = reinterpret_cast<void (*)()>(GetProcAddress(mod, "sketch_loop"));
    if (setup == nullptr || loop == nullptr) {
        FreeLibrary(mod);
        DeleteFileW(dll.c_str());
        sim_log_warn("compiled sketch is missing sketch_setup or sketch_loop");
        set_status("build failed");
        return false;
    }
    g_dll = mod;
    g_dll_path = dll;
    g_setup = setup;
    g_loop = loop;
    g_loaded = true;
    g_setup_done = false;
    for (const std::string& note : notes) {
        sim_log_warn("%s", note.c_str());
    }
    sim_log_info("running %s", label.c_str());
    set_status("running " + label);
    std::string title = "pixxcraft  " + label;
    host_set_title(title.c_str());
    return true;
}

bool same_path(const std::wstring& a, const std::wstring& b) {
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

void add_unique(std::vector<std::wstring>& items, const std::wstring& item) {
    if (item.empty()) {
        return;
    }
    for (const std::wstring& have : items) {
        if (same_path(have, item)) {
            return;
        }
    }
    items.push_back(item);
}

std::vector<std::wstring> list_subdirs(const std::wstring& dir) {
    std::vector<std::wstring> out;
    WIN32_FIND_DATAW data{};
    HANDLE found = FindFirstFileW((dir + L"\\*").c_str(), &data);
    if (found == INVALID_HANDLE_VALUE) {
        return out;
    }
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            continue;
        }
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }
        out.push_back(dir + L"\\" + data.cFileName);
    } while (FindNextFileW(found, &data));
    FindClose(found);
    return out;
}

std::vector<std::wstring> list_sources(const std::wstring& dir) {
    std::vector<std::wstring> out;
    WIN32_FIND_DATAW data{};
    HANDLE found = FindFirstFileW((dir + L"\\*").c_str(), &data);
    if (found == INVALID_HANDLE_VALUE) {
        return out;
    }
    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        std::wstring name = data.cFileName;
        std::wstring ext = extension_of(name);
        if (ext == L".cpp" || ext == L".cc" || ext == L".cxx" || ext == L".c") {
            out.push_back(dir + L"\\" + name);
        }
    } while (FindNextFileW(found, &data));
    FindClose(found);
    return out;
}

bool builtin_header(const std::string& name) {
    static const char* kNames[] = {
        "Arduino.h", "SPI.h", "Wire.h", "Print.h", "WString.h", "pins_arduino.h",
        "wiring_private.h", "pgmspace.h", "binary.h", "limits.h", "stdint.h",
        "stddef.h", "stdio.h", "stdlib.h", "string.h", "math.h", "ctype.h",
        "stdarg.h", "stdbool.h", "inttypes.h", "avr/pgmspace.h",
    };
    for (const char* known : kNames) {
        if (name == known) {
            return true;
        }
    }
    return name.find('.') == std::string::npos || name.find('/') != std::string::npos ||
           name.find('\\') != std::string::npos;
}

void collect_includes(const std::string& text, std::vector<std::string>& out) {
    size_t i = 0;
    while (i < text.size()) {
        size_t end = text.find('\n', i);
        if (end == std::string::npos) {
            end = text.size();
        }
        std::string line = text.substr(i, end - i);
        size_t mark = line.find("#include");
        if (mark != std::string::npos) {
            size_t open = line.find_first_of("<\"", mark + 8);
            if (open != std::string::npos) {
                char close = line[open] == '<' ? '>' : '"';
                size_t shut = line.find(close, open + 1);
                if (shut != std::string::npos && shut > open + 1) {
                    out.push_back(line.substr(open + 1, shut - open - 1));
                }
            }
        }
        i = end + 1;
    }
}

std::wstring widen_header(const std::string& name) {
    std::wstring wide;
    wide.reserve(name.size());
    for (char c : name) {
        wide.push_back(c == '/' ? L'\\' : (wchar_t)(unsigned char)c);
    }
    return wide;
}

std::vector<std::wstring> library_folders(const std::vector<std::wstring>& sketch_dirs) {
    std::vector<std::wstring> parents;
    wchar_t env[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", env, MAX_PATH) > 0) {
        add_unique(parents, std::wstring(env) + L"\\Documents\\Arduino\\libraries");
        add_unique(parents, std::wstring(env) + L"\\OneDrive\\Documents\\Arduino\\libraries");
    }
    for (const std::wstring& start : sketch_dirs) {
        std::wstring walk = start;
        for (int depth = 0; depth < 4 && !walk.empty(); depth++) {
            add_unique(parents, walk + L"\\libraries");
            walk = parent_of(walk);
        }
    }
    std::vector<std::wstring> libs;
    for (const std::wstring& parent : parents) {
        if (!dir_exists(parent)) {
            continue;
        }
        for (const std::wstring& lib : list_subdirs(parent)) {
            add_unique(libs, lib);
        }
    }
    return libs;
}

void attach_arduino_libraries(
    const std::vector<std::wstring>& sketch_dirs,
    const std::vector<std::wstring>& inos,
    std::vector<std::wstring>& include_dirs,
    std::vector<std::wstring>& cpps) {
    std::vector<std::wstring> libs = library_folders(sketch_dirs);
    std::vector<std::string> queue;
    std::vector<std::string> seen_headers;
    std::vector<std::wstring> seen_libs;
    auto enqueue_text = [&](const std::string& text) {
        std::vector<std::string> names;
        collect_includes(text, names);
        for (const std::string& name : names) {
            queue.push_back(name);
        }
    };
    for (const std::wstring& ino : inos) {
        enqueue_text(read_file(ino));
    }
    for (const std::wstring& cpp : cpps) {
        enqueue_text(read_file(cpp));
    }

    for (size_t q = 0; q < queue.size(); q++) {
        const std::string name = queue[q];
        bool seen = false;
        for (const std::string& have : seen_headers) {
            if (have == name) {
                seen = true;
                break;
            }
        }
        if (seen || builtin_header(name)) {
            continue;
        }
        seen_headers.push_back(name);
        std::wstring wide = widen_header(name);

        auto consider_file = [&](const std::wstring& path) {
            if (file_exists(path)) {
                enqueue_text(read_file(path));
                return true;
            }
            return false;
        };
        if (consider_file(g_include + L"\\" + wide)) {
            continue;
        }
        bool found_here = false;
        for (const std::wstring& dir : include_dirs) {
            if (consider_file(dir + L"\\" + wide)) {
                found_here = true;
                break;
            }
        }
        if (found_here) {
            continue;
        }

        std::wstring header_path;
        std::wstring lib_root;
        for (const std::wstring& lib : libs) {
            std::wstring at_root = lib + L"\\" + wide;
            std::wstring at_src = lib + L"\\src\\" + wide;
            if (file_exists(at_root)) {
                header_path = at_root;
                lib_root = lib;
                break;
            }
            if (file_exists(at_src)) {
                header_path = at_src;
                lib_root = lib;
                break;
            }
        }
        if (lib_root.empty()) {
            continue;
        }
        bool already = false;
        for (const std::wstring& have : seen_libs) {
            if (same_path(have, lib_root)) {
                already = true;
                break;
            }
        }
        add_unique(include_dirs, parent_of(header_path));
        enqueue_text(read_file(header_path));
        if (already) {
            continue;
        }
        seen_libs.push_back(lib_root);
        sim_log_info("library %s", narrow(filename_of(lib_root)).c_str());
        std::wstring source_dir = dir_exists(lib_root + L"\\src") ? lib_root + L"\\src" : lib_root;
        add_unique(include_dirs, source_dir);
        for (const std::wstring& source : list_sources(source_dir)) {
            add_unique(cpps, source);
            enqueue_text(read_file(source));
        }
    }
}

bool declares_loop(const std::string& text) {
    size_t at = 0;
    while ((at = text.find("loop", at)) != std::string::npos) {
        size_t line = text.rfind('\n', at);
        line = line == std::string::npos ? 0 : line + 1;
        std::string row = text.substr(line, at - line);
        if (row.find("void") != std::string::npos) {
            size_t paren = text.find('(', at);
            if (paren != std::string::npos && paren < at + 8) {
                return true;
            }
        }
        at += 4;
    }
    return false;
}

bool build_files(const std::vector<std::wstring>& files) {
    if (g_gxx.empty() || !file_exists(g_gxx)) {
        sim_log_warn("g++ was not found. Set PIXXCRAFT_CXX or install the simulator toolchain.");
        set_status("g++ was not found");
        return false;
    }
    if (!file_exists(g_forward)) {
        sim_log_warn("sketch runtime is missing: %s", narrow(g_forward).c_str());
        set_status("build failed");
        return false;
    }

    std::vector<std::wstring> inos;
    std::vector<std::wstring> cpps;
    std::vector<std::wstring> include_dirs;
    auto add_dir = [&](const std::wstring& dir) {
        if (dir.empty()) {
            return;
        }
        for (const std::wstring& have : include_dirs) {
            if (_wcsicmp(have.c_str(), dir.c_str()) == 0) {
                return;
            }
        }
        include_dirs.push_back(dir);
    };

    for (const std::wstring& file : files) {
        if (!file_exists(file)) {
            sim_log_warn("missing file %s", narrow(file).c_str());
            set_status("build failed");
            return false;
        }
        add_dir(parent_of(file));
        std::wstring ext = extension_of(file);
        if (ext == L".ino") {
            inos.push_back(file);
        } else if (ext == L".cpp" || ext == L".cc" || ext == L".cxx" || ext == L".c") {
            cpps.push_back(file);
        } else if (ext == L".h" || ext == L".hpp") {
            continue;
        } else {
            sim_log_warn("skipped %s", narrow(filename_of(file)).c_str());
        }
    }
    if (inos.empty() && cpps.empty()) {
        sim_log_warn("select a .ino or .cpp file");
        set_status("build failed");
        return false;
    }

    std::wstring primary;
    if (!inos.empty()) {
        primary = filename_of(parent_of(inos[0])) + L".ino";
    }
    std::sort(inos.begin(), inos.end(), [&](const std::wstring& a, const std::wstring& b) {
        bool a_primary = _wcsicmp(filename_of(a).c_str(), primary.c_str()) == 0;
        bool b_primary = _wcsicmp(filename_of(b).c_str(), primary.c_str()) == 0;
        if (a_primary != b_primary) {
            return a_primary;
        }
        return _wcsicmp(filename_of(a).c_str(), filename_of(b).c_str()) < 0;
    });

    CreateDirectoryW(g_work.c_str(), nullptr);
    std::wstring joined = g_work + L"\\joined.ino";
    std::wstring plugin = g_work + L"\\plugin.cpp";
    if (!inos.empty()) {
        std::string body;
        for (const std::wstring& ino : inos) {
            std::string shown = narrow(ino);
            for (char& c : shown) {
                if (c == '\\') {
                    c = '/';
                }
            }
            body += "#line 1 \"";
            body += shown;
            body += "\"\n";
            body += read_file(ino);
            if (body.empty() || body.back() != '\n') {
                body += '\n';
            }
        }
        if (!declares_loop(body)) {
            body += "\nvoid loop() {}\n";
            sim_log_info("sketch has no loop(); an empty one was added");
        }
        if (!write_bytes(joined, body)) {
            sim_log_warn("could not write %s", narrow(joined).c_str());
            set_status("build failed");
            return false;
        }
    }

    std::vector<std::wstring> sketch_dirs;
    for (const std::wstring& ino : inos) {
        add_unique(sketch_dirs, parent_of(ino));
    }
    for (const std::wstring& cpp : cpps) {
        add_unique(sketch_dirs, parent_of(cpp));
    }
    attach_arduino_libraries(sketch_dirs, inos, include_dirs, cpps);

    std::string plugin_src = "#include \"Arduino.h\"\nvoid setup();\nvoid loop();\n";
    if (!inos.empty()) {
        plugin_src += "#include \"joined.ino\"\n";
    }
    plugin_src +=
        "extern \"C\" __declspec(dllexport) void sketch_setup() { setup(); }\n"
        "extern \"C\" __declspec(dllexport) void sketch_loop() { loop(); }\n";
    if (!write_bytes(plugin, plugin_src)) {
        sim_log_warn("could not write %s", narrow(plugin).c_str());
        set_status("build failed");
        return false;
    }

    g_generation++;
    std::wstring dll = g_work + L"\\sketch_" + std::to_wstring(g_generation) + L".dll";
    add_dir(g_work);
    add_dir(g_include);

    std::wstring cmd = L"\"" + g_gxx + L"\" -shared -std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable";
    cmd += L" -Wno-sign-compare -Wno-misleading-indentation -DARDUINO=10819 -finput-charset=UTF-8 -x c++";
    cmd += L" -I\"" + g_include + L"\"";
    for (const std::wstring& dir : include_dirs) {
        if (same_path(dir, g_include)) {
            continue;
        }
        cmd += L" -I\"" + dir + L"\"";
    }
    cmd += L" \"" + g_forward + L"\" \"" + plugin + L"\"";
    for (const std::wstring& cpp : cpps) {
        cmd += L" \"" + cpp + L"\"";
    }
    cmd += L" -static-libgcc -static-libstdc++ -Wl,--export-all-symbols -o \"" + dll + L"\"";

    set_status("compiling...");
    host_present();
    std::string output;
    DWORD code = 1;
    if (!run_compiler(cmd, parent_of(g_gxx), &output, &code)) {
        sim_log_warn("could not start g++");
        set_status("build failed");
        return false;
    }
    if (code != 0) {
        if (output.empty()) {
            sim_log_warn("g++ failed with no output (exit %lu)", code);
        } else {
            log_lines(output, 40);
        }
        set_status("build failed");
        return false;
    }

    std::string label = narrow(filename_of(!inos.empty() ? inos[0] : cpps[0]));
    size_t extra = inos.size() + cpps.size();
    if (extra > 1) {
        label += " +" + std::to_string(extra - 1);
    }
    return activate_dll(dll, label, warning_lines(output));
}

}  // namespace

bool loader_init() {
    if (GetProcAddress(GetModuleHandleW(nullptr), "pixxcraft_api") == nullptr) {
        std::fputs("internal: pixxcraft_api export missing\n", stderr);
        return false;
    }
    std::wstring root = exe_dir();
    g_include = full_path(root + L"\\..\\include");
    g_forward = full_path(root + L"\\..\\src\\sketch_forward.cpp");
    g_work = root + L"\\sketches";
    CreateDirectoryW(g_work.c_str(), nullptr);
    g_gxx = find_gxx();
    if (g_gxx.empty()) {
        set_status("g++ was not found");
        sim_log_warn("g++ was not found. Expected it under %%LOCALAPPDATA%%\\pixxcraft-sim-tools\\w64devkit\\bin");
    } else {
        set_status("no sketch loaded");
        sim_log_info("click Load code, or drop .ino and .cpp files on the window");
        sim_log_info("in a .ino, declare a function before you call it");
        sim_log_info("libraries in Documents\\Arduino\\libraries are compiled with the sketch");
    }
    return true;
}

void loader_queue(const std::vector<std::wstring>& files) {
    if (g_busy || files.empty()) {
        return;
    }
    g_pending.clear();
    for (const std::wstring& file : files) {
        if (!file.empty()) {
            g_pending.push_back(full_path(file));
        }
    }
    if (g_pending.empty()) {
        return;
    }
    g_has_pending = true;
    g_jump = 1;
    if (g_armed && g_jmp != nullptr) {
        longjmp(*g_jmp, 1);
    }
}

void loader_queue_narrow(const std::vector<std::string>& files) {
    std::vector<std::wstring> wide;
    wide.reserve(files.size());
    for (const std::string& file : files) {
        wide.push_back(widen_acp(file));
    }
    loader_queue(wide);
}

bool loader_has_pending() {
    return g_has_pending;
}

void loader_compile_pending() {
    if (!g_has_pending || g_busy) {
        return;
    }
    std::vector<std::wstring> files;
    files.swap(g_pending);
    g_has_pending = false;
    bool resume_previous = g_loaded && g_setup_done;
    g_busy = true;
    bool ok = build_files(files);
    g_busy = false;
    if (!ok && g_loaded && !g_setup_done) {
        unload_sketch();
        set_status("build failed");
    } else if (!ok && resume_previous) {
        set_status("build failed - previous sketch still running");
    }
}

bool loader_loaded() {
    return g_loaded;
}

bool loader_busy() {
    return g_busy;
}

const char* loader_status() {
    return g_status;
}

std::wstring loader_start_dir() {
    std::wstring dir = full_path(exe_dir() + L"\\..\\..\\software\\sketch_oct27a");
    if (dir_exists(dir)) {
        return dir;
    }
    return exe_dir();
}

void loader_arm(jmp_buf* buf) {
    g_jmp = buf;
    g_armed = true;
    g_jump = 0;
}

void loader_disarm() {
    g_armed = false;
    g_jmp = nullptr;
}

bool loader_armed() {
    return g_armed;
}

void loader_run_once() {
    if (!g_loaded || g_setup == nullptr || g_loop == nullptr) {
        return;
    }
    if (!g_setup_done) {
        g_setup();
        g_setup_done = true;
        host_present();
    }
    g_loop();
}

void loader_on_abort() {
    g_armed = false;
    g_jmp = nullptr;
    if (g_jump == 2) {
        unload_sketch();
        sim_log_info("sketch stopped");
        set_status("stopped");
        host_set_title("pixxcraft  XIAO ESP32-S3 + 1.69 TFT");
        return;
    }
    loader_compile_pending();
}

void loader_request_stop() {
    if (g_busy) {
        return;
    }
    if (!g_loaded && !g_armed) {
        set_status("stopped");
        return;
    }
    g_jump = 2;
    g_has_pending = false;
    g_pending.clear();
    if (g_armed && g_jmp != nullptr) {
        longjmp(*g_jmp, 1);
    }
    unload_sketch();
    sim_log_info("sketch stopped");
    set_status("stopped");
    host_set_title("pixxcraft  XIAO ESP32-S3 + 1.69 TFT");
}
