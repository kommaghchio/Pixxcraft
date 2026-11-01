#pragma once

void sim_log_info(const char* fmt, ...);
void sim_log_warn(const char* fmt, ...);
void sim_info_once(const char* key, const char* fmt, ...);
void sim_warn_once(const char* key, const char* fmt, ...);

void sim_log_clear();
int sim_warning_count();
int sim_log_count();
const char* sim_log_line(int oldest_index);
int sim_log_level(int oldest_index);
