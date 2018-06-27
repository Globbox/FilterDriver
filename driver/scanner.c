#include <fltKernel.h>
//#include <dontuse.h>
#include <suppress.h>
//#include <ntstrsafe.h>
#include "scanner.h"

FS_SCANNER_DATA ScannerData;


typedef NTSTATUS (*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

QUERY_INFO_PROCESS ZwQueryInformationProcess;
NTSTATUS GetProcessImageName(PUNICODE_STRING ProcessImageName);

NTSTATUS ScSendMessage (IN PFILE_OBJECT FileObject,
      IN PFLT_CALLBACK_DATA Data
    );


NTSTATUS ScPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID ScPortDisconnect (
    __in_opt PVOID ConnectionCookie
    );
    

      
	
      
VOID UstrToUchar(UNICODE_STRING A, UCHAR * Uch);
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, ScUnload)
    #pragma alloc_text(PAGE, ScPortConnect)
    #pragma alloc_text(PAGE, ScPortDisconnect)
    #pragma alloc_text(PAGE, ScInSetup)    
    #pragma alloc_text(PAGE, ScQuDown)
    #pragma alloc_text(PAGE, ScPreCreate)
    #pragma alloc_text(PAGE, ScSendMessage)
    #pragma alloc_text(PAGE, GetProcessImageName)
    #pragma alloc_text(PAGE, ScSendMessage)
#endif


const FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
     0,
      ScPreCreate,
      ScPostCreate},
      {IRP_MJ_SET_INFORMATION,
      0,
      ScPreCreate,
      ScPostCreate},


    { IRP_MJ_OPERATION_END}
};


const FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    NULL,                //  Context Registration.
    Callbacks,                          //  Operation callbacks
    ScUnload,                      //  FilterUnload
    ScInSetup,               //  InstanceSetup
    ScQuDown,               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};




NTSTATUS DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
    {
    	OBJECT_ATTRIBUTES oa;
    	UNICODE_STRING uniString;
    	PSECURITY_DESCRIPTOR sd;
    	NTSTATUS status;
		UNREFERENCED_PARAMETER( RegistryPath );
		
		status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &ScannerData.Filter );
                                
        if (!NT_SUCCESS( status ))
        {
        	DbgPrint("Filter not registered\n");
        	return status;
        }
		RtlInitUnicodeString( &uniString, ScannerPortName );
        
    	status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );
    	
    	if (NT_SUCCESS( status )) {

        InitializeObjectAttributes( &oa,
                                    &uniString,
                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    sd );

        status = FltCreateCommunicationPort( ScannerData.Filter,
                                             &ScannerData.ServerPort,
                                             &oa,
                                             NULL,
                                             ScPortConnect,
                                             ScPortDisconnect,
                                             NULL,
                                             1 );
 

        FltFreeSecurityDescriptor( sd );

        if (NT_SUCCESS( status )) {

       

            status = FltStartFiltering( ScannerData.Filter );

            if (NT_SUCCESS( status )) {
            	DbgPrint("Filter registered. Start filtering I/O.\n");
            
                return STATUS_SUCCESS;
            }

            FltCloseCommunicationPort( ScannerData.ServerPort );
        }
    }
	
    FltUnregisterFilter( ScannerData.Filter );
    DbgPrint("Filter not registered\n");
    return status;
        
			
    }

NTSTATUS ScUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
    {
    	UNREFERENCED_PARAMETER( Flags );
    	 FltCloseCommunicationPort( ScannerData.ServerPort );
    	  FltUnregisterFilter( ScannerData.Filter );
    	  DbgPrint("Filter unregistered\n");
    	  return STATUS_SUCCESS;
    	
    }
    
    
 NTSTATUS ScInSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )   
    {
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    ASSERT( FltObjects->Filter == ScannerData.Filter );


    if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}



NTSTATUS ScQuDown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )    {
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}


FLT_PREOP_CALLBACK_STATUS ScPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
    {
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext = NULL );

    PAGED_CODE();



    if (IoThreadToProcess( Data->Thread ) == ScannerData.UserProcess) {

        DbgPrint( "!!! scanner.sys -- allowing create for trusted process\n" );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
 	
    	
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }
    
FLT_POSTOP_CALLBACK_STATUS ScPostCreate (IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags
    ){
    
  
    	
    	if (FltObjects->FileObject != NULL && Data != NULL )
    	{
    			if (!NT_SUCCESS( Data->IoStatus.Status ) ) {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }
    
    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE )
    {
    	 (VOID)ScSendMessage(FltObjects->FileObject,Data);
    	 
    }else if 	( Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION) 
    {
    	 (VOID)ScSendMessage(FltObjects->FileObject,Data);
    }
   
}
    	
		
		
	
    		
   

	
		
	
		
    

    	return FLT_POSTOP_FINISHED_PROCESSING;
}
    
    
    
    
NTSTATUS ScPortConnect (
    __in PFLT_PORT ClientPort,
    __in_opt PVOID ServerPortCookie,
    __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
    {
    	PAGED_CODE();

    UNREFERENCED_PARAMETER( ServerPortCookie );
    UNREFERENCED_PARAMETER( ConnectionContext );
    UNREFERENCED_PARAMETER( SizeOfContext);
    UNREFERENCED_PARAMETER( ConnectionCookie = NULL );

    ASSERT( ScannerData.ClientPort == NULL );
    ASSERT( ScannerData.UserProcess == NULL );
    ScannerData.UserProcess = PsGetCurrentProcess();
    ScannerData.ClientPort = ClientPort;
    DbgPrint( "!!! scanner.sys --- connected, port=0x%p\n", ClientPort );
    return STATUS_SUCCESS;
    }
    
VOID ScPortDisconnect (
    __in_opt PVOID ConnectionCookie
    )   
    {
    UNREFERENCED_PARAMETER( ConnectionCookie );
    PAGED_CODE();
    DbgPrint( "!!! scanner.sys --- disconnected, port=0x%p\n", ScannerData.ClientPort );
    FltCloseClientPort( ScannerData.Filter, &ScannerData.ClientPort );
    ScannerData.UserProcess = NULL;
    }
    
    



 NTSTATUS ScSendMessage (
     IN PFILE_OBJECT FileObject,
      IN PFLT_CALLBACK_DATA Data
    )
   
   { POUT_DATA tmp = NULL;
   	ULONG createDisposition;
   	UNICODE_STRING processName;
   
   	BOOLEAN del;
 	BOOLEAN  isNewFile;
 
	if (ScannerData.ClientPort == NULL) {

        return STATUS_SUCCESS;
    }
		
		try {
			
			
				tmp = ExAllocatePoolWithTag( NonPagedPool,
                                              sizeof( OUT_DATA ),
                                              SCDATA_TAG );

            if (tmp == NULL)
            {
            	DbgPrint("no memory!!!");
                return STATUS_SUCCESS;
            }
            
			UstrToUchar(FileObject->FileName,tmp->f_path);
 					processName.Length = 0;
					processName.MaximumLength = 254* sizeof(WCHAR);
					processName.Buffer = ExAllocatePoolWithTag(NonPagedPool, processName.MaximumLength,CURRENT_PROCESS_TAG);
					RtlZeroMemory(processName.Buffer, processName.MaximumLength);
				if (NT_SUCCESS(GetProcessImageName(&processName)))	
				{
					UstrToUchar(processName,tmp->f_pross);
				}
			
	
			if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
			{
isNewFile = FALSE;
  createDisposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0x000000FF;
isNewFile = ((FILE_SUPERSEDE == createDisposition)
                || (FILE_CREATE == createDisposition)
                || (FILE_OPEN_IF == createDisposition)
                || (FILE_OVERWRITE == createDisposition)
                || (FILE_OVERWRITE_IF == createDisposition));

  if (isNewFile) {
switch (createDisposition)
   {
   	
   	case 0x00000000:
   		strcpy(tmp->act,"File supersede");
   		goto _send_mess;
   		case 0x00000002:
   		strcpy(tmp->act,"File create");
   	goto _send_mess;
   		case 0x00000003:
   		strcpy(tmp->act,"File open if");
   		goto _send_mess;
   		case 0x00000004:
   		strcpy(tmp->act,"File overwrite");
   		goto _send_mess;
   		case 0x00000005:
   		strcpy(tmp->act,"File overwrite if");
   		goto _send_mess;
   	default:
			goto _end;	
   }
}
goto _end;
				
			}else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
			{
				switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
			{
				case FileRenameInformation:
					strcpy(tmp->act,"File Rename Information\n");
					goto _send_mess;
				case FileDispositionInformation:
				del = ((PFILE_DISPOSITION_INFORMATION) Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile;
				if (del)
				{
					strcpy(tmp->act,"File delete\n");
					goto _send_mess;
				}
				goto _end;
				default:
				goto _end;	
			}
			}
goto _end;			
_send_mess:
		
			
            FltSendMessage( ScannerData.Filter,
                                     &ScannerData.ClientPort,
                                     tmp,
                                     sizeof(OUT_DATA),
                                     NULL,
                                     NULL,
                                     NULL );
_end:
            
             ExFreePoolWithTag( tmp, SCDATA_TAG );
             ExFreePoolWithTag(processName.Buffer,CURRENT_PROCESS_TAG);
           
		}except( EXCEPTION_EXECUTE_HANDLER ) {

                DbgPrint("Error memory!!!");
                return STATUS_SUCCESS;
            }
        return STATUS_SUCCESS;
	}   
	
VOID UstrToUchar(UNICODE_STRING A, UCHAR * Uch)
{
	ANSI_STRING  strVname;
	if(STATUS_SUCCESS == RtlUnicodeStringToAnsiString(&strVname,&A,TRUE))
    {
     strcpy(Uch,strVname.Buffer);
        RtlFreeAnsiString(&strVname);
       
    }  
	return;
}
	

NTSTATUS GetProcessImageName(PUNICODE_STRING ProcessImageName)
{
    NTSTATUS status;
    ULONG returnedLength;
    ULONG bufferLength;
    PVOID buffer;
    PUNICODE_STRING imageName;
    if (NULL == ZwQueryInformationProcess) {

        UNICODE_STRING routineName;

        RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

        ZwQueryInformationProcess =
               (QUERY_INFO_PROCESS) MmGetSystemRoutineAddress(&routineName);

        if (NULL == ZwQueryInformationProcess) {
            DbgPrint("Cannot resolve ZwQueryInformationProcess\n");
        }
    }
    status = ZwQueryInformationProcess( NtCurrentProcess(),
                                        ProcessImageFileName,
                                        NULL, 
                                        0, 
                                        &returnedLength);

    if (STATUS_INFO_LENGTH_MISMATCH != status) {

        return status;

    }
    bufferLength = returnedLength - sizeof(UNICODE_STRING);
   
    if (ProcessImageName->MaximumLength < bufferLength) {

        ProcessImageName->Length = (USHORT) bufferLength;

        return STATUS_BUFFER_OVERFLOW;
       
    }
    buffer = ExAllocatePoolWithTag(PagedPool, returnedLength, 'ipgD');
    if (NULL == buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
       
    }
    status = ZwQueryInformationProcess( NtCurrentProcess(),
                                        ProcessImageFileName,
                                        buffer,
                                        returnedLength,
                                        &returnedLength);

    if (NT_SUCCESS(status)) {
        imageName = (PUNICODE_STRING) buffer;
        RtlCopyUnicodeString(ProcessImageName, imageName);
    }
    ExFreePool(buffer);
    return status;
   
}



