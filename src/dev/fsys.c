/**
 * Kernel calls for file system access
 *
 * This version will be a wrapper around the FATfs file system.
 * In theory, there could be other implementations.
 *
 */

#include <ctype.h>
#include <string.h>
#include "log.h"
#include "syscalls.h"
#include "fsys.h"
#include "fatfs/ff.h"
#include "dev/channel.h"
#include "errors.h"

#define MAX_DRIVES      8       /* Maximum number of drives */
#define MAX_DIRECTORIES 8       /* Maximum number of open directories */
#define MAX_FILES       8       /* Maximum number of open files */
#define MAX_LOADERS     10      /* Maximum number of file loaders */
#define MAX_EXT         4

/*
 * Types
 */

typedef struct s_loader_record {
    unsigned char status;                   /* Is the loader registered or not */
    char extension[MAX_EXT];                /* The file extension for this file loader */
    p_file_loader loader;                   /* Pointer to the loader */
} t_loader_record, *p_loader_record;

/**
 * Module variables
 */

FATFS g_drive[MAX_DRIVES];                  /* File system for each logical drive */
unsigned char g_dir_state[MAX_DIRECTORIES]; /* Whether or not a directory record is allocated */
DIR g_directory[MAX_DIRECTORIES];           /* The directory information records */
unsigned char g_fil_state[MAX_FILES];       /* Whether or not a file descriptor is allocated */
FIL g_file[MAX_FILES];                      /* The file descriptors */
t_dev_chan g_file_dev;                      /* The descriptor to use for the file channels */
t_loader_record g_file_loader[MAX_LOADERS]; /* Array of file types the loader will understand */

/**
 * Convert a FATFS FRESULT code to the Foenix kernel's internal error codes
 *
 * Inputs:
 * r = the FRESULT code from a FATFS call
 *
 * Output:
 * The corresponding Foenix kernel error code (0 on success)
 */
short fatfs_to_foenix(FRESULT r) {
    if (r == 0) {
        return 0;
    } else {
        /* TODO: flesh this out */
        log_num(LOG_DEBUG, "FATFS: ", r);
        return -1;
    }
}

/**
 * Attempt to open a file given the path to the file and the mode.
 *
 * Inputs:
 * path = the ASCIIZ string containing the path to the file.
 * mode = the mode (e.g. r/w/create)
 *
 * Returns:
 * the channel ID for the open file (negative if error)
 */
short fsys_open(const char * path, short mode) {
    p_channel chan = 0;
    short i, fd = -1;

    /* Allocate a file handle */

    for (i = 0; i < MAX_FILES; i++) {
        if (g_fil_state[i] == 0) {
            g_fil_state[i] = 1;
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        return ERR_OUT_OF_HANDLES;
    }

    /* Allocate a channel */

    chan = chan_alloc();
    if (chan) {
        chan->dev = CDEV_FILE;
        FRESULT result = f_open(&g_file[fd], path, mode);
        if (result == 0) {
            chan->data[0] = fd & 0xff;      /* file handle in the channel data block */
            return chan->number;
        } else {
            /* There was an error... deallocate the channel and file descriptor */
            g_fil_state[fd] = 0;
            chan_free(chan);
            return fatfs_to_foenix(result);
        }

    } else {
        /* We couldn't allocate a channel... return our file descriptor */
        g_fil_state[fd] = 0;
        return ERR_OUT_OF_HANDLES;
    }
}

/**
 * Close access to a previously open file.
 *
 * Inputs:
 * c = the channel ID for the file
 *
 * Returns:
 * 0 on success, negative number on failure
 */
short fsys_close(short c) {
    p_channel chan = 0;
    short fd = 0;

    chan = chan_get_record(c);          /* Get the channel record */
    fd = chan->data[0];                 /* Get the file descriptor number */

    f_close(&g_file[fd]);               /* Close the file in FATFS */
    chan_free(chan);                    /* Return the channel to the pool */
    g_fil_state[fd] = 0;                /* Return the file descriptor to the pool. */

    return 0;
}

/**
 * N.B.: fsys_open returns a channel ID, and fsys_close accepts a channel ID.
 * read and write access, seek, eof status, etc. will be handled by the channel
 * calls.
 */

/**
 * Attempt to open a directory for scanning
 *
 * Inputs:
 * path = the path to the directory to open
 *
 * Returns:
 * the handle to the directory if >= 0. An error if < 0
 */
short fsys_opendir(const char * path) {
    short i;
    short dir = -1;
    FRESULT fres;

    /* Allocate a directory handle */
    for (i = 0; i < MAX_DIRECTORIES; i++) {
        if (g_dir_state[i] == 0) {
            dir = i;
            break;
        }
    }

    if (dir < 0) {
        return ERR_OUT_OF_HANDLES;
    } else {
        /* Try to open the directory */
        fres = f_opendir(&g_directory[dir], path);
        if (fres != FR_OK) {
            /* If there was a problem, return an error number */
            return fatfs_to_foenix(fres);
        } else {
            /* Otherwise, allocate and return the handle */
            g_dir_state[dir] = 1;
            return dir;
        }
    }
}

/**
 * Close a previously open directory
 *
 * Inputs:
 * dir = the directory handle to close
 *
 * Returns:
 * 0 on success, negative number on error
 */
short fsys_closedir(short dir) {
    if (g_dir_state[dir]) {
        /* Close and deallocate the handle */
        f_closedir(&g_directory[dir]);
        g_dir_state[dir] = 0;
    }

    return 0;
}

/**
 * Attempt to read an entry from an open directory
 *
 * Inputs:
 * dir = the handle of the open directory
 * file = pointer to the t_file_info structure to fill out.
 *
 * Returns:
 * 0 on success, negative number on failure
 */
short fsys_readdir(short dir, p_file_info file) {
    FILINFO finfo;

    if (g_dir_state[dir]) {
        FRESULT fres = f_readdir(&g_directory[dir], &finfo);
        if (fres != FR_OK) {
            return fatfs_to_foenix(fres);
        } else {
            int i;

            /* Copy file information into the kernel table */
            file->size = finfo.fsize;
            file->date = finfo.fdate;
            file->time = finfo.ftime;
            file->attributes = finfo.fattrib;

            for (i = 0; i < MAX_PATH_LEN; i++) {
                file->name[i] = finfo.fname[i];
                if (file->name[i] == 0) {
                    break;
                }
            }

            return 0;
        }
    } else {
        return ERR_BAD_HANDLE;
    }
}

/**
 * Open a directory given the path and search for the first file matching the pattern.
 *
 * Inputs:
 * path = the path to the directory to search
 * pattern = the file name pattern to search for
 * file = pointer to the t_file_info structure to fill out
 *
 * Returns:
 * the directory handle to use for subsequent calls if >= 0, error if negative
 */
short fsys_findfirst(const char * path, const char * pattern, p_file_info file) {
    return -1;
}

/**
 * Open a directory given the path and search for the first file matching the pattern.
 *
 * Inputs:
 * dir = the handle to the directory (returned by fsys_findfirst) to search
 * file = pointer to the t_file_info structure to fill out
 *
 * Returns:
 * 0 on success, error if negative
 */
short fsys_findnext(short dir, p_file_info file) {
    return -1;
}

/**
 * Create a directory
 *
 * Inputs:
 * path = the path of the directory to create.
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_mkdir(const char * path) {
    return -1;
}

/**
 * Delete a file or directory
 *
 * Inputs:
 * path = the path of the file or directory to delete.
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_delete(const char * path) {
    return -1;
}

/**
 * Rename a file or directory
 *
 * Inputs:
 * old_path = the current path to the file
 * new_path = the new path for the file
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_rename(const char * old_path, const char * new_path) {
    return -1;
}

/**
 * Change the current working directory (and drive)
 *
 * Inputs:
 * path = the path that should be the new current
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_setcwd(const char * path) {
    return -1;
}

/**
 * Get the current working drive and directory
 *
 * Inputs:
 * path = the buffer in which to store the directory
 * size = the size of the buffer in bytes
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_getcwd(char * path, short size) {
    return -1;
}

short fchan_init() {
    /* Nothing needed here */
    return 0;
}

/**
 * Channel driver routines for files.
 */
FIL * fchan_to_file(t_channel * chan) {
    short fd;

    fd = chan->data[0];         /* Get the file descriptor number */
    if (fd < MAX_FILES) {
        return &g_file[fd];     /* Return the pointer to the file descriptor */
    } else {
        return 0;               /* Return NULL if fd is out of range */
    }
}

/**
 * Read a a buffer from the device
 */
short fchan_read(t_channel * chan, unsigned char * buffer, short size) {
    FIL * file;
    FRESULT result;
    int total_read;

    log(LOG_TRACE, "fchan_read");

    file = fchan_to_file(chan);
    if (file) {
        result = f_read(file, buffer, size, &total_read);
        if (result == FR_OK) {
            return (short)total_read;
        } else {
            return fatfs_to_foenix(result);
        }
    }

    return ERR_BADCHANNEL;
}

/**
 * Read a line of text from the device
 */
short fchan_readline(t_channel * chan, unsigned char * buffer, short size) {
    FIL * file;
    char * result;
    int total_read;

    file = fchan_to_file(chan);
    if (file) {
        result = f_gets(buffer, size, file);
        if (result) {
            return strlen(buffer);
        } else {
            return fatfs_to_foenix(f_error(file));
        }
    }

    return ERR_BADCHANNEL;
}

/**
 * read a single byte from the device
 */
short fchan_read_b(t_channel * chan) {
    return 0;
}

/**
 * Write a buffer to the device
 */
short fchan_write(p_channel chan, const unsigned char * buffer, short size) {
    FIL * file;
    FRESULT result;
    int total_written;

    file = fchan_to_file(chan);
    if (file) {
        result = f_write(file, buffer, size, &total_written);
        if (result == FR_OK) {
            return (short)total_written;
        } else {
            return fatfs_to_foenix(result);
        }
    }

    return ERR_BADCHANNEL;
}

/**
 * Write a single unsigned char to the device
 */
short fchan_write_b(t_channel * chan, const unsigned char b) {
    FIL * file;
    FRESULT result;
    int total_written;
    unsigned char buffer[1];

    file = fchan_to_file(chan);
    if (file) {
        buffer[0] = b;
        result = f_write(file, buffer, 1, &total_written);
        if (result == FR_OK) {
            return (short)total_written;
        } else {
            return fatfs_to_foenix(result);
        }
    }

    return ERR_BADCHANNEL;
}

/**
 * Get the status of the device
 */
short fchan_status(t_channel * chan) {
    FIL * file;
    FRESULT result;
    int total_written;

    file = fchan_to_file(chan);
    if (file) {
        short status = 0;

        if (f_eof(file)) {
            status |= CDEV_STAT_EOF;
        }

        if (f_error(file) != FR_OK) {
            status |= CDEV_STAT_ERROR;
        }

        return status;
    }

    return ERR_BADCHANNEL;
}

/**
 * Ensure that any pending writes to the device have been completed
 */
short fchan_flush(t_channel * chan) {
    FIL * file;
    FRESULT result;
    int total_written;

    file = fchan_to_file(chan);
    if (file) {
        result = f_sync(file);
        return fatfs_to_foenix(result);
    }

    return ERR_BADCHANNEL;
}

/**
 * attempt to move the "cursor" position in the channel
 */
short fchan_seek(t_channel * chan, long position, short base) {
    FIL * file;
    FRESULT result;
    int total_written;

    file = fchan_to_file(chan);
    if (file) {
        if (base == CDEV_SEEK_ABSOLUTE) {
            result = f_lseek(file, position);
            return fatfs_to_foenix(result);
        } else if (base == CDEV_SEEK_RELATIVE) {
            long current = f_tell(file);
            result = f_lseek(file, current + position);
            return fatfs_to_foenix(result);
        }
    }

    return ERR_BADCHANNEL;
}

/**
 * Issue a control command to the device
 */
short fchan_ioctrl(t_channel * chan, short command, unsigned char * buffer, short size) {
    return 0;
}

/*
 * Mount, or remount the block device
 *
 * For the hard drive, this need be called only once, but for removable
 * devices, this should be called whenever the media has changed.
 *
 * Inputs:
 * bdev = the number of the block device to mount or re-mount
 *
 * Returns:
 * 0 on success, any other number is an error
 */
short fsys_mount(short bdev) {
    FRESULT fres;
    char drive[3];
    drive[0] = '0' + (char)bdev;
    drive[1] = ':';
    drive[2] = 0;

    fres = f_mount(&g_drive[bdev], drive, 0);
    if (fres != FR_OK) {
        DEBUG("Unable to mount drive:");
        DEBUG(drive);
        return fres;
    } else {
        return 0;
    }
}

/*
 * Default loader to be used if file extension does not match a known file format
 * but a destination address is provided
 *
 * Inputs:
 * path = the path to the file to load
 * destination = the destination address (0 for use file's address)
 * start = pointer to the long variable to fill with the starting address
 *         (0 if not an executable, any other number if file is executable
 *         with a known starting address)
 *
 * Returns:
 * 0 on success, negative number on error
 */
short fsys_default_loader(short chan, long destination, long * start) {
    short n = ERR_GENERAL;
    unsigned char * dest = (unsigned char *)destination;

    TRACE("fsys_default_loader");
    log_num(LOG_DEBUG, "Channel: ", chan);

    /* The default loader cannot be used to load executable files, so clear the start address */
    *start = 0;

    while (1) {
        n = sys_chan_read(chan, dest, DEFAULT_CHUNK_SIZE);
        if (n > 0) {
            /* If we transferred some bytes, keep going */
            dest += n;
        } else {
            /* If we got back nothing or an error, stop and return */
            break;
        }
    }
    return n;
}

#define FYS_SREC_MAX_LINE_LENGTH    80   /* Assume no more than 80 characters per line */

/*
 * Convert a hexadecimal character to a number
 */
unsigned short atoi_hex_1(char hex) {
    if ((hex >= '0') || (hex <= '9')) {
        return hex - '0';
    } else if ((hex >= 'a') || (hex <= 'f')) {
        return hex - 'a' + 10;
    } else if ((hex >= 'A') || (hex <= 'F')) {
        return hex - 'A' + 10;
    } else {
        return 0;
    }
}

/*
 * Convert a two character hex string to a number
 */
unsigned short atoi_hex(char * hex) {
    return (atoi_hex_1(hex[0]) << 4 | atoi_hex_1(hex[1]));
}

/*
 * Loader for the Motorola SREC format
 *
 * See: https://en.wikipedia.org/wiki/SREC_(file_format)
 *
 * Inputs:
 * path = the path to the file to load
 * destination = the destination address (0 for use file's address)
 * start = pointer to the long variable to fill with the starting address
 *         (0 if not an executable, any other number if file is executable
 *         with a known starting address)
 *
 * Returns:
 * 0 on success, negative number on error
 */
short fsys_srec_loader(short chan, long destination, long * start) {
    short n, i, chksum, data_index;
    long address;
    unsigned char count,data;
    unsigned char addr[4];
    char line[FYS_SREC_MAX_LINE_LENGTH];
    unsigned char * dest;

    TRACE("fsys_srec_loader");

    /* TODO: verify the check sum */

    while (1) {
        chksum = 0;
        count = 0;
        n = sys_chan_readline(chan, line, FYS_SREC_MAX_LINE_LENGTH);
        if (n < 0) {
            /* If there was an error, return it */
            return n;

        } else if (n == 0) {
            /* If we got nothing back, we're finished */
            break;

        } else if (n > 6) {
            /* Only process the line if it starts with S and has more than six characters */
            if (line[0] = 'S') {
                /* Get the count and start the check sum */
                count = atoi_hex(&line[1]);
                chksum = count;

                /* Determine how to process the line based on its type */
                switch (line[1]) {
                    case '1':
                        /* 16-bit data line: S1nnaaaadd...ddcc */
                        addr[1] = atoi_hex(&line[4]);
                        addr[0] = atoi_hex(&line[6]);
                        dest = (unsigned char *)(addr[1] << 8 | addr[0]);
                        data_index = 8;
                        count -= 2;
                        chksum += addr[1] + addr[0];

                        /* Copy the data */
                        for (i = 0; i < count * 2; i += 2) {
                            data = atoi_hex(&line[data_index + i]);
                            chksum += data;
                            *dest++ = data;
                        }

                        break;

                    case '2':
                        /* 24-bit address data line: S2nnaaaaaadd..ddcc */
                        addr[2] = atoi_hex(&line[4]);
                        addr[1] = atoi_hex(&line[6]);
                        addr[0] = atoi_hex(&line[8]);
                        dest = (unsigned char *)(addr[2] << 16 | addr[1] << 8 | addr[0]);
                        data_index = 8;
                        count -= 2;
                        chksum += addr[2] + addr[1] + addr[0];

                        /* Copy the data */
                        for (i = 0; i < count * 2; i += 2) {
                            data = atoi_hex(&line[data_index + i]);
                            chksum += data;
                            *dest++ = data;
                        }

                        break;

                    case '3':
                        /* 32-bit address data line: S2nnaaaaaaaadd..ddcc */
                        addr[3] = atoi_hex(&line[4]);
                        addr[2] = atoi_hex(&line[6]);
                        addr[1] = atoi_hex(&line[8]);
                        addr[0] = atoi_hex(&line[10]);
                        dest = (unsigned char *)(addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0]);
                        data_index = 8;
                        count -= 2;
                        chksum += addr[3] + addr[2] + addr[1] + addr[0];

                        /* Copy the data */
                        for (i = 0; i < count * 2; i += 2) {
                            data = atoi_hex(&line[data_index + i]);
                            chksum += data;
                            *dest++ = data;
                        }

                        break;

                    case '5':
                        /* 16-bit starting address */
                        addr[1] = atoi_hex(&line[4]);
                        addr[0] = atoi_hex(&line[6]);
                        *start = (long)(addr[1] << 8 | addr[0]);
                        chksum += addr[2] + addr[1] + addr[0];
                        break;

                    case '6':
                        /* 24-bit starting address */
                        addr[2] = atoi_hex(&line[4]);
                        addr[1] = atoi_hex(&line[6]);
                        addr[0] = atoi_hex(&line[8]);
                        *start = (long)(addr[2] << 16 | addr[1] << 8 | addr[0]);
                        chksum += addr[2] + addr[1] + addr[0];
                        break;

                    case '7':
                        /* 32-bit starting address */
                        addr[3] = atoi_hex(&line[4]);
                        addr[2] = atoi_hex(&line[6]);
                        addr[1] = atoi_hex(&line[8]);
                        addr[0] = atoi_hex(&line[10]);
                        *start = (long)(addr[3] << 24 | addr[2] << 16 | addr[1] << 8 | addr[0]);
                        chksum += addr[3] + addr[2] + addr[1] + addr[0];
                        break;

                    default:
                        /* Ignore any other kind of line */
                        count = 0;
                        break;
                }
            }
        }
    }

    /* If we get here, we should have loaded the file successfully */
    return 0;
}

/*
 * Load a file into memory at the designated destination address.
 *
 * If destination = 0, the file must be in a recognized binary format
 * that specifies its own loading address.
 *
 * Inputs:
 * path = the path to the file to load
 * destination = the destination address (0 for use file's address)
 * start = pointer to the long variable to fill with the starting address
 *         (0 if not an executable, any other number if file is executable
 *         with a known starting address)
 *
 * Returns:
 * 0 on success, negative number on error
 */
short fsys_load(const char * path, long destination, long * start) {
    int i;
    char extension[MAX_EXT];
    short chan = -1;
    p_file_loader loader = 0;

    TRACE("fsys_load");

    /* Clear out the extension */
    for (i = 0; i <= MAX_EXT; i++) {
        extension[i] = 0;
    }

    /* Find the extension */
    char * point = strrchr(path, '.');
    if (point != 0) {
        point++;
        for (i = 0; i < MAX_EXT; i++) {
            char c = *point++;
            if (c) {
                extension[i] = toupper(c);
            } else {
                break;
            }
        }
    }

    TRACE("fsys_load: ext");

    if (extension[0] == 0) {
        if (destination != 0) {
            /* If a destination was specified, just load it into memory without interpretation */
            loader = fsys_default_loader;

        } else {
            /* Couldn't find a file extension to find the correct loader */
            return ERR_BAD_EXTENSION;
        }
    }

    /* Find the loader for the file extension */
    for (i = 0; i < MAX_LOADERS; i++) {
        if (g_file_loader[i].status) {
            if (strcmp(g_file_loader[i].extension, extension) == 0) {
                /* If the extensions match, pass back the loader */
                loader = g_file_loader[i].loader;
                log2(LOG_DEBUG, "loader found: ", g_file_loader[i].extension);
            }
        }
    }

    TRACE("fsys_load: loader search");

    if (loader == 0) {
        if (destination != 0) {
            /* If a destination was specified, just load it into memory without interpretation */
            log(LOG_DEBUG, "Setting default loader.");
            loader = fsys_default_loader;

        } else {
            log(LOG_DEBUG, "Returning a bad extension.");
            /* Return bad extension */
            return ERR_BAD_EXTENSION;
        }
    }

    if (loader == fsys_default_loader) {
        TRACE("default loader!");
    } else {
        TRACE("another loader");
    }

    /* Open the file for reading */
    chan = fsys_open(path, FA_READ);
    if (chan >= 0) {
        /* If it opened correctly, load the file */
        short result = loader(chan, destination, start);
        fsys_close(chan);
        return result;
    } else {
        /* File open returned an error... pass it along */
        return chan;
    }
}

/*
 * Register a file loading routine
 *
 * A file loader, takes a channel number to load from and returns a
 * short that is the status of the load.
 *
 * Inputs:
 * extension = the file extension to map to
 * loader = pointer to the file load routine to add
 *
 * Returns:
 * 0 on success, negative number on error
 */
short fsys_register_loader(const char * extension, p_file_loader loader) {
    int i, j;

    for (i = 0; i < MAX_LOADERS; i++) {
        if (g_file_loader[i].status == 0) {
            g_file_loader[i].status = 1;                    /* Claim this loader record */
            g_file_loader[j].loader = loader;               /* Set the loader routine */
            for (j = 0; j <= MAX_EXT; j++) {                /* Clear out the extension */
                g_file_loader[i].extension[j] = 0;
            }

            for (j = 0; j < MAX_EXT; j++) {                 /* Copy the extension */
                char c;
                c = extension[j];
                if (c) {
                    g_file_loader[i].extension[j] = toupper(c);
                } else {
                    break;
                }
            }

            return 0;
        }
    }
    return ERR_OUT_OF_HANDLES;
}

/**
 * Initialize the file system
 *
 * Returns:
 * 0 on success, negative number on failure.
 */
short fsys_init() {
    int i, j;

    /* Mark all directories as available */
    for (i = 0; i < MAX_DIRECTORIES; i++) {
        g_dir_state[i] = 0;
    }

    /* Mark all file descriptors as available */
    for (i = 0; i < MAX_FILES; i++) {
        g_fil_state[i] = 0;
    }

    /* Mount all logical drives that are present */

    for (i = 0; i < MAX_DRIVES; i++) {
        short res = sys_bdev_status((short)i);
        if (res >= 0) {
            fsys_mount(i);
        }
    }

    for (i = 0; i < MAX_LOADERS; i++) {
        g_file_loader[i].status = 0;
        g_file_loader[i].loader = 0;
        for (j = 0; j <= MAX_EXT; j++) {
            g_file_loader[i].extension[j] = 0;
        }
    }

    /* Register the SREC loader */
    fsys_register_loader("PRS", fsys_srec_loader);

    /* Register the channel driver for files. */

    g_file_dev.number = CDEV_FILE;
    g_file_dev.name = "FILE";
    g_file_dev.init = fchan_init;
    g_file_dev.ioctrl = fchan_ioctrl;
    g_file_dev.read = fchan_read;
    g_file_dev.read_b = fchan_read_b;
    g_file_dev.readline = fchan_readline;
    g_file_dev.write = fchan_write;
    g_file_dev.write_b = fchan_write_b;
    g_file_dev.seek = fchan_seek;
    g_file_dev.status = fchan_status;
    g_file_dev.flush = fchan_flush;

    cdev_register(&g_file_dev);

    return 0;
}