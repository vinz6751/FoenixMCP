/*
 * Foenix MCP bindings for GCC 4.6.4 Motorola 68000 target.
 *
 * Author: Vincent Barrilliot
 * Date: Nov 2021
 * Credits to Peter Weingartner for making FoenixMCP !
 * Public domain
 */

/* Core calls */

#ifndef __FOENIX_H__
#define __FOENIX_H__

#define KFN_EXIT                0x00    /* Quick the current program and return to the command line */
#define KFN_WARMBOOT            0x01    /* Do a soft re-initialization */
#define KFN_INT_REGISTER        0x02    /* Set a handler for an exception / interrupt */
#define KFN_INT_ENABLE          0x03    /* Enable an interrupt */
#define KFN_INT_DISABLE         0x04    /* Disable an interrupt */
#define KFN_INT_ENABLE_ALL      0x05    /* Enable all interrupts */
#define KFN_INT_DISABLE_ALL     0x06    /* Disable all interrupts */
#define KFN_INT_CLEAR           0x07    /* Clear (acknowledge) an interrupt */
#define KFN_INT_PENDING         0x08    /* Return true if the interrupt is pending */
#define KFN_SYS_GET_INFO        0x09    /* Get information about the computer */

#define sys_exit(a) syscall_w_w(KFN_EXIT,a)
#define sys_warmboot() syscall_v_v(KFN_EXIT)
#define sys_int_register(a,b) syscall_l_wl(KFN_INT_REGISTER,a, b)
#define sys_int_enable(a) syscall_v_w(KFN_INT_ENABLE,a)
#define sys_int_disable(a) syscall_v_w(KFN_INT_DISABLE,a)
#define sys_int_enable_all() syscall_v_v(KFN_INT_ENABLE_ALL)
#define sys_int_disable_all() syscall_v_v(KFN_INT_DISABLE_ALL)
#define sys_int_clear(a) syscall_v_w(KFN_INT_CLEAR,a)
#define sys_int_pending(a) syscall_v_w(KFN_INT_PENDING,a)
#define sys_get_information(a) syscall_v_l(KFN_SYS_GET_INFO,a)

/* Channel system calls */

#define KFN_CHAN_READ           0x10    /* Read bytes from an input channel */
#define KFN_CHAN_READ_B         0x11    /* Read a byte from an input channel */
#define KFN_CHAN_READ_LINE      0x12    /* Read a line from an input channel */
#define KFN_CHAN_WRITE          0x13    /* Write bytes to an output channel */
#define KFN_CHAN_WRITE_B        0x14    /* Write a byte to an output channel */
#define KFN_CHAN_FLUSH          0x15    /* Ensure that any pending write on a channel is committed */
#define KFN_CHAN_SEEK           0x16    /* Set the input/output cursor on a channel to a given position */
#define KFN_CHAN_STATUS         0x17    /* Get the status of a channel */
#define KFN_CHAN_IOCTRL         0x18    /* Send a command to a channel (channel dependent functionality) */
#define KFN_CHAN_REGISTER       0x19    /* Register a channel device driver */
#define KFN_CHAN_OPEN           0x1A    /* Open a channel device */
#define KFN_CHAN_CLOSE          0x1B    /* Close an open channel (not for files) */
#define KFN_TEXT_SETSIZES       0x1C    /* Adjusts the screen size based on the current graphics mode */

#define sys_chan_read(a,b,c) syscall_w_wlw(KFN_CHAN_READ,a,b,c)
#define sys_chan_read_b(a) syscall_w_w(KFN_CHAN_READ_B,a)
#define sys_chan_read_line(a,b,c) syscall_w_wlw(KFN_CHAN_READ_LINE,a,b,c)
#define sys_chan_write_b(a,b) syscall_l_ww(KFN_CHAN_WRITE_B,a,b)
#define sys_chan_flush(a) syscall_w_w(KFN_CHAN_FLUSH,a)
#define sys_chan_seek(a,b,c) syscall_w_wlw(KFN_CHAN_SEEK,a,b,c)
#define sys_chan_status(a) syscall_w_w(KFN_CHAN_STATUS,a)
#define sys_chan_ioctrl(a,b,c,d) syscall_w_wwlw(KFN_CHAN_IOCTRL,a,b,c,d)
#define sys_chan_register(a) syscall_w_l(KFN_CHAN_REGISTER,a)
#define sys_chan_open(a,b) syscall_w_lw(KFN_CHAN_OPEN,a,b)
#define sys_chan_close(a) syscall_w_w(KFN_CHAN_CLOSE,a)

/* Block device system calls */

#define KFN_BDEV_GETBLOCK       0x20    /* Read a block from a block device */
#define KFN_BDEV_PUTBLOCK       0x21    /* Write a block to a block device */
#define KFN_BDEV_FLUSH          0x22    /* Ensure that any pending write on a block device is committed */
#define KFN_BDEV_STATUS         0x23    /* Get the status of a block device */
#define KFN_BDEV_IOCTRL         0x24    /* Send a command to a block device (device dependent functionality) */
#define KFN_BDEV_REGISTER       0x25    /* Register a block device driver */

#define sys_bdev_register(a) syscall_w_l(KFN_BDEV_REGISTER,a)
#define sys_bdev_read(a,b,c,d) syscall_w_wllw(KFN_BDEV_GETBLOCK,a,b,c,d)
#define sys_bdev_write(a,b,c,d) syscall_w_wllw(KFN_BDEV_PUTBLOCK,a,b,c,d)
#define sys_bdev_status(a) syscall_w_w(KFN_BDEV_STATUS,a)
#define sys_bdev_flush(a) syscall_w_w(KFN_BDEV_FLUSH,a)
#define sys_bdev_ioctrl(a,b,c,d) syscall_w_wwlw(KFN_BDEV_IOCTRL,a,b,c,d)

/* File/Directory system calls */

#define KFN_OPEN                0x30    /* Open a file as a channel */
#define KFN_CLOSE               0x31    /* Close a file channel */
#define KFN_OPENDIR             0x32    /* Open a directory for reading */
#define KFN_CLOSEDIR            0x33    /* Close an open directory */
#define KFN_READDIR             0x34    /* Read the next directory entry in an open directory */
#define KFN_FINDFIRST           0x35    /* Find the first entry in a directory that matches a pattern */
#define KFN_FINDNEXT            0x36    /* Find the next entry in a directory that matches a pattern */
#define KFN_DELETE              0x37    /* Delete a file */
#define KFN_RENAME              0x38    /* Rename a file */
#define KFN_MKDIR               0x39    /* Create a directory */
#define KFN_LOAD                0x3A    /* Load a file into memory */
#define KFN_GET_LABEL           0x3B    /* Read the label of a volume */
#define KFN_SET_LABEL           0x3C    /* Set the label of a volume */
#define KFN_SET_CWD             0x3D    /* Set the current working directory */
#define KFN_GET_CWD             0x3E    /* Get the current working directory */
#define KFN_LOAD_REGISTER       0x3F    /* Register a file type handler for executable binaries */

#define sys_fsys_open(a,b) syscall_w_lw(KFN_OPEN,a,b)
#define sys_fsys_close(a) syscall_w_w(KFN_CLOSE,a)
#define sys_fsys_opendir(a) syscall_w_l(KFN_OPENDIR,a)
#define sys_fsys_closedir(a) syscall_w_w(KFN_CLOSEDIR,a)
#define sys_fsys_readdir(a,b) syscall_w_wl(KFN_READDIR,a,b)
#define sys_fsys_findfirst(a,b,c) syscall_w_lll(KFN_FINDFIRST,a,b,c)
#define sys_fsys_findnext(a,b) syscall_w_wl(KFN_FINDNEXT,a,b)
#define sys_fsys_delete(a) syscall_w_l(KFN_DELETE,a)
#define sys_fsys_rename(a,b) syscall_w_ll(KFN_RENAME,a,b)
#define sys_fsys_mkdir(a) syscall_w_l(KFN_MKDIR,a)
#define sys_fsys_load(a,b,c) syscall_w_lll(KFN_LOAD,a,b,c)
#define sys_fsys_setcwd(a) syscall_w_l(KFN_SET_CWD,a)
#define sys_fsys_getcwd(a,b) syscall_w_lw(KFN_GET_CWD,a,b)
#define sys_fsys_get_label(a,b) syscall_w_ll(KFN_GET_LABEL,a,b)
#define sys_fsys_set_label(a,b) syscall_w_wl(KFN_SET_LABEL,a,b)
#define fsys_register_loader(a,b) syscall_w_ll(KFN_LOAD_REGISTER,a,b)

/* Process and memory calls */

#define KFN_RUN                 0x40    /* Load an execute a binary file */

#define sys_proc_run(a,b,c) syscall_w_lwl(KFN_RUN,a,b,c)

/* Misc calls */

#define KFN_TIME_JIFFIES        0x50    /* Gets the current time code (increments since boot) */
#define KFN_TIME_SETRTC         0x51    /* Set the real time clock date-time */
#define KFN_TIME_GETRTC         0x52    /* Get the real time clock date-time */
#define KFN_KBD_SCANCODE        0x53    /* Get the next scan code from the keyboard */
#define KFN_KBD_LAYOUT          0x54    /* Set the translation tables for the keyboard */
#define KFN_ERR_MESSAGE         0x55    /* Return an error description, given an error number */

#define sys_rtc_get_jiffies() syscall_l_v(KFN_TIME_JIFFIES)
#define sys_rtc_set_time(a) syscall_v_l(KFN_TIME_SETRTC,a)
#define sys_rtc_get_time() syscall_l_v(KFN_TIME_GETRTC)
#define sys_kbd_scancode() syscall_w_v(KFN_KBD_SCANCODE)
#define sys_err_message(a) syscall_l_w(KFN_ERR_MESSAGE,a)

/* Clobbered registers for non void or void functions */
/* http://www.ibaug.de/vbcc/doc/vbcc_3.html says d0,d1,a0,a1,fp0,fp1 are scratch registers */
#define CLOBBERED_REGS_NON_VOID "d1", "a0", "a1", "memory", "cc"
#define CLOBBERED_REGS_VOID "d0", "d1", "a0", "a1", "memory", "cc"

/* You can change the trap number here */
#define TRAP "trap    #15\n\t"


static __inline__ short syscall_w_w(int op, short a)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_lw(int op, long a, short b)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %3,d2\n\t"
        "move.l  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "ir"(a), "nr"(b)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_wlw(int op, short a, long b, short c)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %4,d3\n\t"
        "move.l  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "ir"(b), "nr"(c)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_wl(int op, short a, long b)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "ir"(b)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_ll(int op, long a, long b)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %3,d2\n\t"
        "move.l  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "ir"(a), "ir"(b)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_v(int op)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ short syscall_w_lll(int op, long a, long b, long c)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %4,d3\n\t"
        "move.l  %3,d2\n\t"
        "move.l  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "ir"(a), "ir"(b), "ir"(c)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}


static __inline__ short syscall_w_lwl(int op, long a, short b, long c)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %4,d3\n\t"
        "move.w  %3,d2\n\t"
        "move.l  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "ir"(a), "nr"(b), "ir"(c)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ void syscall_v_w(int op, short a)
{
    __asm__ volatile (
        "move.w  %1,d1\n\t"
        "move.w  %0,d0\n\t"
        TRAP
         :
         : "nr"(op), "nr"(a)
         : CLOBBERED_REGS_VOID
        );
}

static __inline__ void syscall_v_l(int op, long a)
{
    __asm__ volatile (
        "move.l  %1,d1\n\t"
        "move.w  %0,d0\n\t"
        TRAP
         :
         : "nr"(op), "ir"(a)
         : CLOBBERED_REGS_VOID
        );
}

static __inline__ void syscall_v_v(int op)
{
    __asm__ volatile (
        "move.w  %0,d0\n\t"
        TRAP
         :
         : "nr"(op)
         : CLOBBERED_REGS_VOID
        );
}

static __inline__ long syscall_l_v(int op)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_w_l(int op, long a)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "ir"(a)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_l_w(int op, short a)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_l_ww(int op, short a, short b)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "nr"(b)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_l_wl(int op, short a, long b)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.l  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "ir"(b)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_w_wwlw(int op, short a, short b, long c, short d)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %5,d4\n\t"
        "move.l  %4,d3\n\t"
        "move.w  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "nr"(b), "ir"(c), "nr"(d)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

static __inline__ long syscall_w_wllw(int op, short a, long b, long c, short d)
{
    register long retval __asm__("d0");

    __asm__ volatile (
        "move.w  %5,d4\n\t"
        "move.l  %4,d3\n\t"
        "move.l  %3,d2\n\t"
        "move.w  %2,d1\n\t"
        "move.w  %1,d0\n\t"
        TRAP
         : "=r"(retval)
         : "nr"(op), "nr"(a), "ir"(b), "ir"(c), "nr"(d)
         : CLOBBERED_REGS_NON_VOID
        );
    return retval;
}

#endif