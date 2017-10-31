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

### Steps To Produce A Scheduler

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
	1. (screen 1) 
	
	⋅⋅⋅ *qemu-system-i386 -gdb tcp::5517 -S -nographic -kernel linux-yocto-3.19-patched/arch/x86/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext4 -enable-kvm -net none -usb -localtime --no-reboot 	--append "root=/dev/hda rw console=ttyS0 debug"*
	2. __(screen 2)__ *$GDB*
	3. __(screen 2)__ *target remote :5517*
	4. __(screen 2)__ *continue*
	5. __(screen 1)__ login: *root*
	6. __(screen 1)__ *cat /sys/block/hda/queue/scheduler* and notice the name of your module. In this case LOOK
	7. __(screen 1)__ *echo 'LOOK' > /sys/block/hda/queue/scheduler* this changes the scheduler
	8. __(screen 1)__ *cat /sys/block/hda/queue/scheduler* should now show [LOOK]
	9. __(screen 1)__ *dmesg* (this will allow you to view output from the LOOK scheduler)
11. *reboot* to stop qemu