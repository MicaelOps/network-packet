#include "ntddk.h"
#include <fwpmk.h>
#include <fwpsk.h>
#define INITGUID
#include <guiddef.h>
#include <fwpmu.h>


DEFINE_GUID(CALLOUT_ESTABLISHED_GUID,
	0x05102c55,
	0x769b,
	0x41c6,
	0x97, 0x4c, 0xc7, 0x1c, 0x64, 0x65, 0xda, 0x9e);
DEFINE_GUID(SUBLAYER_ESTABLISHED_GUID,
	0x2efe751b,
	0xc7d7,
	0x4c10,
	0xb9, 0x7e, 0xfc, 0xcd, 0xc3, 0xb9, 0xa7, 0x66);

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\wfppacketransmitter");

UNICODE_STRING SymbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\wfppacketransmitterlink");

PDEVICE_OBJECT DeviceObject = NULL;
HANDLE EngineHandle = NULL;

UINT32 RegCalloutId = 0, AddCalloutId;
UINT64 filterId = 0;
UCHAR* buffer[2010];
VOID Unload(PDRIVER_OBJECT DriverObject) {


	IoDeleteSymbolicLink(&SymbolicLinkName);

	if (EngineHandle != NULL) {
		if (filterId != 0) {
			FwpmFilterDeleteById(EngineHandle, filterId);
			FwpmSubLayerDeleteByKey(EngineHandle, &SUBLAYER_ESTABLISHED_GUID);
		}
		
		if (AddCalloutId != 0) 
			FwpmCalloutDeleteById(EngineHandle, AddCalloutId);
		

		if (RegCalloutId != 0) {
			FwpsCalloutUnregisterById(RegCalloutId);
		}
		FwpmEngineClose(EngineHandle);
	}
	IoDeleteDevice(DeviceObject);
	DbgPrint("unloading ");

}

NTSTATUS NotifyCallback(const FWPS_CALLOUT_NOTIFY_TYPE type, const GUID* filterkey, const FWPS_FILTER* filter) {

	return STATUS_SUCCESS;
}
VOID FlowDeleteCallBack(UINT layerid, UINT32 calloudId, UINT64 flowcontext) {

}
VOID FilterCallback(const FWPS_INCOMING_VALUE0* Value, const FWPS_INCOMING_METADATA_VALUES0 Metadata, PVOID layerdata, PVOID context, const FWPS_FILTER* filter, const UINT64 flowcontext, FWPS_CLASSIFY_OUT0* classifyout) {
	DbgPrint("Data is here \n");
	FWPS_STREAM_CALLOUT_IO_PACKET* packet;
	FWPS_STREAM_DATA* streamdata;

	UCHAR data[201] = { 0 };

	ULONG length = 0;
	SIZE_T bytes;

	packet = (FWPS_STREAM_CALLOUT_IO_PACKET*)layerdata;
	streamdata = packet->streamData;

	
	RtlZeroMemory(classifyout, sizeof(FWPS_CLASSIFY_OUT));

	// Inbound packets only

	if ((streamdata->flags & FWPS_STREAM_FLAG_RECEIVE)) {

		// certain packet length for test
		length = streamdata->dataLength <= 200 ? streamdata->dataLength : 200;
		FwpsCopyStreamDataToBuffer(streamdata, data, length, &bytes);
	}

	packet->streamAction = FWPS_STREAM_ACTION_NONE;
	classifyout->actionType = FWP_ACTION_PERMIT; // allow connection
	
	if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT) {
		classifyout->actionType &= FWPS_RIGHT_ACTION_WRITE;
	}
}

NTSTATUS AddWFPCallout() {
	FWPM_CALLOUT callout = { 0 };
	callout.flags = 0;
	callout.displayData.name = L"CalloutEstablished";
	callout.displayData.description = L"CalloutEstablished";
	callout.calloutKey = CALLOUT_ESTABLISHED_GUID;
	callout.applicableLayer = FWPM_LAYER_STREAM_V4;
	FwpmCalloutAdd(&EngineHandle, &callout, NULL, &AddCalloutId);
	return STATUS_SUCCESS;
}


NTSTATUS AddWFPSublayer() {
	FWPM_SUBLAYER sublayer = { 0 };

	sublayer.displayData.name = L"SublayerEstablished";
	sublayer.displayData.description= L"SublayerEstablished";
	sublayer.subLayerKey = SUBLAYER_ESTABLISHED_GUID;
	sublayer.weight = 65000;

	FwpmSubLayerAdd(EngineHandle, &sublayer, NULL);
	return STATUS_SUCCESS;
}
NTSTATUS WFPAddFilter() {

	FWPM_FILTER filter = { 0 };
	FWPM_FILTER_CONDITION condition[1] = { 0 };

	filter.displayData.name = L"FilterCalloutEstablished";
	filter.displayData.description = L"FilterCalloutEstablished";;
	filter.layerKey = FWPM_LAYER_STREAM_V4;
	filter.subLayerKey = SUBLAYER_ESTABLISHED_GUID;
	filter.weight.type = FWP_EMPTY;
	filter.numFilterConditions = 1;
	filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
	filter.action.calloutKey = CALLOUT_ESTABLISHED_GUID;
	filter.filterCondition = condition;

	condition[0].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
	condition[0].matchType = FWP_MATCH_LESS_OR_EQUAL;
	condition[0].conditionValue.type = FWP_UINT16;
	condition[0].conditionValue.uint16 = 65000;
	FwpmFilterAdd(EngineHandle, &filter, NULL, &filterId);
}

NTSTATUS startFilteringEngine() {

	if (!NT_SUCCESS(FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &EngineHandle))) {

	}
	FWPS_CALLOUT callout = {0};


	callout.calloutKey = CALLOUT_ESTABLISHED_GUID;
	callout.flags = 0;
	callout.classifyFn = FilterCallback;
	callout.notifyFn = NotifyCallback;
	callout.flowDeleteFn = FlowDeleteCallBack;
	FwpsCalloutRegister(DeviceObject, &callout, &RegCalloutId);

	
	return STATUS_SUCCESS;
}

NTSTATUS DispatchRoutine(PDEVICE_OBJECT DeviceObject, PIRP irp) {

	PIO_STACK_LOCATION irpstackloc = IoGetCurrentIrpStackLocation(irp);

	ULONG returnlength = 0;

	NTSTATUS status = STATUS_SUCCESS;

	switch (irpstackloc->MajorFunction) {

	case IRP_MJ_READ:
		DbgPrint("Starting Packet Gathering..");

	default:
		break;
	}
	irp->IoStatus.Information = returnlength;
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {

	DbgPrint("loaading  ");
	NTSTATUS status;

	DriverObject->DriverUnload = Unload;

	status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status))
		return status;

	status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(DeviceObject);
		return;
	}

	status = startFilteringEngine();

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(DeviceObject);
		return;
	}


	status = AddWFPCallout();

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(DeviceObject);
		return;
	}

	status = AddWFPSublayer();

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(DeviceObject);
		return;
	}

	status = WFPAddFilter();

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(DeviceObject);
		return;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchRoutine;
	}
	
	return status;
}