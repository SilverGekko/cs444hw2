/*
 * elevator look
 * Nick's implementation
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#define DEBUG 0

struct look_data {
	struct list_head queue;
};

/* this can remain the same */
static void look_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}


static int look_dispatch(struct request_queue *q, int force)
{
#if DEBUG
	printk("look_dispatch\n");
#endif
	struct look_data *ld = q->elevator->elevator_data;
	char data_dir;

	if (!list_empty(&ld->queue)) {
		struct request *rq;
		rq = list_entry(ld->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		
		/* debug */
		if (rq_data_dir(rq) == READ)
			data_dir = 'R';
		else
			data_dir = 'W';

#if DEBUG
		 printk("look dispatch\ndir: %c\nrq: %lu\n", data_dir, blk_rq_pos(rq));
#endif

		return 1;
	}
	return 0;
}

/* insert request rq into request_queue q in the proper position*/
static void look_add_request(struct request_queue *q, struct request *rq)
{
#if DEBUG
	printk("look_add_request\n");
#endif
	struct look_data *ld = q->elevator->elevator_data;
	int already_added = 0;
	/* if the list is empty, no need for fancy algorithm: insert it at the tail*/
	if (list_empty(&ld->queue)) {
#if DEBUG
		printk("empty list\n");
#endif
		list_add_tail(&rq->queuelist, &ld->queue);
	}
	/* else, we need to add the new request in the best possible location*/
	else {
#if DEBUG
		printk("inside else statement\n");
#endif
		/* variable to hold the current reuqest we are looking at*/
		struct list_head* cursor_rq = NULL;
		/* sector that the incoming request is targeting*/
		sector_t rq_sector = blk_rq_pos(rq);
#if DEBUG
		printk("rq_sector: %d\n", rq_sector);
#endif

		/* for each item in the request queue...*/
		list_for_each(cursor_rq, &ld->queue) {
			/* get the request in the queue at the current position*/
#if DEBUG
		printk("inside list_for_each\n");
#endif
			struct request *cursor;
			cursor = list_entry(cursor_rq, struct request, queuelist);
			/* if the incoming request is for a sector less than the current sector*/
			if(rq_sector < blk_rq_pos(cursor)) {
#if DEBUG
				printk("inside if statement\n");
#endif
				/* add the incoming requests to the rquest queue*/
				list_add(&rq->queuelist, cursor_rq);
				already_added = 1;
				break;
			}
		}
		/* if the list traversal finds no place to put the list, back merge the incoming request */
		if (already_added == 0)
			list_add_tail(&rq->queuelist, &ld->queue);
	}
#if DEBUG
	printk("end of look_add function");
#endif
}

static struct request *
look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *ld = q->elevator->elevator_data;

	if (rq->queuelist.prev == &ld->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *ld = q->elevator->elevator_data;

	if (rq->queuelist.next == &ld->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int look_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct look_data *ld;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	ld = kmalloc_node(sizeof(*ld), GFP_KERNEL, q->node);
	if (!ld) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = ld;

	INIT_LIST_HEAD(&ld->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *ld = e->elevator_data;

	BUG_ON(!list_empty(&ld->queue));
	kfree(ld);
}

static struct elevator_type elevator_look = {
	.ops = {
		.elevator_merge_req_fn		= look_merged_requests,
		.elevator_dispatch_fn		= look_dispatch,
		.elevator_add_req_fn		= look_add_request,
		.elevator_former_req_fn		= look_former_request,
		.elevator_latter_req_fn		= look_latter_request,
		.elevator_init_fn		= look_init_queue, 
		.elevator_exit_fn		= look_exit_queue, 
	},
	.elevator_name = "look",
	.elevator_owner = THIS_MODULE,
};

static int __init look_init(void)
{
	return elv_register(&elevator_look);
}

static void __exit look_exit(void)
{
	elv_unregister(&elevator_look);
}

module_init(look_init);
module_exit(look_exit);


MODULE_AUTHOR("Nicholas Pugliese");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Look IO scheduler");
