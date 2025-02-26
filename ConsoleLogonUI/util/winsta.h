#pragma once
#include <windows.h>
#include <winternl.h>

// winsta.dll private structs

typedef struct _ICA_COUNTERS
{
    DWORD Reserved;
} ICA_COUNTERS;

typedef struct _TSHARE_COUNTERS
{
    DWORD Reserved;
} TSHARE_COUNTERS;


typedef struct _PROTOCOLCOUNTERS
{
    DWORD WdBytes;
    DWORD WdFrames;
    DWORD WaitForOutBuf;
    DWORD Frames;
    DWORD Bytes;
    DWORD CompressedBytes;
    DWORD CompressFlushes;
    DWORD Errors;
    DWORD Timeouts;
    DWORD AsyncFramingError;
    DWORD AsyncOverrunError;
    DWORD AsyncOverflowError;
    DWORD AsyncParityError;
    DWORD TdErrors;
    WORD ProtocolType;
    WORD Length;
    union
    {
        ICA_COUNTERS IcaCounters;
        TSHARE_COUNTERS TShareCounters;
        DWORD Reserved[100];
    } Specific;
} PROTOCOLCOUNTERS;

typedef struct _THINWIRECACHE
{
    DWORD CacheReads;
    DWORD CacheHits;
} THINWIRECACHE;

typedef struct _ICA_CACHE
{
    THINWIRECACHE ThinWireCache[4];
} ICA_CACHE;

typedef struct _TSHARE_CACHE
{
    DWORD Reserved;
} TSHARE_CACHE;

struct CACHE_STATISTICS
{
    WORD ProtocolType;
    WORD Length;
    union
    {
        ICA_CACHE IcaCacheStats;
        TSHARE_CACHE TShareCacheStats;
        DWORD Reserved[20];
    } Specific;
};


typedef struct _PROTOCOLSTATUS
{
    PROTOCOLCOUNTERS Output;
    PROTOCOLCOUNTERS Input;
    CACHE_STATISTICS Cache;
    DWORD AsyncSignal;
    DWORD AsyncSignalMask;
} PROTOCOLSTATUS;

typedef enum _WINSTATIONSTATECLASS
{
    State_Active = 0x0,
    State_Connected = 0x1,
    State_ConnectQuery = 0x2,
    State_Shadow = 0x3,
    State_Disconnected = 0x4,
    State_Idle = 0x5,
    State_Listen = 0x6,
    State_Reset = 0x7,
    State_Down = 0x8,
    State_Init = 0x9,
} WINSTATIONSTATECLASS;

typedef struct _WINSTATIONINFORMATIONPRIVATEW
{
    WINSTATIONSTATECLASS ConnectState;
    WCHAR WinStationName[33];
    DWORD LogonId;
    LARGE_INTEGER ConnectTime;
    LARGE_INTEGER DisconnectTime;
    LARGE_INTEGER LastInputTime;
    LARGE_INTEGER LogonTime;
    PROTOCOLSTATUS Status;
    WCHAR Domain[18];
    WCHAR UserName[21];
    LARGE_INTEGER CurrentTime;
} WINSTATIONINFORMATIONPRIVATEW;