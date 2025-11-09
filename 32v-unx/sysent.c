/*
Copyright(C) Caldera International Inc. 2001-2002. All rights reserved. 
*/

/*
 * This table is the switch used to transfer
 * to the appropriate routine for processing a system call.
 * Each row contains the number of arguments expected
 * and a pointer to the routine.
 */

#include "h/param.h"
#include "h/systm.h"

int     UNIX_alarm();
int     UNIX_chdir();
int     UNIX_chmod();
int     UNIX_chown();
int     UNIX_chroot();
int UNIX_close();
int UNIX_creat();
int UNIX_dup();
int UNIX_exec();
int UNIX_exece();
int UNIX_fork();
int UNIX_fstat();
int UNIX_getgid();
int UNIX_getpid();
int UNIX_getuid();
int UNIX_gtime();
int UNIX_gtty();
int     UNIX_ioctl();
int     UNIX_kill();
int UNIX_link();
int UNIX_mknod();
int UNIX_mpxchan();
int UNIX_nice();
int UNIX_ftime();
int nosys();
int nullsys();
int UNIX_open();
int UNIX_pause();
int UNIX_pipe();
int UNIX_profil();
int UNIX_ptrace();
int UNIX_read();
int UNIX_rexit();
int UNIX_saccess();
int UNIX_sbreak();
int UNIX_seek();
int UNIX_setgid();
int UNIX_setuid();
int UNIX_smount();
int UNIX_ssig();
int UNIX_stat();
int UNIX_stime();
int UNIX_stty();
int UNIX_sumount();
int UNIX_sync();
int UNIX_sysacct();
int UNIX_syslock();
int UNIX_sysphys();
int UNIX_times();
int UNIX_umask();
int UNIX_unlink();
int UNIX_utime();
int UNIX_wait();
int UNIX_write();

struct sysent sysent[64] =
{
        0, 0, nosys,                    /* 0 = indir */
        1, 0, UNIX_rexit,                       /*  1 = exit */
        0, 0, UNIX_fork,                        /*  2 = fork */
        3, 0, UNIX_read,                        /*  3 = read */
        3, 0, UNIX_write,                       /*  4 = write */
        2, 0, UNIX_open,                        /*  5 = open */
        1, 0, UNIX_close,                       /*  6 = close */
        0, 0, UNIX_wait,                        /*  7 = wait */
        2, 0, UNIX_creat,                       /*  8 = creat */
        2, 0, UNIX_link,                        /*  9 = link */
        1, 0, UNIX_unlink,                      /* 10 = unlink */
        2, 0, UNIX_exec,                        /* 11 = exec */
        1, 0, UNIX_chdir,                       /* 12 = chdir */
        0, 0, UNIX_gtime,                       /* 13 = time */
        3, 0, UNIX_mknod,                       /* 14 = mknod */
        2, 0, UNIX_chmod,                       /* 15 = chmod */
        3, 0, UNIX_chown,                       /* 16 = chown; now 3 args */
        1, 0, UNIX_sbreak,                      /* 17 = break */
        2, 0, UNIX_stat,                        /* 18 = stat */
        3, 0, UNIX_seek,                        /* 19 = seek */
        0, 0, UNIX_getpid,                      /* 20 = getpid */
        3, 0, UNIX_smount,                      /* 21 = mount */
        1, 0, UNIX_sumount,                     /* 22 = umount */
        1, 0, UNIX_setuid,                      /* 23 = setuid */
        0, 0, UNIX_getuid,                      /* 24 = getuid */
        1, 0, UNIX_stime,                       /* 25 = stime */
        4, 0, UNIX_ptrace,                      /* 26 = ptrace */
        1, 0, UNIX_alarm,                       /* 27 = alarm */
        2, 0, UNIX_fstat,                       /* 28 = fstat */
        0, 0, UNIX_pause,                       /* 29 = pause */
        2, 0, UNIX_utime,                       /* 30 = utime */
        2, 0, UNIX_stty,                        /* 31 = stty */
        2, 0, UNIX_gtty,                        /* 32 = gtty */
        2, 0, UNIX_saccess,                     /* 33 = access */
        1, 0, UNIX_nice,                        /* 34 = nice */
        1, 0, UNIX_ftime,                       /* 35 = ftime; formally sleep;  */
        0, 0, UNIX_sync,                        /* 36 = sync */
        2, 0, UNIX_kill,                        /* 37 = kill */
        0, 0, nullsys,                  /* 38 = switch; inoperative */
        0, 0, nullsys,                  /* 39 = setpgrp (not in yet) */
        0, 0, nosys,                    /* 40 = tell - obsolete */
        2, 0, UNIX_dup,                         /* 41 = dup */
        0, 0, UNIX_pipe,                        /* 42 = pipe */
        1, 0, UNIX_times,                       /* 43 = times */
        4, 0, UNIX_profil,                      /* 44 = prof */
        0, 0, nosys,                    /* 45 = tiu */
        1, 0, UNIX_setgid,                      /* 46 = setgid */
        0, 0, UNIX_getgid,                      /* 47 = getgid */
        2, 0, UNIX_ssig,                        /* 48 = sig */
        0, 0, nosys,                    /* 49 = reserved for USG */
        0, 0, nosys,                    /* 50 = reserved for USG */
        1, 0, UNIX_sysacct,                     /* 51 = turn acct off/on */
        3, 0, UNIX_sysphys,                     /* 52 = set user physical addresses */
        1, 0, UNIX_syslock,                     /* 53 = lock user in core */
        3, 0, UNIX_ioctl,                       /* 54 = ioctl */
        0, 0, nosys,                    /* 55 = reboot */
        4, 0, UNIX_mpxchan,                     /* 56 = creat mpx comm channel */
        0, 0, nosys,                    /* 57 = reserved for USG */
        0, 0, nosys,                    /* 58 = reserved for USG */
        3, 0, UNIX_exece,                       /* 59 = exece */
        1, 0, UNIX_umask,                       /* 60 = umask */
        1, 0, UNIX_chroot,                      /* 61 = chroot */
        1, 0, nosys,            /* 62 = unused */
        0, 0, nosys                     /* 63 = used internally */
};
