/*
* @file ManualSEH.cpp
*
* @brief All functionality required for ManualSEH 
*/

#include "ManualSEH.h"

#define	MANUALSEH_START_TRY_MAGIC 0xDEADBEEF000005E1
#define	MANUALSEH_END_TRY_MAGIC   0xDEADBEEF000005E2

#if MANUALSEH_KERNEL_MODE
#define ManualSehAlloc( Len )  ExAllocatePool( NonPagedPool, Len )
#define ManualSehFree( Block ) ExFreePool    ( Block )
#else
#define ManualSehAlloc( Len )  VirtualAlloc( NULL, Len, MEM_COMMIT, PAGE_READWRITE )
#define ManualSehFree( Block ) VirtualFree ( Block, NULL, MEM_RELEASE )
#endif

PMANUALSEH_DATA   ManualSEH::g_SEHData    = NULL;
#if MANUALSEH_OBTAIN_INFO
PMANUALSEH_RECORD ManualSEH::g_SEHRecords = NULL;
#endif

/*
* @brief Obtain the current thread Id
*/
DECLSPEC_NOINLINE
HANDLE
ManualSehCurrentThread( 
	VOID 
	)
{
#if MANUALSEH_KERNEL_MODE
	return PsGetCurrentThreadId( );
#else
	return ( HANDLE )GetCurrentThreadId( );
#endif
}

//
// Spinlock to prevent very rare race conditions
//
BOOLEAN g_ManualSehPushEntry_Lock = FALSE;
/*
* @brief Takes a snapshot of the current context and pushes it to g_SEHData
*		 along with a thread identifier and an active status
* 
* @param [in] ContextRecord: The current context record
* @param [in] ThreadId: The current thread identifier
* 
* @return TRUE if there was an available slot and the entry was pushed
* @return FALSE if there were no available slots to push the entry to
*/
DECLSPEC_NOINLINE
BOOLEAN
ManualSehPushEntry( 
	IN PCONTEXT ContextRecord,
	IN HANDLE   ThreadId
	)
{
	BOOLEAN Result = FALSE;

	if ( ManualSEH::g_SEHData == NULL ) {
		return Result;
	}
  
	//
	// Acquire the spinlock 
	//
	while ( _InterlockedExchange8( ( CHAR* )&g_ManualSehPushEntry_Lock, TRUE ) == TRUE )
		;

	for ( UINT32 i = NULL; i < MANUALSEH_MAX_ENTRIES; i++ )
	{
		PMANUALSEH_DATA CurrentEntry = &ManualSEH::g_SEHData[ i ];

		//
		// If the current entry is already occupied by an active entry,
		// continue searching for the next available slot
		//
		if ( CurrentEntry->Active == FALSE ) 
		{
			Result = TRUE;

			//
			// Save the current context snapshot in the available entry
			//
			RtlCopyMemory( &CurrentEntry->SavedContext, ContextRecord, sizeof( CONTEXT ) );

			CurrentEntry->Active   = TRUE;
			CurrentEntry->ThreadID = ThreadId;

			break;
		}
	}

	//
	// Release the spinlock
	//
	g_ManualSehPushEntry_Lock = FALSE;

	return Result;
}

/*
* @brief Obtain the last entry belonging to the given thread id that was pushed
* 
* @param [in] ThreadId: The current thread identifier
* 
* @return TRUE if the entry belonging to the thread id exists and was obtained
* @return FALSE if the entry belonging to the thread id does not exist
*/
DECLSPEC_NOINLINE
PMANUALSEH_DATA
ManualSehGetCurrentEntry(
	IN HANDLE ThreadId 
	)
{
	if ( ManualSEH::g_SEHData == NULL ) {
		return FALSE;
	}

	//
	// Reverse iterate through the entries to obtain the latest entry
	//
	for ( UINT32 i = MANUALSEH_MAX_ENTRIES; i > NULL; i-- )
	{
		PMANUALSEH_DATA CurrentEntry = &ManualSEH::g_SEHData[ i - 1 ];

		if  ( CurrentEntry->Active   == TRUE &&
			  CurrentEntry->ThreadID == ThreadId ) 
		{ 
			return CurrentEntry;
		}
	}

	return NULL;
}

/*
* @brief Pop the latest entry belonging to a given thread id back off the list
* 
* @param [in] ThreadId: The current thread identifier
* 
* @return TRUE if the latest entry was found and removed off the list
* @return FALSE if there was no entry belonging to the thread id in the first place
*/
DECLSPEC_NOINLINE
BOOLEAN
ManualSehPopEntry(
	IN HANDLE ThreadId
	)
{
	if ( ManualSEH::g_SEHData == NULL ) {
		return FALSE;
	}

	//
	// Reverse iterate through the entries to obtain the latest entry 
	// and pop it back off the list
	//
	for ( UINT32 i = MANUALSEH_MAX_ENTRIES; i > NULL; i-- )
	{
		PMANUALSEH_DATA CurrentEntry = &ManualSEH::g_SEHData[ i - 1 ];

		if  ( CurrentEntry->Active   == TRUE &&
			  CurrentEntry->ThreadID == ThreadId ) 
		{ 
			CurrentEntry->Active = FALSE;

			return TRUE;
		}
	}

	return FALSE;
}

#if MANUALSEH_OBTAIN_INFO
//
// Spinlock to prevent very rare race conditions
//
BOOLEAN g_ManualSehPushRecord_Lock = FALSE;
/*
* @brief Takes the current context and exception record pushes it to g_SEHRecords
*		 along with a thread identifier and an active status
* 
* @param [in] ContextRecord: The current context record
* @param [in] ExceptionRecord: The current exception record
* 
* @return TRUE if there was an available slot and the record was pushed
* @return FALSE if there were no available slots to push the record to
*/
DECLSPEC_NOINLINE
BOOLEAN
ManualSehPushRecord( 
	IN PCONTEXT          ContextRecord,
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN HANDLE            ThreadId
	)
{
	BOOLEAN Result = FALSE;

	if ( ManualSEH::g_SEHRecords == NULL )
	{
		return Result;
	}

	//
	// Acquire the spinlock
	//
	while ( _InterlockedExchange8( ( CHAR* )&g_ManualSehPushRecord_Lock, TRUE ) == TRUE )
		;

	for ( UINT32 i = NULL; i < MANUALSEH_MAX_ENTRIES; i++ )
	{
		PMANUALSEH_RECORD CurrentRecord = &ManualSEH::g_SEHRecords[ i ];

		if ( ( CurrentRecord->ThreadID == ThreadId ) ||
		       CurrentRecord->ThreadID == NULL )
		{
			Result = TRUE;

			RtlCopyMemory( &CurrentRecord->ContextRecord, ContextRecord, sizeof( CONTEXT ) );
			RtlCopyMemory( &CurrentRecord->ExceptionRecord, ExceptionRecord, sizeof( EXCEPTION_RECORD ) );

			CurrentRecord->ThreadID = ThreadId;

			break;
		}
	}

	//
	// Release the spinlock
	//
	g_ManualSehPushRecord_Lock = FALSE;

	return Result;
}

/*
* @brief Obtain the last info record belonging to the given thread id that was pushed
* 
* @param [in] ThreadId: The current thread identifier
* 
* @return TRUE if the record belonging to the thread id exists and was obtained
* @return FALSE if the record belonging to the thread id does not exist
*/
DECLSPEC_NOINLINE
PMANUALSEH_RECORD
ManualSehGetCurrentRecord( 
	IN HANDLE ThreadId 
	)
{
	if ( ManualSEH::g_SEHRecords == NULL ) {
		return NULL;
	}

	for ( UINT32 i = NULL; i < MANUALSEH_MAX_ENTRIES; i++ )
	{
		PMANUALSEH_RECORD CurrentEntry = &ManualSEH::g_SEHRecords[ i ];

		if ( CurrentEntry->ThreadID == ThreadId ) {
			return CurrentEntry;
		}
	}

	return NULL;
}

DECLSPEC_NOINLINE
PCONTEXT
ManualSEH::GetContextRecord( 
	VOID 
	)
{
	PMANUALSEH_RECORD CurrentRecord = ManualSehGetCurrentRecord( ManualSehCurrentThread( ) );

	if ( CurrentRecord != NULL ) {
		return &CurrentRecord->ContextRecord;
	}

	return NULL;
}

DECLSPEC_NOINLINE
PEXCEPTION_RECORD
ManualSEH::GetExceptionRecord(
	VOID
	)
{	
	PMANUALSEH_RECORD CurrentRecord = ManualSehGetCurrentRecord( ManualSehCurrentThread( ) );

	if ( CurrentRecord != NULL ) {
		return &CurrentRecord->ExceptionRecord;
	}

	return NULL;
}

DECLSPEC_NOINLINE
UINT32
ManualSEH::GetCode( 
	VOID 
	)
{
	PMANUALSEH_RECORD CurrentRecord = ManualSehGetCurrentRecord( ManualSehCurrentThread( ) );

	if ( CurrentRecord != NULL ) {
		return CurrentRecord->ExceptionRecord.ExceptionCode;
	}

	return NULL;
}
#endif

DECLSPEC_NOINLINE
BOOLEAN
ManualSEH::ExceptionHandler(
	IN PCONTEXT          ContextRecord,
	IN PEXCEPTION_RECORD ExceptionRecord
	)
{
	if ( g_SEHData == NULL ) {
		return FALSE;
	}

	//
	// Attempt to obtain the latest entry in the list
	//
	PMANUALSEH_DATA CurrentEntry = ManualSehGetCurrentEntry( ManualSehCurrentThread( ) );

	//
	// If the entry is active and populated, we're inside a __TRY region
	// and we need to handle the exception appropriately
	//
	if ( CurrentEntry != NULL )
	{
#if MANUALSEH_OBTAIN_INFO
		//
		// Push the current context record and exception record to the record buffer
		// so it can be obtained in a __TRY region
		//
		ManualSehPushRecord( ContextRecord, ExceptionRecord, ManualSehCurrentThread( ) );
#endif

		//
		// Reset the context back to it's unwound state at the __TRY region
		//
		RtlCopyMemory( ContextRecord, &CurrentEntry->SavedContext, sizeof( CONTEXT ) );

		//
		// Pop the latest entry off the list since we've reset the context and
		// the entry is no longer needed
		//
		ManualSehPopEntry( ManualSehCurrentThread( ) );

		//
		// Since context has been unwound back to the return address of the init __TRY function,
		// we set the return value to 0 to indicate failure. This will make it jump to the __EXCEPT region
		//
		ContextRecord->Rax = FALSE;

		return TRUE;
	}

	return FALSE;
}

DECLSPEC_NOINLINE
BOOLEAN
ManualSEH::Initialize( 
	VOID 
	)
{
	CONST UINT32 AllocLength = MANUALSEH_MAX_ENTRIES * sizeof( MANUALSEH_DATA );

	g_SEHData = ( PMANUALSEH_DATA )ManualSehAlloc( AllocLength );

	if ( g_SEHData == NULL ) {
		return FALSE;
	}

	RtlZeroMemory( g_SEHData, AllocLength );

#if MANUALSEH_OBTAIN_INFO
	g_SEHRecords = ( PMANUALSEH_RECORD )ManualSehAlloc( AllocLength );

	if ( g_SEHRecords == NULL ) {
		return FALSE;
	}

	RtlZeroMemory( g_SEHRecords, AllocLength );
#endif

	return TRUE;
}

DECLSPEC_NOINLINE
VOID
ManualSEH::Shutdown(
	VOID
	)
{
	ManualSehFree( g_SEHData );
	g_SEHData = NULL;
#if MANUALSEH_OBTAIN_INFO
	ManualSehFree( g_SEHRecords );
	g_SEHRecords = NULL;
#endif
}