/*
 * syscalls.c
 *
 *  Created on: Dec 18, 2019
 *      Author: lukasz
 */


#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#include <fcntl.h>
register char * stack_ptr asm("sp");

caddr_t _sbrk(int incr)
{
	extern char end asm("end");
	static char *heap_end;
	char *prev_heap_end,*min_stack_ptr;

	if (heap_end == 0)
		heap_end = &end;

	prev_heap_end = heap_end;

	/* Use the NVIC offset register to locate the main stack pointer. */
	min_stack_ptr = (char*)(*(unsigned int *)*(unsigned int *)0xE000ED08);
	/* Locate the STACK bottom address */
	extern unsigned long long _Min_Stack_Size;
	min_stack_ptr -= (char*)(&_Min_Stack_Size);

	if (heap_end + incr > min_stack_ptr)
	{
		errno = ENOMEM;
		return (caddr_t) -1;
	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}

extern UART_HandleTypeDef huart3;

int _write(int file, char *ptr, int len)
{
	if (file == STDOUT_FILENO || file == STDERR_FILENO)
	{
		HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, 0xFFFFFFFF);

		return len;
	}
	else
	{
		return -1;
	}
}
