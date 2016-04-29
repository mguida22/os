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

static char* makeDaFilePath(char* prefix, char* daPath)
{
	char* daNewName = (char *) malloc(1 + strlen(prefix) + strlen(daPath));
	strcpy(daNewName, prefix);
	strcat(daNewName, daPath);

	return daNewName;
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = lstat(daPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = access(daPath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = readlink(daPath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

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

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

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

static int xmp_mkdir(const char *path, mode_t mode)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = mkdir(daPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = unlink(daPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = rmdir(daPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = chmod(daPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = lchown(daPath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = truncate(daPath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

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

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = open(daPath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	(void) path;
	(void) offset;
	(void) size;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;
	int action = -1;
	(void) fi;

	// check attr to see if it's encrypted
	// decrypt if needed & write to tmp file
	// read from file to buffer and return data
	FILE* daRealOgFile = fopen(daPath, "rb");
	char* daTmpPath = makeDaFilePath(daPath, "pimpinReadPrefix");
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

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

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

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res;

	res = statvfs(daPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	(void) fi;

	int res;
	res = creat(daPath, mode);
	if(res == -1)
		return -errno;

	close(res);

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
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
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res = lsetxattr(daPath, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res = lgetxattr(daPath, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res = llistxattr(daPath, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	(void) path;
	char daPath[PATH_MAX];
	strcpy(daPath, ((struct fs_state *) fuse_get_context()->private_data)->mirror_directory);

	int res = lremovexattr(daPath, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
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

	return fuse_main(argc-2, argv, &xmp_oper, fs_data);
}
