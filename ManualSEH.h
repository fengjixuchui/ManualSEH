/*
* @file ManualSEH.h
* 
* @brief A header that adds a manually written version of SEH for manually mapped images
*		 or images that do not support or contain any SEH data
*/

#ifndef __MANUALSEH_H__
#define __MANUALSEH_H__

//////////////////////////////////////////////
//            MANUALSEH  OPTIONS            //
//////////////////////////////////////////////
//
// Choose wether or not to use ManualSEH in kernelmode
//
#define MANUALSEH_KERNEL_MODE 0  // ( 1 or 0 )
//
// Choose wether or not to obtain exception info to be
// used in the __EXCEPT region
//
#define MANUALSEH_OBTAIN_INFO 0  // ( 1 or 0 )
//
// Maximum number of entries allowed to be allocated
// for stored context snapshots
//
#define MANUALSEH_MAX_ENTRIES 64 //
//////////////////////////////////////////////

#if MANUALSEH_KERNEL_MODE
#include <ntddk.h>
#else
#include <Windows.h>
#endif

#define __MSEH_EXIT_TRY( ) ManualSehPopEntry( ManualSehCurrentThread( ) )

/*
* @brief Starting statement of the ManualSEH __TRY scope.
*		 When an exception occurs within this scope, it will be unwound to the
*		 adjacent __EXCEPT scope.
*/
#define __TRY    if ( __MSEH_ENTER_TRY( ) ) \
				 {
/*
* @brief Statring statement of the ManualSEH __TRY scope.
*		 When an exception occurs within this scope, it will be unwound to this location.
*/
#define __EXCEPT     __MSEH_EXIT_TRY( );    \
				 }                          \
                 else

DECLSPEC_ALIGN( 2048 ) 
typedef struct _MANUALSEH_DATA
{
	CONTEXT SavedContext;
	BOOLEAN Active;
	HANDLE  ThreadID;
}MANUALSEH_DATA, *PMANUALSEH_DATA;

DECLSPEC_ALIGN( 2048 )
typedef struct _MANUALSEH_RECORD
{
	CONTEXT          ContextRecord;
	EXCEPTION_RECORD ExceptionRecord;
	HANDLE           ThreadID;
}MANUALSEH_RECORD, *PMANUALSEH_RECORD;

namespace ManualSEH
{
	//
	// Global buffer containing space for 64 total MANUALSEH_DATA objects
	//
	extern PMANUALSEH_DATA g_SEHData;

#if MANUALSEH_OBTAIN_INFO
	//
	// Global buffer containing space for 64 total MANUALSEH_RECORD objects
	//
	extern PMANUALSEH_RECORD g_SEHRecords;

	/**
	 * @brief Obtain the context record of the current exception in an __EXCEPT region
	*/
	DECLSPEC_NOINLINE
	PCONTEXT
	GetExceptionContext( 
		VOID 
		);

	/**
	 * @brief Obtain the exception record of the current exception in an __EXCEPT region
	*/
	DECLSPEC_NOINLINE
	PEXCEPTION_RECORD
	GetExceptionRecord( 
		VOID 
		);
#endif

	/**
	 * @brief Initialize ManualSEH
	*/
	DECLSPEC_NOINLINE
	BOOLEAN
	Initialize( 
		VOID 
		);

	/**
	 * @brief Shut down ManualSEH
	*/
	DECLSPEC_NOINLINE
	VOID
	Shutdown(
		VOID
		);

	/**
	 * @brief The exception handler for the ManualSEH try excepts
	 * 
	 * @param [in] ContextRecord: The context record of the exception
	 * @param [in] ExceptionRecord: The exception record of the exception
	 * 
	 * @return TRUE if the exception was recognized and handled
	 * @return FALSE if the exception was not recognized
	*/
	DECLSPEC_NOINLINE
	BOOLEAN
	ExceptionHandler( 
		IN PCONTEXT          ContextRecord, 
		IN PEXCEPTION_RECORD ExceptionRecord
		);
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
	);

EXTERN_C
{ 
	DECLSPEC_NOINLINE
	HANDLE
	ManualSehCurrentThread( 
		VOID 
		);

	DECLSPEC_NOINLINE
	BOOLEAN
	ManualSehPushEntry( 
		IN PCONTEXT ContextRecord,
		IN HANDLE   ThreadId
		);

	UINT64
	__MSEH_ENTER_TRY(
	    VOID
	    );
}

#endif // !__MANUALSEH_H__
