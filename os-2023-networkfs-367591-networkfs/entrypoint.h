#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "http.h"

struct entries {
  size_t entries_count;
  struct entry {
    unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
    ino_t ino;
    char name[256];
  } entries[16];
};

struct entry_info {
  unsigned char entry_type;
  ino_t ino;
};

struct inode *networkfs_get_inode(struct super_block *, const struct inode *,
                                  umode_t, int);

void networkfs_kill_sb(struct super_block *);

int networkfs_fill_super(struct super_block *, struct fs_context *);

int networkfs_get_tree(struct fs_context *);

int networkfs_init_fs_context(struct fs_context *);

struct dentry *networkfs_lookup(struct inode *, struct dentry *, unsigned int);

int networkfs_create(struct user_namespace *, struct inode *, struct dentry *,
                     umode_t, bool);

int networkfs_unlink(struct inode *, struct dentry *);

int networkfs_iterate(struct file *, struct dir_context *);

int networkfs_mkdir(struct user_namespace *, struct inode *, struct dentry *,
                    umode_t);

int networkfs_rmdir(struct inode *, struct dentry *);

struct file_system_type networkfs_fs_type = {
    .name = "networkfs",
    .kill_sb = networkfs_kill_sb,
    .init_fs_context = networkfs_init_fs_context};

struct fs_context_operations networkfs_context_ops = {
    .get_tree = networkfs_get_tree,
};

struct inode_operations networkfs_inode_ops = {
    .lookup = networkfs_lookup,
    .create = networkfs_create,
    .unlink = networkfs_unlink,
    .mkdir = networkfs_mkdir,
    .rmdir = networkfs_rmdir,
};

struct file_operations networkfs_dir_ops = {
    .iterate = networkfs_iterate,
};