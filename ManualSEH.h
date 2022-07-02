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
#define MANUALSEH_KERNEL_MODE 0 // ( 1 or 0 )
//////////////////////////////////////////////

#if MANUALSEH_KERNEL_MODE
#include <ntddk.h>
#else
#include <Windows.h>
#endif

/*
* @brief Starting statement of the ManualSEH __TRY scope.
*		 When an exception occurs within this scope, it will be unwound to the
*		 adjacent __EXCEPT scope.
*/
#define __TRY( _ )    if ( __MSEH_ENTER_TRY( ) ) \
                      {                          \
                            _                    \
                            __MSEH_EXIT_TRY( );  \
                      }
/*
* @brief Statring statement of the ManualSEH __TRY scope.
*		 When an exception occurs within this scope, it will be unwound to this location.
*/
#define __EXCEPT( _ ) else                       \
                      {                          \
                            _                    \
                      }



DECLSPEC_ALIGN( 2048 ) 
typedef struct _MANUALSEH_DATA
{
	BOOLEAN Active;
	HANDLE  ThreadID;
	CONTEXT SavedContext;
}MANUALSEH_DATA, *PMANUALSEH_DATA;

namespace ManualSEH
{
	//
	// Global buffer containing space for 20 total MANUALSEH_DATA objects
	//
	extern PMANUALSEH_DATA g_SEHData;

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

EXTERN_C
{ 
	UINT64
	__MSEH_ENTER_TRY(
	    VOID
	    );
	
	VOID
	__MSEH_EXIT_TRY(
	    VOID
	    );
}

#endif // !__MANUALSEH_H__
