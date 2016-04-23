/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Thu Nov 13 12:28:44 EST 2003
    copyright            : (C) 2003 by zhang
    email                : zhalfa@sina.com.cn
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<headio.h>

int DeviceIoControl( 	int fid,
			unsigned long cmd ,
			void *inbuffer,
			long insize,
			void *outbuffer,
			long outsize,
			long *bytes_returned,
			void *b,
			long *result   //return value for rdp
			) ;

static char name[] = "/dev/zy";
int fid ;
char buffer[ 1024 ];

void thread_c(){
	unsigned long  tmp = (SERIAL_PURGE_TXABORT	|
												SERIAL_PURGE_RXABORT	|
												SERIAL_PURGE_TXCLEAR		|
												SERIAL_PURGE_RXCLEAR		) ;
	
	int  result_c ;
	
	printf("start c\n");
	result_c =ioctl(fid , IOCTL_SERIAL_PURGE, &tmp);
	printf("reslult_c =%d\n", result_c );
	
}
void thread_b(void ){
	char buffer[ 1024];
	int  result_b ;
	
	if ( fork()){
		thread_c();	 	
	}else{

	printf("start b\n");
	result_b = read(fid ,buffer, 200);
	printf("result_b = %d\n", result_b );	
	}
		
}
void thread_a(void ){
	char buffer[ 1024];
	int  result_a ;
	
	if ( fork() ){
			thread_b();
	}else{

	printf("start a\n");
	result_a =read(fid , buffer, 230) ;
	printf("result_a = %d\n", result_a );
	}
}

int main( void ){

       int i , j ;

		int result = 0;

		Serial_xoff_counter tmp ;

	fid = open ( name , O_RDWR );
	printf("fid = %d\n", fid );
	if ( 0 ){ // set chars
			Serial_chars tmp ;
		tmp.EofChar=1;
		tmp.ErrorChar=2;
		tmp.BreakChar=3;
		tmp.EventChar=4;
		tmp.XonChar='a';
		tmp.XoffChar='m';
		result = result = ioctl( fid, IOCTL_SERIAL_SET_CHARS, &tmp);
		printf("set chars = %d\n", result );
	}
	if ( 0 ){ // get chars
			Serial_chars tmp ;

		result = ioctl( fid, IOCTL_SERIAL_GET_CHARS, & tmp);
		if (! result ){
				printf("EofChar=%c\n;ErrorChar=%c\n;BreakChar=%c\n;EventChar=%c\n;XonChar=%c\n;XoffChar=%c\n",
					tmp.EofChar,tmp.ErrorChar,tmp.BreakChar,tmp.EventChar,tmp.XonChar,tmp.XoffChar
				);
		}
	}
	for ( i = 0 ; i < 1 ; i++ ){
		char *tmp ="123456789012345678901234567890";

		result = write ( fid , tmp , 30 );
		printf("NO : %d  write result = %d\n", i , result );
		//scanf("%d", &j );
		result = 0 ;
	}
	scanf("%c", &j);
	for ( i = 0 ; i < 0 ; i++ ){
		int k ;
		result = read( fid, buffer , 200);
		printf("No: %d  result = %d\n" , i , result );
		for( k=0 ; k< result ; k++){
			printf("%c",buffer[k]);
		}
		printf("\n");
		result =0 ;
	}
	if ( 1){
		tmp.Counter = 100;
		tmp.XoffChar = 13;
		result = ioctl( fid , IOCTL_SERIAL_SET_BREAK_ON, NULL);//&tmp
		printf("ioctl result = %d\n", result );
	}

	if ( 1 ) thread_a();

	if ( 1){

	ULONG tmp = SERIAL_EV_BREAK|SERIAL_EV_RX80FULL|SERIAL_EV_ERR;
	ULONG get;
		printf("@mask= %x\n", tmp);
		result =ioctl(fid , IOCTL_SERIAL_SET_WAIT_MASK, &tmp);
		printf("@setmask =%d\n", (int) result );
		result =ioctl(fid, IOCTL_SERIAL_WAIT_ON_MASK,&get );
		printf("@wait_mask= %x\n", get);
		//result = read(fid , buffer, 200 );
		//printf( "read =%d\n ", (int) result );
	}

	scanf("%c", &j);
	close( fid);
	return 0 ;
}

void para_error( char *str, long *result ){
	printf(str);

	if ( result ) *result = 0xff ;
}

int  DeviceIoControl( 	int fid,
										unsigned long cmd ,
										void *inbuffer ,
										long insize,
										void *outbuffer,
										long outsize ,
										long *bytes_returned,
										void *b,		//old overlapped
										long *result	//return value for rdp
									){

	switch ( cmd){
	
		case IOCTL_SERIAL_SET_BAUD_RATE : {
			Serial_baud_rate *p_baud_rate = (Serial_baud_rate*)inbuffer ;
			
				if ( ! p_baud_rate ){
                  para_error("p_baud_rate=NULL\n", result );
					return -1 ;
				}
				if ( insize < sizeof(Serial_baud_rate) ){
                  para_error("insize < sizeof(Serial_baud_rate)\n", result );
					return -1 ;
				}
				return ( (*result) = ioctl( fid, cmd, p_baud_rate) ) ;
		}
		case IOCTL_SERIAL_GET_BAUD_RATE :{
			Serial_baud_rate *p_baud_rate = (Serial_baud_rate*)outbuffer ;

				if ( ! p_baud_rate ){
                  para_error("p_baud_rate=NULL\n", result );
					return -1 ;
				}
				if ( outsize < sizeof(Serial_baud_rate) ){
                  para_error("insize < sizeof(Serial_baud_rate)\n", result );
					return -1 ;
				}
				return ( (*result) = ioctl( fid, cmd, p_baud_rate) ) ;
		}
		case IOCTL_SERIAL_SET_QUEUE_SIZE :{
			Serial_queue_size *p_queue_size =(Serial_queue_size*)inbuffer ;

				if ( ! p_queue_size ){
					para_error("p_queue_size=NULL\n", result );
					return -1 ;
				}
				if ( insize < sizeof(Serial_queue_size) ){
                  para_error("insize < sizeof(Serial_baud_rate)\n", result );
					return -1 ;
				}
				return ( (*result) = ioctl( fid, cmd, p_queue_size) ) ;
		}
		case IOCTL_SERIAL_SET_BREAK_ON :
		case IOCTL_SERIAL_SET_BREAK_OFF: {

				return  ( (*result) = ioctl( fid, cmd, NULL) ) ;
		}
		case IOCTL_SERIAL_SET_TIMEOUTS :{
			Serial_timeouts *p_timeouts = (Serial_timeouts*)inbuffer ;

				if ( ! p_timeouts){
					para_error("p_timeouts= NULL\n", result );
					return -1;
				}
				if ( insize < sizeof(Serial_timeouts) ){
					para_error("insize < sizeof(Serial_timeouts)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_timeouts) );
		}
		case IOCTL_SERIAL_GET_TIMEOUTS :{
			Serial_timeouts *p_timeouts = (Serial_timeouts*)outbuffer ;

				if ( ! p_timeouts){
					para_error("p_timeouts= NULL\n", result );
					return -1;
				}
				if ( outsize < sizeof(Serial_timeouts) ){
					para_error("insize < sizeof(Serial_timeouts)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_timeouts) );
		}
		case IOCTL_SERIAL_SET_DTR  :
		case IOCTL_SERIAL_CLR_DTR  : {
		       return  ( (*result) = ioctl( fid, cmd, NULL) );
		}
		case IOCTL_SERIAL_RESET_DEVICE :{
				return  ( (*result) = ioctl( fid, cmd, NULL) );
		}
		case IOCTL_SERIAL_SET_RTS  :
		case IOCTL_SERIAL_CLR_RTS  : {
				return   ( (*result) = ioctl( fid, cmd, NULL) );
		}
		case IOCTL_SERIAL_SET_XON :
		case IOCTL_SERIAL_SET_XOFF :{
				return   ( (*result) = ioctl( fid, cmd, NULL) );
		}
		case IOCTL_SERIAL_WAIT_ON_MASK :
		case IOCTL_SERIAL_GET_WAIT_MASK :{
			ULONG *p_long = (ULONG*)outbuffer;
				if ( ! p_long ){
					para_error("get_wait_mask error\n", result);
					return -1 ;
				}
				if ( outsize < sizeof (ULONG) ){
					para_error("outsize < sizeof (ULONG)\n", result);
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_long) );
		}
		case	IOCTL_SERIAL_SET_WAIT_MASK: {
			ULONG *p_long = (ULONG*)inbuffer;
				if ( ! p_long ){
					para_error("get_wait_mask error\n", result);
					return -1 ;
				}
				if ( insize < sizeof (ULONG) ){
					para_error("outsize < sizeof (ULONG)\n", result);
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_long) );
		}
		case IOCTL_SERIAL_GET_LINE_CONTROL:{
			Serial_line_control *p_line_control = (Serial_line_control*)outbuffer ;

				if ( ! p_line_control ){
					para_error("p_line_control =NULL\n", result );
					return -1 ;
				}
				if ( outsize < sizeof(Serial_line_control) ){
					para_error("outsize < sizeof(Serial_line_control)\n", result);
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_line_control) );
		}
		case IOCTL_SERIAL_SET_LINE_CONTROL:{
			Serial_line_control *p_line_control = (Serial_line_control*)inbuffer ;
						
				if ( ! p_line_control ){
					para_error("p_line_control =NULL\n", result );
					return -1 ;
				}
				if ( insize < sizeof(Serial_line_control) ){
					para_error("outsize < sizeof(Serial_line_control)\n", result);
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_line_control) );	
		}
		case IOCTL_SERIAL_GET_CHARS :{
			Serial_chars *p_chars = (Serial_chars*) outbuffer;
			
				if ( ! p_chars ){
					para_error("p_chars=NULL\n", result );
					return -1;
				}
				if ( outsize < sizeof(Serial_chars) ){
					para_error("outsize < sizeof(Serial_chars)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_chars) );
		}
		case IOCTL_SERIAL_SET_CHARS :{
			Serial_chars *p_chars = (Serial_chars*) inbuffer;
			
				if ( ! p_chars ){
					para_error("p_chars=NULL\n", result );
					return -1;
				}
				if ( insize < sizeof(Serial_chars) ){
					para_error("outsize < sizeof(Serial_chars)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_chars) );
		}
		case IOCTL_SERIAL_IMMEDIATE_CHAR :{
			char *pchar = (char*)inbuffer ;
			
				if ( ! pchar ){
					para_error("pchar=NULL\n", result);
					return -1 ;
				}
				if ( insize < sizeof(char) ){
					para_error("insize < sizeof(char)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, pchar) );
		}
		case IOCTL_SERIAL_GET_HANDFLOW :{
			Serial_handflow *p_handflow = (Serial_handflow*)outbuffer;
			
				if ( ! p_handflow ){
					para_error( "p_handflow= NULL\n", result );
					return -1 ;				
				}
				if ( outsize < sizeof( Serial_handflow) ){
					para_error("outsize < sizeof( Serial_handflow)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_handflow) );
		}
		case IOCTL_SERIAL_SET_HANDFLOW : {
			Serial_handflow *p_handflow = (Serial_handflow*)inbuffer;
			
				if ( ! p_handflow ){
					para_error( "p_handflow= NULL\n", result );
					return -1 ;				
				}
				if ( insize < sizeof( Serial_handflow) ){
					para_error("insize < sizeof( Serial_handflow)\n", result );
					return -1 ;
				}
				return  ( (*result) = ioctl( fid, cmd, p_handflow) );		
		}		 		
	
	
	default :
		return -1 ;//no support ;
	}
}
