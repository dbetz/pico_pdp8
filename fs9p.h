//
// simple test program for 9p access to host files
// Written by Eric R. Smith, Total Spectrum Software Inc.
// Released into the public domain.
//

#ifndef FS9P_H
#define FS9P_H

#define _NAME_MAX (64)

#define NOTAG 0xffffU
#define NOFID 0xffffffffU

#define QTDIR 0x80

enum {
    t_version = 100,
    r_version,
    t_auth = 102,
    r_auth,
    t_attach = 104,
    r_attach,
    t_error = 106,
    r_error,
    t_flush = 108,
    r_flush,
    t_walk = 110,
    r_walk,
    t_open = 112,
    r_open,
    t_create = 114,
    r_create,
    t_read = 116,
    r_read,
    t_write = 118,
    r_write,
    t_clunk = 120,
    r_clunk,
    t_remove = 122,
    r_remove,
    t_stat = 124,
    r_stat,
};

// maximum length we're willing to send/receive from host
// write: 4 + 1 + 2 + 4 + 8 + 4 + 1024 = 1048

#define MAXLEN 1048

/* generic file buffer code */
/* put a "struct _default_buffer" at the start of your vfsdata to use the
 * default versions of putc and getc
 */
#define _DEFAULT_BUFSIZ 128

struct _default_buffer {
    int cnt;
    unsigned char *ptr;
    unsigned flags;
    unsigned char buf[_DEFAULT_BUFSIZ];
};

// functions for the 9p file system
typedef struct fsfile {
    struct _default_buffer b;
    uint32_t offlo;
    uint32_t offhi;
} fs9_file;

// send/receive function; sends a buffer to the host
// and reads a reply back
typedef int (*sendrecv_func)(uint8_t *startbuf, uint8_t *endbuf, int maxlen);

// initialize
int fs_init(sendrecv_func fn);

// walk a file from fid "dir" along path, creating fid "newfile"
int fs_walk(fs9_file *dir, fs9_file *newfile, const char *path);

// open a file f using path "path" (relative to root directory)
// for reading or writing
int fs_open(fs9_file *f, const char *path, int fs_mode);

#define FS_MODE_READ 0
#define FS_MODE_WRITE 1
#define FS_MODE_RDWR 2
#define FS_MODE_TRUNC 16

// create a new file if necessary, or truncate an existing one
int fs_create(fs9_file *f, const char *path, uint32_t permissions);
    
// close a file
int fs_close(fs9_file *f);

// read/write data
int fs_read(fs9_file *f, uint8_t *buf, int count);
int fs_write(fs9_file *f, const uint8_t *buf, int count);

// seek
int fs_seek(fs9_file *f, uint64_t off);

#endif
