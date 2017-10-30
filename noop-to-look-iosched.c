/*
	 * Below is the LOOK IO elevator scheduler
	 * The structure is based off the NOOP scheduluer in 
	 * 	linux-yocto-3.19/block/noop-iosched.c
	 * This website is VERY helpful in understanding the API calls in this
		 code: http://www.cse.unsw.edu.au/~aaronc/iosched/doc/api/index.html
	* This old assignment is also a very helpful resource:
		http://classes.engr.oregonstate.edu/eecs/fall2011/cs411/proj03.pdf
	* Helpful for changing scheduler from command line:
		https://www.techrepublic.com/article/how-to-change-the-linux-io-scheduler-to-fit-your-needs/
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>


/*
 	* sector_t is the type used for indexing onto a disc or disc partition.
		Linux always considers sectors to be 512 bytes long independently
		of the devices real block size.

	* typedef u64 sector_t;
*/
struct look_data {
	struct list_head queue; //head pointer to circularly linked list
	int direction;
	sector_t disk_head;
};
/*
	* Called when two requests have been turned into a single request. 
		This occurs when a request grows (as a result of a bio merge) 
		to become adjacent with an existing request.
*/
static void look_merged_requests(struct request_queue *req_q, struct request *cur_req,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}
/*
	* Requests the scheduler to populate the dispatch queue 
		with requests. Once requests have been dispatched, 
		the scheduler may not manipulate them. 
	* Returns the number of requests dispatched.
*/
static int look_dispatch(struct request_queue *req_q, int force)
{
	struct look_data *look = req_q->elevator->elevator_data;
	struct request *next_req, *prev_req, *cur_req;
	if (!list_empty(&look->queue)){
		next_req = list_entry(look->queue.next, struct request, queuelist);
		prev_req = list_entry(look->queue.prev, struct request, queuelist);	
		cur_req = next_req; //in case there's only one req in queue

		/*If we're moving forward*/
		if(look->direction == 1){
			/*If there is another req in this direction take it*/
			if(blk_rq_pos(next_req) >= look->disk_head){
				printk("Moving: 'forward' ---- NextReq: 'forward' ---- Continuing Forward\n");
				cur_req = next_req;
			}
			/*Change direction if there's no more reqs in fwrd direction
			  	Note how this traverses the list backwards instead of simply
				following the circular list to the front again. This is what
				differentiates this LOOK scheduler from a C-LOOK
			*/			
			else{
				printk("Moving: 'forward' ---- NextReq: 'backward' ---- Turning Around Now \n");				
				look->direction = 0;
				cur_req = prev_req; //turn around
			}
		}
		/*Else we're moving backward*/
		else{
			/*If there is another req in this direction take it*/			
			if(blk_rq_pos(prev_req) < look->disk_head){
				printk("Moving: 'backward' ---- NextReq: 'backward' ---- Continuing Backward\n");				
				cur_req = prev_req;
			}
			/*Change direction if there's no more reqs in back direction*/
			else{
				printk("Moving: 'backward' ---- NextReq: 'forward' ---- Turning Around Now \n");								
				look->direction = 1;
				cur_req = next_req; //turn around
			}
		}
		/*Assert that cur_req is valid*/
		if(cur_req){
			printk("disk_head is currently at: %llu ---- ", (unsigned long long)look->disk_head);
			look->disk_head = blk_rq_pos(cur_req); //keep track of head
			printk("disk_head is moving to: %llu \n", (unsigned long long)look->disk_head);			
			list_del_init(&cur_req->queuelist); //remove the cur_req from scheduler
			elv_dispatch_add_tail(req_q, cur_req); //add cur_req to the block's queue
			return 1;
		}
	}
	/*No requests*/
	return 0;
}
/*
	* Queues a new request with the scheduler.
	* This is like an insertion sort on the fly
	* 1) Check if the run queue is empty
			Yes --> add the cur_req directly
	* 2) If its not
			Loop until cur_req's correct position is found
			Insert cur_req there	
*/
static void look_add_request(struct request_queue *req_q, struct request *cur_req)
{
	struct look_data *look = req_q->elevator->elevator_data;
	struct request *next_req, *prev_req;

	if(list_empty(&look->queue)){
		printk("request_queue is empty, adding cur_req directly \n");
		list_add(&cur_req->queuelist, &look->queue);		
	}
	else{
		next_req = list_entry(look->queue.next, struct request, queuelist);
		prev_req = list_entry(look->queue.prev, struct request, queuelist);
		/*traverse the queue looking for where to place the cur_req	*/
		printk("Request needing to be added - location: %llu \n", (unsigned long long)blk_rq_pos(cur_req));
		printk("request_que up until the position of the new request: ");
		while(blk_rq_pos(cur_req) >= blk_rq_pos(next_req)){
			printk("%llu ", (unsigned long long)blk_rq_pos(next_req));
			/*list_entry() gets the struct for the given entry*/
			next_req = list_entry(next_req->queuelist.next, struct request, queuelist);
			prev_req = list_entry(prev_req->queuelist.prev, struct request, queuelist);
		}
		/*insert the cur_req ahead of the prev_req and behind the next_req*/
		list_add(&cur_req->queuelist, &prev_req->queuelist);
	}
}
/*
	* Returns the request BEFORE the current req in start-sector order. 
		Used to discover opportunities to place two adjacent 
		requests next to eachother (i.e. drop two 
		people off at same floor).
*/
static struct request *
look_former_request(struct request_queue *req_q, struct request *cur_req)
{
	struct look_data *look = req_q->elevator->elevator_data;

	if (cur_req->queuelist.prev == &look->queue)
		return NULL;
	return list_entry(cur_req->queuelist.prev, struct request, queuelist);
}
/*
	* Returns the request AFTER the current req in start-sector order. 
		Used to discover opportunities to place two adjacent 
		requests next to eachother (i.e. drop two 
		people off at same floor).
*/
static struct request *
look_latter_request(struct request_queue *req_q, struct request *cur_req)
{
	struct look_data *look = req_q->elevator->elevator_data;

	if (cur_req->queuelist.next == &look->queue)
		return NULL;
	return list_entry(cur_req->queuelist.next, struct request, queuelist);
}
/*
	* Initialize a scheduler instance
*/
static int look_init_queue(struct request_queue *req_q, struct elevator_type *e)
{
	struct look_data *look;
	struct elevator_queue *elev_q;

	elev_q = elevator_alloc(req_q, e);
	if (!elev_q)
		return -ENOMEM;

	look = kmalloc_node(sizeof(*look), GFP_KERNEL, req_q->node);
	if (!look) {
		kobject_put(&elev_q->kobj);
		return -ENOMEM;
	}
	elev_q->elevator_data = look;

	INIT_LIST_HEAD(&look->queue);

	spin_lock_irq(req_q->queue_lock);
	req_q->elevator = elev_q;
	spin_unlock_irq(req_q->queue_lock);
	return 0;
}
/*
	* Clean up scheduler instance
*/
static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *look = e->elevator_data;

	BUG_ON(!list_empty(&look->queue));
	kfree(look);
}
/*
	* Have the built in elevator functions point to our
		implemented functions
*/
static struct elevator_type elevator_look = {
	.elevator_name = "LOOK",
	.elevator_owner = THIS_MODULE,	
	.ops = {
        .elevator_merge_req_fn = look_merged_requests,
        .elevator_dispatch_fn = look_dispatch,
        .elevator_add_req_fn = look_add_request,
		.elevator_former_req_fn = look_former_request,
        .elevator_latter_req_fn = look_latter_request,
        .elevator_init_fn = look_init_queue,
        .elevator_exit_fn = look_exit_queue,
	},
};
/*
	* Register our LOOK elevator
*/
static int __init look_init(void)
{
	return elv_register(&elevator_look);
}
/*
	* On exit, unregister out LOOK Elevator
*/
static void __exit look_exit(void)
{
	elv_unregister(&elevator_look);
}

module_init(look_init);
module_exit(look_exit);

MODULE_AUTHOR("Jeremy Fischer");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LOOK IO scheduler");