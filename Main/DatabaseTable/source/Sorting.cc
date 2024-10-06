
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"

using namespace std;

void mergeIntoFile ( MyDB_TableReaderWriter &sortIntoMe, vector <MyDB_RecordIteratorAltPtr> &mergeUs,
                     function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs ) {

}

vector <MyDB_PageReaderWriter> mergeIntoList ( MyDB_BufferManagerPtr parent, MyDB_RecordIteratorAltPtr leftIter,
        MyDB_RecordIteratorAltPtr rightIter, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs ) { 
		return vector <MyDB_PageReaderWriter> (); 
} 
	
void sort ( int runSize, MyDB_TableReaderWriter &sortMe, MyDB_TableReaderWriter &sortIntoMe,
			function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs ) {

} 

#endif
