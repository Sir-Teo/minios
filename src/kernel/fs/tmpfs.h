#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

/**
 * Initialize tmpfs and create test files
 */
void tmpfs_init(void);

/**
 * Create a tmpfs file node
 *
 * @param name File name
 * @return VFS node, or NULL on error
 */
vfs_node_t *tmpfs_create_file(const char *name);

#endif // TMPFS_H
