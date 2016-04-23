
//
//Macro for Register to manipulate serial
//
#define TRANS_FIFO_SIZE	16
#define ARCA2

#ifndef ARCA2
#define baseaddr  0x00000300
#else
#define baseaddr  0xa8000300
#endif


#define reg_wr	( baseaddr +0xf8 )
#define reg_rd	( baseaddr +0xf8 )

#define reg_ier	( baseaddr +0xf9 )
#define reg_iir	( baseaddr +0xfa )

#define reg_fcr	( baseaddr +0xfa )

#define reg_lcr	( baseaddr +0xfb )
#define reg_lsr	( baseaddr +0xfd )

#define reg_mcr	( baseaddr +0xfc )
#define reg_msr	( baseaddr +0xfe )

#define reg_scr	( baseaddr +0xff )

#define reg_dll	( baseaddr +0xf8 )
#define reg_dlh	( baseaddr +0xf9 )


//
//Macro for Register end
//
//for sleep_or_start parameter mode
#define	MODE_WRITE        5
#define	MODE_READ          6
#define	MODE_WAIT          7
#define	MODE_PURGE        8

//
//internal data struct
//
typedef struct _timer_control{
	char already_in_queue ;
	struct timer_list  timer ;
	unsigned long  data ;
	
} timer_control , *Ptimer_control ;

typedef struct  _current_control{
	struct task_struct * ptask ;
	wait_queue_head_t   wq ;
	unsigned char  wake_up_reason ;
} current_control ;

typedef struct  _operation_queue{
	int count ;
	wait_queue_head_t  wq ;
} operation_queue ;

typedef  struct  _driver_data{
	int unused;
	BOOL purge_tx ;
	BOOL purge_rx ;
	unsigned long	supported_bauds;
	unsigned long	clock_rate ;
	unsigned long	current_baud ;
	unsigned long	tx_holding ;
	unsigned long	rx_holding ;
	char line_control ;
	char valid_data_mask;
	char escape_char ;
	
	BOOL transmit_immediate ;
	char	immediate_char ;
	BOOL holding_empty ;
	BOOL send_xon ;
	BOOL send_xoff ;
	
	BOOL fifo_present ;
	unsigned long tx_fifo_amount;
	
	BOOL emptied_transmit ; //      no sure
		
	unsigned long buffer_size ;
	unsigned long buffer_size_p8 ;
	unsigned long error_word ;
	unsigned long read_by_isr ;
	
	unsigned long chars_interrupt_buffer ;
	char *read_buffer_base ;
	char *current_char_slot ;
	char *last_char_slot ;
	char *first_readable ;
	char *interrupt_read_buffer ;
	char *pcurrent_write_char ;
	unsigned long *p_wait_value;
	unsigned long write_length ;
	unsigned long write_remain ;
	unsigned long read_already;
	
	unsigned long count_since_xoff ;
	unsigned long isr_wait_mask ;
	unsigned long history_mask ;
		
	Serial_timeouts   timeouts ;
	Serial_chars  special_chars ;
	Serial_handflow  hand_flow ;
	Serial_perf_stats  perf_stats ;
		
//	timer_control	wait_mask_timer ;
	timer_control low_rts_timer ;
	timer_control read_timer ;
	timer_control write_timer ;
	timer_control immediate_timer ;
	timer_control xoff_count_timer ;
	timer_control error_timer ;
	
} Drvdata, *PDrvData;
