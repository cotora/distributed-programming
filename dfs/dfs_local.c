#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

static void *
dfuse_init(struct fuse_conn_info *conn)
{
        return (NULL);
}

static int
dfuse_getattr(const char *path, struct stat *st)
{
        int ret;

        ret = lstat(path, st);
        if (ret == -1)
                return (-errno);
        return (0);
}

static int
dfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
        DIR *dp;
        struct dirent *dent;
        struct stat sb;

        dp = opendir(path);
        if (dp == NULL)
                return (-errno);
        while ((dent = readdir(dp)) != NULL) {
                memset(&sb, 0, sizeof(sb));
                sb.st_ino = dent->d_ino;
                sb.st_mode = dent->d_type << 12;
                if (filler(buf, dent->d_name, &sb, 0))
                        break;
        }
        closedir(dp);
        return (0);
}

static int
dfuse_open(const char *path, struct fuse_file_info *fi)
{
        int ret;

        ret = open(path, fi->flags);
        if (ret == -1)
                return (-errno);
        fi->fh = ret;
        return (0);
}

static int
dfuse_read(const char *path, char *buf, size_t size, off_t off,
        struct fuse_file_info *fi)
{
        int ret, fd;

        if (fi == NULL)
                fd = open(path, O_RDONLY);
        else
                fd = fi->fh;
        if (fd == -1)
                return (-errno);
        ret = pread(fd, buf, size, off);
        if (ret == -1)
                ret = -errno;
        if (fi == NULL)
                close(fd);
        return (ret);
}

static const struct fuse_operations dfs_op = {
        .init           = dfuse_init,
        .getattr        = dfuse_getattr,
        .readdir        = dfuse_readdir,
        .open           = dfuse_open,
        .read           = dfuse_read,
};

int
main(int argc, char *argv[])
{
        int ret;
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        ret = fuse_main(args.argc, args.argv, &dfs_op, NULL);
        fuse_opt_free_args(&args);
        return (ret);
}