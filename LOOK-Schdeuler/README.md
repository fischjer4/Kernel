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
1. git clone git://git.yoctoproject.org/linux-yocto-3.19  linux-yocto-3.19-patched
2. git clone git://git.yoctoproject.org/linux-yocto-3.19  linux-yocto-3.19-withFiles
3. cp  neededFiles/Makefile neededFiles/Kconfig.ioschedneededFiles/ look_iosched.c linux-yocto-3.19-withFiles/block
4. Cp neededFiles/config  linux-yocto-3.19-withFiles/.config
5. diff -Naur linux-yocto-3.19-patched/block   linux-yocto-3.19-withFiles/block > kernelAssn2.patch
6. Apply Patch
..a. cp kernelAssn2.patch  linux-yocto-3.19-patched/block 
..b. cd linux-yocto-3.19-patched/block 
..c. patch < kernelAssn2.patch
7. Make -j4 all
8. Cd ../
9. Use screen command to create to split screens
10. In both screen make sure to source the enviornment variables
..a. (screen 1) qemu-system-i386 -gdb tcp::5517 -S -nographic -kernel linux-yocto-3.19-patched/arch/x86/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext4 -enable-kvm -net none -usb -localtime --no-reboot 	--append "root=/dev/hda rw console=ttyS0 debug"
..b. _(screen 2)_ $GDB
..c. _(screen 2)_ target remote :5517
..d. _(screen 2)_ continue
..e. _(screen 1)_ login as root
..f. _(screen 1)_ Cat /sys/block/hda/queue/scheduler and notice the name of your module. In this case LOOK
..g. _(screen 1)_ Echo 'LOOK' > /sys/block/hda/queue/scheduler this changes the scheduler
..h. _(screen 1)_ Cat /sys/block/hda/queue/scheduler should now show [LOOK]
..i. _(screen 1)_ *dmesg* (this will allow you to view output from the LOOK scheduler)
11. Reboot to stop qemu