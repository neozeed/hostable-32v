Welcome to version 0.1b! 

Ive included 2 copies of the source, 32v-unx is in Unix format, and 32v-wat is in MSDOS style, and includes a win32 32v.exe

Anyways First off you will need a file system. You will want to compile the mkfs
program in the mkfs directory.. Here is a sample:


-------8<-------8<-------8<-------8<-------8<-------8<-------8<
C:\proj\mkfs>cl mkfs.c
Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 13.10.3077 for 80x86
Copyright (C) Microsoft Corporation 1984-2002. All rights reserved.

mkfs.c
c:\proj\mkfs\PARAM.H(64) : warning C4005: 'NULL' : macro redefinition
        d:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\include\stdio.h(183) : see previous definition of 'NULL'c:\proj\mkfs\mkfs.c(69) : warning C4716: 'ask_param' : must return a value
c:\proj\mkfs\mkfs.c(169) : warning C4716: 'mkfs' : must return a value
c:\proj\mkfs\mkfs.c(216) : warning C4716: 'blk_io' : must return a value
Microsoft (R) Incremental Linker Version 7.10.3077
Copyright (C) Microsoft Corporation.  All rights reserved.

/out:mkfs.exe
mkfs.obj
-------8<-------8<-------8<-------8<-------8<-------8<-------8<
now run the program.. I'm guessing that this is right, it's probably 
full of a million errors, but it generates a file..

-------8<-------8<-------8<-------8<-------8<-------8<-------8<
C:\proj\mkfs>mkfs
MaKeFileSystem

Device physical characteristics


File system logical characteristics

Size of volume : 5
Number of I-nodes : 1

Mkfs in progress...
rblk 0 mode 5 buf[]
rblk 1 mode 5 buf[]
rblk 2 mode 5 buf[]
rblk 3 mode 5 buf[]
rblk 4 mode 5 buf[]
Zeroing Superblock & I-nodes
rblk 0 mode 3 buf[]
rblk 1 mode 3 buf[]
rblk 2 mode 3 buf[]
Zeroed: 2
Freeing blocks
Freed :4
Freeing I-nodes
Writing Super Block
rblk 1 mode 3 buf[?]
Writing root dir.
rblk 2 mode 3 buf[fA]
rblk 3 mode 3 buf[?]

MKFS done

C:\proj\mkfs>
-------8<-------8<-------8<-------8<-------8<-------8<-------8<
Ok, now we have our filesytem.. I would keep it small as I don't trus this thing 100%!

Now let's build the kernel.. I'll provide some more makefiles later, but
the big thing is that you don't want standard includes(-nostdinc ?), and you will
have to fix ansidisk.c to the actual path of you stdio.h.
-------8<-------8<-------8<-------8<-------8<-------8<-------8<
cl /X *.c -o 32v.exe
-------8<-------8<-------8<-------8<-------8<-------8<-------8<
now copy the unix.fs to the path of your 32v executable, and run..

-------8<-------8<-------8<-------8<-------8<-------8<-------8<
RESTRICTED RIGHTS
USE, DUPLICATION OR DISCLOSURE IS
SUBJECT TO RESTRICTION STATED IN YOUR
CONTRACT WITH WESTERN ELECTRIC COMPANY INC.

Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
Copyright(C) S.C.O. International Inc. 2002-2003. All rights reserved.
ansiopen
ansifilesystem() read only block device driver.
ansistrategy    Flags:0x9
ansiustart
ansistrategy B_READ 512 bytes from 0
ansistrategy    Flags:0x9
ansiustart
ansistrategy B_READ 512 bytes from 1
Making Devices
1000user = 0 0 0 0
ps = 1
pc = 411BA5
trap type 4
code = 12FFC0
panic: trap
-------8<-------8<-------8<-------8<-------8<-------8<-------8<

Just a quick run down... I know the ansidisk is very chatty at the moment.. 
Also the nubmer 1000 is the number of loops in the sched part of slp.c before
it dies.  After that I throw a trap.


