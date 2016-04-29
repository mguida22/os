#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "aes-crypt.h"

struct fs_state {
	char* password;
	char* mirror_directory;
};

static void getDaPath(char daPath[PATH_MAX], const char* path)
{
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);
	strcat(daPath, path);
}

static int og_getattr(const char *path, struct stat *stbuf)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = lstat(daPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_access(const char *path, int mask)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = access(daPath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_readlink(const char *path, char *buf, size_t size)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = readlink(daPath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

static int og_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(daPath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int og_mknod(const char *path, mode_t mode, dev_t rdev)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(daPath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(daPath, mode);
	else
		res = mknod(daPath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_mkdir(const char *path, mode_t mode)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = mkdir(daPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_unlink(const char *path)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = unlink(daPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_rmdir(const char *path)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = rmdir(daPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_symlink(const char *from, const char *to)
{
	char fromPath[PATH_MAX];
	getDaPath(fromPath, from);
	char toPath[PATH_MAX];
	getDaPath(toPath, to);
	int res;

	res = symlink(fromPath, toPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_rename(const char *from, const char *to)
{
	char fromPath[PATH_MAX];
	getDaPath(fromPath, from);
	char toPath[PATH_MAX];
	getDaPath(toPath, to);
	int res;

	res = rename(fromPath, toPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_link(const char *from, const char *to)
{
	char fromPath[PATH_MAX];
	getDaPath(fromPath, from);
	char toPath[PATH_MAX];
	getDaPath(toPath, to);
	int res;

	res = link(fromPath, toPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_chmod(const char *path, mode_t mode)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = chmod(daPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_chown(const char *path, uid_t uid, gid_t gid)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = lchown(daPath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_truncate(const char *path, off_t size)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = truncate(daPath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_utimens(const char *path, const struct timespec ts[2])
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(daPath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_open(const char *path, struct fuse_file_info *fi)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = open(daPath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int og_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	(void) offset;
	(void) size;
	char daPath[PATH_MAX];
	getDaPath(daPath, path);

	int res;
	int action = 0;
	(void) fi;

	// check attr to see if it's encrypted
	// decrypt if needed & write to tmp file
	// read from file to buffer and return data
	FILE* daRealOgFile = fopen(daPath, "rb");
	char daTmpPath[PATH_MAX];
	getDaPath(daTmpPath, daPath);
	getDaPath(daTmpPath, "read_suffix");
	FILE* daTmpFile = fopen(daTmpPath, "wb+");

	if (!do_crypt(daRealOgFile, daTmpFile, action, ((struct fs_state *) fuse_get_context()->private_data)->password))
	{
		printf("Encryption got messed up.... sorry :'(\n");
		return -1;
	}

	fseek(daTmpFile, 0, SEEK_END);
	size_t daTmpFileLength = ftell(daTmpFile);
	fseek(daTmpFile, 0, SEEK_SET);

	res = fread(buf, 1, daTmpFileLength, daTmpFile);

	fclose(daRealOgFile);
  fclose(daTmpFile);
  remove(daTmpPath);

  return res;
}

static int og_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int fd;
	int res;

	(void) fi;
	fd = open(daPath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int og_statfs(const char *path, struct statvfs *stbuf)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res;

	res = statvfs(daPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int og_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	(void) fi;

	int res;
	res = creat(daPath, mode);
	if(res == -1)
		return -errno;

	close(res);

	return 0;
}

static int og_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int og_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int og_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res = lsetxattr(daPath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int og_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res = lgetxattr(daPath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int og_listxattr(const char *path, char *list, size_t size)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res = llistxattr(daPath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int og_removexattr(const char *path, const char *name)
{
	char daPath[PATH_MAX];
	getDaPath(daPath, path);
	int res = lremovexattr(daPath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations og_oper = {
	.getattr	= og_getattr,
	.access		= og_access,
	.readlink	= og_readlink,
	.readdir	= og_readdir,
	.mknod		= og_mknod,
	.mkdir		= og_mkdir,
	.symlink	= og_symlink,
	.unlink		= og_unlink,
	.rmdir		= og_rmdir,
	.rename		= og_rename,
	.link		= og_link,
	.chmod		= og_chmod,
	.chown		= og_chown,
	.truncate	= og_truncate,
	.utimens	= og_utimens,
	.open		= og_open,
	.read		= og_read,
	.write		= og_write,
	.statfs		= og_statfs,
	.create         = og_create,
	.release	= og_release,
	.fsync		= og_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= og_setxattr,
	.getxattr	= og_getxattr,
	.listxattr	= og_listxattr,
	.removexattr	= og_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);

	if (argc < 4) {
		printf("Invalid usage: ./pa5-encfs <password> <mirror directory> <mount point>\n");
		exit(EXIT_FAILURE);
	}

	// idea from here --> http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/
	struct fs_state *fs_data = malloc(sizeof *fs_data);

	fs_data->password = argv[1];
	fs_data->mirror_directory = realpath(argv[2], NULL);

	argv[1] = argv[3];
	argv[2] = argv[4];
	argv[3] = NULL;
	argv[4] = NULL;

	return fuse_main(argc-2, argv, &og_oper, fs_data);
}
