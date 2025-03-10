/**
 *
 * cpulimit - a CPU limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <angelo dot marletta at gmail dot com> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/vfs.h>

static int check_proc()
{
	struct statfs mnt;
	if (statfs("/proc", &mnt) < 0)
		return 0;
	if (mnt.f_type != 0x9fa0)
		return 0;
	return 1;
}

int init_process_iterator(struct process_iterator *it, struct process_filter *filter)
{
	if (!check_proc())
	{
		fprintf(stderr, "procfs is not mounted!\nAborting\n");
		exit(-2);
	}
	//open a directory stream to /proc directory
	if ((it->dip = opendir("/proc")) == NULL)
	{
		perror("opendir");
		return -1;
	}
	it->filter = filter;
	return 0;
}

static int read_process_info(pid_t pid, struct process *p)
{
	static char buffer[1024];
	static char statfile[32];
	static char exefile[1024];
	p->pid = pid;
	FILE *fd;
	//read command line
	sprintf(exefile, "/proc/%d/cmdline", p->pid);
	fd = fopen(exefile, "r");
	if (fd == NULL)
		goto error_out1;
	if (fgets(buffer, sizeof(buffer), fd) == NULL)
	{
		fclose(fd);
		goto error_out1;
	}
	fclose(fd);
	strcpy(p->command, buffer);

	//read stat file
	sprintf(statfile, "/proc/%d/stat", p->pid);
	fd = fopen(statfile, "r");
	if (fd == NULL)
		goto error_out2;
	if (fgets(buffer, sizeof(buffer), fd) == NULL)
	{
		fclose(fd);
		goto error_out2;
	}
	fclose(fd);
	// pid
	char *token = strtok(buffer, " ");
	if (token == NULL)
		goto error_out2;
	// comm
	token = strtok(NULL, " ");
	if (token == NULL)
		goto error_out2;
	if (token[0] == '(')
	{
		while (token[strlen(token) - 1] != ')')
		{
			token = strtok(NULL, " ");
			if (token == NULL)
				goto error_out2;
		}
	}
	// state
	token = strtok(NULL, " ");
	if (token == NULL)
		goto error_out2;
	if (strcmp(token, "Z") == 0 || strcmp(token, "X") == 0)
		goto error_out2;
	// ppid
	token = strtok(NULL, " ");
	if (token == NULL)
		goto error_out2;
	p->ppid = atoi(token);
	int i;
	for (i = 0; i < 10; i++)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
			goto error_out2;
	}
	p->cputime = atoi(token) * 1000 / HZ;
	token = strtok(NULL, " ");
	if (token == NULL)
		goto error_out2;
	p->cputime += atoi(token) * 1000 / HZ;
	for (i = 0; i < 7; i++)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
			goto error_out2;
	}
	p->starttime = atoi(token) / sysconf(_SC_CLK_TCK);
	return 0;

error_out1:
	p->command[0] = '\0';
error_out2:
	return -1;
}

static pid_t getppid_of(pid_t pid)
{
	char statfile[20];
	char buffer[1024];
	sprintf(statfile, "/proc/%d/stat", pid);
	FILE *fd = fopen(statfile, "r");
	if (fd == NULL)
		return -1;
	if (fgets(buffer, sizeof(buffer), fd) == NULL)
	{
		fclose(fd);
		return -1;
	}
	fclose(fd);
	// pid
	char *token = strtok(buffer, " ");
	if (token == NULL)
		return -1;
	// comm
	token = strtok(NULL, " ");
	if (token == NULL)
		return -1;
	if (token[0] == '(')
	{
		while (token[strlen(token) - 1] != ')')
		{
			token = strtok(NULL, " ");
			if (token == NULL)
				return -1;
		}
	}
	// state
	token = strtok(NULL, " ");
	if (token == NULL)
		return -1;
	// ppid
	token = strtok(NULL, " ");
	if (token == NULL)
		return -1;
	return atoi(token);
}

static int is_child_of(pid_t child_pid, pid_t parent_pid)
{
	int ppid = child_pid;
	while (ppid > 1 && ppid != parent_pid)
	{
		ppid = getppid_of(ppid);
	}
	return ppid == parent_pid;
}

int get_next_process(struct process_iterator *it, struct process *p)
{
	if (it->dip == NULL)
	{
		//end of processes
		return -1;
	}
	if (it->filter->pid != 0 && !it->filter->include_children)
	{
		int ret = read_process_info(it->filter->pid, p);
		closedir(it->dip);
		it->dip = NULL;
		if (ret != 0)
			return -1;
		return 0;
	}
	struct dirent *dit = NULL;
	//read in from /proc and seek for process dirs
	while ((dit = readdir(it->dip)) != NULL)
	{
		if (strtok(dit->d_name, "0123456789") != NULL)
			continue;
		p->pid = atoi(dit->d_name);
		if (it->filter->pid != 0 && it->filter->pid != p->pid && !is_child_of(p->pid, it->filter->pid))
			continue;
		if (it->filter->pid == 0)
		{
			read_process_info(p->pid, p);
			break;
		}
		else
		{
			if (read_process_info(p->pid, p))
				break;
		}
	}
	if (dit == NULL)
	{
		//end of processes
		closedir(it->dip);
		it->dip = NULL;
		return -1;
	}
	return 0;
}

int close_process_iterator(struct process_iterator *it)
{
	if (it->dip != NULL && closedir(it->dip) == -1)
	{
		perror("closedir");
		return 1;
	}
	it->dip = NULL;
	return 0;
}
