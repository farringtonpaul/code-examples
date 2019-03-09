//---------------------------------------------------------------
// Two vectors of integers. One represents a specification
// or configuration of data records that must be captured.
// Each record is identified by a non-zero integer, so the
// specification vector may look like:
//	{ 1, 4, 8, 9 } meaning that 4 data records must be
// captured, and they'll be identified #1, #4, #8 and #9.
// The numbers will always be in ascending order, but don't
// have to be contiguous.
//
// The second vector represents a file in which data is being
// captured. Since the specification vector indicates 4
// records must be captured, the second vector starts out
// with 4 zeros: { 0, 0, 0, 0 }. As data capture proceeds,
// these "empty" records are replaced with the corresponding
// specification numbers, so the second vector should end
// up identical to the first: { 1, 4, 8, 9 }.
//
// One wrinkle is that we may get the second vector in an
// intermediate state, where some or all of the entries are
// still the initial zero value, e.g.: { 1, 0, 0, 9 }.
//
// The problem is, if the specification vector changes
// (elements may be added or deleted, but will remain in
// ascending order), how to bring the second vector in sync,
// since some of the elements may still have the initial
// zero values?
//
// Example:
// The original specification vector was { 1, 4, 8, 9 }.
// A file vector is created with initial zeros { 0, 0, 0, 0 };
// One record gets populated with the identifying number:
// { 0, 0, 8, 0 }.
// Now, the specification changes. 4 is removed and 10 is added
// to the end: { 1, 8, 9, 10 }. 
// We have to remove the entry in the file vector that corresponds
// to the entry removed from the specification vector, and we 
// have to add a zero-filled entry in the position where 
// new specification entry was added. 
// We are given the new state of the specification vector and
// the current state of the file vector:
// Spec = { 1, 8, 9, 10 }
// File = { 0, 0, 8, 0 }
//---------------------------------------------------------------
#include <vector>
#include <stdio.h>
#include <string>
using namespace std;

//---------------------------------------------------------------
// globals
//---------------------------------------------------------------
int FailCount = 0;

struct NotFoundSeq {
	int start;
	int end;
};

//---------------------------------------------------------------
// forward declarations
//---------------------------------------------------------------
int removeZeros(vector<int> &as, vector<int> &wf);

//---------------------------------------------------------------
// utility functions
//---------------------------------------------------------------
// make it easier to initialize vectors in one line
// in test program
void
loadVec(vector<int> &vec, const string &s)
{
	vec.clear();
	string eat = s;
	string part;
	while (eat.length())
	{
		size_t pos = eat.find(",");
		if (pos != string::npos)
		{
			part = eat.substr(0, pos);
			eat = eat.substr(pos+1);
		}
		else
		{
			part = eat;
			eat = "";
		}
		vec.push_back(atoi(part.c_str()));
	}
}

// debug logging
void
logVecs(vector<int> &asFields, vector<int> &wfFields)
{
	unsigned int max = asFields.size();
	if (wfFields.size() > max)
		max = wfFields.size();

	string line = "   Config         Actual \n";
	for (unsigned int i = 0; i < max; i++)
	{
		char buf[8];
		if (asFields.size() > i)
		{
			sprintf(buf, "     %d", asFields[i]);
			line += buf;
			if (strlen(buf) == 6)
				line += " ";
		}
		else
			line += "       ";
		line += "            ";
		if (wfFields.size() > i)
		{
			sprintf(buf, "%d", wfFields[i]);
			line += buf;
		}
		line += "\n";
	}
	printf("%s", line.c_str());
}

// debug logging
void
logPossibles(vector<int> const &possibles, const char *text)
{
	string log = text;
	log += ": ";
	vector<int>::const_iterator it;
	for (it = possibles.begin(); it != possibles.end(); it++)
	{   
		char buf[8];
		sprintf(buf, "%d", (*it));
		log += buf;
		log += ", ";
	}
	log += "\n";
	printf("%s", log.c_str());
}

bool
fldNumListsMatch(vector<int> &as, vector<int> &wf)
{
	if (as.size() != wf.size())
		return false;

	bool matched = true;
	for (unsigned int i = 0; i < as.size(); i++)
	{
		if (wf[i] != 0 && wf[i] != as[i])
		{
			matched = false;
			break;
		}
	}
	return matched;
}

// new...............
void
makeNotFoundVector(vector<NotFoundSeq> &nfsVec, vector<int> &a, vector<int> &w)
{
	// scan the a vector. for each element, scan the w vector
	// and count whether it is found or not
	vector<int>::iterator aIt;
	vector<int>::iterator wIt;
	int pos = 1;
	int seqStart = 0;
	int seqEnd = 0;
	for (aIt = a.begin(); aIt != a.end(); aIt++)
	{
		bool found = false;
		for (wIt = w.begin(); wIt != w.end(); wIt++)
		{
			if ((*aIt) == (*wIt))
			{
				found = true;
				break;
			}
		}
		if (found)
		{
			// if we have a sequence of not found, store it in vector
			if (seqStart)
			{
				NotFoundSeq nfs;
				nfs.start = seqStart;
				nfs.end = seqEnd;
				nfsVec.push_back(nfs);
				seqStart = 0;
				seqEnd = 0;
			}
		}
		else
		{
			if (!seqStart)
				seqStart = pos;
			seqEnd = pos;
		}
		pos++;
	}
	// if we have an unfinished sequence of not found, store it in vector
	if (seqStart)
	{
		NotFoundSeq nfs;
		nfs.start = seqStart;
		nfs.end = pos-1;;
		nfsVec.push_back(nfs);
		seqStart = 0;
		seqEnd = 0;
	}
}

void
logNotFoundVector(vector<NotFoundSeq> &nfsVec)
{
	string str = "nfsVec:\n";
	vector<NotFoundSeq>::iterator nfsIt;
	for (nfsIt = nfsVec.begin(); nfsIt != nfsVec.end(); nfsIt++)
	{
		str += "sequence start=";
		char buf[8];
		sprintf(buf, "%d", (*nfsIt).start);
		str += buf;
		str += ", end=";
		sprintf(buf, "%d", (*nfsIt).end);
		str += buf;
		str += "\n";
	}
	printf("%s", str.c_str());
}

bool
specValsInW(vector<int> &a, vector<int> &w)
{
	logVecs(a, w);
	vector<NotFoundSeq> nfsVec;

	makeNotFoundVector(nfsVec, a, w);

	printf("specValsInW, a size=%u, w size=%u, nfsVec.size=%u\n"
			, a.size(), w.size(), nfsVec.size());

	if (nfsVec.size() == 0)
		return true;

	if (nfsVec.size())
	{
		logNotFoundVector(nfsVec);
		vector<NotFoundSeq>::iterator nfsIt;
		// each sequence is either:
		// i) starting at position 1
		// ii) ending at end of vector a (not exclusive of i)
		// iii) sandwiched between found elements
		//
		// if (i) then vector w must begin with a sequence of 0's
		//  at least as long as the not-found sequence.
		// if (ii) then vector w must end with a sequence of 0's
		//  at least as long as the not-found sequence.
		// if (iii) then there are found positions before and
		//  after the sequence. the distance between them in
		//  vector w must be at least the same as the distance
		//  between them in vector a.
		//
		// return false for any failing case, otherwise return
		// true at the bottom.
		for (nfsIt = nfsVec.begin(); nfsIt != nfsVec.end(); nfsIt++)
		{
			unsigned int len = ((*nfsIt).end - (*nfsIt).start) + 1;
			if (w.size() < len)
				return false;
			if ((*nfsIt).start == 1)
			{
				// case i)
				printf("case i: ");
				for (unsigned int i = 0; i < len; i++)
				{
					if (w[i] != 0)
					{
						printf("fail\n");
						return false;
					}
				}
				printf("good\n");
			}
			else if ((*nfsIt).end == (int)a.size())
			{
				// case ii)
				printf("case ii: ");
				for (unsigned int i = 1; i <= len; i++)
				{
					unsigned int idx = (w.size() - i);
					if (w[idx] != 0)
					{
						printf("fail\n");
						return false;
					}
				}
				printf("good\n");
			}
			else
			{
				// case iii)
				printf("case iii: ");
				int beforePosA = (*nfsIt).start - 1;
				int afterPosA = (*nfsIt).end + 1;
				int beforeVal = a[beforePosA-1];
				int afterVal = a[afterPosA-1];
				int beforePosW = 0;
				int afterPosW = 0;
				for (unsigned int i = 0; i < w.size(); i++)
				{
					if (w[i] == beforeVal)
						beforePosW = i+1;
					else if (w[i] == afterVal)
						afterPosW = i+1;
				}
				if ((afterPosW-beforePosW) < (afterPosA-beforePosA))
				{
					printf("fail\n");
					return false;
				}
				printf("good\n");
			}
		}
	}

	return true;
}

// call this after any non-zero values in w that aren't in a
// have already been removed. this function figures out what elements
// in a don't appear in w, and makes sure that there are corresponding
// 0s in w in the right positions for those missing elements from a.
//
// it does this by making a list of sequences of a values that aren't
// found in w.
// each sequence is either:
// i) starting at position 1
// ii) ending at end of vector a (not exclusive of i)
// iii) sandwiched between found elements
//
// if (i) then vector w must begin with a sequence of 0's
//  as long as the not-found sequence.
// if (ii) then vector w must end with a sequence of 0's
//  as long as the not-found sequence.
// if (iii) then there are found positions before and
//  after the sequence. the distance between them in
//  vector w must be the same as the distance
//  between them in vector a.
//
// set pos to 0 to prepend at beginning of w vector
// set pos to N to insert after pos N in w vector
// set pos to -N to delete from position N in w vector
bool
fixingW(vector<int> &a, vector<int> &w, int &pos)
{
	if (fldNumListsMatch(a, w))
		return false;
	if (w.size() == 0 && a.size() > 0)
	{
		pos = 0;
		return true;
	}

	vector<NotFoundSeq> nfsVec;

	makeNotFoundVector(nfsVec, a, w);

	printf("fixingW, a size=%u, w size=%u, nfsVec.size=%u\n"
			, a.size(), w.size(), nfsVec.size());

	if (nfsVec.size() == 0)
		return false;

	logNotFoundVector(nfsVec);
	vector<NotFoundSeq>::iterator nfsIt;

	int beforePosA = 0;
	int afterPosA = 0;
	int beforeVal = 0;
	int afterVal = 0;
	int beforePosW = 0;
	int afterPosW = 0;
	for (nfsIt = nfsVec.begin(); nfsIt != nfsVec.end(); nfsIt++)
	{
		if ((*nfsIt).start == 1)
		{
			// special case for all zeros in w
			if ((*nfsIt).end == (int)a.size())
			{
				if (a.size() == w.size())
					return false;
				else if (w.size() < a.size())
				{
					pos = 0;
					return true;
				}
				else	// w.size() > a.size()
				{
					pos = -1;
					return true;
				}
			}
			// case i)
			afterPosA = (*nfsIt).end+1;
			afterVal = a[afterPosA-1];
			afterPosW = 0;
			for (unsigned int i = 0; i < w.size(); i++)
			{
				if (w[i] == afterVal)
				{
					afterPosW = i+1;
					break;
				}
			}
			if (afterPosW == afterPosA)
				continue;
			else if (afterPosW < afterPosA)
			{
				// too few 0s at beginning
				pos = 0;
				return true;
			}
			else	// afterPosW > afterPosA
			{
				// too many 0s at beginning
				pos = -1;
				return true;
			}
		}
		else if ((*nfsIt).end == (int)a.size())
		{
			// case ii)
			beforePosA = (*nfsIt).start-1;
			beforeVal = a[beforePosA-1];
			beforePosW = 0;
			for (unsigned int i = 1; i <= w.size(); i++)
			{
				unsigned int idx = (w.size() - i);
				if (w[idx] == beforeVal)
				{
					beforePosW = idx+1;
					break;
				}
			}
			int fromEndA = a.size() - beforePosA;
			int fromEndW = w.size() - beforePosW;
			if (fromEndA == fromEndW)
				continue;
			else if (fromEndA < fromEndW)
			{
				// too many 0s at end
				pos = (0 - w.size());
				return true;
			}
			else	// fromEndA > fromEndW
			{
				// too few 0s at end
				pos = w.size();
				return true;
			}
		}
		else
		{
			// case iii)
			beforePosW = 0;
			afterPosW = 0;
			beforePosA = (*nfsIt).start-1;
			beforeVal = a[beforePosA-1];
			afterPosA = (*nfsIt).end+1;
			afterVal = a[afterPosA-1];
			for (unsigned int i = 0; i < w.size(); i++)
			{
				if (w[i] == beforeVal)
					beforePosW = i+1;
				else if (w[i] == afterVal)
					afterPosW = i+1;
			}
			int aLen = afterPosA - beforePosA;
			int wLen = afterPosW - beforePosW;
			if (aLen == wLen)
				continue;
			else if (aLen < wLen)
			{
				// there are too many 0s
				pos = 0 - (beforePosW+1);
				return true;
			}
			else	// aLen > wLen
			{
				// there are too few 0s
				pos = (beforePosW);
				return true;
			}
		}
	}

	return false;
}

// new...............

// make a list of everything in the suspect vector that
// is not represented in the reference vector
void
findPossibles(vector<int> &possibles, const vector<int> suspect
		, const vector<int> reference)
{
	possibles.clear();
	vector<int>::const_iterator it;
	for (unsigned int i = 0; i < suspect.size(); i++)
	{
		int search = suspect[i];
		bool found = false;
		for (unsigned int j = 0; j < reference.size(); j++)
		{
			int comp = reference[j];
			if (comp == search)
			{
				found = true;
				break;
			}
		}
		if (!found && search != 0)
			possibles.push_back(search);
	}
}

// scan forward through wf vector looking for the first non-zero
// value after 'start' pos, and record its index. then find the 
// matching value in the as vector, and record its index.
bool
findPosMatchedVals(unsigned int start, vector<int> &as, vector<int> &wf
		, int &matchedVal, unsigned int &asPos, unsigned int &wfPos)
{
	bool changed = false;
	unsigned int savedASPos = asPos;
	unsigned int savedWFPos = wfPos;
	for (unsigned int i = start; i <= wf.size(); i++)
	{
		if (wf[i] != 0)
		{
			matchedVal = wf[i];
			wfPos = i+1;
			break;
		}
	}
	for (unsigned int i = start; i <= as.size(); i++)
	{
		if (as[i] == matchedVal)
		{
			asPos = i+1;
			changed = true;
			break;
		}
	}
	if (!changed)
	{
		wfPos = savedWFPos;
		asPos = savedASPos;
	}
	return (changed);
}

// return 1-based index of the position in wf vector AFTER which
// we can insert a 0 element
int
figureWhereToAdd(vector<int> &as, vector<int> &wf)
{
	logVecs(as, wf);

	// as vector has more entries than wf vector.
	// find list of possible additions (things appearing in
	// as vector that don't appear in the wf vector)
	vector<int> possibles;
	findPossibles(possibles, as, wf);
	logPossibles(possibles, "Possible additions");

	vector<int>::iterator it;
	bool look = false;
	for (it = wf.begin(); it != wf.end(); it++)
	{
		if ((*it) == 0)
		{
			look = true;
			break;
		}
	}

	// it may be that we've already inserted a 0 in wf for
	// one or more of the possibles. try to trim the possibles
	// list. If a 0 in wf corresponds to the position of a
	// value X in the possibles list, and some later non-zero value
	// Y is at the same position in both as and wf, then we can
	// remove value X from the possibles list.
	unsigned int asPos = 0, wfPos = 0;
	int knownWFVal;
	unsigned int start = 0;
	while (look)
	{
		if (findPosMatchedVals(start, as, wf, knownWFVal, asPos, wfPos))
		{
			if (asPos == wfPos && wfPos > start)
			{
				// first position with matching values is greater
				// than one. that must mean that position 1 in
				// wf is 0. If possibles[0] < knownWFVal, we can
				// remove it from possibles
				// new
				while (possibles[0] < knownWFVal)
					possibles.erase(possibles.begin());
				logPossibles(possibles, "Revised additions");
				start = wfPos;
				continue;
			}
			look = false;
		}
		else
			look = false;
	}

	// simplest case is if we can add on the end. scan forward
	// through both vectors, to see if each entry in wf either is 
	// 0 or matches the corresponding entry in as. if we get to the
	// end of wf, we can just append
	bool mismatch = false;
	for (unsigned int i = 0; i < wf.size(); i++)
	{
		if (wf[i] == 0 || wf[i] == as[i])
			continue;
		mismatch = true;
		break;
	}
	if (!mismatch)
		return wf.size();

	// find highest field num in wf that is not in possibles and is
	// less than the lowest number in possibles. return
	// that number. we can add after it.
	int lowestPoss = possibles[0];
	int target = 0;
	int targetPos = -1;
	for (int i = (wf.size()-1); i >= 0; i--)
	{
		if (wf[i] == 0)
			continue;
		if (wf[i] < lowestPoss)
		{
			target = wf[i];
			targetPos = i+1;
			break;
		}
	}

	// if we didn't find a point to insert after, it means we
	// have to insert at the head
	if (targetPos == -1)
		targetPos = 0;

	return targetPos;
}

// return 1-based index of element in wf vector that can be deleted
int
figureWhereToRemove(vector<int> &as, vector<int> &wf)
{
	logVecs(as, wf);

	// wf vector has more entries than as vector.
	// find list of possible subtractions (things appearing in
	// wf vector that don't appear in the as vector)
	vector<int> possibles;
	findPossibles(possibles, wf, as);
	logPossibles(possibles, "Possible removals");

	// if we know the number of a possible being removed,
	// return the position of that
	if (possibles.size())
	{
		int remFld = possibles[0];
		for (unsigned int i = 0; i <= wf.size()-1; i++)
		{
			if (wf[i] == remFld)
				return (i+1);
		}
	}

	return removeZeros(as, wf);
}

int
removeZeros(vector<int> &as, vector<int> &wf)
{
	// if wf vector is all 0s, we can just return the
	// first position
	if (wf.size() > as.size())
	{
		vector<int>::iterator it;
		bool allZeros = true;
		for (it = wf.begin(); it != wf.end(); it++)
		{
			if ((*it) != 0)
				allZeros = false;
		}
		if (allZeros)
			return 1;
	}

	// the only remaining cases are where the field to be removed
	// is currently 0, but other fields are populated with non-zero
	// values. The field to be removed could be:
	//	a) before the first non-zero field
	//	b) after the last non-zero field
	//	c) between two non-zero fields

	unsigned int asPos = 0, wfPos = 0;
	int knownWFVal;
	unsigned int start = 0;
	bool look = true;
	while (look)
	{
		if (findPosMatchedVals(start, as, wf, knownWFVal, asPos, wfPos))
		{
			// if the positions are the same, then our field to be 
			// removed must be after. try looking again
			if (asPos == wfPos)
			{
				if (asPos == as.size())
				{
					// we're at the end, better remove next wf field
					look = false;
					return wfPos+1;
				}
				// otherwise, try next position
				start = asPos;
				continue;
			}
			else if (wfPos > asPos)
			{
				// wfPos greater means there must be a zero right
				// before wfPos that can be removed
				if (wf[wfPos-2] == 0)
				{
					look = false;
					return wfPos-1;
				}
				// otherwise, try next position
				start = asPos;
				continue;
			}
		}
		look = false;
		if (start > 0)
		{
			// this means wfPos is now on the last known good
			// position, we we can remove the next position, which
			// must be a 0
			if (wf[wfPos] == 0)
				return wfPos+1;
		}
	}

	return -1;
}

// do the delete (translate 1-based idx as needed)
void
delPos(int pos, vector<int> &vec)
{
	printf("delPos remove element #%d\n", pos);
	int curr = 1;
	vector<int>::iterator it;
	for (it = vec.begin(); it != vec.end(); it++)
	{
		if (pos == curr)
		{
			vec.erase(it);
			break;
		}
		curr++;
	}
}

// insert a 0 value (translate 1-based idx as needed)
void
insPos(int pos, vector<int> &vec)
{
	if (pos == (int)vec.size())
		vec.push_back(0);
	else
	{
		int curr = 0;
		vector<int>::iterator it;
		for (it = vec.begin(); it != vec.end(); it++)
		{
			if (pos == curr)
			{
				vec.insert(it, 0);
				break;
			}
			curr++;
		}
	}
}

//---------------------------------------------------------------
// the logic to reconcile the vectors, including
// lots of debug printing
//---------------------------------------------------------------
void fixVectorsOld(vector<int> &a, vector<int> &w)
{
	printf("===========================================\n");
	bool match = fldNumListsMatch(a, w);
	printf("fldNumListsMatch returned %s\n"
			, (match ? "true" : "false"));
	// if actual (w) has labeled fields that aren't listed in
	// config (a), then we should delete them.
	int pos = 0;
	vector<int> possibles;
	findPossibles(possibles, w, a);
	while (possibles.size())
	{
		logPossibles(possibles, "In Actual, not config");
		pos = figureWhereToRemove(a, w);
		if (pos < 0)
			break;
		delPos(pos, w);
		findPossibles(possibles, w, a);
		logPossibles(possibles, "Now, in Actual, not config");
	}

	//if (a.size() == w.size())
	if (fldNumListsMatch(a, w))	// new
		printf("OK, were done!\n");
	else
	{
		//specValsInW(a, w);	// new
		//while (a.size() > w.size())
		while (!specValsInW(a, w))	// new
		{
			pos = figureWhereToAdd(a, w);
			if (pos > -1)
			{
				printf("result: insert after position %d\n", pos);
				insPos(pos, w);
			}
			else
			{
				printf("ERROR: cannot resolve, maybe done adding\n");
				//FailCount++;
				break;
			}
		}
		while (a.size() < w.size())
		//while (!fldNumListsMatch(a, w))	// new
		{
			pos = figureWhereToRemove(a, w);
			if (pos > -1)
			{
				printf("result: delete position %d\n", pos);
				delPos(pos, w);
			}
			else
			{
				FailCount++;
				printf("ERROR: cannot resolve\n");
				break;
			}
		}
	}
	logVecs(a, w);
	if (!fldNumListsMatch(a, w))
	{
		printf("ERROR: vectors out of sync\n");
		FailCount++;
	}
}

// new..............................
void fixVectors(vector<int> &a, vector<int> &w)
{
	printf("===========================================\n");
	bool match = fldNumListsMatch(a, w);
	printf("fldNumListsMatch returned %s\n"
			, (match ? "true" : "false"));
	logVecs(a, w);
	if (match)
	{
		return;
	}
	// if actual (w) has labeled fields that aren't listed in
	// config (a), then we should delete them.
	vector<int> possibles;
	findPossibles(possibles, w, a);
	while (possibles.size())
	{
		logPossibles(possibles, "In Actual, not config");
		for (unsigned int i = 0; i < w.size(); i++)
		{
			if (w[i] == possibles[0])
			{
				delPos(i+1, w);
				break;
			}
		}
		findPossibles(possibles, w, a);
		logPossibles(possibles, "Now, in Actual, not config");
	}

	if (fldNumListsMatch(a, w))	// new
	{
		logVecs(a, w);
		printf("OK, were done!\n");
	}
	else
	{
		int pos = 0;
		// make sure things in a not in w have corresponding
		// zeros
		while (fixingW(a, w, pos))	// new
		{
			if (pos >= 0)
				insPos(pos, w);
			else	// pos < 0
				delPos(0-pos, w);
			logVecs(a, w);
		}
		// remove any extra zeros
		while (w.size() > a.size())
		{
			pos = removeZeros(a, w);
			if (pos > 0)
			{
				delPos(pos, w);
				logVecs(a, w);
			}
			else
				break;
		}
		if (!fldNumListsMatch(a, w))
		{
			logVecs(a, w);
			printf("ERROR: vectors out of sync\n");
			FailCount++;
		}
		else
			printf("OK, were done!\n");
	}
}
// new..............................

//---------------------------------------------------------------
// main test program
//---------------------------------------------------------------
int
main()
{
	printf("hello w\n");
	vector<int> asVec;
	vector<int> wfVec;

	loadVec(asVec, "1,2,3");
	loadVec(wfVec, "1,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,3");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "1,3");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "5,10,15,20");
	loadVec(wfVec, "0,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,10,15");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "10,15");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,15");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,6,10");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,10,15,20,25");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,0,0,20,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,0,0,0,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,10,15,0,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,0,0,0,0");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,0,0,0,40");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,0,0,40");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "5,10,15,20");
	loadVec(wfVec, "5,6,15");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "1,5,10,15,20");
	loadVec(wfVec, "5,6,15,17");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "15,20");
	loadVec(wfVec, "5,6,15,17,0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "15,20");
	loadVec(wfVec, "0,6,0,17,0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "1,5,10,15,17,18");
	loadVec(wfVec, "1,5,10,15,17,18");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "1,5,11,15,17,18");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "1,5,0,15,17,18");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,0,0,15,17,0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "18");
	loadVec(wfVec, "0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "");
	loadVec(wfVec, "");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "5,10,15,16,20,25");
	loadVec(wfVec, "10,15,20,25");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,15,20,25");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "5,10,15,16,20,25");
	loadVec(wfVec, "0,5,10,15,16,20,25");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "5,10,15,16,20,25");
	loadVec(wfVec, "0,5,10,15,16,20,25,29");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "5,10,15,0,0,16,20,25");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,0,5,10,15,16,20,25,29");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,5,10,16,20,0,25");
	fixVectors(asVec, wfVec);

	loadVec(wfVec, "0,5,10,15,0,0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "");
	loadVec(wfVec, "0,5,0");
	fixVectors(asVec, wfVec);

	loadVec(asVec, "3,13,23");
	loadVec(wfVec, "");
	fixVectors(asVec, wfVec);

	if (FailCount)
		printf("%d test(s) FAILED\n", FailCount);
	return 0;
}
