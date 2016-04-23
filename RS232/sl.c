/***************************************************************************
                          sl.c  -  description
                             -------------------
    begin                : Mon Jul 28 2003
    copyright            : (C) 2003 by yuanzhang
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/**
static char  *win_driver ="serial for windows";
static char  *version = "0.2 2003-12-26";
static char  *author = "timer";
*/
/**
modification:
2003-12-26 add some prinks for debug  wait_mask  unction
*/

#define     __KERNEL__
#define    MODULE
#define    zy_driver
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/malloc.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/string.h>

#include "headio.h"
#include "internal.h"

//varibles==global
static struct timer_list  trigger;
static const unsigned int  zyMJ_dev = 139;
static char zyDevName[] = "zhang";

#ifndef    ARCA2
static unsigned int zyIrq = 4 ;
#else
static unsigned int zyIrq = 27 ;
#endif

static int  zyOpend ;

PDrvData  zy_driver_data = NULL ;

current_control current_read , current_write , current_mask ;

operation_queue	read_queue , write_queue , mask_queue , purge_queue ;    //wait_queue used to serialize io request

current_control	current_wait_mask;

current_control	current_immediate ; //for transmit_immediate

current_control	current_xoff_count ;
current_control	current_purge ;
//operation_queue   ;

wait_queue_head_t   wq_query ; // only used  in  set_wait_mask
wait_queue_head_t	wq_immediate ; //only used in  trans_immediate

//varibles
//FUN HEAD
BOOL get_divisor( ULONG ClockRate, long DesiredBaud, short *AppropriateDivisor);
void set_divisor( PDrvData pdata, short divisor);
char process_lsr(PDrvData pdata );
void timeout_write( Ptimer_control ptc );
void put_char(PDrvData pdata , char char_to_put );
void timeout_error( Ptimer_control ptc );
void zy_wake_up( operation_queue* poq );
void dec_ref( operation_queue * poq );
void inc_ref( operation_queue * poq );
void zyIsr( int irq , void *dev_id , struct pt_regs *regs );
//
// driver fuctions which is used to manipulate hardware
//

void disable_all_intp( void){
	outb( 0x00 ,  reg_ier) ;
}
void enable_all_intp( void ){
	outb( 0x0f ,  reg_ier ) ;
}
void invoke_hardware( unsigned long pd){
	printk("trigger interrupt \n");
	disable_all_intp();
	enable_all_intp();
}
void init_current_control(current_control *pcc ){
	pcc->ptask = NULL ;
	pcc->wake_up_reason = WAKE_UP_REASON_NODATA ;
	init_waitqueue_head( &(pcc->wq) );
}

void init_operation_queue(operation_queue  *poq ){
	poq->count = 0 ;
	init_waitqueue_head( &(poq->wq) );
}
int zy_buffer_init( PDrvData pdata ){
		void *temp = NULL;

	pdata->interrupt_read_buffer = NULL ;
	pdata->buffer_size = BUFFER_SIZE ;
	
	temp = kmalloc( pdata->buffer_size, GFP_KERNEL );
	if ( temp ){

		pdata->interrupt_read_buffer = temp ;
		pdata->buffer_size_p8 = ( 3*( pdata->buffer_size >> 2) ) + ( pdata->buffer_size >> 4 ) ;
		
		printk("buffer_size =%d \n", ( int )pdata->buffer_size );
		return TRUE ;
	}else{
		
		return FALSE ;
	}
}
void init_serial( PDrvData pdata ){
	printk("device initiation \n");
	
	if( zy_buffer_init( pdata ) == FALSE ){
		printk("fatal error\n");
		return ;
	}
	pdata->chars_interrupt_buffer = 0 ;
	pdata->last_char_slot = pdata->interrupt_read_buffer +( pdata->buffer_size -1 );
	pdata->read_buffer_base = pdata->interrupt_read_buffer ;
	pdata->current_char_slot = pdata->interrupt_read_buffer ;
	pdata->first_readable = pdata->interrupt_read_buffer ;
	
	init_current_control( &current_read );
	init_current_control( &current_write );	
	init_current_control( &current_mask );
	init_current_control( &current_wait_mask );
	init_current_control( &current_immediate );
	init_current_control( &current_xoff_count);
	init_current_control( &current_purge);
	
	init_operation_queue( &read_queue );
	init_operation_queue( &write_queue );
	init_operation_queue( &mask_queue );
	init_operation_queue( &purge_queue );
	
	pdata->p_wait_value = NULL ;
	pdata->low_rts_timer.already_in_queue = 0 ;
	pdata->immediate_timer.already_in_queue = 0 ;
	pdata->xoff_count_timer.already_in_queue = 0 ;
	pdata->error_timer.already_in_queue =0 ;

	pdata->isr_wait_mask = 0 ;
	pdata->history_mask = 0 ;
	
	pdata->tx_holding = 0 ;
	pdata->rx_holding = 0 ;
	pdata->escape_char = 0;
	
	pdata->read_by_isr = 0 ;
	
	pdata->transmit_immediate = FALSE ;
	pdata->holding_empty = TRUE ;
	pdata->send_xon= FALSE;
	pdata->send_xoff = FALSE;
	pdata->purge_tx = FALSE;
	pdata->purge_rx = FALSE;
	
	pdata->write_length = 0;
	pdata->pcurrent_write_char = NULL;
	
	pdata->hand_flow.XoffLimit = pdata->buffer_size >> 3 ;
	pdata->hand_flow.XonLimit = pdata->buffer_size >>1 ;
		
	pdata->error_word = 0;
	pdata->count_since_xoff = 0 ;
	pdata->clock_rate = 115200*16 ;
	pdata->current_baud = 9600 ;   //default

	{ // set baud rate divisor
		unsigned long flags ;
		short divisor ;
		BOOL status ;
		
		status = get_divisor( pdata->clock_rate, pdata->current_baud ,&divisor);
		if ( status ){
			save_flags( flags);
			cli();
			
			set_divisor( pdata, divisor);			
			restore_flags( flags );		
		}	
	}
	
	pdata->line_control = 0x03 ;              //LCR
	outb( pdata->line_control , reg_lcr ); 	 	
	pdata->valid_data_mask = 0xff ;
	
	pdata->special_chars.XonChar = 0x11 ;
	pdata->special_chars.XoffChar = 0x13 ;
	pdata->hand_flow.ControlHandShake = SERIAL_DTR_CONTROL ;
	pdata->hand_flow.FlowReplace = SERIAL_RTS_CONTROL ;


	outb( 0, reg_mcr );		//disable uart
	disable_all_intp();
	enable_all_intp();
	outb( 0, reg_fcr );   		//disable and clean fifo
	outb( 1, reg_fcr );
	outb( 0x87, reg_fcr ); 	//	
	
	if ( inb(reg_iir) & 0xc0 ){
		pdata->fifo_present = TRUE ;
		pdata->tx_fifo_amount = TRANS_FIFO_SIZE ;
	}else{
	      pdata->fifo_present = FALSE ;
	}
	outb( 0x08, reg_mcr );	//enable uart	
}
//
//---------------------helper--------------------------------
//
char get_low_byte(short  num ){

	char * pchar = (char*) &num;

	return *pchar ;
}

char get_high_byte(short  num ){

	char * pchar = (char*) &num;
	
	pchar++;
	return *pchar;
}

short get_low_2bytes( int  num ){
		short *pshort = (short*)&num ;

	return  *pshort ;
}


void read_io(unsigned int Port , void * point_out ){
	
	unsigned char  data;
	data  = inb( Port ) ;
	copy_to_user( point_out , &data , 1 ); // vary with  OS version
	
	printk("Read Port  %x  data is %d \n", Port , data );
	
}

void write_io( unsigned int Port , unsigned value ){

	char data =  get_low_byte( get_low_2bytes( value ) );
	
	printk("Write  Port  %x  data is %d \n", Port, data );
	outb( data , Port );
	
}
int timer_del( Ptimer_control ptc ){
	unsigned long flags;

	save_flags( flags );
	cli();
	if( ptc->already_in_queue ==1){
		del_timer( &(ptc->timer) );
		ptc->already_in_queue =0 ;
		printk( "delete timer ok\n");
		restore_flags( flags);
		return 1 ;
	}else{
		restore_flags( flags);
		return 0 ;
	}
} 
int timer_set( Ptimer_control  ptc ,  unsigned long expires , FUN  fun, unsigned long para){
		unsigned long flags ;

	printk("timer_set \n");
	if ( ptc == NULL ) return -1 ;
	if ( expires == 0 ) return -1 ;
	if ( fun == NULL ) return -1 ;
		
	save_flags( flags);
	cli();
	
	if ( ptc->already_in_queue ==1){
	
		restore_flags( flags );
		printk("already in queue \n");
		return -1;
		
	}else{
		
		init_timer( &(ptc->timer) );
		ptc->timer.expires = expires+jiffies ;
		ptc->timer.function = fun ;
		ptc->timer.data = (unsigned long)ptc ;
		ptc->data = para ;
		ptc->already_in_queue =1;
		add_timer( &(ptc->timer) );
		
    	restore_flags( flags);
		return 0 ;
	}
}


//-------------------------------------0---------------------------9------------------------------------------------------------
void process_empty_transmit( PDrvData pdata ){

	if ( pdata->isr_wait_mask && ( pdata->isr_wait_mask &SERIAL_EV_TXEMPTY )&&
		pdata->emptied_transmit &&( ! pdata->transmit_immediate )&&
		( current_write.ptask ==NULL)&& ( write_queue.count == 0)
	){
		pdata->history_mask |= SERIAL_EV_TXEMPTY ;
		if ( pdata->p_wait_value ){
			printk("SERIAL_EV_TXEMPTY\n");
			*( pdata->p_wait_value) = pdata->history_mask ;
			pdata->p_wait_value = NULL ;
			pdata->history_mask =0 ;
			current_wait_mask.wake_up_reason = WAKE_UP_REASON_OK ;
			wake_up(&(current_wait_mask.wq));
		}
	}
}

BOOL get_divisor( ULONG ClockRate, long DesiredBaud, short *AppropriateDivisor){

   BOOL status = TRUE;
   short calculatedDivisor;
   ULONG denominator;
   ULONG remainder;

// Allow up to a 1 percent error

   ULONG maxRemain18 = 18432;
   ULONG maxRemain30 = 30720;
   ULONG maxRemain42 = 42336;
   ULONG maxRemain80 = 80000;
   ULONG maxRemain;

// Reject any non-positive bauds.
   denominator = DesiredBaud*(ULONG)16;

   if (DesiredBaud <= 0) {

      *AppropriateDivisor = -1;

   } else if ((long)denominator < DesiredBaud) {

// If the desired baud was so huge that it cause the denominator
// calculation to wrap, don't support it.

      *AppropriateDivisor = -1;

   } else {

      if (ClockRate == 1843200) {
         maxRemain = maxRemain18;
      } else if (ClockRate == 3072000) {
         maxRemain = maxRemain30;
      } else if (ClockRate == 4233600) {
         maxRemain = maxRemain42;
      } else {
         maxRemain = maxRemain80;
      }

      calculatedDivisor = (short)(ClockRate / denominator);
      remainder = ClockRate % denominator;
// Round up.
      if (((remainder*2) > ClockRate) && (DesiredBaud != 110)) {

         calculatedDivisor++;
      }
// Only let the remainder calculations effect us if
// the baud rate is > 9600.

      if (DesiredBaud >= 9600) {

         // If the remainder is less than the maximum remainder (wrt
         // the ClockRate) or the remainder + the maximum remainder is
         // greater than or equal to the ClockRate then assume that the
         // baud is ok.
         if ((remainder >= maxRemain) && ((remainder+maxRemain) < ClockRate)) {
            calculatedDivisor = -1;
         }
      }

      // Don't support a baud that causes the denominator to
      // be larger than the clock.
      if (denominator > ClockRate) {

         calculatedDivisor = -1;
      }
      // Ok, Now do some special casing so that things can actually continue
      // working on all platforms.
      if (ClockRate == 1843200) {

         if (DesiredBaud == 56000) {
            calculatedDivisor = 2;
         }

      } else if (ClockRate == 3072000) {

         if (DesiredBaud == 14400) {
            calculatedDivisor = 13;
         }

      } else if (ClockRate == 4233600) {

         if (DesiredBaud == 9600) {
            calculatedDivisor = 28;
         } else if (DesiredBaud == 14400) {
            calculatedDivisor = 18;
         } else if (DesiredBaud == 19200) {
            calculatedDivisor = 14;
         } else if (DesiredBaud == 38400) {
            calculatedDivisor = 7;
         } else if (DesiredBaud == 56000) {
            calculatedDivisor = 5;
         }

      } else if (ClockRate == 8000000) {

         if (DesiredBaud == 14400) {

            calculatedDivisor = 35;
         } else if (DesiredBaud == 56000) {
            calculatedDivisor = 9;
         }
      }
      *AppropriateDivisor = calculatedDivisor;
   }
   if (*AppropriateDivisor == -1) {

      status = FALSE;
   }
   return status;
}
void set_divisor( PDrvData pdata, short divisor){
		char high = get_high_byte( divisor);
		char low = get_low_byte( divisor);
		char lcr ;
		
	lcr = inb( reg_lcr);
	lcr |= (short)0x80 ;
	outb( lcr, reg_lcr); //enable DLAB
	
	outb( high, reg_dlh );
	outb( low,   reg_dll );
	
	lcr &= (short)0x7f ;
	outb( lcr, reg_lcr); //disable DLAB
	
}
int  baud_set ( PDrvData pdata , void *pvoid){
    	Serial_baud_rate  tmp ;
    	unsigned long flags ;
    	short	divisor ;
    	BOOL status ;
    	
    if ( copy_from_user( &tmp, pvoid, sizeof(Serial_baud_rate) ) )return -1 ;
    if ( tmp.BaudRate <= 0 ) return -1 ;

    status = get_divisor( pdata->clock_rate ,tmp.BaudRate, &divisor );

    if ( status == FALSE ) return -1 ;

    save_flags( flags);
    cli();
    pdata->current_baud = tmp.BaudRate ;
    set_divisor( pdata , divisor);
    restore_flags( flags);

    return 0 ;
}

int baud_get( PDrvData pdata , void  *pvoid ){
		Serial_baud_rate tmp ;
		unsigned long flags ;
	
	printk("baud get \n");
	save_flags( flags );
	cli();
	tmp.BaudRate = pdata->current_baud ;
	restore_flags( flags );

	if ( copy_to_user( pvoid , &tmp, sizeof(Serial_baud_rate) ) )  return -1;
	
	return  0 ;
}
int mcr_get( PDrvData pdata,  void *pvoid ){
		long  temp = 0 ;
		unsigned long flags ;
	
	printk("mcr_get \n");
	if( pvoid ==NULL ) return -1;
	save_flags( flags );
	cli();	
	temp = inb(reg_mcr) ;
	restore_flags( flags);
	printk("mcr value is %x \n", (int) temp );

	if ( copy_to_user( pvoid, &temp , sizeof ( long ) )  ) return -1 ;
	
	return 0;	
}
int mcr_set( PDrvData pdata, void *pvoid ){
       long value ;
		short mid ;
		unsigned long flags ;
		
	if ( copy_from_user( &value, pvoid, sizeof(long) ) ) return -1 ;
	mid = get_low_2bytes(value) ;
	printk("mcr_set \n");
	save_flags( flags );
	cli();
	outb( get_low_byte( mid ), reg_mcr ) ;	
	restore_flags( flags);
	return 0 ;	
}
int fcr_set( PDrvData pdata,  void *pvoid ){	 //cautious
   	unsigned long value ;
	short mid ;
	unsigned long flags ;
	
	if ( copy_from_user(&value, pvoid , sizeof(long) ) )	return -1 ;	
	mid =get_low_2bytes(value) ;
	printk("fcr_set \n");
	save_flags( flags );
	cli();
	
	outb( get_low_byte( mid ), reg_fcr ) ;	
	restore_flags( flags );
	return 0 ;
}

int lcr_get( PDrvData pdata , void  *pvoid ){
		Serial_line_control  temp;
		unsigned long flags;

    printk("lcr_get \n");
    if ( pvoid == 0 ){	return  -1 ;}
	save_flags( flags);
	cli();
    switch( pdata->line_control & SERIAL_DATA_MASK ) {
     	case  SERIAL_5_DATA:
     		temp.WordLength = 5 ;
     		break;
     	case   SERIAL_6_DATA:
     		temp.WordLength = 6 ;
     		break;
     	case   SERIAL_7_DATA:
     		temp.WordLength = 7 ;
     		break;
     	case   SERIAL_8_DATA:
     		temp.WordLength = 8 ;
     		break;
     	default:
     	    restore_flags( flags );
     		return -1;
       }

	switch( pdata->line_control & SERIAL_PARITY_MASK ){
	   	case  SERIAL_NONE_PARITY:
	   		temp.Parity = NO_PARITY ;
	   		break ;
	   	case  SERIAL_ODD_PARITY:
	   		temp.Parity = ODD_PARITY ;
	   		break ;
	   	case  SERIAL_EVEN_PARITY:
	   		temp.Parity = EVEN_PARITY ;
	   		break ;
	   	case  SERIAL_MARK_PARITY:
	   		temp.Parity = MARK_PARITY ;
	   		break ;
	   	case  SERIAL_SPACE_PARITY:
	   		temp.Parity = SPACE_PARITY ;
	   		break ;
	   	default :
	   		restore_flags( flags);
	   		return -1 ;
	}
	if (pdata->line_control & SERIAL_2_STOP ){
	
		if ( temp.WordLength == 5 ) temp.StopBits =STOP_BITS_1_5;
		else
			temp.StopBits =STOP_BITS_2;

	}else{
	    temp.StopBits =STOP_BIT_1;
	}
	
	restore_flags( flags);
	
	if ( copy_to_user( pvoid , &temp , sizeof (Serial_line_control) ) == 0 )	
		return 0 ;
	else {
		return -1;
	}	
}

int lcr_set( PDrvData pdata , void  *pvoid ){

	Serial_line_control	temp ;
	pSerial_line_control  pslc ;
	unsigned long flags ;
	char data ;
	char stop ;
	char parity ;
	char mask = 0xff ;
	 printk("lsc_get \n");
	if  ( copy_from_user( &temp , pvoid , sizeof(Serial_line_control) ) ==0  )
		pslc =& temp ;
	else{
		return -1 ;
	}	
	
	switch( pslc-> WordLength ){		
  	 case 5:
  	 	data = SERIAL_5_DATA;
  	 	mask = 0x1f;
  	 	break ;
  	 case 6:
  	 	data = SERIAL_6_DATA;
  	 	mask = 0x3f;
  	 	break;
  	 case 7:
  	 	data = SERIAL_7_DATA;
  	 	mask = 0x7f ;
  	 	break ;
  	 case 8:
  	 	data = SERIAL_8_DATA;
  		break;
  	default:
		return -1; //error
	} ;
	switch( pslc-> Parity){
	
		case    NO_PARITY	:
			parity = SERIAL_NONE_PARITY ;
			break;			
		case    ODD_PARITY:
			parity = SERIAL_ODD_PARITY ;
			break;
		case    EVEN_PARITY:
			parity = SERIAL_EVEN_PARITY ;
			break;
		case    MARK_PARITY:
			parity = SERIAL_MARK_PARITY ;
			break;
		case    SPACE_PARITY:
			parity = SERIAL_SPACE_PARITY ;
			break;
	    default:
	    	return -1; //error
	}
	switch( pslc->StopBits ){
		case STOP_BIT_1 :
			stop = SERIAL_1_STOP;
			break;		    	
		case STOP_BITS_1_5 :
			if (data != SERIAL_5_DATA ) {
				return -1; //error
			 }
			stop = SERIAL_1_5_STOP;
			break ;		
		case STOP_BITS_2 :
			if (data == SERIAL_5_DATA ) {
				return -1; //error
			}
			stop = SERIAL_2_STOP;
			break;		
	    default :
	    	return -1;
	}
     save_flags( flags );
     cli();
     pdata->line_control = (pdata->line_control & SERIAL_LCR_BREAK ) | (data | parity |stop) ;
     pdata->valid_data_mask = mask ;
     outb( pdata->line_control , reg_lcr) ;
     restore_flags( flags);	
	return 0 ;
}

int timeouts_set( PDrvData  pdata , void  *pvoid ){
	Serial_timeouts  temp ;
	unsigned long flags ;
	
	if ( pvoid ==NULL ) return -1 ;	
	if ( copy_from_user( &temp, pvoid , sizeof(Serial_timeouts) ) !=0  ){
		return -1;
	}
	if (  ( temp.ReadIntervalTimeout == MAXULONG )
		  &&
		  ( temp.ReadTotalTimeoutMultiplier == MAXULONG )
		  &&
		  ( temp.ReadTotalTimeoutConstant == MAXULONG )
	   )
	   return -1 ;
	save_flags( flags);
	cli();	
	pdata->timeouts.ReadIntervalTimeout =temp. ReadIntervalTimeout ;
	pdata->timeouts.ReadTotalTimeoutMultiplier =temp. ReadTotalTimeoutMultiplier ;
	pdata->timeouts.ReadTotalTimeoutConstant =temp.ReadTotalTimeoutConstant ;
	pdata->timeouts.WriteTotalTimeoutMultiplier =temp. WriteTotalTimeoutMultiplier ;
	pdata->timeouts.WriteTotalTimeoutConstant =temp. WriteTotalTimeoutConstant ;
	restore_flags( flags );
	return 0 ;
}

int timeouts_get( PDrvData  pdata , void  *pvoid ){
	Serial_timeouts temp ;
	unsigned long flags ;
	
	if ( pvoid == NULL ) return -1;
	save_flags( flags );
	cli();	
	temp.ReadIntervalTimeout = pdata->timeouts.ReadIntervalTimeout ;
	temp.ReadTotalTimeoutMultiplier =  pdata->timeouts. ReadTotalTimeoutMultiplier ;
	temp.ReadTotalTimeoutConstant =  pdata->timeouts.ReadTotalTimeoutConstant ;
	temp.WriteTotalTimeoutMultiplier =  pdata->timeouts.WriteTotalTimeoutMultiplier ;
	temp.WriteTotalTimeoutConstant =  pdata->timeouts.WriteTotalTimeoutConstant ;
	restore_flags( flags );

    if ( copy_to_user( pvoid , &temp , sizeof(Serial_timeouts) ) ) return -1 ;
    return 0 ;
}

int chars_set(PDrvData pdata , void * pvoid ){
	Serial_chars temp ;	
	unsigned long flags ;
	
	if ( pvoid == NULL) return -1;
	if ( copy_from_user( &temp , pvoid , sizeof (Serial_chars) ) ) return -1 ;

	save_flags( flags );
	cli();
	if ( pdata->escape_char ){
		if  ( 	( pdata->escape_char == temp.XonChar ) ||
				(  pdata->escape_char == temp.XoffChar )
		){
			restore_flags( flags );			
			return -1 ;
		}
	}
//memory_copy !!!!
	pdata->special_chars.EofChar = temp. EofChar;
	pdata->special_chars.ErrorChar = temp.ErrorChar ;
	pdata->special_chars.BreakChar = temp.BreakChar ;
	pdata->special_chars.EventChar = temp.EventChar ;
	pdata->special_chars.XonChar = temp.XonChar ;
	pdata->special_chars.XoffChar = temp.XoffChar ;
		
	restore_flags( flags );
	return 0 ;	
}

int chars_get( PDrvData pdata , void  *pvoid ){
	Serial_chars temp ;	
	unsigned long flags ;
	
	if ( pvoid == NULL ) return -1;
	save_flags( flags );
	cli();
//memcpy
	temp.EofChar = pdata->special_chars.EofChar ;
	temp.ErrorChar = pdata->special_chars.ErrorChar ;
	temp.BreakChar = pdata->special_chars.BreakChar ;
	temp.EventChar = pdata->special_chars.EventChar ;
	temp.XonChar = pdata->special_chars.XonChar ;
	temp.XoffChar = pdata->special_chars.XoffChar ;
	
	restore_flags( flags );
	if (  copy_to_user( pvoid ,&(pdata->special_chars) ,sizeof (Serial_chars) ) ) return -1 ;
	return  0;
}

void dtr_set( void ){
	char temp = inb( reg_mcr ) ;
	
	temp |= SERIAL_MCR_DTR ;
	outb( temp , reg_mcr);
}

void dtr_clr( void ){
	char temp = inb( reg_mcr ) ;
	
	temp &= ( ~SERIAL_MCR_DTR ) ;
	outb( temp , reg_mcr);
}

void rts_set(void ){
	char temp = inb( reg_mcr ) ;
	
	temp |= ( SERIAL_MCR_RTS ) ;
	outb( temp , reg_mcr);
}

void rts_clr(void){
	char temp = inb( reg_mcr ) ;
	
	temp &= ( ~SERIAL_MCR_RTS ) ;
	outb( temp , reg_mcr);
}

void perhaps_low_rts( Ptimer_control ptc ){
		PDrvData pdata = (PDrvData)ptc->data ;
	
	ptc->already_in_queue = 0 ;
	printk("timer_routine :perhaps_low_rts \n");
	//close interrupt
	if ( ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE ){
	
		if ( 	( pdata->tx_holding & SERIAL_TX_BREAK )
				||
				( current_write.ptask || pdata->transmit_immediate || ( (write_queue.count !=0 )&&(pdata->tx_holding != 0 ) ) )
	      ){
	          ;//nothing
	      }else{
	
	           if ( ( process_lsr (pdata)&( SERIAL_LSR_THRE | SERIAL_LSR_TEMT ) )
	           	  != ( SERIAL_LSR_THRE | SERIAL_LSR_TEMT )
	           ){
	                timer_del (ptc);
	                timer_set( ptc , 1*HZ , (FUN) perhaps_low_rts , (unsigned long) pdata );
	           }else{
	                 timer_del(ptc);
	                 rts_clr();
	           }	
	      }
	}
	//open interrupt
}

int pretend_xoff( PDrvData pdata ){ //always return 0
	unsigned long flags ;
	save_flags( flags);
	cli();
	pdata->tx_holding |= SERIAL_TX_XOFF ;
	if ( ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE ){
	
		timer_set(  &( pdata->low_rts_timer ) , 1*HZ ,(FUN)perhaps_low_rts ,(unsigned long) pdata) ;
	}			
	restore_flags( flags);
	return 0 ;
}
int pretend_xon( PDrvData pdata ){  //?????
		unsigned long flags;
	
	save_flags( flags );
	cli();
	if ( pdata->tx_holding ){
		pdata->tx_holding &=( ~SERIAL_TX_XOFF ) ;
	
		if ( ( pdata->tx_holding == 0 )&& ( pdata->transmit_immediate || pdata->write_length )&&pdata->holding_empty
		){
	       	disable_all_intp();
	       	enable_all_intp();
		}
	}
   	restore_flags( flags );
	return 0 ;
}
int breakon_set_on( PDrvData pdata ){
		char temp ;
		unsigned long flags ;
	
	save_flags( flags );
	cli();
	if ( (pdata->hand_flow.FlowReplace &SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE ){
		rts_set();
	}
	temp =inb( reg_lcr  ) ;
	temp |= SERIAL_LCR_BREAK ;
	outb( temp , reg_lcr ) ;
	pdata->tx_holding |= SERIAL_TX_BREAK ;
	restore_flags( flags );
	return 0 ;
}
int breakon_clr( PDrvData pdata ){
	char temp ;
	unsigned long flags;
	
	save_flags( flags );
	cli();
	if ( pdata->tx_holding & SERIAL_TX_BREAK ){
		temp = inb( reg_lcr );
		temp &=( ~SERIAL_TX_BREAK ) ;
		outb( temp , reg_lcr ) ;
		pdata->tx_holding &=( ~SERIAL_TX_BREAK );
		
		if(  ( pdata->tx_holding == 0)&&( pdata->transmit_immediate || pdata->write_length )&&pdata->holding_empty
		){
			disable_all_intp() ;
			enable_all_intp() ;		
		}else{
			// perhaps lower rts has changed
			timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
			printk("set timer in breakon_clr \n");		
		}			
	}
	restore_flags( flags);
	return 0;
}

int wait_mask_get( PDrvData pdata, void *pvoid ){
   unsigned long temp ;
   unsigned long flags ;

	if ( pvoid ==NULL ) return -1;
	save_flags( flags );
	cli();
	temp = pdata->isr_wait_mask ;
	restore_flags( flags );
	if ( copy_to_user( pvoid , &temp ,sizeof(long) ) == 0 )
		return 0 ;
	else
		return -1 ;
}

int hand_flow_get( PDrvData pdata , void *pvoid ){
	Serial_handflow  temp ;
	unsigned long flags;
	if( pvoid == NULL ) return  -1 ;
	
	save_flags( flags );
	cli();
	temp.ControlHandShake = pdata->hand_flow.ControlHandShake ;
	temp.FlowReplace = pdata->hand_flow.FlowReplace ;
	temp.XonLimit  = pdata->hand_flow.XonLimit ;
	temp.XoffLimit  = pdata->hand_flow.XoffLimit ;
	restore_flags( flags);
	
	if ( copy_to_user( pvoid ,& temp ,sizeof ( Serial_handflow) ) ==0 )
		return 0 ;
	else
		return -1;
}

void prod_xon_xoff( PDrvData pdata, BOOL  send_xon ){

	if ( ( pdata->send_xon==0 )&&( pdata->send_xoff ==0 )&&( pdata->holding_empty ) ){
		disable_all_intp();
		enable_all_intp();
	}
	if( send_xon ){
		pdata->send_xon = TRUE ;
		pdata->send_xoff =  FALSE ;
	}else{
		pdata->send_xon =  FALSE ;
		pdata->send_xoff =	 TRUE ;
	}

}
void handle_reduce_int_buffer( PDrvData pdata){ // call in interrupt mode
	if ( pdata->rx_holding ){
		if ( pdata->chars_interrupt_buffer <= (ULONG) pdata->hand_flow.XonLimit ){
		
			if ( pdata->rx_holding & SERIAL_RX_DTR){
				pdata->rx_holding &= ~SERIAL_RX_DTR ;
				dtr_set();
			}
			if ( pdata->rx_holding & SERIAL_RX_RTS ){
				pdata->rx_holding &= ~SERIAL_RX_RTS ;
				rts_set();
			}
			if ( pdata->rx_holding & SERIAL_RX_XOFF){
				prod_xon_xoff( pdata, TRUE );
			}
			
		}	
	}
}


char process_lsr(PDrvData pdata ){
	char line_status ;
	unsigned  long flags ;
	//close interrupt
	save_flags( flags );
	cli();
	line_status = inb(reg_lsr );
	if ( line_status & ~( SERIAL_LSR_THRE | SERIAL_LSR_TEMT | SERIAL_LSR_DR ) ){
			
			if ( pdata->escape_char ){
			
					put_char( pdata , pdata->escape_char );
					put_char( pdata , (char)  ( ( line_status & SERIAL_LSR_DR ) ?
						( SERIAL_LSRMST_LSR_DATA ):( SERIAL_LSRMST_LSR_NODATA ) )
					);
					put_char( pdata , line_status );
					
					if ( line_status & SERIAL_LSR_DR ){
					      pdata->perf_stats.ReceivedCount ++;
						put_char( pdata , inb(reg_rd ) );
					}
			}
			if ( line_status & SERIAL_LSR_OE ){
					pdata->perf_stats.SerialOverrunErrorCount ++;
					pdata->error_word |= SERIAL_ERROR_OVERRUN ;
					
					if ( pdata->hand_flow.FlowReplace & SERIAL_ERROR_CHAR ){
						put_char( pdata , pdata->special_chars.ErrorChar );
						
						if ( line_status & SERIAL_LSR_DR ){
							pdata->perf_stats.ReceivedCount ++ ;
							inb( reg_rd );
						}						
					}else{
						if ( line_status & SERIAL_LSR_DR ){
							pdata->perf_stats.ReceivedCount ++;
							put_char( pdata , inb( reg_rd ) );
						}
					}
			}
			if ( line_status & SERIAL_LSR_BI ){
				pdata->error_word  |= SERIAL_ERROR_BREAK ;
				
				if( pdata->hand_flow.FlowReplace  & SERIAL_BREAK_CHAR ){
				     put_char( pdata , pdata->special_chars.BreakChar );
				}				
			}else{
					if ( line_status & SERIAL_LSR_PE ){
						pdata->perf_stats.ParityErrorCount ++;
						pdata->error_word |= SERIAL_ERROR_PARITY ;
						
						if ( pdata->hand_flow.FlowReplace & SERIAL_ERROR_CHAR ){
								put_char( pdata , pdata->special_chars.ErrorChar );
								if ( line_status & SERIAL_LSR_DR ){
								     pdata->perf_stats.ReceivedCount ++ ;
								     inb( reg_rd );
								}
						}
					}
					if ( line_status & SERIAL_LSR_FE ){
							pdata->perf_stats.FrameErrorCount ++ ;
							pdata->error_word |= SERIAL_ERROR_FRAMING ;
							
							if( pdata->hand_flow.FlowReplace & SERIAL_ERROR_CHAR ){
								put_char( pdata , pdata->special_chars.ErrorChar ) ;
								if ( line_status & SERIAL_LSR_DR ){
										pdata->perf_stats.ReceivedCount ++;
										inb( reg_rd );
								}
							}
					}
			
			}//else
			if ( pdata->hand_flow.ControlHandShake & SERIAL_ERROR_ABORT ){
			//WAKE_UP ALL
			//CommErrorDpc	
				pdata->error_timer.data = (unsigned long)pdata ;
				timeout_error( &(pdata->error_timer) );
			}
			if ( pdata->isr_wait_mask ){
					if (  (pdata->isr_wait_mask & SERIAL_EV_ERR)&&
						   ( line_status &( SERIAL_LSR_OE | SERIAL_LSR_PE |  SERIAL_LSR_FE))
					   ){
					   	pdata->history_mask |= SERIAL_EV_ERR ;
					   	printk( "SERIAL_EV_ERR\n" );
					   }
					if ( (pdata->isr_wait_mask & SERIAL_EV_BREAK)&&
							( line_status & SERIAL_LSR_BI )
					   ){
						pdata->history_mask |= SERIAL_EV_BREAK ;
						printk("SERIAL_EV_BREAK\n");
					}
					//wake_up()
					//complete wait_mask ;
					if ( pdata->p_wait_value && pdata->history_mask){
						printk("process_lsr: wait_mask return %x\n", pdata->history_mask );
						*( pdata->p_wait_value)= pdata->history_mask;
						pdata->p_wait_value = NULL;
						pdata->history_mask = 0 ;
						current_wait_mask.wake_up_reason =	WAKE_UP_REASON_OK ;	
						wake_up( &(current_wait_mask.wq));
					}
			}
			if ( line_status & SERIAL_LSR_THRE ){
				if(  pdata->write_length  |  pdata->transmit_immediate ){
					disable_all_intp();
					enable_all_intp();
				}
			}				
	}
	//open interrupt	
	restore_flags( flags );
	return line_status ;	
}

long handle_modem_update( PDrvData pdata , BOOL doing_tx ){
	long old_tx_holding ;
	char modem_status ;
	unsigned long flags ;
//close interrupt
	save_flags( flags );
	cli();
	old_tx_holding = pdata->tx_holding ;
	modem_status = inb( reg_msr);
	if ( pdata->escape_char ){
		if (modem_status & ( SERIAL_MSR_DCTS | SERIAL_MSR_DDSR | SERIAL_MSR_TERI | SERIAL_MSR_DDCD ) ){
		   put_char( pdata , pdata->escape_char );
		   put_char( pdata , SERIAL_LSRMST_MST );
		   put_char( pdata , modem_status );
		}
	}
	if ( pdata->hand_flow.ControlHandShake & SERIAL_DSR_SENSITIVITY ){
	
		if( modem_status & SERIAL_MSR_DSR ){
			pdata->rx_holding &= ~SERIAL_MSR_DSR ;
		}else{
			pdata->rx_holding |= SERIAL_MSR_DSR ;
		}	
	}else{
	      pdata->rx_holding &= ~SERIAL_MSR_DSR ;
	}
	if ( pdata->isr_wait_mask ){
	
		if( ( pdata->isr_wait_mask & SERIAL_EV_CTS )&&( modem_status & SERIAL_MSR_DCTS ) ){
		    pdata->history_mask |= SERIAL_EV_CTS ;
		}
		if( ( pdata->isr_wait_mask & SERIAL_EV_DSR )&&( modem_status & SERIAL_MSR_DDSR ) ){
		    pdata->history_mask |= SERIAL_EV_DSR;
		}
		if( ( pdata->isr_wait_mask & SERIAL_EV_RING )&&( modem_status &SERIAL_MSR_TERI ) ){
			pdata->history_mask |=	SERIAL_MSR_TERI ;
		}
		if( ( pdata->isr_wait_mask & SERIAL_EV_RLSD )&&( modem_status & SERIAL_MSR_DDCD ) ){
			pdata->history_mask |= SERIAL_EV_RLSD ;
		}
//		wakeup(curren_wait_mask);
					if ( pdata->p_wait_value && pdata->history_mask){
						printk("process_lsr: wait mask return %x\n", pdata->history_mask );
						*( pdata->p_wait_value)= pdata->history_mask;
						pdata->p_wait_value = NULL;
						pdata->history_mask = 0 ;
						current_wait_mask.wake_up_reason =	WAKE_UP_REASON_OK ;	
						wake_up( &(current_wait_mask.wq));
					}		
		
	}
	if ( pdata->hand_flow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK ){
	
		if( pdata->hand_flow.ControlHandShake & SERIAL_CTS_HANDSHAKE ){
			if (modem_status & SERIAL_MSR_CTS)
				pdata->tx_holding  &= ~SERIAL_TX_CTS ;
			else
				pdata->tx_holding  &= SERIAL_TX_CTS ;
		}else{
		       pdata->tx_holding  &= ~SERIAL_TX_CTS ;
		}
		
		if( pdata->hand_flow.ControlHandShake & SERIAL_DSR_HANDSHAKE ){
			if (modem_status & SERIAL_MSR_DSR)
				pdata->tx_holding  &= ~SERIAL_TX_DSR ;
			else
				pdata->tx_holding  &= SERIAL_TX_DSR ;
		
		}else{
		      pdata->tx_holding  &= ~SERIAL_TX_DSR ;
		}
		
		if( pdata->hand_flow.ControlHandShake & SERIAL_DCD_HANDSHAKE ){
			if (modem_status & SERIAL_MSR_DCD )
				pdata->tx_holding  &= ~SERIAL_TX_DCD ;
			else
				pdata->tx_holding  &= SERIAL_TX_DCD ;		
		}else{
		       pdata->tx_holding  &= ~SERIAL_TX_DCD ;
		}
		if ( ( old_tx_holding ==0 ) &&( pdata->tx_holding )
			 && ( ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) ==SERIAL_TRANSMIT_TOGGLE )
			 ){
			    timer_set( &(pdata->low_rts_timer) , 1*HZ , (FUN) perhaps_low_rts , (unsigned long) pdata );
			 }
		if ( ( doing_tx ==0 )&&( old_tx_holding ) && ( pdata->tx_holding ==0)  ){
		 	
			if( ( pdata->tx_holding ==0 )&&( pdata->transmit_immediate || pdata->write_length )
				 &&( pdata->holding_empty ==1)
			  ){
				disable_all_intp();
				enable_all_intp();
			  }
		}
		
	}else{
		
		if ( pdata->tx_holding & ( SERIAL_TX_DCD | SERIAL_TX_DSR | SERIAL_TX_CTS ) ){
		
				pdata->tx_holding &=  ~( SERIAL_TX_DCD | SERIAL_TX_DSR | SERIAL_TX_CTS ) ;
			
				if ( ( doing_tx ==0 )&&( old_tx_holding ) && ( pdata->tx_holding ==0)  ){
		 	
					if( ( pdata->tx_holding ==0 )&&( pdata->transmit_immediate || pdata->write_length )
				 		&&( pdata->holding_empty ==1)
			 		 ){
						disable_all_intp();
						enable_all_intp();
			 		 }
				}						
		}		
	}
//open interrupt
	restore_flags( flags);
	return ( long) modem_status ;
}
void put_char( PDrvData pdata , char char_to_put ){
	unsigned long flags ;
	//close interrupt
	save_flags( flags );
	cli();
	if ( pdata->hand_flow.ControlHandShake &	SERIAL_DSR_SENSITIVITY ){
		handle_modem_update( pdata , FALSE );
		if ( pdata->rx_holding & 	SERIAL_RX_DSR ){
		 	restore_flags( flags );
		 	return ;
		}
	}
	if ( pdata->count_since_xoff ){
		pdata->count_since_xoff -- ;
		if ( pdata->count_since_xoff == 0){
		    pdata->count_since_xoff = 0 ;
			current_xoff_count.wake_up_reason = WAKE_UP_REASON_OK;
			wake_up( &(current_xoff_count.wq) );
		}//wake_up the process that wait for count_since_off;

	}
	if ( pdata->read_buffer_base  != pdata->interrupt_read_buffer ){
	
		pdata->read_by_isr ++;
		(*pdata->current_char_slot ) =char_to_put ;
		
		if ( pdata->current_char_slot == pdata->last_char_slot ){
		    pdata->read_already = pdata->current_char_slot - pdata->read_buffer_base +1 ;
			pdata->read_buffer_base = pdata->interrupt_read_buffer ;
			pdata->current_char_slot = pdata->interrupt_read_buffer ;
			pdata->last_char_slot = pdata->interrupt_read_buffer + ( pdata->buffer_size -1 );
			pdata->chars_interrupt_buffer = 0 ;			
//wake_up because read completed;
			current_read.wake_up_reason = WAKE_UP_REASON_OK ;	
			wake_up( &(current_read.wq) );		
		}else{
			pdata->current_char_slot ++ ;
		}
	}else{
	
		if (  ( pdata->hand_flow.ControlHandShake & SERIAL_DTR_MASK ) == SERIAL_DTR_HANDSHAKE  ){
		
		    if ( ( pdata->rx_holding & SERIAL_RX_DTR )==0  ){
		
				if ( ( pdata->buffer_size - pdata->hand_flow.XoffLimit ) <= (pdata->chars_interrupt_buffer +1) ){
					pdata->rx_holding |= SERIAL_RX_DTR ;
					dtr_clr();
				}
			}		
		}
		
	   if (  (pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE  ){
		
		    if ( ( pdata->rx_holding & SERIAL_RX_RTS )==0  ){
		
				if ( ( pdata->buffer_size - pdata->hand_flow.XoffLimit ) <= (pdata->chars_interrupt_buffer +1) ){
					pdata->rx_holding |= SERIAL_RX_RTS ;
					rts_clr();
				}
			}		
		}
		
		if ( pdata->hand_flow.FlowReplace & SERIAL_AUTO_RECEIVE ){
		
			if( (pdata->rx_holding & SERIAL_RX_XOFF)== 0 ){
				if ( (pdata->buffer_size - pdata->hand_flow.XoffLimit )== (pdata->chars_interrupt_buffer +1) ){
					pdata->rx_holding |= SERIAL_RX_XOFF ;
					prod_xon_xoff( pdata , FALSE );
				}
			}
		}
		
		if ( pdata->chars_interrupt_buffer < pdata->buffer_size ){
		
			*( pdata->current_char_slot ) = char_to_put ;
			pdata->chars_interrupt_buffer ++ ;
			
			if ( pdata->chars_interrupt_buffer == pdata->buffer_size_p8 ){
			
				if( pdata->isr_wait_mask & SERIAL_EV_RX80FULL ){
					//wake_up()  history_mask  need return
					pdata->history_mask |= SERIAL_EV_RX80FULL;
					if ( pdata->p_wait_value ){
						printk("SERIAL_EV_RX80FULL\n");
						*( pdata->p_wait_value) = pdata->history_mask ;
						pdata->p_wait_value = NULL ;
						pdata->history_mask = 0 ;
						current_wait_mask.wake_up_reason =	WAKE_UP_REASON_OK ;
						wake_up( &(current_wait_mask.wq) );
					}
				}
			}
			if ( pdata->current_char_slot == pdata->last_char_slot ){
				pdata->current_char_slot = pdata->interrupt_read_buffer ;
			}else{
			    pdata->current_char_slot ++ ;
			}
					
		}else{
		    pdata->perf_stats.BufferOverrunErrorCount ++;
		    pdata->error_word |= SERIAL_ERROR_QUEUEOVERRUN ;
		
		    if ( pdata->hand_flow.FlowReplace & SERIAL_ERROR_CHAR  ){
		
		    	if( pdata->current_char_slot == pdata->interrupt_read_buffer ){
		    	    *( pdata->interrupt_read_buffer + ( pdata->buffer_size -1) ) = pdata->special_chars.ErrorChar ;
		    	}else{
		    	    *(pdata->current_char_slot -1 ) = pdata->special_chars.ErrorChar ;
		    	}		
		    }
		    if( pdata->hand_flow.ControlHandShake & SERIAL_ERROR_ABORT ){
		       //wake_up(); commErrorDPC
		       pdata->error_timer.data = (unsigned long)pdata ;
				timeout_error( &(pdata->error_timer) );
		    }
		}
	}
	//open interrupt
	restore_flags( flags );
}

void clean_current_control( current_control *pcc){
		unsigned long flags;
	
	save_flags( flags );
	cli();
	pcc->ptask =NULL ;
	pcc->wake_up_reason = WAKE_UP_REASON_NODATA ;
	restore_flags( flags);	
}
/**
*when an request start  it may sleep
*/
// return  value  contine or break ;

int sleep_or_start ( current_control *pcc , operation_queue *poq ,PDrvData pdata , int mode ){
		unsigned long flags ;
		wait_queue_t  wait ;
	
	if ( (mode == MODE_WRITE)&&(pdata->purge_tx) ) return -1 ;
	if ( (mode == MODE_READ)&&(pdata->purge_rx) ) return -1;	
	save_flags( flags );
	cli();
	init_waitqueue_entry( &wait, current );
	
	if ( ( pcc->ptask == NULL)&&( poq->count == 0) ){
	
	    pcc->ptask = current ;
	    pcc->wake_up_reason = 	WAKE_UP_REASON_NODATA ;
	    restore_flags( flags);
	    return 0;		
	}else{
		add_wait_queue(&(poq->wq), &wait );
		inc_ref(poq);
		restore_flags( flags );
		
		while( 1){
			set_current_state( TASK_UNINTERRUPTIBLE );
			schedule();
			
			save_flags( flags);
			cli();
			if ( ( (mode == MODE_WRITE)&&(pdata->purge_tx) ) ||
					( (mode == MODE_READ)&&(pdata->purge_rx) )
			){  // pdata->
				dec_ref(poq);
				remove_wait_queue(&(poq->wq), &wait );
				restore_flags( flags);
				return -1;
			}
			if ( pcc->ptask == NULL){
				dec_ref(poq);
				remove_wait_queue(&(poq->wq), &wait );
				pcc->ptask = current ;
				pcc->wake_up_reason = WAKE_UP_REASON_NODATA ;
				restore_flags(flags);
				return 0	;		
			}
			restore_flags( flags);
		}
	}		
}
int wait_mask( PDrvData pdata , void *pvoid ){  //need rewrite
		unsigned long temp ;
		unsigned long flags ;
	
	printk("wait_mask \n");	
	if( pvoid == NULL ) return -1 ;

	if ( sleep_or_start( &current_mask, &mask_queue, pdata, MODE_WAIT ) ) return -1 ;
//
   	save_flags( flags );
   	cli();
   	if ( current_wait_mask.ptask == NULL ){
   	
   		if( pdata->isr_wait_mask ){
   			printk("pdata->history_mask = %x\n", (unsigned int)pdata->history_mask );

	   		if ( pdata->history_mask != 0 ){
	   			temp = pdata->history_mask ;
	   			pdata->history_mask = 0 ;
	   			pdata->emptied_transmit = FALSE ;
	   			restore_flags( flags );
	   			if( copy_to_user( pvoid ,  &temp, sizeof ( long ) ) ==0 ){
	   				clean_current_control( &current_mask );	   			
	   				zy_wake_up( &mask_queue );
	   				return 0;
	   			}else{
	   				clean_current_control( &current_mask );	   			
	   				wake_up( &(mask_queue.wq) );
	   				printk("copy_to_user error \n");
	   				return -1;	   			
	   			}	   			
	   		}else{
				//	   			   		
	   			current_wait_mask.ptask = current ;
	   			temp = 0 ;
	   			pdata->p_wait_value = &temp ;
	   			clean_current_control( &current_mask );
	   			zy_wake_up( &mask_queue );
//   		
 		  		restore_flags( flags );
 		  		printk("wait _mask sleep\n");
	   			sleep_on( & (current_wait_mask.wq ) );
	   			printk("wait_mask wake up\n");
	   			
	   			if ( current_wait_mask.wake_up_reason == WAKE_UP_REASON_OK ){
	   				save_flags( flags );
	   				cli();
					if ( temp == 0 )printk("fatal error in wait mask\n");
					printk("wait_mask return %x \n", temp );
	   				restore_flags( flags );
	   				if ( copy_to_user( pvoid , &temp , sizeof (long) )== 0 ){
	   					//clean_waiting_mask();
	   					clean_current_control( &current_wait_mask );
	   					return 0 ;
	   				   				
	   				}else{
	   					//clean_waiting_mask();
	   					clean_current_control( &current_wait_mask );
	   					return -1 ;
	   				}
	   				
	   			}else{
	   				//clean_waiting_mask();
	   				clean_current_control( &current_wait_mask );
	   				return -1 ;
	   			}
	   		}
	   		
   		}else{
   			//current_mask.ptask = NULL ;
   			clean_current_control( &current_mask );
   			wake_up( &( mask_queue.wq) );			
			restore_flags( flags);
			printk("error :isr_wait_mask is  0 \n");
			return -1 ;   		   		
   		}
   		
   	}else{
   	   	//current_mask.ptask = NULL ;
   	   	clean_current_control( &current_mask );
   		wake_up( &( mask_queue.wq) );	
//
   		restore_flags( flags );
   		return -1 ;
   	}
}
//
int set_wait_mask( PDrvData pdata, void *pvoid ){  //need rewrite
		unsigned long flags;
		unsigned long temp_mask ;
		
	if( pvoid == NULL ) return  -1 ;
	
   	if ( copy_from_user( &temp_mask, pvoid , sizeof ( long ) ) ){
   		printk("set_wait_mask copy_from_user error \n");
   		return -1 ; 	
   	}
   	if ( temp_mask & ~(    SERIAL_EV_RXCHAR 	|
   						SERIAL_EV_RXFLAG   	|
   						SERIAL_EV_TXEMPTY  	|
   						SERIAL_EV_CTS		|
   						SERIAL_EV_DSR		|
   						SERIAL_EV_RLSD		|
   						SERIAL_EV_BREAK		|
   						SERIAL_EV_ERR		|
   						SERIAL_EV_RING		|
   						SERIAL_EV_PERR		|
   						SERIAL_EV_RX80FULL	|
   						SERIAL_EV_EVENT1	|
   						SERIAL_EV_EVENT2	) ){   						
   		return -1 ;   						
   	}

	if ( sleep_or_start( &current_mask, &mask_queue, pdata, MODE_WAIT ) ) return -1 ;
	
	save_flags( flags );
	cli();
	if ( pdata->p_wait_value ){//finish old wait mask
		printk("finish old wait mask\n");
		*( pdata->p_wait_value) = 0;
		pdata->p_wait_value = NULL ;
		current_wait_mask.wake_up_reason = WAKE_UP_REASON_OK ;
		wake_up( &(current_wait_mask.wq) );
		init_waitqueue_head( &wq_query );
		restore_flags( flags );
		
		while ( current_wait_mask.ptask ){
		
			schedule_timeout( 800 );
		}
		save_flags( flags );
		cli();
	}
		pdata->isr_wait_mask = temp_mask ;
		pdata->history_mask &= temp_mask ;

		restore_flags( flags );
		clean_current_control( &( current_mask ) );
		zy_wake_up( &mask_queue );
		return 0 ;
}
void timeout_immediate( Ptimer_control ptc){
		PDrvData pdata =  (PDrvData)ptc->data ;
	ptc->already_in_queue = 0 ;
	
	if ( pdata->transmit_immediate == TRUE ){
		pdata->transmit_immediate = FALSE ;
		current_immediate.wake_up_reason = WAKE_UP_REASON_TIMEOUT ;
	}
	wake_up( &(current_immediate.wq));	
}
int trans_immediate( PDrvData pdata, void *pvoid ){
      	unsigned long flags;
      	char temp ;
      	
      if (copy_from_user( &temp, pvoid , sizeof(char) ) ) return -1 ;

      save_flags( flags );
      cli();
      if ( current_immediate.ptask ){
			restore_flags( flags );
			return -1 ;
      }
      current_immediate.ptask = current ;
      current_immediate.wake_up_reason = WAKE_UP_REASON_NODATA ;
      pdata->immediate_char = temp ;
      pdata->transmit_immediate = TRUE ;
      //
      printk("trans_immediate =%d\n", temp);
      //trigger
      if ( ( ! pdata->write_length )&&( pdata->holding_empty ) ) enable_all_intp();

      timer_del( &(pdata->immediate_timer) );
      timer_set( &(pdata->immediate_timer), 1*HZ, (FUN)timeout_immediate, (unsigned long )pdata);
      restore_flags( flags);

      if ( pdata->transmit_immediate ){
			sleep_on( &(current_immediate.wq));
      }
      timer_del( &(pdata->immediate_timer) );     	

		if ( current_immediate.wake_up_reason == WAKE_UP_REASON_OK) printk("trans_immediate ok\n");
		clean_current_control( &current_immediate );
		process_empty_transmit( pdata );
		return 0 ;
}
void clean_perf_stats( PDrvData pdata ){
      	unsigned long flags;
      	
      save_flags( flags );
      cli();

	pdata->perf_stats.ReceivedCount = 0 ;
	pdata->perf_stats.TransmittedCount = 0 ;
	pdata->perf_stats.FrameErrorCount = 0 ;
	pdata->perf_stats.SerialOverrunErrorCount = 0 ;
	pdata->perf_stats.BufferOverrunErrorCount = 0 ;
	pdata->perf_stats.ParityErrorCount = 0 ;
	
	restore_flags( flags );
}
int get_perf_stats( PDrvData pdata , void *pvoid ){
		Serial_perf_stats temp ;
		unsigned long flags ;
		
	save_flags( flags );
	cli();
	
	temp.ReceivedCount = pdata->perf_stats.ReceivedCount ;
	temp.TransmittedCount  = pdata->perf_stats.TransmittedCount  ;
	temp.FrameErrorCount = pdata->perf_stats.FrameErrorCount ;
	temp. SerialOverrunErrorCount= pdata->perf_stats.SerialOverrunErrorCount ;
	temp.BufferOverrunErrorCount = pdata->perf_stats.BufferOverrunErrorCount ;
	temp.ParityErrorCount = pdata->perf_stats.ParityErrorCount ;
	
	restore_flags( flags );
	if ( copy_to_user( pvoid , &temp , sizeof ( Serial_perf_stats ) ) ){
		return -1 ;
	}
	
	return 0 ;
}
int get_comm_status( PDrvData pdata , void *pvoid ){
		unsigned long flags ;
		Serial_status tmp ;
	
	save_flags( flags );
	cli();
	tmp.Errors = pdata->error_word ;
	pdata->error_word = 0 ;
	tmp.EofReceived = FALSE ;
	tmp.AmountInInQueue = pdata->chars_interrupt_buffer ;
	tmp.AmountInOutQueue = pdata->write_length ;
	tmp.WaitForImmediate =pdata->transmit_immediate ;

	tmp.HoldReasons = 0 ;
	if ( pdata->tx_holding ){
		if ( pdata->tx_holding & SERIAL_TX_CTS )		tmp.HoldReasons |= SERIAL_TX_WAITING_FOR_CTS ;
		if ( pdata->tx_holding & SERIAL_TX_DSR )		tmp.HoldReasons |= SERIAL_TX_WAITING_FOR_DSR ;
		if ( pdata->tx_holding & SERIAL_TX_DCD )		tmp.HoldReasons |= SERIAL_TX_WAITING_FOR_DCD ;
		if ( pdata->tx_holding & SERIAL_TX_XOFF )		tmp.HoldReasons |= SERIAL_TX_WAITING_FOR_XON ;
		if ( pdata->tx_holding & SERIAL_TX_BREAK )	tmp.HoldReasons |= SERIAL_TX_WAITING_ON_BREAK ;
	}
	if ( pdata->rx_holding & SERIAL_RX_DSR ){
		tmp.HoldReasons |= SERIAL_RX_WAITING_FOR_DSR ;		
	}
	if ( pdata->rx_holding & SERIAL_RX_XOFF ){
		tmp.HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT ;		
	}	
	restore_flags( flags );
	if ( copy_to_user (pvoid ,&tmp, sizeof (Serial_status) ) ){
		return -1 ;
	}
	return 0 ;
}
int properties_get( PDrvData pdata , void * pvoid ){
		Serial_commprop tmp ;
		
	tmp.PacketLength = sizeof (Serial_commprop);
	tmp.PacketVersion = 2 ;
	tmp.ServiceMask =SERIAL_SP_SERIALCOMM ;//
	tmp.MaxTxQueue = 0 ;
	tmp.MaxRxQueue = 0 ;
	
	tmp.MaxBaud = SERIAL_BAUD_USER ;
	tmp.SettableBaud = pdata->supported_bauds ;
	
	tmp.ProvSubType = SERIAL_SP_RS232 ;
	tmp.ProvCapabilities = 	SERIAL_PCF_DTRDSR |
												SERIAL_PCF_RTSCTS |
												SERIAL_PCF_CD |
												SERIAL_PCF_PARITY_CHECK |
												SERIAL_PCF_XONXOFF |
												SERIAL_PCF_SETXCHAR |
												SERIAL_PCF_TOTALTIMEOUTS |
												SERIAL_PCF_INTTIMEOUTS ;

	tmp.SettableParams = 	SERIAL_SP_PARITY |
												SERIAL_SP_BAUD |
												SERIAL_SP_DATABITS |
												SERIAL_SP_STOPBITS |
												SERIAL_SP_HANDSHAKING |
												SERIAL_SP_PARITY_CHECK |
												SERIAL_SP_CARRIER_DETECT ;
												
	tmp.SettableData = SERIAL_DATABITS_5 |
										SERIAL_DATABITS_6 |
										SERIAL_DATABITS_7 |
										SERIAL_DATABITS_8 ;
										
	tmp.SettableStopParity = 	SERIAL_STOPBITS_10 |
													SERIAL_STOPBITS_15 |
													SERIAL_STOPBITS_20 |
													SERIAL_PARITY_NONE |
													SERIAL_PARITY_ODD |
													SERIAL_PARITY_EVEN |
													SERIAL_PARITY_MARK |
													SERIAL_PARITY_SPACE ;
													
	tmp.CurrentTxQueue = 0 ;
	tmp.CurrentRxQueue = pdata->buffer_size ;
	
	if ( copy_to_user( pvoid, &tmp, sizeof(Serial_commprop) ) ) return -1 ;
	
	return 0 ;	
}
int config_size( PDrvData pdata, void *pvoid){
		unsigned long tmp = 0 ;
		
	if (copy_to_user(pvoid , &tmp, sizeof (unsigned long ))) return -1;
	return 0 ;
}
int lsr_mst_insert( PDrvData pdata, void *pvoid ){
		char tmp ;
		unsigned long flags ;
		
	if ( copy_from_user( &tmp, pvoid , sizeof (char)) ) return -1;
	if ( tmp == 0 ) return -1 ;
	save_flags( flags );
	cli();
	if ( (tmp==pdata->special_chars.XoffChar)
			||( tmp==pdata->special_chars.XonChar )
			||(pdata->hand_flow.FlowReplace&SERIAL_ERROR_CHAR)
	){
		restore_flags( flags );
		return -1 ;
	}
	pdata->escape_char = tmp ;
	restore_flags( flags);	
	return 0 ;
}
int get_dtr_rts( PDrvData pdata , void *pvoid ){		
		ULONG modem_control ;

	modem_control = inb( reg_mcr );
	modem_control |= SERIAL_DTR_STATE | SERIAL_RTS_STATE ;
	
	if ( copy_to_user( pvoid , &modem_control, sizeof (ULONG) ) ) return -1 ;	
	return 0 ;
}
int get_modem_status( PDrvData pdata , void *pvoid ){
		int result ;
		
	result = handle_modem_update( pdata, FALSE );
	if ( copy_to_user( pvoid , &result , sizeof (int) ) ) return -1 ;
	
	return 0 ;
}
//check this function
void setup_new_handflow(PDrvData pdata , pSerial_handflow phf ){
	
//DTR  flow control ;
	if ( (pdata->hand_flow.ControlHandShake & SERIAL_DTR_MASK)!=
			( phf->ControlHandShake & SERIAL_DTR_MASK)
	){
		if ( phf->ControlHandShake & SERIAL_DTR_MASK ){
		
			if ( (phf->ControlHandShake &SERIAL_DTR_MASK)==SERIAL_DTR_HANDSHAKE ){
				if ( (pdata->buffer_size - phf->XoffLimit ) > pdata->chars_interrupt_buffer ){
					if ( pdata->rx_holding & SERIAL_RX_DTR ){
						if (pdata->chars_interrupt_buffer > phf->XonLimit ){
							pdata->rx_holding &= ~SERIAL_RX_DTR ;
							dtr_set();
						}
					}else{
						dtr_set();
					}					
				}else{
					pdata->rx_holding |= SERIAL_RX_DTR ;
					dtr_clr();
				}
			}else{
				if ( pdata->rx_holding &SERIAL_RX_DTR ) pdata->rx_holding &= ~SERIAL_RX_DTR ;
				dtr_set();
			}			
		}else{
			if ( pdata->rx_holding & SERIAL_RX_DTR ) pdata->rx_holding &= ~SERIAL_RX_DTR ;
			dtr_clr();
		}	
	}
//RTS flow control
	if ( (pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK)!=
			(phf->FlowReplace & SERIAL_RTS_MASK)
	){
			if ( (phf->FlowReplace & SERIAL_RTS_MASK)==SERIAL_RTS_HANDSHAKE ){
				if ( (pdata->buffer_size -phf->XoffLimit) > pdata->chars_interrupt_buffer ){
					if ( pdata->rx_holding & SERIAL_RX_RTS ){
						if ( pdata->chars_interrupt_buffer > phf->XonLimit ){
							pdata->rx_holding &= ~SERIAL_RX_RTS ;
							rts_set();
						}
					}else{
						rts_set();
					}
				}else{
					pdata->rx_holding |= SERIAL_RX_RTS ;
					rts_clr();
				}
			}else if ( (phf->FlowReplace &SERIAL_RTS_MASK)==SERIAL_RTS_CONTROL ){
				if ( pdata->rx_holding & SERIAL_RX_RTS ){
					pdata->tx_holding &= ~SERIAL_RX_RTS ;
				}
				rts_set();
			}else if ( (phf->FlowReplace & SERIAL_RTS_MASK)==SERIAL_TRANSMIT_TOGGLE ){
			
				if ( pdata->rx_holding & SERIAL_RX_RTS ){
					pdata->rx_holding &= ~SERIAL_RX_RTS ;
				}
				pdata->hand_flow.FlowReplace &=~SERIAL_RTS_MASK ;
				pdata->hand_flow.FlowReplace |= SERIAL_TRANSMIT_TOGGLE ;
				
				if ( ( pdata->tx_holding & SERIAL_TX_BREAK )||
						((process_lsr(pdata) & (SERIAL_LSR_THRE | SERIAL_LSR_TEMT) )!=
						 (SERIAL_LSR_THRE | SERIAL_LSR_TEMT))||
						 ( current_write.ptask || pdata->transmit_immediate || (write_queue.count && (!pdata->tx_holding)) )
				 ){// condition is puzzling
				 	rts_set();
				 }else{
				
				 	pdata->low_rts_timer.data=(long)pdata ;
				  	perhaps_low_rts( &(pdata->low_rts_timer) );
				 }				
			}else{
				if ( pdata->rx_holding  &SERIAL_RX_RTS ){
					pdata->rx_holding &= ~ SERIAL_RX_RTS ;
				}
				rts_clr();
			}
	}
//auto receive  flow control
	if ( (pdata->hand_flow.FlowReplace & SERIAL_AUTO_RECEIVE)!=
	 		( phf->FlowReplace & SERIAL_AUTO_RECEIVE )
	 ){
	 	if( phf->FlowReplace & SERIAL_AUTO_RECEIVE ){
	 		if ( (pdata->buffer_size - phf->XoffLimit) <= pdata->chars_interrupt_buffer ){
	 			pdata->rx_holding  |= SERIAL_RX_XOFF ;
	 			prod_xon_xoff( pdata, FALSE );
	 		}
	 	}else{
	 		if ( pdata->rx_holding & SERIAL_RX_XOFF ){
	 			pdata->rx_holding &= ~ SERIAL_RX_XOFF ;
	 			prod_xon_xoff( pdata, TRUE );
	 		}
	 	}
	}
//auto transmit flow control
	if ( (pdata->hand_flow.FlowReplace & SERIAL_AUTO_TRANSMIT) !=
			( phf->FlowReplace & SERIAL_AUTO_TRANSMIT)
	){
		if ( phf->FlowReplace & SERIAL_AUTO_TRANSMIT ){
		// nothing
		}else{
			if ( pdata->tx_holding & SERIAL_TX_XOFF ){
				pdata->tx_holding &= ~ SERIAL_TX_XOFF ;
				prod_xon_xoff ( pdata, TRUE );
			}
		}
	}
//final process
	pdata->hand_flow.ControlHandShake = phf->ControlHandShake ;
	pdata->hand_flow.FlowReplace = phf->FlowReplace ;
	pdata->hand_flow.XonLimit = phf->XonLimit ;
	pdata->hand_flow.XoffLimit = phf->XoffLimit ;

}
int set_handflow( PDrvData pdata , void *pvoid ){
		Serial_handflow tmp ;
		unsigned long flags;
		
	if ( ! pvoid ) return -1 ;
	if ( copy_from_user( &tmp, pvoid , sizeof (Serial_handflow) ) ) return -1;
	
	if ( tmp.ControlHandShake & SERIAL_CONTROL_INVALID ) return -1 ;
	if ( tmp.FlowReplace & SERIAL_FLOW_INVALID ) return -1 ;
	if ( (tmp.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_MASK ) return -1 ;
	if ( (tmp.XonLimit < 0 ) || (tmp.XonLimit > pdata->buffer_size ) ) return -1 ;
	if ( (tmp.XoffLimit < 0 ) || (tmp.XoffLimit > pdata->buffer_size ) ) return -1 ;
	
	save_flags( flags );
	cli();
	if ( pdata->escape_char ){
		if ( tmp.FlowReplace & SERIAL_ERROR_CHAR ){
			restore_flags(flags);
			return -1 ;
		}
	}
	setup_new_handflow( pdata, &tmp);
	handle_modem_update( pdata , FALSE );
	restore_flags( flags);
	return 0 ;
}
int get_handflow( PDrvData pdata , void *pvoid){
		Serial_handflow tmp ;
		unsigned long flags ;
	
	if ( pvoid == NULL ) return -1 ;
	save_flags( flags );
	cli();
	tmp.ControlHandShake = pdata->hand_flow.ControlHandShake ;
	tmp.FlowReplace = pdata->hand_flow.FlowReplace ;
	tmp.XonLimit = pdata->hand_flow.XonLimit ;
	tmp.XoffLimit = pdata->hand_flow.XoffLimit ;	
	restore_flags( flags );
	if ( copy_to_user( pvoid , &tmp, sizeof (Serial_handflow) ) ) return -1 ;	
	return 0 ;
}
int set_queue_size( PDrvData pdata , void *pvoid ){
		Serial_queue_size tmp ;
		unsigned long flags ;
		char *old_buffer = NULL ;
		void *ptmp = NULL ;
		
	if ( pvoid == NULL ) return  -1 ;
	if ( copy_from_user( &tmp, pvoid , sizeof (Serial_queue_size) )  ) return -1 ;
	if ( tmp.InSize <= pdata->buffer_size ) return  0 ;

	if ( sleep_or_start( &current_read, &read_queue , pdata, MODE_READ) < 0 ) return -1;
	
	ptmp =kmalloc( tmp.InSize , GFP_KERNEL);
	if ( ptmp == NULL ){
		clean_current_control( &current_read );
		zy_wake_up( &read_queue );
		printk("can not get memory for buffer \n");
		return -1 ;// can not get resource .		
	}
	save_flags( flags );
	cli();
	{ 		char* new = ptmp ;
			int  i = 0 ;
		while( pdata->first_readable != pdata->current_char_slot ){
			*new = *(pdata->first_readable) ;
			new++;	i++;
			if ( pdata->first_readable == (pdata->interrupt_read_buffer + (pdata->buffer_size -1) ) ){
				pdata->first_readable = pdata->interrupt_read_buffer ;
			}else{
				pdata->first_readable +=1 ;
			}
		}
		if ( pdata->chars_interrupt_buffer != i ) printk("set_queue_size fatal error \n");
		
		old_buffer = pdata->interrupt_read_buffer ;
		kfree( old_buffer);
		pdata->interrupt_read_buffer = ptmp ;
		pdata->read_buffer_base = ptmp ;
		pdata->buffer_size = tmp.InSize ;
		pdata->buffer_size_p8 = ( 3*( pdata->buffer_size >> 2) ) + ( pdata->buffer_size >> 4 ) ;
		
		pdata->first_readable = pdata->interrupt_read_buffer ;
		pdata->current_char_slot = pdata->interrupt_read_buffer + pdata->chars_interrupt_buffer ;
		pdata->last_char_slot = pdata->interrupt_read_buffer + (pdata->buffer_size -1) ;
		handle_reduce_int_buffer( pdata);
		
	 }
	restore_flags( flags);
	clean_current_control( &current_read );
	zy_wake_up( &read_queue );	
	return 0 ;
}
void timeout_xoff_count( Ptimer_control ptc ){
		PDrvData pdata =( PDrvData )ptc->data ;
		ptc->already_in_queue = 0 ;
	printk("xoff_count timeouts\n");	
	if ( pdata->count_since_xoff ){
		pdata->count_since_xoff = 0 ;
		current_xoff_count.wake_up_reason = WAKE_UP_REASON_TIMEOUT ;
	}
	wake_up( &(current_xoff_count.wq ));// reson for putting here is important ; when interrupt happens  quickly ,
}
int xoff_counter( PDrvData pdata , void *pvoid ){
		Serial_xoff_counter tmp ;
		unsigned long flags ;
		
	if ( pvoid == NULL ) return -1 ;
	if ( copy_from_user( &tmp, pvoid , sizeof(Serial_xoff_counter) ) ) return -1 ;
	if ( tmp.Counter <= 0 ) return -1;

	if ( sleep_or_start( &current_write, &write_queue , pdata, MODE_WRITE ) < 0 ) return -1;
	save_flags( flags );
	cli();
	pdata->pcurrent_write_char = &( tmp.XoffChar );
	pdata->write_length =1 ;
	restore_flags( flags );

	timer_del( &(pdata->write_timer) );
	timer_set( &(pdata->write_timer),30*HZ,(FUN)timeout_write ,(unsigned long) pdata );
	//invoke_hardware();
	init_timer( &trigger );
	trigger.expires = HZ+jiffies;
	trigger.function = invoke_hardware ;
	trigger.data = (unsigned long)NULL;
	add_timer( &trigger );
	sleep_on( &(current_write.wq) );
	timer_del( &(pdata->write_timer) ); //write completed
	del_timer( &trigger );
	
	{  int reason  ;
		reason = current_write.wake_up_reason ;		
	}
	save_flags( flags );
	cli();
	current_xoff_count.ptask = current ;
	current_xoff_count.wake_up_reason = WAKE_UP_REASON_NODATA ;
	restore_flags( flags );
	
	clean_current_control( &current_write );
	zy_wake_up( &write_queue );
	
	save_flags( flags );
	cli();
	pdata->count_since_xoff = tmp.Counter ;
	restore_flags( flags );
	
	timer_set( &(pdata->xoff_count_timer),10*HZ,(FUN)timeout_xoff_count ,(unsigned long) pdata );
	if ( pdata->count_since_xoff ){
		sleep_on( &(current_xoff_count.wq) );
	}
	timer_del( &(pdata->xoff_count_timer) );//get reason for father process 	
	return 0 ;
}
void purge_wait( operation_queue *poq ){
		unsigned long flags;
		
	save_flags( flags);
	cli();
	while( poq->count ){
		zy_wake_up( poq);
		restore_flags( flags );		
		schedule_timeout(800);			
		save_flags( flags);
		cli();
	}
	restore_flags( flags);	
}
int zy_purge( PDrvData pdata ,void *pvoid ){
		unsigned long mask ;
		unsigned long flags ;
		
	if ( pvoid == NULL) return -1 ;
	if ( copy_from_user( &mask , pvoid , sizeof(long) ) )	return -1 ;
	if ( ( !mask )||( mask &(~(SERIAL_PURGE_TXABORT	|
												SERIAL_PURGE_RXABORT	|
												SERIAL_PURGE_TXCLEAR		|
												SERIAL_PURGE_RXCLEAR		) ) )
	 ) return -1;
	 printk("purge_mask =%x\n", (int)mask);
    if ( sleep_or_start( &current_purge, &purge_queue, pdata, MODE_PURGE ) < 0 )return -1 ;
	
	if ( mask & SERIAL_PURGE_TXABORT ){
		save_flags( flags);
		cli();
		pdata->purge_tx =TRUE;
		restore_flags( flags);
		purge_wait( &write_queue);
//		purge_current_write( &current_write);
		{
			save_flags( flags);
			cli();
			if ( pdata->write_length ){
				pdata->write_remain =  pdata->write_length ;
				pdata->write_length = 0 ;
				current_write.wake_up_reason = WAKE_UP_REASON_PURGE ;				
				wake_up( &(current_write.wq) );
				restore_flags( flags);
				while( current_write.ptask ){
					schedule_timeout(800);
				}							
			}else restore_flags( flags);
		}
//		purge_xoff_count( &current_xoff_count );		
		{
			save_flags( flags);
			cli();
			if ( pdata->count_since_xoff ){
				pdata->count_since_xoff  = 0 ;
				current_xoff_count.wake_up_reason = WAKE_UP_REASON_PURGE ;				
				wake_up( &(current_xoff_count.wq) );
				restore_flags( flags);
				while( current_xoff_count.ptask ){
					schedule_timeout(800);
				}							
			}else restore_flags( flags);
		}
		save_flags( flags);
		cli();
		pdata->purge_tx =FALSE;
		restore_flags( flags);		
	}
	if ( mask & SERIAL_PURGE_RXABORT ){
		save_flags( flags);
		cli();
		pdata->purge_rx =TRUE;
		restore_flags( flags);
		purge_wait( &read_queue);
//		purge_current_read( &current_read);
		{
		    save_flags( flags);
		    cli();
			if  ( pdata->read_buffer_base != pdata->interrupt_read_buffer ){
	
				pdata->read_already = pdata->current_char_slot - pdata->read_buffer_base ;
				pdata->read_buffer_base =pdata->interrupt_read_buffer;
				pdata->current_char_slot = pdata->interrupt_read_buffer;
				pdata->first_readable = pdata->interrupt_read_buffer ;
				pdata->last_char_slot = pdata->interrupt_read_buffer + ( pdata->buffer_size -1);
				pdata->chars_interrupt_buffer = 0 ;
				current_read.wake_up_reason = WAKE_UP_REASON_PURGE ;
				wake_up( &(current_read.wq) );
				restore_flags( flags);
				
				while ( current_read.ptask ){
					schedule_timeout(800);
				}
											
			}else restore_flags( flags);			
		}
		save_flags( flags);
		cli();
		pdata->purge_rx =FALSE;
		restore_flags( flags);			
	}
	if ( mask & SERIAL_PURGE_RXCLEAR ){
		save_flags( flags );
		cli();
		if ( pdata->read_buffer_base == pdata->interrupt_read_buffer ){

			pdata->current_char_slot = pdata->interrupt_read_buffer;
			pdata->first_readable = pdata->interrupt_read_buffer ;
			pdata->last_char_slot = pdata->interrupt_read_buffer + ( pdata->buffer_size -1);
			pdata->chars_interrupt_buffer = 0;
			handle_reduce_int_buffer( pdata);
		}		
		restore_flags( flags);		
	}
	clean_current_control( &current_purge );
	zy_wake_up( &purge_queue );
	return 0 ;
}
void timeout_error( Ptimer_control ptc ){ // = commERRdpc
		PDrvData  pdata = (PDrvData)ptc->data ;
		BOOL again = FALSE ;		
	ptc->already_in_queue = 0 ;
	
	pdata->purge_rx = TRUE ;
	pdata->purge_tx = TRUE ;
	
	if ( write_queue.count ){
		zy_wake_up( &write_queue );
		again =TRUE ;
	}
	if ( read_queue.count ){
		zy_wake_up( &read_queue);
		again =TRUE;
	}
	     if ( current_write.ptask){
	
				if ( pdata->write_length ){
					pdata->write_remain =  pdata->write_length ;
					pdata->write_length = 0 ;
					current_write.wake_up_reason = WAKE_UP_REASON_PURGE ;				
					wake_up( &(current_write.wq) );
				}
				again = TRUE ;
        }

        if ( current_read.ptask){

				if ( pdata->read_buffer_base != pdata->interrupt_read_buffer ){
	
					pdata->read_already = pdata->current_char_slot - pdata->read_buffer_base ;
					pdata->read_buffer_base =pdata->interrupt_read_buffer;
					pdata->current_char_slot = pdata->interrupt_read_buffer;
					pdata->first_readable = pdata->interrupt_read_buffer ;
					pdata->last_char_slot = pdata->interrupt_read_buffer + ( pdata->buffer_size -1);
					pdata->chars_interrupt_buffer = 0 ;
					current_read.wake_up_reason = WAKE_UP_REASON_PURGE ;
					wake_up( &(current_read.wq) );
				}
				again = TRUE ;
		}
		if ( again == TRUE ){
			timer_set( ptc, 5*HZ,(FUN)timeout_error, (unsigned long)pdata);		
		}else{
		 	pdata->purge_rx = FALSE ;
			pdata->purge_tx = FALSE ;
		}				
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
static int zyioctl( struct inode * pNode, struct file *pFile, unsigned int Cmd , unsigned long  Apd){
		PDrvData  pData = (PDrvData) pFile->private_data ;
		void * pvoid = ( void* ) Apd ;
		unsigned long flags ;
		
	printk( "zyioctl  Cmd = %d, %d\n", Cmd, ( (Cmd &0x3fff ) >> 2) );
		
	switch ( Cmd ){			
//============================================

	case	IOCTL_SERIAL_SET_BAUD_RATE://  mid
	      return baud_set( pData , pvoid );
	case     IOCTL_SERIAL_GET_BAUD_RATE://  easy
		return baud_get( pData, pvoid );	 	
	case     IOCTL_SERIAL_SET_QUEUE_SIZE ://mid finish
		return set_queue_size( pData , pvoid );
	case     IOCTL_SERIAL_SET_BREAK_ON ://unknown  finish
		return breakon_set_on( pData );	
	case     IOCTL_SERIAL_SET_BREAK_OFF: //unknown finish
		return breakon_clr( pData );
//============================================	
	case     IOCTL_SERIAL_SET_TIMEOUTS: //mid  finish
		return timeouts_set( pData , pvoid ) ;
	case     IOCTL_SERIAL_GET_TIMEOUTS : //easy  finish
		return timeouts_get( pData , pvoid ) ;
	case     IOCTL_SERIAL_SET_DTR  :     //unknown :finish
	case     IOCTL_SERIAL_CLR_DTR  :
	       {     //close interrupt
	            save_flags( flags );
	            cli();
	            if (  ( pData->hand_flow.ControlHandShake & SERIAL_DTR_MASK ) == SERIAL_DTR_HANDSHAKE ){
	            //open interrupt
	            	restore_flags( flags );		
	            	return  -1 ;
	            }	
	            if ( Cmd == IOCTL_SERIAL_SET_DTR ){
					dtr_set() ;
	            }else{	
					dtr_clr() ;
	            }	
	            //open interrupt
	            restore_flags( flags );
				  return  0 ;
	       }   	
	case     IOCTL_SERIAL_RESET_DEVICE: //finish
	 		return  0 ;
//============================================	
	case     IOCTL_SERIAL_SET_RTS : //unknown  finish
	case     IOCTL_SERIAL_CLR_RTS :
	       {
	            save_flags( flags );
	            cli();                    //close interrupt
	            if (   ( ( pData->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_RTS_HANDSHAKE )
	            		||
	            		 ( ( pData->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE )
	            ){	
	            	restore_flags( flags );	//open interrupt
	            	return  -1 ;
	            }
	
				if ( Cmd == IOCTL_SERIAL_SET_RTS ){
					rts_set() ;
				}else{
					rts_clr() ;
				}		
				restore_flags( flags); //open interrupt
				return  0 ;
	        }    	
	case     IOCTL_SERIAL_SET_XOFF : //unknown  finish
		return pretend_xoff( pData ) ;				
	case     IOCTL_SERIAL_SET_XON  :  //unknown  finish
		return pretend_xon( pData ) ;
	case     IOCTL_SERIAL_PURGE  : //hard  finish
		return zy_purge( pData , pvoid );
//============================================	
	case     IOCTL_SERIAL_GET_WAIT_MASK : //easy finish
		return wait_mask_get( pData , pvoid );
	case	IOCTL_SERIAL_SET_WAIT_MASK:  //finish
	      return set_wait_mask( pData , pvoid );
	case	IOCTL_SERIAL_WAIT_ON_MASK :  //finish
		return wait_mask( pData , pvoid );
	case     IOCTL_SERIAL_GET_LINE_CONTROL: //easy  finish
		return lcr_get( pData , pvoid ) ;
	case     IOCTL_SERIAL_SET_LINE_CONTROL: //unknown   finish
		return lcr_set( pData , pvoid ) ;
//============================================	
	case     IOCTL_SERIAL_GET_CHARS : //easy  finish
		return  chars_get( pData , pvoid ) ;
	case     IOCTL_SERIAL_SET_CHARS : //unknown  finish
		return  chars_set( pData , pvoid ) ;
	case     IOCTL_SERIAL_IMMEDIATE_CHAR: //finish
		return  trans_immediate( pData, pvoid );
	case     IOCTL_SERIAL_GET_HANDFLOW : //easy  finish
		return get_handflow( pData , pvoid );
	case     IOCTL_SERIAL_SET_HANDFLOW : //unknown   finish
		return set_handflow( pData, pvoid ) ;
//============================================	
	case     IOCTL_SERIAL_GET_MODEMSTATUS : //unknown   finish
		return get_modem_status( pData, pvoid );
	case     IOCTL_SERIAL_SET_FIFO_CONTROL : //unknown  finish
		return fcr_set ( pData, pvoid );
		
	case     IOCTL_SERIAL_XOFF_COUNTER: // hard    finish
		return  xoff_counter( pData , pvoid ); ;
	case     IOCTL_SERIAL_GET_PROPERTIES: //unknown    finish
		return  properties_get( pData , pvoid );
	case     IOCTL_SERIAL_GET_DTRRTS  : //unknown    finish
		return  get_dtr_rts( pData, pvoid );
//============================================	
	case     IOCTL_SERIAL_LSRMST_INSERT :  //unknown  finish
		return lsr_mst_insert( pData, pvoid );
	case     IOCTL_SERIAL_CONFIG_SIZE  :     //unknown  finish
		return config_size( pData, pvoid );
	case     IOCTL_SERIAL_GET_COMMCONFIG:
	case     IOCTL_SERIAL_SET_COMMCONFIG :
		return 0 ;
//============================================	
	case     IOCTL_SERIAL_GET_STATS : //easy	finish			
		return get_perf_stats( pData , pvoid );
	case     IOCTL_SERIAL_CLEAR_STATS : //finish
		clean_perf_stats( pData );
		return 0 ;
	case     IOCTL_SERIAL_GET_COMMSTATUS: //unknown  finish
	      return get_comm_status( pData , pvoid );
	case     IOCTL_SERIAL_GET_MODEM_CONTROL: //easy  finish
		return mcr_get( pData, pvoid) ;		
	case     IOCTL_SERIAL_SET_MODEM_CONTROL : //finish
		return mcr_set( pData, pvoid );
		
//============================================	
	default:
		;	 	
	}	
	return 0 ;
}

int zyopen( struct inode *pNode, struct file* pFile ){

		void *temp = NULL;
		kdev_t   DevNum;
//printk(" the minor num is %d \n ", MINOR( pNode->i_rdev) );	
	pFile->private_data = NULL;
	
	if (zyOpend ){
		
		printk("the device in use already\n");
	 	return -EBUSY ;	
	 	
	}else{
	
		if ( ( DevNum = MAJOR ( pNode->i_rdev ) ) == zyMJ_dev ){
				
			if ( ( temp = kmalloc(sizeof (Drvdata) ,GFP_KERNEL ) ) != NULL){
				unsigned long flags;
				save_flags( flags);
				cli();
			
				if ( request_irq (  zyIrq , zyIsr , SA_INTERRUPT, zyDevName , NULL ) == 0 ){
					printk("interrupt ok\n");
					zyOpend = TRUE;
					pFile->private_data = temp;
					zy_driver_data = temp;
					MOD_INC_USE_COUNT;
					init_serial( temp);		
					printk ( "open device success\n " );
					restore_flags(flags);
					return 0 ;
				}else{
				      kfree( temp);
				      printk("interrupt invalid\n");
				      restore_flags(flags);
				      return -1;
				}
			
			}else{
			
				printk ("can not get resource needed\n") ;
				return  -1;
			}
		}else{
				printk ("wrong MJ_dev\n") ;
				return -1;
		}
	}
}

int zyrelease( struct inode *pNode , struct file *pFile){
		unsigned long flags;
		
	save_flags( flags );
	cli();
	disable_all_intp();
	del_timer( &trigger );
	restore_flags( flags );
	if ( pFile->private_data ) {
		{	
			PDrvData pdata = pFile->private_data ;
			if ( pdata->interrupt_read_buffer ){
				kfree( pdata->interrupt_read_buffer );
			}else{
			      printk("caution: interrupt_read_buffer is NULL before free\n");
			}
		}
		
		kfree( pFile->private_data);
		pFile->private_data = NULL;
		zy_driver_data = NULL;
	}
	
	MOD_DEC_USE_COUNT;
	
	zyOpend = FALSE;
	disable_all_intp();
	free_irq( zyIrq , NULL ) ;
	printk( "close device \n");
	
	return 0 ;
}
void inc_ref( operation_queue * poq ){
	unsigned long flags ;
	
	save_flags( flags );
	cli();
	poq->count ++ ;
	restore_flags( flags);
}
void dec_ref( operation_queue * poq ){
	unsigned long flags ;
	
	save_flags( flags );
	cli();
	poq->count -- ;
	restore_flags( flags);
}
void zy_wake_up( operation_queue* poq ){
		unsigned long flags ;
		
		save_flags(flags);
		cli();
		if ( poq->count  ){
			wake_up( &(poq->wq) );
		}
		restore_flags(flags);
}
char read_from_intp_buf( PDrvData  pdata ){
	char temp ;
	temp =*( pdata->first_readable );
	pdata->chars_interrupt_buffer --;
	if (pdata->first_readable == ( pdata->interrupt_read_buffer +BUFFER_SIZE -1) ){
		pdata->first_readable = pdata->interrupt_read_buffer ;
	}else{
		pdata->first_readable ++ ;
	}
	return temp ;
}
void timeout_read( Ptimer_control ptc ){
		PDrvData pdata =( PDrvData )ptc->data ;
		unsigned long flags ;
	ptc->already_in_queue = 0 ;	
	printk("read timeouts\n");
	save_flags( flags);
	cli();
	if  ( pdata->read_buffer_base != pdata->interrupt_read_buffer ){
	
		pdata->read_already = pdata->current_char_slot - pdata->read_buffer_base ;
		pdata->read_buffer_base =pdata->interrupt_read_buffer;
		pdata->current_char_slot = pdata->interrupt_read_buffer;
		pdata->first_readable = pdata->interrupt_read_buffer ;
		pdata->last_char_slot = pdata->interrupt_read_buffer + ( pdata->buffer_size -1);
		pdata->chars_interrupt_buffer = 0 ;
		current_read.wake_up_reason = WAKE_UP_REASON_TIMEOUT ;	
		
	}
	wake_up(&(current_read.wq));
	restore_flags( flags);
}	
int zyread ( struct file  *pFile , char *user_buffer ,size_t  num , loff_t  * p64 ){
		PDrvData pdata = pFile->private_data ;
		char *temp = NULL ;
		char *help = NULL  ;
		int index ;		
		unsigned long flags ;
		
	printk("zyread %d bytes\n ", num );
		
	if ( user_buffer == NULL ){ return   -EINVAL ;}
	if ( num < 0 ){ return   -EINVAL ;}
	if ( num == 0 ){ return  0 ;}

	if ( sleep_or_start( &current_read, &read_queue , pdata, MODE_READ) < 0 ) return -1;
	
	if ( ( temp =  kmalloc(num , GFP_KERNEL) ) == NULL ){
		
		clean_current_control( &current_read );
		zy_wake_up( &read_queue );
		printk("can not get memory for buffer \n");
		return -1 ;// can not get resource .
	}
	printk("get memory temp = %x \n",(unsigned int) temp);
	save_flags( flags );
	cli();
//	
	if ( pdata->chars_interrupt_buffer >= num ){
		
		for( index =0, help=temp ;index < num ; index ++, help ++){
				(*help )=read_from_intp_buf( pdata ) ;			
		}// maybe need change : refer put_char ;
		handle_reduce_int_buffer( pdata);
		restore_flags( flags );
				
		if( copy_to_user( user_buffer , temp ,num ) == 0){
			kfree( temp );
			//clean_reading ();
			clean_current_control( &current_read );
			zy_wake_up( & read_queue ) ;
			printk("read success \n");
			return num ;
		}else{
			kfree( temp );
			//clean_reading ();
			clean_current_control( &current_read );
			zy_wake_up( & read_queue ) ;
			printk("copy_to_user :error\n");
			return -1 ;		
		}
		
	}else{  //need wait hardware complete
			unsigned char reason ;
			int  actual_num ;
	    if ( (pdata->chars_interrupt_buffer > 0 )&&(pdata->chars_interrupt_buffer < num ) ){
	    	int  zy = pdata->chars_interrupt_buffer ;
	    		printk(" copy data from intp buffer partly \n");
	    		handle_reduce_int_buffer( pdata);
	    		for( index=0 ,help=temp ; index < zy ; index++ , help++){
	    			*help= read_from_intp_buf( pdata );
	    		}
	    		handle_reduce_int_buffer( pdata);
	    		pdata->read_buffer_base =temp ;
	    		pdata->current_char_slot =help ;
	    		pdata->last_char_slot = temp+( num - 1 );
	    }else{
	    		pdata->read_buffer_base =temp ;
	    		pdata->current_char_slot =temp ;
	    		pdata->last_char_slot = temp+( num - 1 );	
	    }
	
		restore_flags( flags );
//		timer_del(pdata->);
		timer_set( &(pdata->read_timer), 20*HZ ,(FUN) timeout_read, (unsigned long )pdata  );
		sleep_on( &(current_read.wq ) );
		timer_del( &(pdata->read_timer) );
		
		save_flags( flags );
		cli();
		
		reason = current_read.wake_up_reason ;
		if ( reason == WAKE_UP_REASON_OK ){
			actual_num = num ;
		}else{// purge and so on
		      actual_num = pdata->read_already ;
		}
		pdata->read_buffer_base = pdata->interrupt_read_buffer ;
		pdata->current_char_slot = pdata->read_buffer_base ;
		pdata->last_char_slot = pdata->read_buffer_base + ( pdata->buffer_size -1 );
		pdata->first_readable = pdata->read_buffer_base ;
		pdata->read_by_isr = 0 ;
		
		restore_flags( flags );		
//		copy_to_user();
		if ( copy_to_user( user_buffer, temp, actual_num ) != 0 ){
		 	kfree(temp);
			clean_current_control( &current_read );
			zy_wake_up( &read_queue );
			printk("copy to user error\n");
			return  -1 ;
		}
		
		kfree(temp);
		//clean_reading();
		clean_current_control( &current_read );
		zy_wake_up( &read_queue );
		return actual_num ;
	}	
}
void timeout_write( Ptimer_control ptc ){
		PDrvData pdata = ( PDrvData )ptc->data ;
		unsigned long flags;
	ptc->already_in_queue = 0;		
	printk("write timeouts\n");
	save_flags( flags);
	cli();	
	if ( pdata->write_length ){
		pdata->write_remain = pdata->write_length;	
		pdata->write_length = 0 ;
		current_write.wake_up_reason = WAKE_UP_REASON_TIMEOUT ;
		wake_up( &(current_write.wq ) );
	}
	restore_flags( flags);	
}
int zywrite( struct file *pFile , const char *user_buffer , size_t num , loff_t *p64 ){
		PDrvData pdata = pFile->private_data ;
		char *temp = NULL ;
		unsigned long flags ;

    printk("-->zywrite \n");
    if ( num == 0) return 0 ;
    if ( ( num < 0 ) || ( user_buffer == NULL )  ){
    	printk("parameter error \n");
    	return -1 ;
    }

	if ( sleep_or_start( &current_write, &write_queue , pdata, MODE_WRITE) < 0 ) return -1;
	
	save_flags( flags); //clear pending xoff_count
	cli();
	if ( pdata->count_since_xoff ){
		pdata->count_since_xoff = 0 ;
		current_xoff_count.wake_up_reason =	WAKE_UP_REASON_PURGE ;
		wake_up( &(current_xoff_count.wq));	
	}
	restore_flags( flags);
	while( current_xoff_count.ptask ){
		schedule_timeout(800);
	}//clean pending xoff_count
	
    temp = kmalloc( num , GFP_KERNEL);

    if( temp == NULL ){
    	printk("can not get memory \n");
    	clean_current_control( & current_write );
    	zy_wake_up( & write_queue );
    	return -1;
    }else{
    		if ( copy_from_user( temp, user_buffer, num ) != 0){
    			printk("copy_from_user : error\n");
    			kfree( temp );
    			clean_current_control( & current_write );
    			zy_wake_up( & write_queue );
    			return -1;
    		}else{
    			unsigned char reason ;

    			save_flags( flags );
    			cli();
    			pdata->write_length = num;
    			pdata->pcurrent_write_char= temp;
    			restore_flags( flags );
    			//
    			timer_del( &(pdata->write_timer) );
    			timer_set( &(pdata->write_timer),30*HZ,(FUN)timeout_write ,(unsigned long) pdata );
    			//invoke_hardware();
			init_timer( &trigger );
			trigger.expires = HZ+jiffies;
			trigger.function = invoke_hardware ;
			trigger.data = (unsigned long)NULL;
			add_timer( &trigger );

			printk("sleep on\n");
    			sleep_on( &(current_write.wq));
    			del_timer(& trigger );
    			timer_del( &(pdata->write_timer) );
    			save_flags( flags );
    			cli();
    			reason =current_write.wake_up_reason ;
    			restore_flags( flags );
			printk("wake_up_reason =%d\n", reason );	//used for test
    			if ( reason == WAKE_UP_REASON_OK ){
    				
    				kfree( temp );
    				clean_current_control( & current_write );
    				process_empty_transmit( pdata);
    				zy_wake_up( & write_queue );
    				return num ;
    			}else{
                  long  write_remain = pdata->write_remain ;
    				kfree( temp );
    				clean_current_control( &current_write );
    				zy_wake_up( &write_queue );
    				process_empty_transmit( pdata );
    				return  ( num - write_remain ) ;
    			}    		
    		}
    }
}

void process_RDA_CTI( PDrvData pdata ){
		char recv_char ;
		char tempLSR ;
	do{
		recv_char = inb( reg_rd );
		pdata->perf_stats.ReceivedCount ++ ;		
		recv_char &=pdata->valid_data_mask ;
		
		if ( ! recv_char  && ( pdata->hand_flow.FlowReplace & SERIAL_NULL_STRIPPING ) ){
			goto ReceiveDoLineStatus;
		}
		if (  ( pdata->hand_flow.FlowReplace & SERIAL_AUTO_TRANSMIT )&&
			( (recv_char==pdata->special_chars.XonChar ) || ( recv_char==pdata->special_chars.XoffChar ) )
		){
			if ( recv_char == pdata->special_chars.XoffChar ){
				pdata->tx_holding |= SERIAL_TX_XOFF ;
				if (  (pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK)== SERIAL_TRANSMIT_TOGGLE ){
					//serial_insert_queue_dpc  lower RTS
					timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
				}
			}else{
				if ( pdata->tx_holding & SERIAL_TX_XOFF ){
					pdata->tx_holding &= ( ~SERIAL_TX_XOFF);
				}			
			}
			goto ReceiveDoLineStatus ;		
		}
		if ( pdata->isr_wait_mask ){
			if ( pdata->isr_wait_mask & SERIAL_EV_RXCHAR ){
				pdata->history_mask |= SERIAL_EV_RXCHAR ;
			}
			if( ( pdata->isr_wait_mask & SERIAL_EV_RXFLAG) &&
				( pdata->special_chars.EventChar == recv_char )
			 ){
			 	pdata->history_mask |= SERIAL_EV_RXFLAG ;
			}
			//FLOWING  CHANGE FOR  COMPLETE  WAIT
			if (  (pdata->p_wait_value)&& pdata->history_mask  ){
			//wake_up  wait_mask ;
				printk("process_RDA_CTI : wait_mask return %x\n", pdata->history_mask);
				*( pdata->p_wait_value) =pdata->history_mask ;
				pdata->history_mask = 0 ;
				pdata->p_wait_value = NULL ;
				current_wait_mask.wake_up_reason = WAKE_UP_REASON_OK ;
				wake_up( &(current_wait_mask.wq) );
			}		
		}
		
		put_char( pdata, recv_char );
		
		if ( pdata->escape_char && ( pdata->escape_char == recv_char ) ){
			put_char( pdata, SERIAL_LSRMST_ESCAPE );
		}
		
ReceiveDoLineStatus:

		if ( ! ( ( tempLSR = process_lsr( pdata ) ) & SERIAL_LSR_DR )  ){
			return ;
		}
		if (  ( tempLSR & ~( SERIAL_LSR_THRE | SERIAL_LSR_TEMT | SERIAL_LSR_DR) )
			&&pdata->escape_char
		){
			return ;
		}
		
	}while( TRUE );
	return ;	
}
void process_THR( PDrvData pdata ){
	
	printk("-> process_THR \n");
	pdata->holding_empty = TRUE ;
	
	if ( pdata->write_length || pdata->transmit_immediate || pdata->send_xon || pdata->send_xoff ){
	
		pdata->emptied_transmit =TRUE ;
		
		if ( pdata->hand_flow.ControlHandShake & SERIAL_OUT_HANDSHAKEMASK ){
			handle_modem_update( pdata, TRUE );
		}
		if ( pdata->send_xon && !( pdata->tx_holding & ( ~SERIAL_TX_XOFF ) ) ){  //only SERIAL_TX_XOFF exists
		
			if ( ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE ){
				rts_set();
				pdata->perf_stats.TransmittedCount ++;
				outb( pdata->special_chars.XonChar , reg_wr ); // XON
				// serial_insert_QueueDpc for lower RTS
				timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
				
			}else{
				pdata->perf_stats.TransmittedCount ++;
				outb( pdata->special_chars.XonChar , reg_wr );				
			}
			pdata->send_xon = FALSE ;
			pdata->holding_empty = FALSE ;
			pdata->tx_holding &=  ~SERIAL_TX_XOFF ;
			pdata->rx_holding &=  ~SERIAL_RX_XOFF;
			
		}else if ( pdata->send_xoff && ( ! pdata->tx_holding ) ){ // XOFF
		
			if ( ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE ){
				rts_set();
				pdata->perf_stats.TransmittedCount ++;
				outb( pdata->special_chars.XoffChar , reg_wr );
				// serial_insert_QueueDpc for lower RTS
				timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
				
			}else{
				pdata->perf_stats.TransmittedCount ++;
				outb( pdata->special_chars.XoffChar , reg_wr );				
			}
			if ( ! ( pdata->hand_flow.FlowReplace & SERIAL_XOFF_CONTINUE ) ){
			
				pdata->tx_holding |= SERIAL_TX_XOFF ;
				
				if (  ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE ){
				 	//serial_insert_dpc  lower RTS
				 	timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
				}
			}
			pdata->send_xoff = FALSE ;
			pdata->holding_empty = FALSE ;	
				
		}else if ( pdata->transmit_immediate &&
						( ! pdata->tx_holding || ( pdata->tx_holding == SERIAL_TX_XOFF ) )
			){
			pdata->transmit_immediate = FALSE ;
			printk("trans_immediate in ISR = %d", pdata->immediate_char );
			
			if (  ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) == SERIAL_TRANSMIT_TOGGLE  ){
				rts_set();
				outb( pdata->immediate_char , reg_wr );
				// insert_queue_dpc  lower RTS
				timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
			}else{
				outb( pdata->immediate_char , reg_wr );
			}
			pdata->perf_stats.TransmittedCount +=1 ;
			pdata->holding_empty = FALSE ;
			current_immediate.wake_up_reason = 	WAKE_UP_REASON_OK ;
			//wake up for immediate_char  write complete
			wake_up( &(current_immediate.wq) );

		}else if (  ! pdata->tx_holding ){		
				unsigned long amount_to_write ;
								
			if ( pdata->fifo_present ){			
				amount_to_write =
					( pdata->tx_fifo_amount < pdata->write_length  ) ? pdata->tx_fifo_amount : pdata->write_length ;				
			}else{
			      amount_to_write = 1 ;
			}
			if (  ( pdata->hand_flow.FlowReplace & SERIAL_RTS_MASK ) ==  SERIAL_TRANSMIT_TOGGLE ){
				int i ;
				rts_set();
				
				for( i = 0 ; i < amount_to_write ; i++ ){
					outb( *( pdata->pcurrent_write_char ), reg_wr ) ;
					pdata->pcurrent_write_char ++ ;
					pdata->perf_stats.TransmittedCount ++ ;
					pdata->write_length -- ;	
				}
				
			      //insert_queueDpc  lower RTS
			      timer_set( &(pdata->low_rts_timer ), 1*HZ, (FUN) perhaps_low_rts , (unsigned long) pdata );
			}else{
			      int i ;
				for( i = 0 ; i < amount_to_write ; i++ ){
					outb( *( pdata->pcurrent_write_char ), reg_wr ) ;
					pdata->pcurrent_write_char ++ ;
					pdata->perf_stats.TransmittedCount ++ ;
					pdata->write_length -- ;	
				}					
			}
			if ( pdata->write_length == 0 ){
				//wake_up  current_write
				printk("->here...");
				pdata->pcurrent_write_char = NULL;
				pdata->write_remain = 0 ;
				current_write.wake_up_reason = WAKE_UP_REASON_OK;
				wake_up( &(current_write.wq ) );
				printk("<-here\n");
			}					
		}					
	}
      return ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void zyIsr( int irq , void *dev_id , struct pt_regs *regs ){
		unsigned char iir;
		PDrvData  dr_data=NULL;
#ifdef	zhang_dbg
	printk("->ISR irq=%d\n",irq);
#endif
	iir=inb(reg_iir);
	if ( iir & SERIAL_IIR_NO_INTERRUPT_PENDING ) return;

	if ( zyOpend == FALSE ){
#ifdef	zhang_dbg
            printk("zyOpend = FALSE\n");
#endif
		do{
			iir &= (~SERIAL_IIR_FIFOS_ENABLED) ;
#ifdef	zhang_dbg
			printk("iir =%d\n",iir);
#endif
			switch( iir ) {
			case SERIAL_IIR_RLS:
					inb( reg_lsr );
					break;
			case SERIAL_IIR_RDA:
			case SERIAL_IIR_CTI:
					inb( reg_rd );
					break;
			case SERIAL_IIR_THR:
					break;
			case SERIAL_IIR_MS:
					inb( reg_msr );
					break;	
			default:
					;
			}			
		}while(  ! ( ( iir = inb( reg_iir ) ) & SERIAL_IIR_NO_INTERRUPT_PENDING ) ) ;
	}else{
		dr_data = zy_driver_data;
#ifdef	zhang_dbg
		printk( "zyOpend =TRUE\n");
		printk("dr_data =%x\n", (unsigned int) dr_data ) ;
#endif
            do{
			iir &= ( SERIAL_IIR_RLS | SERIAL_IIR_RDA | SERIAL_IIR_CTI | SERIAL_IIR_THR | SERIAL_IIR_MS ) ;
			printk("iir =%d\n",iir);
			switch( iir ) {
			case SERIAL_IIR_RLS:
					printk("process_IIR_RLS\n");
					process_lsr( dr_data );
					break;
			case SERIAL_IIR_RDA:
			case SERIAL_IIR_CTI:
#ifdef	zhang_dbg
					printk("process_RDA_CTI\n");
#endif
					process_RDA_CTI( dr_data );
					break;
			case SERIAL_IIR_THR:
DoTransmitStuff :			
                              process_THR( dr_data );
					break;
			case SERIAL_IIR_MS:
					printk("process_IIR_MS\n");
					handle_modem_update( dr_data, FALSE );
					break;	
			default:
					printk("some thing wrong\n");
			}
			
		}while(  ! ( ( iir = inb( reg_iir ) ) & SERIAL_IIR_NO_INTERRUPT_PENDING ) ) ;
		
		if ( process_lsr( dr_data ) & SERIAL_LSR_THRE ){
		
			if(  ! dr_data->tx_holding  && ( dr_data->write_length || dr_data->transmit_immediate) ){
				goto DoTransmitStuff ;
			}
		}		
	}
#ifdef	zhang_dbg
	printk("<-ISR\n");
#endif
}


//
// module functions to install driver in system
//
struct file_operations opr ={

             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL, //ioctl
             NULL,
             NULL, //open
             NULL, //release
             NULL,
             NULL,
             NULL,
             NULL, //open
             NULL, //release
             NULL,
             NULL,
             NULL,
             NULL
};
int init_module(void ){
	int i;
//	unsigned  long  flags;
	
	zyOpend = FALSE ;
	opr.ioctl = zyioctl ;	
	opr.open = zyopen ;
	opr.release = zyrelease ;
	opr.read = zyread;
	opr.write = zywrite ;
	i = register_chrdev( zyMJ_dev , zyDevName,  &opr  ) ;

    //printk ("the value of register is %d\n", i );

	if (  i < 0 ){    // MJ_dev
		printk(" register chrdev error\n " );
		return  -1;
	
	}else{
/**
		disable_all_intp();
		outb( 0xb, reg_mcr );
		flags |= SA_INTERRUPT;
		
		if ( request_irq (  zyIrq , zyIsr , flags , zyDevName , NULL ) == 0 ){
			if(check_region(reg_rd, 8)!= 0){
				release_region( reg_rd,8);
			}
			request_region(reg_rd, 8, zyDevName);
			
			printk("zhalfa: Driver loadded\n");
			return	0	;
			
		} else{
		    printk("erro : can not get interruption !\n");
		    unregister_chrdev( zyMJ_dev, zyDevName );
		    return  -1;		
		}
*/
		printk("zhalfa: Driver loadded\n");
		return 0 ;
	}
}

void cleanup_module(){

	unregister_chrdev( zyMJ_dev, zyDevName );
	
//	free_irq( zyIrq , NULL ) ;
//	release_region( reg_rd, 8);
	
	printk("zhalfa: Driver unloadded\n");
	
}

