#define bool int
#define true 1
#define false 0
#include <stdio.h>
#include "mlib.h"
typedef unsigned long size_t ;
typedef struct {
	_MObject;
	size_t uid, page_count;
	const char * author, *publisher;
} _Book;
const char * _Book_get_author(_Book *self) {
	return self->author;
}
void _Book_set_author_const_char_ptr(_Book *self, const char *name) {
	_Book test;
	self->author= name ;
}

typedef struct {
	_Book;
	double reading_level;
} _Dictionary;

int main( int argc, char **argv) { _Book test;
_Book_set_author_const_char_ptr(&test, "Robert Frost");
printf( "%s\n" , _Book_get_author(&test)) ;
return 0 ;
} 