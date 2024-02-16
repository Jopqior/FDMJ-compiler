#ifndef LIBSYSY64_H
#define LIBSYSY64_H

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdarg.h>

/* Input & output functions */

long long getint(), getch(), getarray(long long a[]);
void putint(long long a), putch(long long a), putarray(long long n, long long a[]);

/* Timing function implementation */

struct timeval _sysy_start, _sysy_end;
#define _SYSY_N 1024
int _sysy_l1[_SYSY_N], _sysy_l2[_SYSY_N];
int _sysy_h[_SYSY_N], _sysy_m[_SYSY_N], _sysy_s[_SYSY_N], _sysy_us[_SYSY_N];
int _sysy_idx;
__attribute((constructor)) void before_main();
__attribute((destructor)) void after_main();
void _sysy_starttime(int lineno);
void _sysy_stoptime(int lineno);

#endif
