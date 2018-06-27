#ifndef __SCANNER_H__
#define __SCANNER_H__





#define SC_OUT_SIZE   512
#define STRING_TAG    'SCst'
#define SCDATA_TAG    'SCdt'
#define CURRENT_PROCESS_TAG 'cpnt'


typedef struct _OUT_DATA
{
	UCHAR f_path[SC_OUT_SIZE];
	UCHAR f_pross[SC_OUT_SIZE];
	UCHAR act[SC_OUT_SIZE];

} OUT_DATA, *POUT_DATA;

typedef struct _FS_SCANNER_DATA {

    PDRIVER_OBJECT DriverObject;
    PFLT_FILTER Filter;
    PFLT_PORT ServerPort;
    PEPROCESS UserProcess;
    PFLT_PORT ClientPort;


} FS_SCANNER_DATA, *PFS_SCANNER_DATA;


extern FS_SCANNER_DATA ScannerData;

const PWSTR ScannerPortName = L"\\ScannerPort";



//////////////////////////
//
// Прототипы функций
//
//////////////////////////


DRIVER_INITIALIZE DriverEntry;

NTSTATUS DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );
    
NTSTATUS 
ScUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );
    
FLT_POSTOP_CALLBACK_STATUS ScPostCreate ( IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags);
    
FLT_PREOP_CALLBACK_STATUS ScPreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );
    
NTSTATUS ScInSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );    
    
NTSTATUS ScQuDown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );
    
    
    
    
#endif
