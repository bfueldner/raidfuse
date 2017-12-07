/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <string>
#include <vector>

#include <raidfuse/drive.hpp>

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

std::vector<std::string> entries;

int raid_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
//	if (strcmp(path, hello_path) == 0)
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
		stbuf->st_atime = 0;
	}
/*
	else
		res = -ENOENT;
*/
	return res;
}

int raid_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
	{
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (std::string &entry: entries)
	{
		filler(buf, entry.c_str(), NULL, 0);
	}

	return 0;
}

int raid_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

int raid_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;

	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len)
	{
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	}
	else
		size = 0;

	return size;
}

fuse_operations fuse_callback;

int main(int argc, char **argv)
{
	fuse_callback.getattr = raid_getattr;
	fuse_callback.readdir = raid_readdir;
	fuse_callback.open = raid_open;
	fuse_callback.read = raid_read;

	entries.push_back("disk");
	entries.push_back("partition1");
	entries.push_back("partition2");

	return fuse_main(argc, argv, &fuse_callback, NULL);
}
