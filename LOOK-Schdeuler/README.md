# Implementing the LOOK Scheduler
| API Functions        | Definition  |
| ------------- |:-------------:| 
| *elevator_latter_req_fn*      | Returns the request AFTER req in start-sector order. Used to discover opportunities to place two adjacent requests next to each other (i.e. drop two people off at same floor. | 
| *elevator_former_req_fn*      | Returns the request BEFORE req in start-sector order. Used to discover opportunities to place two adjacent requests next to each other (i.e. drop two people off at same floor. |  
| *elevator_merge_req_fn * 		| Called when two requests have been turned into a single request. This occurs when a request grows (as a result of a bio merge. to become adjacent with an existing request |
| *elevator_init_fn*			| Initialize a scheduler instance |
| *elevator_exit_fn*			| Clean up scheduler instance |
| *elevator_add_req_fn*			| Queues a new request with the scheduler |
| *elevator_dispatch_fn*		| Requests the scheduler to populate the dispatch queue with the next request. Returns the number of requests dispatched |
| *list_add*					| list_head to add it after |

### Steps To Run With The Patch
1. *git clone git://git.yoctoproject.org/linux-yocto-3.19  linux-yocto-3.19-patched*
2. *git clone git://git.yoctoproject.org/linux-yocto-3.19  linux-yocto-3.19-withFiles*
3. *cp  neededFiles/Makefile neededFiles/Kconfig.iosched neededFiles/look_iosched.c linux-yocto-3.19-withFiles/block*
4. *diff -Naur linux-yocto-3.19-patched/block   linux-yocto-3.19-withFiles/block > kernelAssn2.patch*
5. *cp neededFiles/config-3.19.2-yocto-standard  linux-yocto-3.19-withFiles/.config*
6. Apply Patch
	1. *cp kernelAssn2.patch  linux-yocto-3.19-patched/block *
	2. *cd linux-yocto-3.19-patched/block *
	3. *patch < kernelAssn2.patch*
7. *Make -j4 all*
8. *Cd ../*
9. Use screen command to create to split screens
10. In both screen make sure to source the enviornment variables
	1. __(screen 1)__
	
		*qemu-system-i386 -gdb tcp::5517 -S -nographic -kernel linux-yocto-3.19-patched/arch/x86/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext4 -enable-kvm -net none -usb -localtime --no-reboot 	--append "root=/dev/hda rw console=ttyS0 debug"*

	2. __(screen 2)__ *$GDB*
	3. __(screen 2)__ *target remote :5517*
	4. __(screen 2)__ *continue*
	5. __(screen 1)__ login: *root*
	6. __(screen 1)__ *cat /sys/block/hda/queue/scheduler* and notice the name of your module. In this case LOOK
	7. __(screen 1)__ *echo 'LOOK' > /sys/block/hda/queue/scheduler* this changes the scheduler
	8. __(screen 1)__ *cat /sys/block/hda/queue/scheduler* should now show [LOOK]
	9. __(screen 1)__ *dmesg* (this will allow you to view output from the LOOK scheduler)
11. *reboot* to stop qemu

### Steps To Produce A New Scheduler
1. *git clone git://git.yoctoproject.org/linux-yocto-3.19
2. Copy block/noop-iosched.c  file to look-iosched.c This is the NOOP scheduler. This is the skeleton for the LOOK scheduler module
2. Implement the LOOK scheduler in the look-iosched.c file
3. Add *obj-$(CONFIG_IOSCHED_CLOOK) += clook-iosched.o* to the block/Makefile. This is used to compile the LOOK scheduler's object file
4. Inside of block/Kconfig.iosched  
	- Copy the IOSCHED_DEADLINE section
	- Paste it below
	- Change IOSCHED_DEADLINE to IOSCHED_LOOK 
	- Change the value of tristate to be "LOOK I/O Scheduler"
	- Edit the --help-- section
	- Add 
		```config DEFAULT_LOOK
			bool "LOOK" if IOSCHED_LOOK=y
		```
	Inside the Choice section at the bottom
5. Use printk() statements throughout your scheduler's code to verify that it is doing what is expected. The output can be seen in the message log by typing *dmesg* in the command line or by viewing /var/log/messages. 
6. Run the ```make menuconfig`` command. 
	a. navigate to Block layer --> IO Schedulers, and mark the LOOK scheduler to be compiled as a module. 
	b. Make sure to leave the Default I/O scheduler selection alone. 
	c. Save the configuration changes.
7. Considering that nothing has change but the new LOOK module. It is possible to just compile it by typeing make && make modules_install but just to be safe type make -j4 all.
8. Now To verify the scheduler works
	- Use screen command to create to split screens
	- In both screen make sure to source the enviornment variables
		- __(screen 1)__ 

				*qemu-system-i386 -gdb tcp::5517 -S -nographic -kernel linux-yocto-3.19-patched/arch/x86/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext4 -enable-kvm -net none -usb -localtime --no-reboot 	--append "root=/dev/hda rw console=ttyS0 debug"*

		- __(screen 2)__ *$GDB*
		-  __(screen 2)__ *target remote :5517*
		- __(screen 2)__ *continue*
		- __(screen 1)__ login: *root*
		- __(screen 1)__ *cat /sys/block/hda/queue/scheduler* and notice the name of your module. In this case LOOK
		- __(screen 1)__ *echo 'LOOK' > /sys/block/hda/queue/scheduler* this changes the scheduler
		- __(screen 1)__ *cat /sys/block/hda/queue/scheduler* should now show [LOOK]
		- __(screen 1)__ *dmesg* (this will allow you to view output from the LOOK scheduler)
9. Create the patch file
	- Mkdir clean_dir
	- Cd clean_dir
	- Git clone git clone git://git.yoctoproject.org/linux-yocto-3.19
	- diff -Naur clean_dir/linux-yocto-3.19/block linux-yocto-3.19 > kernelAssn2.patch