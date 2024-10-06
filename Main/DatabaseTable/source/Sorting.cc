
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"
#include <vector>

using namespace std;

// void mergeIntoFile(MyDB_TableReaderWriter &sortIntoMe, vector<MyDB_RecordIteratorAltPtr> &mergeUs,
// 				   function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
// }
void mergeIntoFile(MyDB_TableReaderWriter &sortIntoMe, vector<vector<MyDB_RecordIteratorAltPtr>> &mergeUs,
				   function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	vector<int> indexs(mergeUs.size(), 0);
	auto comp = [&](pair<MyDB_RecordIteratorAltPtr, int> a, pair<MyDB_RecordIteratorAltPtr, int> b) {
		a.first->getCurrent(lhs);
		b.first->getCurrent(rhs);
		return comparator();
	};
	priority_queue<pair<MyDB_RecordIteratorAltPtr, int>, vector<pair<MyDB_RecordIteratorAltPtr, int>>, decltype(comp)> pq(comp);
	for (int i = 0; i < mergeUs.size(); i++) {
		if (mergeUs[i].size() > 0) {
			pq.push({mergeUs[i][0], i});
		}
	}

	while (!pq.empty()) {
		auto top = pq.top();
		pq.pop();
		top.first->getCurrent(lhs);
		sortIntoMe.append(lhs);

		if (!top.first->advance()) {
			indexs[top.second]++;
			if (indexs[top.second] < mergeUs[top.second].size()) {
				pq.push({mergeUs[top.second][indexs[top.second]], top.second});
			}
		}
	}
}

// vector <MyDB_PageReaderWriter> mergeIntoList ( MyDB_BufferManagerPtr parent, MyDB_RecordIteratorAltPtr leftIter,
//         MyDB_RecordIteratorAltPtr rightIter, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs ) {
// 		return vector <MyDB_PageReaderWriter> ();
// }
vector<MyDB_PageReaderWriter> mergeIntoList(MyDB_BufferManagerPtr parent, vector<MyDB_PageReaderWriter> leftIterVec,
											vector<MyDB_PageReaderWriter> rightIterVec, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	vector<MyDB_PageReaderWriter> res;
	MyDB_PageHandle page = parent->getPage();
	MyDB_PageReaderWriterPtr pageRW = make_shared<MyDB_PageReaderWriter>(parent);
	int leftIndex = 0, rightIndex = 0;
	MyDB_RecordIteratorAltPtr leftIter = leftIterVec[leftIndex].getIteratorAlt();
	MyDB_RecordIteratorAltPtr rightIter = rightIterVec[rightIndex].getIteratorAlt();
	while (leftIndex < leftIterVec.size() && rightIndex < rightIterVec.size()) {
		leftIter->getCurrent(lhs);
		rightIter->getCurrent(rhs);

		if (comparator()) {
			if (!pageRW->append(lhs)) {
				res.push_back(*pageRW);
				pageRW = make_shared<MyDB_PageReaderWriter>(parent);
				pageRW->append(lhs);
			}
			if (!leftIter->advance()) {
				leftIndex++;
				if (leftIndex < leftIterVec.size()) {
					leftIter = leftIterVec[leftIndex].getIteratorAlt();
				}
			}
		} else {
			if (!pageRW->append(rhs)) {
				res.push_back(*pageRW);
				pageRW = make_shared<MyDB_PageReaderWriter>(parent);
				pageRW->append(rhs);
			}
			if (!rightIter->advance()) {
				rightIndex++;
				if (rightIndex < rightIterVec.size()) {
					rightIter = rightIterVec[rightIndex].getIteratorAlt();
				}
			}
		}
	}

	// process remaining records from the left iterator
	while (leftIndex < leftIterVec.size()) {
		leftIter->getCurrent(lhs);
		if (!pageRW->append(lhs)) {
			res.push_back(*pageRW);
			pageRW = make_shared<MyDB_PageReaderWriter>(parent);
			pageRW->append(lhs);
		}
		if (!leftIter->advance()) {
			leftIndex++;
			if (leftIndex < leftIterVec.size()) {
				leftIter = leftIterVec[leftIndex].getIteratorAlt();
			}
		}
	}

	// process remaining records from the right iterator
	while (rightIndex < rightIterVec.size()) {
		rightIter->getCurrent(rhs);
		if (!pageRW->append(rhs)) {
			res.push_back(*pageRW);
			pageRW = make_shared<MyDB_PageReaderWriter>(parent);
			pageRW->append(rhs);
		}
		if (!rightIter->advance()) {
			rightIndex++;
			if (rightIndex < rightIterVec.size()) {
				rightIter = rightIterVec[rightIndex].getIteratorAlt();
			}
		}
	}
	res.push_back(*pageRW);
	return res;
}

void sort(int runSize, MyDB_TableReaderWriter &sortMe, MyDB_TableReaderWriter &sortIntoMe,
		  function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	const int numPages = sortMe.getNumPages();
	vector<vector<MyDB_RecordIteratorAltPtr>> sortedPagesRuns;

	for (int i = 0; i < numPages; i += runSize) {
		vector<vector<MyDB_PageReaderWriter>> pagesRun;
		// load a run of pages into RAM
		for (int j = 0; j < runSize && i + j < numPages; j++) {
			MyDB_PageReaderWriter tempPageRW = sortMe[i + j];
			// Sort each individual page in the vector
			vector<MyDB_PageReaderWriter> tempVec;
			tempVec.push_back(*(tempPageRW.sort(comparator, lhs, rhs)));
			pagesRun.push_back(tempVec);
		}

		// merge into list
		/*
			1: [[1], [2], [3], [4], [5], [6], [7], [8]]
			2: [[1, 2], [3, 4], [5, 6], [7, 8]]
			3: [[1, 2, 3, 4], [5, 6, 7, 8]]
			4: [[1, 2, 3, 4, 5, 6, 7, 8]]
		*/
		while (pagesRun.size() > 1) {
			vector<vector<MyDB_PageReaderWriter>> newPagesRun;
			for (int i = 0; i < pagesRun.size(); i += 2) {
				if (i == pagesRun.size() - 1) {
					newPagesRun.push_back(pagesRun[i]);
					break;
				}
				vector<MyDB_PageReaderWriter> page1 = pagesRun[i], page2 = pagesRun[i + 1];
				newPagesRun.push_back(mergeIntoList(sortMe.getBufferMgr(), page1, page2, comparator, lhs, rhs));
			}
			pagesRun = newPagesRun;
		}
		vector<MyDB_RecordIteratorAltPtr> tempRecIterAltVec;
		for (MyDB_PageReaderWriter &pageRW : pagesRun[0]) {
			tempRecIterAltVec.push_back(pageRW.getIteratorAlt());
		}
		sortedPagesRuns.push_back(tempRecIterAltVec);
	}

	mergeIntoFile(sortIntoMe, sortedPagesRuns, comparator, lhs, rhs);
}

#endif
