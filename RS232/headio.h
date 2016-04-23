/***************************************************************************
                          headio.h  -  description
                             -------------------
    begin                : Mon Aug 18 2003
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
#ifdef zy_driver
#include <linux/time.h>
#endif

typedef unsigned char  BOOL ;
typedef BOOL BOOLEAN ;

typedef unsigned short  USHORT ;
typedef unsigned long ULONG ;

typedef void(*FUN)(unsigned long );

//---------------------------------data---------struct----------------------
typedef struct  _serial_line_control{
	char  StopBits ;
	char  Parity ;
	char  WordLength ;
		
} Serial_line_control , *pSerial_line_control ;

typedef struct  _serial_timeouts{
	long  ReadIntervalTimeout ;
	long  ReadTotalTimeoutMultiplier ;
	long  ReadTotalTimeoutConstant ;
	long  WriteTotalTimeoutMultiplier ;
	long  WriteTotalTimeoutConstant ;

} Serial_timeouts, *pSerial_timeouts;

typedef struct  _serial_chars{
	char EofChar ;
	char ErrorChar ;
	char BreakChar ;
	char EventChar ;
	char XonChar ;
	char XoffChar ;
	
} Serial_chars ,*pSerial_chars ;

typedef struct _serial_handflow{
	long ControlHandShake ;
	long FlowReplace ;
	long XonLimit ;
	long XoffLimit ;
	
} Serial_handflow, *pSerial_handflow ;

typedef struct  _serial_perf_stats{
	unsigned long ReceivedCount ;
	unsigned long TransmittedCount ;
	unsigned long FrameErrorCount ;
	unsigned long SerialOverrunErrorCount ;
	unsigned long BufferOverrunErrorCount ;
	unsigned long ParityErrorCount ;

} Serial_perf_stats ;

typedef struct  _serial_commprop{
	unsigned short		PacketLength ;
	unsigned short		PacketVersion ;
	
	unsigned long		ServiceMask ;
	unsigned long		Reserved1 ;
	unsigned long		MaxTxQueue ;
	unsigned long		MaxRxQueue ;
	unsigned long		MaxBaud ;
	unsigned long		ProvSubType ;
	unsigned long      ProvCapabilities ;
	unsigned long      SettableParams ;
	unsigned long      SettableBaud ;
	
	unsigned short		SettableData ;
	unsigned short		SettableStopParity ;
	
	unsigned long      CurrentTxQueue ;
	unsigned long      CurrentRxQueue  ;
	unsigned long		ProvSpec1 ;
	unsigned long		ProvSpec2 ;
	
	unsigned short		ProvChar[ 1 ];
	
}  Serial_commprop , PSerial_commprop ;

typedef  struct  _serial_baud_rate{

unsigned long BaudRate ;

} Serial_baud_rate ;

typedef	struct _serial_status{
	unsigned long Errors ;
	unsigned long HoldReasons ;
	unsigned long AmountInInQueue ;
	unsigned long AmountInOutQueue ;
	BOOLEAN EofReceived ;
	BOOLEAN WaitForImmediate ;

} Serial_status ;

typedef struct  _serial_queue_size{
	unsigned long InSize ;
	unsigned long OutSize ;
	
} Serial_queue_size ;

typedef	struct  _serial_xoff_counter{
	unsigned long Timeout ;
	long Counter ;
	unsigned char XoffChar ;

} Serial_xoff_counter ;


//-------------------------------- IOCTL_CODE --------------------------------

#define CTL_CODE( DeviceType , Function ,Method,Access )  (    				  \
( (DeviceType)<< 16  ) |  ( (Access)  <<14 ) |  ( (Function)<<2) |  (Method ) \
)

#define		FILE_DEVICE_SERIAL_PORT			0x0000001b
#define	 	METHOD_BUFFERED					0
#define	 	FILE_ANY_ACCESS					0

//
//--------------------------ioctl-----code-------------------------------
//

#define IOCTL_SERIAL_SET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  1,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_QUEUE_SIZE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  2,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  3,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_BREAK_ON       CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  4,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_BREAK_OFF      CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  5,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_IMMEDIATE_CHAR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  6,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  7,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  8,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT,  \
  9,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_DTR				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,10,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_RESET_DEVICE			\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_RTS				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_RTS				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_XOFF				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_XON				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,15,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_WAIT_MASK		\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,16,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_WAIT_MASK		\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,17,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_WAIT_ON_MASK			\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,18,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_PURGE				\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,19,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_BAUD_RATE		\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,20,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_LINE_CONTROL		\
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,21,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_CHARS            \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,22,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_CHARS            \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,23,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_HANDFLOW         \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,24,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_HANDFLOW         \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,25,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_MODEMSTATUS      \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,26,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_COMMSTATUS       \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,27,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_XOFF_COUNTER         \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,28,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_PROPERTIES       \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,29,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_DTRRTS           \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,30,METHOD_BUFFERED,FILE_ANY_ACCESS)

  //
  // Serenum reserves function codes between 128 and 255.  Do not use.
  //

  // begin_winioctl

#define IOCTL_SERIAL_LSRMST_INSERT        \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,31,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERENUM_EXPOSE_HARDWARE     \
  CTL_CODE(FILE_DEVICE_SERENUM,128,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERENUM_REMOVE_HARDWARE     \
  CTL_CODE(FILE_DEVICE_SERENUM,129,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERENUM_PORT_DESC           \
  CTL_CODE(FILE_DEVICE_SERENUM,130,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERENUM_GET_PORT_NAME       \
  CTL_CODE(FILE_DEVICE_SERENUM,131,METHOD_BUFFERED,FILE_ANY_ACCESS)

  // end_winioctl

#define IOCTL_SERIAL_CONFIG_SIZE          \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,32,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_COMMCONFIG       \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,33,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_COMMCONFIG       \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,34,METHOD_BUFFERED,FILE_ANY_ACCESS)


#define IOCTL_SERIAL_GET_STATS            \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,35,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLEAR_STATS          \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,36,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_GET_MODEM_CONTROL    \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,37,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_MODEM_CONTROL    \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,38,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_FIFO_CONTROL     \
  CTL_CODE(FILE_DEVICE_SERIAL_PORT,39,METHOD_BUFFERED,FILE_ANY_ACCESS)


  //
  // internal serial IOCTL's
  //

#define IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 1,     \
  METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE  CTL_CODE(FILE_DEVICE_SERIAL_PORT, 2,     \
  METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 3,     \
  METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS  CTL_CODE(FILE_DEVICE_SERIAL_PORT, 4,     \
  METHOD_BUFFERED, FILE_ANY_ACCESS)
//
//----------------------------------ioctl------------code------------end--------------------------------------------------
//

#define SERIAL_NONE_PARITY		   0X00
#define SERIAL_ODD_PARITY          0X08
#define SERIAL_EVEN_PARITY         0X18
#define SERIAL_MARK_PARITY         0X28
#define SERIAL_SPACE_PARITY        0X38
#define SERIAL_PARITY_MASK         0X38

#define NO_PARITY				0
#define ODD_PARITY           	1
#define EVEN_PARITY          	2
#define MARK_PARITY             3
#define SPACE_PARITY      	    4

#define	STOP_BIT_1          	0
#define	STOP_BITS_1_5     	    1
#define STOP_BITS_2         	2

#define	SERIAL_1_STOP        	  0X00
#define	SERIAL_1_5_STOP     	  0X04
#define	SERIAL_2_STOP         	  0X04
#define	SERIAL_STOP_MASK		  0X04

#define	SERIAL_5_DATA             0X00
#define	SERIAL_6_DATA             0X01
#define	SERIAL_7_DATA             0X02
#define	SERIAL_8_DATA             0X03
#define	SERIAL_DATA_MASK		  0X03

#define	SERIAL_LCR_BREAK          0X40

//begin==handflow

#define SERIAL_DTR_MASK                    		( (long)0x03    )
#define SERIAL_DTR_CONTROL            			( (long)0x01    )
#define SERIAL_DTR_HANDSHAKE         			( (long)0x02    )
#define SERIAL_CTS_HANDSHAKE          			( (long)0x08    )
#define SERIAL_DSR_HANDSHAKE         			( (long)0x10    )
#define SERIAL_DCD_HANDSHAKE         			( (long)0x20    )
#define SERIAL_OUT_HANDSHAKEMASK				( (long)0x38    )

#define	SERIAL_DSR_SENSITIVITY					( (long)0x40    )
#define	SERIAL_ERROR_ABORT						( (long)0x80000000    )
#define	SERIAL_CONTROL_INVALID					( (long)0x7fffff84    )
#define	SERIAL_AUTO_TRANSMIT					( (long)0x01    )
#define	SERIAL_AUTO_RECEIVE						( (long)0x02    )

#define	SERIAL_ERROR_CHAR						( (long)0x04    )
#define	SERIAL_NULL_STRIPPING					( (long)0x08    )
#define	SERIAL_BREAK_CHAR                       ( (long)0x10    )
#define	SERIAL_RTS_MASK                         ( (long)0xc0   )
#define	SERIAL_RTS_CONTROL						( (long)0x40    )

#define	SERIAL_RTS_HANDSHAKE					( (long)0x80    )
#define	SERIAL_TRANSMIT_TOGGLE					( (long)0xc0    )
#define	SERIAL_XOFF_CONTINUE					( (long)0x80000000    )
#define	SERIAL_FLOW_INVALID                     ( (long)0x7fffff20    )


//end==handflow

#define SERIAL_MCR_DTR				0X01
#define SERIAL_MCR_RTS				0X02
#define SERIAL_MCR_OUT1				0X04
#define SERIAL_MCR_OUT2				0X08
#define SERIAL_MCR_LOOP				0X10

#define MAXULONG			( (long) 0xffffffff )

#define SERIAL_TX_CTS               ( (long) 0x01 )
#define SERIAL_TX_DSR               ( (long) 0x02 )
#define SERIAL_TX_DCD               ( (long) 0x04 )
#define SERIAL_TX_XOFF				( (long) 0x08 )
#define SERIAL_TX_BREAK				( (long) 0x10 )

#define	SERIAL_LSR_DR              0x01
#define	SERIAL_LSR_OE              0x02
#define	SERIAL_LSR_PE              0x04
#define	SERIAL_LSR_FE              0x08
#define	SERIAL_LSR_BI              0x10
#define	SERIAL_LSR_THRE            0x20
#define	SERIAL_LSR_TEMT			   0x40
#define	SERIAL_LSR_FIFOERR		   0x80

#define SERIAL_MSR_DCTS            0x01
#define SERIAL_MSR_DDSR            0x02
#define SERIAL_MSR_TERI            0x04
#define SERIAL_MSR_DDCD            0x08

#define SERIAL_MSR_CTS             0x10
#define SERIAL_MSR_DSR             0x20
#define SERIAL_MSR_RI              0x40
#define SERIAL_MSR_DCD             0x80

#define SERIAL_LSRMST_MST 		   0x03

#define SERIAL_EV_RXCHAR		   0x0001
#define SERIAL_EV_RXFLAG		   0x0002
#define SERIAL_EV_TXEMPTY		   0x0004
#define SERIAL_EV_CTS			   0x0008
#define SERIAL_EV_DSR              0x0010
#define SERIAL_EV_RLSD             0x0020
#define SERIAL_EV_BREAK            0x0040
#define SERIAL_EV_ERR              0x0080
#define SERIAL_EV_RING             0x0100
#define SERIAL_EV_PERR             0x0200
#define SERIAL_EV_RX80FULL		   0x0400
#define SERIAL_EV_EVENT1           0x0800
#define SERIAL_EV_EVENT2           0x1000

///FOR DDK RULES
#define FALSE		0
#define TRUE		1

#define	SERIAL_RX_DTR           0x01
#define	SERIAL_RX_XOFF			0x02
#define	SERIAL_RX_RTS           0x04
#define	SERIAL_RX_DSR           0x08

#define BUFFER_SIZE			1024

#define SERIAL_ERROR_BREAK              ( (unsigned long) 0x00000001  )
#define SERIAL_ERROR_FRAMING			( (unsigned long) 0x00000002  )
#define SERIAL_ERROR_OVERRUN			( (unsigned long) 0x00000004  )
#define SERIAL_ERROR_QUEUEOVERRUN		( (unsigned long) 0x00000008  )
#define SERIAL_ERROR_PARITY             ( (unsigned long) 0x00000010  )

#define SERIAL_LSRMST_ESCAPE       		( (unsigned char) 0x00  )
#define SERIAL_LSRMST_LSR_DATA          ( (unsigned char) 0x01  )
#define SERIAL_LSRMST_LSR_NODATA		( (unsigned char) 0x02  )
#define SERIAL_LSRMST_MST               ( (unsigned char) 0x03  )

#define WAKE_UP_REASON_NODATA               4
#define WAKE_UP_REASON_OK                   1
#define WAKE_UP_REASON_TIMEOUT				2
#define WAKE_UP_REASON_ERROR                3
#define WAKE_UP_REASON_CANCELED           5
#define WAKE_UP_REASON_PURGE					6
//
  // The following structure (and defines) are passed back by
  // the serial driver in response to the get properties ioctl.
  //

  #define SERIAL_SP_SERIALCOMM        ((ULONG)0x00000001)

  //
  // Provider subtypes
  //
  #define SERIAL_SP_UNSPECIFIED       ((ULONG)0x00000000)
  #define SERIAL_SP_RS232             ((ULONG)0x00000001)
  #define SERIAL_SP_PARALLEL          ((ULONG)0x00000002)
  #define SERIAL_SP_RS422             ((ULONG)0x00000003)
  #define SERIAL_SP_RS423             ((ULONG)0x00000004)
  #define SERIAL_SP_RS449             ((ULONG)0x00000005)
  #define SERIAL_SP_MODEM             ((ULONG)0X00000006)
  #define SERIAL_SP_FAX               ((ULONG)0x00000021)
  #define SERIAL_SP_SCANNER           ((ULONG)0x00000022)
  #define SERIAL_SP_BRIDGE            ((ULONG)0x00000100)
  #define SERIAL_SP_LAT               ((ULONG)0x00000101)
  #define SERIAL_SP_TELNET            ((ULONG)0x00000102)
  #define SERIAL_SP_X25               ((ULONG)0x00000103)

  //
  // Provider capabilities flags.
  //

  #define SERIAL_PCF_DTRDSR        ((ULONG)0x0001)
  #define SERIAL_PCF_RTSCTS        ((ULONG)0x0002)
  #define SERIAL_PCF_CD            ((ULONG)0x0004)
  #define SERIAL_PCF_PARITY_CHECK  ((ULONG)0x0008)
  #define SERIAL_PCF_XONXOFF       ((ULONG)0x0010)
  #define SERIAL_PCF_SETXCHAR      ((ULONG)0x0020)
  #define SERIAL_PCF_TOTALTIMEOUTS ((ULONG)0x0040)
  #define SERIAL_PCF_INTTIMEOUTS   ((ULONG)0x0080)
  #define SERIAL_PCF_SPECIALCHARS  ((ULONG)0x0100)
  #define SERIAL_PCF_16BITMODE     ((ULONG)0x0200)

  //
  // Comm provider settable parameters.
  //

  #define SERIAL_SP_PARITY         ((ULONG)0x0001)
  #define SERIAL_SP_BAUD           ((ULONG)0x0002)
  #define SERIAL_SP_DATABITS       ((ULONG)0x0004)
  #define SERIAL_SP_STOPBITS       ((ULONG)0x0008)
  #define SERIAL_SP_HANDSHAKING    ((ULONG)0x0010)
  #define SERIAL_SP_PARITY_CHECK   ((ULONG)0x0020)
  #define SERIAL_SP_CARRIER_DETECT ((ULONG)0x0040)

  //
  // Settable baud rates in the provider.
  //

  #define SERIAL_BAUD_075          ((ULONG)0x00000001)
  #define SERIAL_BAUD_110          ((ULONG)0x00000002)
  #define SERIAL_BAUD_134_5        ((ULONG)0x00000004)
  #define SERIAL_BAUD_150          ((ULONG)0x00000008)
  #define SERIAL_BAUD_300          ((ULONG)0x00000010)
  #define SERIAL_BAUD_600          ((ULONG)0x00000020)
  #define SERIAL_BAUD_1200         ((ULONG)0x00000040)
  #define SERIAL_BAUD_1800         ((ULONG)0x00000080)
  #define SERIAL_BAUD_2400         ((ULONG)0x00000100)
  #define SERIAL_BAUD_4800         ((ULONG)0x00000200)
  #define SERIAL_BAUD_7200         ((ULONG)0x00000400)
  #define SERIAL_BAUD_9600         ((ULONG)0x00000800)
  #define SERIAL_BAUD_14400        ((ULONG)0x00001000)
  #define SERIAL_BAUD_19200        ((ULONG)0x00002000)
  #define SERIAL_BAUD_38400        ((ULONG)0x00004000)
  #define SERIAL_BAUD_56K          ((ULONG)0x00008000)
  #define SERIAL_BAUD_128K         ((ULONG)0x00010000)
  #define SERIAL_BAUD_115200       ((ULONG)0x00020000)
  #define SERIAL_BAUD_57600        ((ULONG)0x00040000)
  #define SERIAL_BAUD_USER         ((ULONG)0x10000000)

  //
  // Settable Data Bits
  //

  #define SERIAL_DATABITS_5        ((USHORT)0x0001)
  #define SERIAL_DATABITS_6        ((USHORT)0x0002)
  #define SERIAL_DATABITS_7        ((USHORT)0x0004)
  #define SERIAL_DATABITS_8        ((USHORT)0x0008)
  #define SERIAL_DATABITS_16       ((USHORT)0x0010)
  #define SERIAL_DATABITS_16X      ((USHORT)0x0020)

  //
  // Settable Stop and Parity bits.
  //

  #define SERIAL_STOPBITS_10       ((USHORT)0x0001)
  #define SERIAL_STOPBITS_15       ((USHORT)0x0002)
  #define SERIAL_STOPBITS_20       ((USHORT)0x0004)
  #define SERIAL_PARITY_NONE       ((USHORT)0x0100)
  #define SERIAL_PARITY_ODD        ((USHORT)0x0200)
  #define SERIAL_PARITY_EVEN       ((USHORT)0x0400)
  #define SERIAL_PARITY_MARK       ((USHORT)0x0800)
  #define SERIAL_PARITY_SPACE      ((USHORT)0x1000)

#define SERIAL_IIR_FIFOS_ENABLED	0xc0
#define SERIAL_IIR_NO_INTERRUPT_PENDING	0x01

#define  SERIAL_IIR_RLS		0x06
#define  SERIAL_IIR_RDA	0x04
#define  SERIAL_IIR_CTI 	0x0c
#define  SERIAL_IIR_THR	0x02
#define  SERIAL_IIR_MS		0x00

#define	SERIAL_TX_WAITING_FOR_CTS		( (ULONG) 	0X00000001)
#define	SERIAL_TX_WAITING_FOR_DSR		( (ULONG) 	0X00000002)
#define	SERIAL_TX_WAITING_FOR_DCD		( (ULONG) 	0X00000004)
#define	SERIAL_TX_WAITING_FOR_XON		( (ULONG) 	0X00000008)
#define	SERIAL_TX_WAITING_XOFF_SENT		( (ULONG) 	0X00000010)
#define	SERIAL_TX_WAITING_ON_BREAK		( (ULONG) 	0X00000020)
#define	SERIAL_RX_WAITING_FOR_DSR		( (ULONG) 	0X00000040)

#define	SERIAL_DTR_STATE	( (ULONG )0X00000001 )
#define	SERIAL_RTS_STATE	( (ULONG )0X00000002 )
#define	SERIAL_CTS_STATE	( (ULONG )0X00000010 )
#define	SERIAL_DSR_STATE	( (ULONG )0X00000020 )
#define	SERIAL_RI_STATE		( (ULONG )0X00000040 )
#define	SERIAL_DCD_STATE	( (ULONG )0X00000080 )

#define	SERIAL_PURGE_TXABORT		0X00000001
#define	SERIAL_PURGE_RXABORT		0X00000002
#define	SERIAL_PURGE_TXCLEAR			0X00000004
#define	SERIAL_PURGE_RXCLEAR			0X00000008
