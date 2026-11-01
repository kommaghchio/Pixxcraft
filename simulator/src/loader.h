#pragma once

#include <setjmp.h>

#include <string>
#include <vector>

bool loader_init();
void loader_queue(const std::vector<std::wstring>& files);
void loader_queue_narrow(const std::vector<std::string>& files);
bool loader_has_pending();
void loader_compile_pending();
bool loader_loaded();
bool loader_busy();
const char* loader_status();
std::wstring loader_start_dir();

void loader_arm(jmp_buf* buf);
void loader_disarm();
bool loader_armed();
void loader_run_once();
void loader_on_abort();
void loader_request_stop();
