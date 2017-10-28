/*
	 * Below is the LOOK IO elevator scheduler
	 * The structure was taken from linux-yocto-3.19/block/noop-iosched.c
	 * This website is VERY helpful in understanding the API calls in this
		 code: http://www.cse.unsw.edu.au/~aaronc/iosched/doc/api/index.html
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
static void look_merged_requests(struct request_queue *req_q, struct request *rq,
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

	if (!list_empty(&look->queue)) {
		struct request *rq;
		rq = list_entry(look->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(req_q, rq);
		return 1;
	}
	return 0;
}
/*
	* Queues a new request with the scheduler.
*/
static void look_add_request(struct request_queue *req_q, struct request *rq)
{
	struct look_data *look = req_q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &look->queue);
}
/*
	* Returns the request BEFORE the current req in start-sector order. 
		Used to discover opportunities to place two adjacent 
		requests next to eachother (i.e. drop two 
		people off at same floor).
*/
static struct request *
look_former_request(struct request_queue *req_q, struct request *rq)
{
	struct look_data *look = req_q->elevator->elevator_data;

	if (rq->queuelist.prev == &look->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}
/*
	* Returns the request AFTER the current req in start-sector order. 
		Used to discover opportunities to place two adjacent 
		requests next to eachother (i.e. drop two 
		people off at same floor).
*/
static struct request *
look_latter_request(struct request_queue *req_q, struct request *rq)
{
	struct look_data *look = req_q->elevator->elevator_data;

	if (rq->queuelist.next == &look->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
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
	.elevator_name = "LOOK ELEVATOR",
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