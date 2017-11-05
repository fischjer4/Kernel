/*
	* The skeleton for this code was taken from Pat Paterson at
		http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
	* Crypto resource
		https://01.org/linuxgraphics/gfx-docs/drm/crypto/intro.html
*/



/*
 * A sample, extra-simple block driver. Updated for kernel 2.6.31.
 *
 * (C) 2003 Eklektix, Inc.
 * (C) 2010 Pat Patterson <pat at superpat dot com>
 * Redistributable under the terms of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/crypto.h>

MODULE_LICENSE("GPL");
static char *Version = "1.4";

static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, int, 0);

/*We can tweak our hardware sector size, but the kernel talks to us
	in terms of 512 byte sectors */
#define KERNEL_SECTOR_SIZE 512

/*Request Queue*/
static struct request_queue *Queue;

/*Internal Representation Of The Block Device*/
static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/*Crypto Handle*/
struct crypto_cipher *tfm;







/*
 	* Handle an I/O request.
 */
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
						 unsigned long nsect, char *buffer, int write) {
	
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size; //num bytes to r/w
	unsigned long i = 0; //used as block indexer

	if ((offset + nbytes) > dev->size) {
		printk(KERN_NOTICE "~BLKDEVCRYPT~ sbd_transfer() -- Beyond-end write (%ld %ld)\n", offset, nbytes);			
		return;
	}
	/*	
		nbytes represents the number of bytes to be read/written.
		The en/decrypt function handles a block at a time, so if
		nbytes > blocksize then we must en/decrypt each block sequentially
		one at a time
	*/
	printk("Read/Write %lu bytes from the block device", nbytes);
	if (write){
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Write: before encryption: %s\n", buffer);			
		while(i < nbytes){
			crypto_cipher_encrypt_one(tfm, (dev->data + offset + i), buffer + i);
			i += crypto_cipher_blocksize(tfm) //go to next block
		}
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Write: after encryption: %s\n", dev->data + offset);					
	}
	else{
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Read: before decryption: %s\n", dev->data + offset);							
		while(i < nbytes){
			crypto_cipher_decrypt_one(tfm, buffer + i, (dev->data + offset + i));
			i += crypto_cipher_blocksize(tfm) //go to next block
		}
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Read: after decryption: %s\n", buffer);					
	}
}

static void sbd_request(struct request_queue *q) {
	struct request *req;

	/*get next request*/
	req = blk_fetch_request(q);
	while (req != NULL) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk(KERN_NOTICE "~BLKDEVCRYPT~ sbd_request() -- [req] non-CMD request\n");			
			__blk_end_request_all(req, -EIO);
			continue;
		}
		sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req), req->buffer, rq_data_dir(req));
		/*if this req is done, get the next one*/
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

/*
	* The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
	* calls this. We need to implement getgeo, since we can't
	* use tools such as fdisk to partition the drive otherwise.
 */
int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/*
 	* The device operations structure.
 */
static struct block_device_operations sbd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_getgeo
};

static int __init sbd_init(void) {
	/*Set Up Kernel Internal Device*/
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL)
		return -ENOMEM;
	
	/*Initilize Crypto Handle*/
	/*Middle arguement being 0 means use default cipher handling*/
	tfm = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(tfm)){
		printk("~BLKDEVCRYPT~ sbd_init() -- [tfm] failed to initialize\n");
		goto out;		
	}
	
	/*Initilize The Request Queue */
	Queue = blk_init_queue(sbd_request, &Device.lock);
	if (Queue == NULL){
		printk("~BLKDEVCRYPT~ sbd_init() -- [Queue] failed to initialize\n");		
		goto out;
	}
	blk_queue_logical_block_size(Queue, logical_block_size);

	/*Register The Block Device*/
	major_num = register_blkdev(major_num, "sbd");
	if (major_num < 0) {
		printk(KERN_WARNING "~BLKDEVCRYPT~ sbd_init() -- [major_num] failed to initialize\n");
		goto out;
	}
	
	/*And the gendisk structure*/
	Device.gd = alloc_disk(16);
	if (!Device.gd){
		printk("~BLKDEVCRYPT~ sbd_init() -- [Device.gd] failed to initialize\n");				
		goto out_unregister;
	}

	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &sbd_ops;
	Device.gd->private_data = &Device;
	strcpy(Device.gd->disk_name, "sbd0");
	set_capacity(Device.gd, nsectors);
	Device.gd->queue = Queue;

	/*Final Registration Step With Kernel*/
	add_disk(Device.gd);
	printk("~BLKDEVCRYPT~ sbd_init() -- Successfully initialized block device\n");				

	return 0;

out_unregister:
	unregister_blkdev(major_num, "sbd");
out:
	vfree(Device.data);
	/*clear the crypto handle*/
	crypto_free_cipher(tfm);
	return -ENOMEM;
}

static void __exit sbd_exit(void)
{
	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "sbd");
	blk_cleanup_queue(Queue);
	vfree(Device.data);
	/*clear the crypto handle*/
	crypto_free_cipher(tfm);
}

MODULE_AUTHOR("Jeremy Fischer and Omeed");
MODULE_DESCRIPTION("A block device driver with encryption and decryption");
module_init(sbd_init);
module_exit(sbd_exit);