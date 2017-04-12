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

//static FILE *mypopen(const char *command, const char *type);
//static int mypclose(FILE *stream);
static void do_child(const char *type);
static void do_parent(const char *type);
static FILE *fp = NULL;
static pid_t childpid;
static int fd[2];
static int x = 0;
static int y = 0;

static FILE *mypopen(const char *command, const char *type)
{
	//int fd[2];
	
	if(fp != NULL)
	{
		errno = EAGAIN;
		return NULL;
	}
	if(*type != 'r' && *type != 'w' && type[1] == '\0')
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
	if(pipe(fd) == -1)
	{
		return NULL;
	}
	
	childpid = fork();
	
	if(childpid < 0)
	{
		close(fd[0]);
		close(fd[1]);
		return NULL;
	}
	else if(childpid == 0)//Child Proces
	{
		if(*type == 'r')
		{
			(void) close(fd[0]);
			if(fd[1] != STDOUT_FILENO)
			{
				if(dup2(fd[1], STDOUT_FILENO) == -1)
				{
					_exit(EXIT_FAILURE);
				}
				(void) close(fd[1]);
			}
		}
		else if(*type == 'w')
		{
			(void )close(fd[1]);
			if(fd[0] != STDIN_FILENO)
			{
				if(dup2(fd[0], STDIN_FILENO) == -1)
				{
					_exit(EXIT_FAILURE);
				}
				(void) close(fd[0]);
			}
		}
		(void) execl("/bin/sh","sh","-c",command,(char *) NULL);
	}
	else if(childpid > 0)//Parent Process
	{
		if(*type == 'r')
		{
			if((fp = fdopen(fd[0], type)) == NULL)
			{
				return NULL;
			}
			(void) close(fd[1]);
		}
		else if(*type == 'w')
		{
			if((fp = fdopen(fd[1], type)) == NULL)
			{
				return NULL;
			}
			(void) close(fd[0]);
		}
	}
	return(fp);
}
static int mypclose(FILE *stream)
{
	pid_t child_pid;
	pid_t wait_pid;
	int status;
	
	if(fp == NULL || fp != stream)//mypopen set fp
	{
		errno = ECHILD;
		return -1;
	}	
	if(stream == NULL)//parameter valid
	{	
		errno = EINVAL;
		return -1;
	}
	if(fclose(stream) == EOF)
	{
		return -1;
	}
	
	while((wait_pid = waitpid(child_pid, &status, 0)) != child_pid)
	{
		if(wait_pid == -1)
		{
			if(errno == EINTR)
			{
				continue;
			}
		}
		//error
		return -1;
	}
	if(WIFEXITED(status))
	{
		return WEXITSTATUS(status);//child terminated normal
	}
	else
	{
		errno = ECHILD;
		return -1;
	}
}
static void do_child(const char *type)
{
	(void)close(fd[x]);
	if(fd[y] != y)
	{
		if(dup2(fd[y],y) == -1)
		{
			_exit(EXIT_FAILURE);
		}
		(void)close(fd[y]);
	}
}
static void do_parent(const char *type)
{
	if((fp = fdopen(fd[x], type)) == NULL)
	{
		return NULL;
	}
	(void)close(fd[y]);
}
