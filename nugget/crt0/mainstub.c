#include <stddef.h>

typedef void ( *fptr )();

extern fptr __preinit_array_start[] __attribute__( ( weak ) );
extern fptr __preinit_array_end[] __attribute__( ( weak ) );
extern fptr __init_array_start[] __attribute__( ( weak ) );
extern fptr __init_array_end[] __attribute__( ( weak ) );

void main();

void mainstub() {
    size_t count, i;

    count = __preinit_array_end - __preinit_array_start;
    for( i = 0; i < count; i++ ) {
        fptr f = __preinit_array_start[ i ];
        if( f ) {
            f();
        }
    }

    count = __init_array_end - __init_array_start;
    for( i = 0; i < count; i++ ) {
        fptr f = __init_array_start[ i ];
        if( f ) {
            f();
        }
    }

    main();
}

void abort() {
    // TODO: make this better
    while( 1 )
        ;
}

void __cxa_pure_virtual() { abort(); }
