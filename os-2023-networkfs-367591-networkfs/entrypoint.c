#include "entrypoint.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Khakimov Ruslan");
MODULE_VERSION("0.01");

int create_impl(struct inode *parent, struct dentry *child, unsigned int flag,
                const char *type) {
  ino_t *response = kmalloc((sizeof(ino_t)), GFP_KERNEL);
  if (!response) {
    printk(KERN_ERR "allocated error");
    return -ENOMEM;
  }
  const char *name = child->d_name.name;
  const char *token = (char *)child->d_sb->s_fs_info;
  const char *method = "create";
  char parent_str[11];
  sprintf(parent_str, "%lu", parent->i_ino);

  int64_t ret =
      networkfs_http_call(token, method, (char *)response, sizeof(ino_t), 3,
                          "parent", parent_str, "name", name, "type", type);

  if (ret != 0) {
    printk(KERN_ERR "create-call error");
    return -1;
  }

  struct inode *inode =
      networkfs_get_inode(parent->i_sb, parent, flag, *response);
  d_add(child, inode);
  kfree(response);
  return 0;
}

int unlink_impl(struct inode *parent, struct dentry *child,
                const char *method) {
  const char *name = child->d_name.name;
  const char *token = (char *)child->d_sb->s_fs_info;
  char parent_str[11];
  sprintf(parent_str, "%lu", parent->i_ino);

  int ret = networkfs_http_call(token, method, NULL, 0, 2, "parent", parent_str,
                                "name", name);
  if (ret != 0) {
    printk(KERN_ERR "unlink-call error");
    return -1;
  }
  return ret;
}

struct dentry *networkfs_lookup(struct inode *parent, struct dentry *child,
                                unsigned int flag) {
  struct entry_info *response = kmalloc(sizeof(struct entry_info), GFP_KERNEL);
  if (!response) {
    printk(KERN_ERR "allocated error");
    return NULL;
  }
  memset(response, 0, sizeof(struct entry_info));
  const char *name = child->d_name.name;
  const char *token = (char *)child->d_sb->s_fs_info;
  const char *method = "lookup";
  char parent_str[11];
  sprintf(parent_str, "%lu", parent->i_ino);
  int64_t ret = networkfs_http_call(token, method, (char *)response,
                                    sizeof(struct entry_info), 2, "parent",
                                    parent_str, "name", name);
  if (ret != 0) {
    printk(KERN_ERR "lookup-call error");
    return NULL;
  }
  umode_t type;
  if (response->entry_type == DT_DIR) {
    type = S_IFDIR;
  } else {
    type = S_IFREG;
  }
  struct inode *inode = networkfs_get_inode(parent->i_sb, parent,
                                            type | S_IRWXUGO, response->ino);
  d_add(child, inode);
  kfree(response);
  return NULL;
}

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct inode *inode2;
  int offset;
  struct entries *response = kmalloc(sizeof(struct entries), GFP_KERNEL);
  if (!response) {
    printk(KERN_ERR "allocated error");
    return -ENOMEM;
  }
  memset(response, 0, sizeof(struct entries));

  inode2 = filp->f_inode;

  offset = ctx->pos;
  ino_t ino = inode2->i_ino;
  const char *token = (char *)inode2->i_sb->s_fs_info;
  const char *method = "list";
  char ino_str[11];
  sprintf(ino_str, "%lu", ino);
  int64_t ret =
      networkfs_http_call(token, method, (char *)response,
                          sizeof(struct entries), 1, "inode", ino_str);
  if (ret != 0) {
    printk(KERN_ERR "list-call error");
    return -1;
  }
  // if (offset >= response->entries_count) {
  //   return 0;
  // }

  loff_t record_counter = 0;

  struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;
  while (true) {
    switch (ctx->pos) {
      case 0:
        dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR);
        break;

      case 1:
        struct inode *parent_inode = dentry->d_parent->d_inode;
        dir_emit(ctx, "..", 2, parent_inode->i_ino, DT_DIR);
        break;

      default:
        if (response->entries_count <= ctx->pos - 2) {
          kfree(response);
          return record_counter;
        }
        struct entry *file = &(response->entries[ctx->pos - 2]);
        dir_emit(ctx, file->name, strlen(file->name), file->ino,
                 file->entry_type);
    }

    ++record_counter;
    ++ctx->pos;

    // dir_emit(ctx, ".", 1, ino, DT_DIR);
    // offset++;
    // ctx->pos++;
    // struct dentry *dentry = filp->f_path.dentry;
    // struct inode *parent_inode = dentry->d_parent->d_inode;
    // dir_emit(ctx, "..", 2, parent_inode->i_ino, DT_DIR);
    // offset++;
    // ctx->pos++;
    // // dir_emit(ctx, "..", 2, ino, DT_DIR);

    // // dir_emit_dot(filp, ctx);
    // // ctx->pos++;
    // // offset++;
    // // dir_emit_dotdot(filp, ctx);
    // // ctx->pos++;
    // // offset++;

    // loff_t record_counter = 0;
    // // for (; offset < response->entries_count; offset++) {
    // //   struct entry curr = response->entries[offset];
    // //   dir_emit(ctx, curr.name, strlen(curr.name), curr.ino,
    // curr.entry_type);
    // //   record_counter++;
    // //   ctx->pos++;
    // // }
    // for (; ctx->pos < response->entries_count; ctx->pos += 1) {
    //   struct entry curr = response->entries[ctx->pos];
    //   dir_emit(ctx, curr.name, strlen(curr.name), curr.ino, curr.entry_type);
    //   record_counter++;
    //   // ctx->pos++;
    // }
    // kfree(response);
    // return response->entries_count;
  }
}

int networkfs_create(struct user_namespace *user_ns, struct inode *parent,
                     struct dentry *child, umode_t mode, bool b) {
  return create_impl(parent, child, S_IFREG | S_IRWXUGO, "file");
}

int networkfs_unlink(struct inode *parent, struct dentry *child) {
  return unlink_impl(parent, child, "unlink");
}

int networkfs_mkdir(struct user_namespace *user_ns, struct inode *parent,
                    struct dentry *child, umode_t mode) {
  return create_impl(parent, child, S_IFDIR | S_IRWXUGO, "directory");
}

int networkfs_rmdir(struct inode *parent, struct dentry *child) {
  return unlink_impl(parent, child, "rmdir");
}

int networkfs_init(void) {
  int ret = register_filesystem(&networkfs_fs_type);
  if (ret != 0) {
    printk(KERN_ERR "networkfs: unable to register filesystem: error code %d",
           ret);
  }
  return ret;
}

void networkfs_exit(void) {
  unregister_filesystem(&networkfs_fs_type);
  printk(KERN_INFO "Goodbye!\n");
}

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *parent, umode_t mode,
                                  int i_ino) {
  struct inode *inode;
  inode = new_inode(sb);

  if (inode != NULL) {
    inode->i_ino = i_ino;
    inode->i_op = &networkfs_inode_ops;
    inode->i_fop = &networkfs_dir_ops;
    inode_init_owner(&init_user_ns, inode, parent, mode);
  }

  return inode;
}

void networkfs_kill_sb(struct super_block *sb) {
  printk(KERN_INFO "token: %s\n", (char *)sb->s_fs_info);

  printk(KERN_INFO "networkfs: superblock is destroyed");
  kfree(sb->s_fs_info);
}

int networkfs_fill_super(struct super_block *sb, struct fs_context *fc) {
  struct inode *inode;

  inode = networkfs_get_inode(sb, NULL, S_IFDIR | S_IRWXUGO, 1000);

  sb->s_root = d_make_root(inode);

  if (sb->s_root == NULL) {
    return -ENOMEM;
  }

  sb->s_fs_info = kstrdup(fc->source, GFP_KERNEL);

  return 0;
}

int networkfs_get_tree(struct fs_context *fc) {
  int ret = get_tree_nodev(fc, networkfs_fill_super);

  if (ret != 0) {
    printk(KERN_ERR "networkfs: unable to mount: error code %d", ret);
  }

  return ret;
}

int networkfs_init_fs_context(struct fs_context *fc) {
  fc->ops = &networkfs_context_ops;
  return 0;
}

module_init(networkfs_init);
module_exit(networkfs_exit);