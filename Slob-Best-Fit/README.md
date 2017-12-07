# Implementing the SLOB Best-Fit Algorithm


### Implementing the Effeciency Metric Algorithm
This was written to compute the effeciency of the first-fit algorithm and best-fit algorithm and compare the fragmentation sufferred by each algorithm.

#### Steps to Run with Patch
1. *git clone "git://git.yoctoproject.org/linux-yocto-3.19" linux-yocto-3.19-patched*
2. *cd linux-yocto-3.19-patched*
3. *git checkout v3.19.2*
4. Apply Patch
	1. *cd ../*
	2. *patch -p0 < slobBestFit.patch*
5. *cp /scratch/files/core-image-lsb-sdk-qemux86.ext4 .*
6. *cd linux-yocto-3.19-patched
7. *cp /scratch/files/config-3.19.2-yocto-standard .config*
8. *make menuconfig*

9. Press the */* key
10. Type *"slob"*, then press *enter*
11. Press *1*, then press *enter*
12. Check the box for *SLOB*
13. Save and Exit, saving to .config when prompted

14. *make -j4 all*

15. Use the \textit{screen} command to create two split-screens.
9. Use screen command to create to split screens
10. In both screen make sure to source the enviornment variable (_environment-setup-i586-poky-linux_)
	1. __(screen 1)__
	
		*qemu-system-i386 -gdb tcp::5517 -S -nographic -kernel arch/x86/boot/bzImage -drive file=../core-image-lsb-sdk-qemux86.ext4,if=ide -enable-kvm -usb -localtime --no-reboot --append "root=/dev/hda rw console=ttyS0 debug"*

	2. __(screen 2)__ *$GDB*
	3. __(screen 2)__ *target remote :5517*
	4. __(screen 2)__ *continue*
	5. __(screen 1)__ login: *root*
	6. __(screen 1)__ *scp __username__@os2.engr.oregonstate.edu:__you dir__/fragTest.c /home/root*
	7. __(screen 1)__ *gcc fragTest.c*
	8. __(screen 1)__ *./a.out then view output*
11. *reboot* to stop qemu