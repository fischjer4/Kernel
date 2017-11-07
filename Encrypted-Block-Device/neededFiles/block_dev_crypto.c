 /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
*	- The skeleton for the block device code was taken from Pat Paterson at
*		http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
*	- Crypto
*		https://01.org/linuxgraphics/gfx-docs/drm/crypto/intro.html
*		https://01.org/linuxgraphics/gfx-docs/drm/crypto/api-skcipher.html#single-block-cipher-api
*	- Module Parameters
*		http://www.makelinux.net/ldd3/chp-2-sect-8
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
#include <linux/crypto.h> /* For Linux Crypto API */

MODULE_LICENSE("GPL");

static int major_num = 0;
module_param(major_num, int, 0);

static int logical_block_size = 512;
module_param(logical_block_size, int, 0);

/* How big the drive is */
static int nsectors = 1024; 
module_param(nsectors, int, 0);

/* keylen, perm: rw_r_____ */
static unsigned int keylen = 16;
module_param(keylen, int, 0640);

/* key, charp is char pointer type, perm: rw_r_____ */
static char *key = "abcdefghijklmnop";
module_param(key, charp, 0640);

/* The kernel talks to us in terms of 512 byte sectors */
#define KERNEL_SECTOR_SIZE 512

/* Request Queue */
static struct request_queue *Queue;

/* Internal Representation Of The Block Device */
static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/* The device operations structure */
int sbd_getgeo(struct block_device* , struct hd_geometry* );
static struct block_device_operations sbd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_getgeo
};

/* Crypto Handle */
struct crypto_cipher *tfm;





/*
	* Print limit bytes from source
	* Used to print from the buffer and block device memory
*/
static void print_mem(unsigned char *source, unsigned int limit){
	unsigned int i = 0;
	while (i < limit){
		print("%02X", (unsigned)source[i]);
     	i++;
	}
	printk("\n");
}
/*
 	* Handle an I/O request.
 */
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
						 unsigned long nsect, char *buffer, int write) {
	
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size; //num bytes to r/w
	unsigned long i = 0; //used as block indexer

	/* if there's no more space in the drive, stop */
	if ((offset + nbytes) > dev->size) {
		printk(KERN_NOTICE "~BLKDEVCRYPT~ sbd_transfer() -- Beyond-end write (%ld %ld)\n", offset, nbytes);			
		return;
	}

	/* set the crypto key */
	if(crypto_cipher_setkey(tfm, key, keylen) < 0){
		printk("~BLKDEVCRYPT~ sbd_transfer() -- [key] failed to initialize \n");			
		return;
	}

	/*	
		nbytes represents the number of bytes to be read/written.
		The en/decrypt function handles one block at a time, so if
		nbytes > blocksize then we must en/decrypt each block sequentially
		one at a time
	*/
	printk("%s %lu bytes from the block device", (write ? "write" : "read"), nbytes);
	if (write){
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Write: before encryption: ");	
		print_mem(buffer, nbytes);			
		/* while theres more blocks to write to, write */
		while(i < nbytes){
			crypto_cipher_encrypt_one(tfm, (dev->data + offset + i), buffer + i);
			i += crypto_cipher_blocksize(tfm); //go to next block
		}
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Write: after encryption: ");					
		print_mem(dev->data + offset, nbytes);					
	}
	else{
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Read: before decryption: ");					
		print_mem(dev->data + offset, nbytes);					
		/* while theres more blocks to read from, read */									
		while(i < nbytes){
			crypto_cipher_decrypt_one(tfm, buffer + i, (dev->data + offset + i));
			i += crypto_cipher_blocksize(tfm); //go to next block
		}
		printk("~BLKDEVCRYPT~ sbd_transfer() -- Read: after decryption: ");					
		print_mem(buffer, nbytes);					
	}
}
/*
	* Get the next request in the Queue
	* If a request isn't finished, keep working on it
		If it is, get the next one in line
*/
static void sbd_request(struct request_queue *q) {
	struct request *req;

	/* get next request */
	req = blk_fetch_request(q);
	while (req != NULL) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk(KERN_NOTICE "~BLKDEVCRYPT~ sbd_request() -- [req] non-CMD request\n");			
			__blk_end_request_all(req, -EIO);
			continue;
		}
		sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req), bio_data(req->bio), rq_data_dir(req));
		/* if this req is done, get the next one */
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
	* Initialize Block Device With Kernel, which includes:
		- block size
		- crypto handle
		- request queue
		- major number
		- gendisk structure
		- tell kernel we're ready
*/
static int __init sbd_init(void) {
	/* Set Up Kernel Internal Device */
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL)
		return -ENOMEM;
	
	/* Initilize Crypto Handle */
	/* Middle arg being 0 means use default cipher handle */
	tfm = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(tfm)){
		printk("~BLKDEVCRYPT~ sbd_init() -- [tfm] failed to initialize\n");
		goto out;		
	}
	
	/* Initilize The Request Queue */
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
	
	/* For proof that key was loaded */
	printk("~BLKDEVCRYPT~ sbd_init() -- crypto key: %s\n", key);				

	return 0;

	out_unregister:
		unregister_blkdev(major_num, "sbd");
	out:
		vfree(Device.data);
		/*clear the crypto handle*/
		crypto_free_cipher(tfm);
		return -ENOMEM;
}
/*
	* Exit the module, but first clean up
*/
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
