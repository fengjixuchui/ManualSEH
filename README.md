# ManualSEH
A lightweight utility containing manually written SEH for manually mapped images or images that do not contain any exception data

# Things worth noting
* Works in both usermode and kernelmode as long as you have an exception handler
* Asynchronous, thread safe performance
* Supports exceptions that occur deeper within the call stack and outside the executable
* Supports both x86-64 and x86-32
* Supports nested __TRY __EXCEPT statements
* Supports obtaining both the exception record and the context record of the exception inside a __EXCEPT region

# Starting the ManualSEH API
Starting the API is fairly straightforward, simply use `ManualSEH::Initialize` to initialize everything
and make sure `ManualSEH::ExceptionHandler` is running inside a working exception handler.

# Examples
### Regular __TRY __EXCEPT statements
```cpp
#include "ManualSEH.h"

int main( void )
{
    __TRY
    {
        *( int* )( 0 ) = 69;
    }
    __EXCEPT
    {
        printf( "Exception caught...\n" );
    }
}
```
### Nested __TRY __EXCEPT statements
```cpp
#include "ManualSEH.h"

int main( void )
{
    __TRY
    {
        __TRY
        {
            *( int* )( 0 ) = 69;
        }
        __EXCEPT
        {
            printf( "Nested exception caught...\n" );
        }
        
        *( int* )( 0 ) = 69;
    }
    __EXCEPT
    {
        printf( "Exception caught...\n" );
    }
}
```
### Obtaining additional information about the exception
##### To obtain information belonging to the most recent exception simply use one of the following API functions:
1) `ManualSEH::GetExceptionRecord( );`
2) `ManualSEH::GetContextRecord( );`
3) `ManualSEH::GetCode( );`
```cpp
#include "ManualSEH.h"

int main( void )
{
    __TRY
    {
        *( int* )( 0 ) = 69;
    }
    __EXCEPT
    {
        PEXCEPTION_RECORD ExceptionRecord = ManualSEH::GetExceptionRecord( );
        
        printf( "Exception caught... (Exception code: %08X)\n", ExceptionRecord->ExceptionCode );
    }
}
```
