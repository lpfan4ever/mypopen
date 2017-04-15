/*
 * mypopen.c
 * 
 * Copyright 2017 dominik <dominik@dominik-VirtualBox>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include "mypopen.h"


void do_child(void);
void do_parent(const char *type);
static FILE *fp = NULL;//Filepointer, return Value from mypopen
static pid_t childpid;//PID from child
static int fd[2];//Pipe
static int x = 0;
static int y = 0;

FILE *mypopen(const char *command, const char *type)
{
	
	if(fp != NULL)//mypopen in use
	{
		errno = EAGAIN;
		return NULL;
	}
	if((*type != 'r' && *type != 'w') || (type[1] != '\0'))//Argument invalid
	{
		errno = EINVAL;
		return NULL;
	}
	if(*type == 'r')
	{
		x = 0;
		y = 1;
	}
	else if(*type == 'w')
	{
		x = 1;
		y = 0;
	}
	if(pipe(fd) == -1)//Pipe error
	{
		return NULL;
	}
	
	childpid = fork();//create child
	
	if(childpid < 0)//error at create
	{
		close(fd[0]);
		close(fd[1]);
		return NULL;
	}
	else if(childpid == 0)//Child Proces
	{
		do_child();
		(void) execl("/bin/sh","sh","-c",command,(char *) NULL);
		_exit(127);//terminate calling process, 127 command not found
	}
	else if(childpid > 0)//Parent Process
	{
		do_parent(type);
	}
	return(fp);
}
int mypclose(FILE *stream)
{
	pid_t wait_pid;
	int status;
	
	if(fp == NULL)//mypopen set fp
	{
		errno = ECHILD;
		return -1;
	}	
	if((stream == NULL) || (stream != fp))//parameter valid
	{	
		errno = EINVAL;
		return -1;
	}
	if(fclose(stream) == EOF)//failure at fclsoe
	{
		return -1;
	}
	
	while((wait_pid = waitpid(childpid, &status, 0)) != childpid)
	{
		if(wait_pid == -1)//waiting
		{
			if(errno == EINTR)//still wait
			{
				continue;
			}
		}
		//error
		return -1;
	}
	if(WIFEXITED(status))//child terminated normal
	{
		return WEXITSTATUS(status);//child terminated normal
	}
	else
	{
		errno = ECHILD;
		return -1;
	}
}
void do_child(void)
{
	(void)close(fd[x]);
	if(fd[y] != y)
	{
		if(dup2(fd[y],y) == -1)
		{
			_exit(1);
		}
		(void)close(fd[y]);
	}
}
void do_parent(const char *type)
{
	if((fp = fdopen(fd[x], type)) == NULL)
	{
		return NULL;
	}
	(void)close(fd[y]);
}
