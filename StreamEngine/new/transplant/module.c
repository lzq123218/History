
#define __KERNEL__
#define MODULE
#define EXPORT_SYMTAB

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/miscdevice.h>

#include <asm/uaccess.h>

#include "eng.h"
#include "engine.c"
#include "viruslib_in.c"

static
int eng_open(struct inode *inode, struct file *file){

	MOD_INC_USE_COUNT;
	return 0;
}

static
int eng_release(struct inode *inode, struct file *file){

	MOD_DEC_USE_COUNT;
	return 0;
}

#define ENG_ADDLIB       0x90
#define ENG_ADDLIB2      0x91
#define ENG_UNLOADLIB    0x92 

#define ENG_CONNECT      0x80
#define ENG_DISCON       0X81
#define ENG_SCAN         0x82

struct rs_connect{
	long protocol;
	long ret;
};
struct rs_scan{
	long context;
	char* buff;
	long len;
	long ret;
};

static
int eng_ioctl(struct inode *inode, struct file *file, unsigned int cmd,unsigned long arg){

	LibRec record;
	long cout =0 ;
	BOOL res;

	if (
		(cmd == ENG_ADDLIB)||(cmd == ENG_ADDLIB2)		
	){

		if ( arg == 0 ) return -EINVAL;

		memset( &record, 0, sizeof(LibRec) );

		cout = copy_from_user( &record, (void*)arg, sizeof(LibRec) );

		if ( cout ) return -EINVAL; 

		if ( cmd == ENG_ADDLIB ){

			res = VirusRecProcess(&record);
		}else{
		
			res = VirusRecProcess2(&record);
			//printk("res = %d\n", res);
		}

		if ( res == FALSE ){

			UnloadVirusLib();
			return -EFAULT ;
		}

		return 0;

	}
	if ( cmd == ENG_UNLOADLIB ){

		UnloadVirusLib();
		return 0;
	}

	if ( arg== NULL ) return -1;

	switch( cmd ){
	
	case ENG_CONNECT:
		{struct rs_connect a;
		
		if ( copy_from_user( &a, (void*)arg, sizeof(struct rs_connect)) ){
			return -1;
		}
		printk("protocol =%d\n", a.protocol);			
		a.ret = Connect_Event(a.protocol);
		
		if (a.ret == 0) return -1;
		printk("ret =%d\n",a.ret);
		if ( copy_to_user( (void*)arg, &a, sizeof(struct rs_connect))){
			return -1;
		}
		return 0;
		}
	case ENG_DISCON:
		{ long val;

		if ( copy_from_user( &val, (void*)arg ,sizeof(long) )){
			return -1;
		}
		if ( val == 0 )return -1;
		printk("disconnet %d\n", val);
		Disconnect_Event( val );
		return 0;
		}
	case ENG_SCAN:
		{struct rs_scan a; char* buff = NULL;
		
		if ( copy_from_user( &a, (void*)arg, sizeof(struct rs_scan) ) ){
			return -1;
		}
		if (a.len == 0) return -1;

		buff = kmalloc( a.len, GFP_KERNEL);
		
		if ( buff == NULL ) return -1;
                
		if ( copy_from_user( buff,(void*)a.buff,a.len ) ){
			kfree( buff);
			return -1;
		}

		//printk("buff len = %d\n", a.len);

		a.ret = DataScan( a.context, buff, a.len);
		//printk("scan res = %d\n", a.ret);	
		kfree(buff);
		buff = NULL;
		if ( copy_to_user( (void*)arg, &a, sizeof(struct rs_scan)))
			return -1;
		return 0;
		}

	default:
		;
	}
	return -EINVAL;
}


static
struct file_operations eng_fops = {

	.owner	= THIS_MODULE,
	.llseek	= NULL,
	.read	= NULL,
	.poll	= NULL,
	.ioctl	= eng_ioctl,
	.open	= eng_open,
	.release= eng_release,
	.fasync	= NULL,
};

#define ENGINE_MINOR  104

static
struct miscdevice eng_dev={
	
	ENGINE_MINOR,
	"engine",
	&eng_fops
};


static
int eng_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data){

	return -1;
}

int Init_engine(){

	if (misc_register(&eng_dev)){
		
		printk( "misc_register error\n");
		return -ENODEV;		
	}
	
	if (create_proc_read_entry ("driver/engine", 0, 0, eng_read_proc, NULL) == NULL) {
		
		printk( "entry create error\n");
		misc_deregister(&eng_dev);
		return -ENOMEM;	
	}
	EngineInit();
	return 0;
}

void UnInit_engine(){

	remove_proc_entry ("driver/engine", NULL);
	misc_deregister(&eng_dev);	
}

module_init( Init_engine );
module_exit( UnInit_engine);

int RS_data_scan( DWORD Context, const BYTE* pbDataBuff, DWORD Length ){

	return DataScan( Context, pbDataBuff, Length );	
}

int RS_connect_event( int Protocol){

	return Connect_Event( Protocol);	
}

void RS_disconnect_event( int Context){

	Disconnect_Event( Context);
}

EXPORT_SYMBOL_NOVERS( RS_data_scan );
EXPORT_SYMBOL_NOVERS( RS_connect_event );
EXPORT_SYMBOL_NOVERS( RS_disconnect_event );

