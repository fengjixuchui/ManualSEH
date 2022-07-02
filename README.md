# ManualSEH
A lightweight utility containing manually written SEH for manually mapped images

# Things worth noting
* Works in both usermode and kernel mode as long as you have an exception handler
* Asynchronous, thread safe performance
* Supports exceptions that occur deeper within the call stack and outside the executable
* Supports nested __TRY __EXCEPT statements

# Examples
## Regular __TRY __EXCEPT statements
```cpp
#include "ManualSEH.h"

int main( void )
{
    __TRY
    (
        *( int* )( 0 ) = 69;
    )
    __EXCEPT
    (
        printf( "Exception caught...\n" );
    )
}
```
## Nested __TRY __EXCEPT statements
```cpp
#include "ManualSEH.h"

int main( void )
{
    __TRY
    (
        __TRY
        (
            *( int* )( 0 ) = 69;
        )
        __EXCEPT
        (
            printf( "Nested exception caught...\n" );
        )
        
        *( int* )( 0 ) = 69;
    )
    __EXCEPT
    (
        printf( "Exception caught...\n" );
    )
}
```
