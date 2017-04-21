#ifndef TIMER_OPENCLOSE 

#define TIMER_OPENCLOSE

NTSTATUS CreateOpen(	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

NTSTATUS Close( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

VOID ShowIRP( IN PIRP Irp );

VOID ShowCurrentStack( IN PIRP Irp );


#endif 
