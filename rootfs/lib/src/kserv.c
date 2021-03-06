#include "kserv.h"
#include <ipc.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int kserv_register(const char* reg_name) {
	return syscall1(SYSCALL_KSERV_REG, (int)reg_name);
}

int kserv_run(kserv_func_t servFunc, void* p) {
  while(true) {
    package_t* pkg = ipc_roll();
    if(pkg != NULL) {
      servFunc(pkg, p);
      free(pkg);
    }
    else
      sleep(0);
  }
	return 0;
}

int kserv_ready() {
	return syscall0(SYSCALL_KSERV_READY);
}

int kserv_get_by_name(const char* reg_name) {
	return syscall1(SYSCALL_KSERV_GET_BY_NAME, (int)reg_name);
}

int kserv_get_by_pid(int32_t pid) {
	return syscall1(SYSCALL_KSERV_GET_BY_PID, pid);
}

void kserv_wait_by_name(const char* reg_name) {
	while(kserv_get_by_name(reg_name) < 0)
		sleep(0);
}

void kserv_wait_by_pid(int32_t pid) {
	while(kserv_get_by_pid(pid) < 0)
		sleep(0);
}
