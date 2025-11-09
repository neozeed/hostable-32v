The included .wpj is for Watcom (I'm using the Open version which can be found @ http://www.openwatcom.org/
(you can find more compilers @ http://www.thefreecountry.com/compilers/cpp.shtml

Anyways there is a bunch of work to do. All the low level code is un-implemented (missing.c).  
Since there is no proper initialaztion, most things don't work correctly.  Also there are no
proper block or char devices.  However It compiles, and without doing an iinit, it will go into
the scheduler (Down the line the scheduler really only should set guest thread priorites, and 
act to kill process).  If anything elese, it's cool to see that 32v mostly compiles out of the box!


All of the kernel functions have been re-named to UNIX_XXX (ie binit is now UNIX_binit) I did this so
that I can use the native OS's c runtime, with the kernel code, and this way I'm sure which printf
is which. 

I have also tested this with GCC (win32/OpenBSD/MacOSX).

At the very least this is a cool way to run a UNIX kernel under a debugger to see exactly how it works!
