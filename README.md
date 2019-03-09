# code-examples
Examples of my code

seqmodify.cpp has a solution for the problem of making one vector of integers look like another, but with some complexity. One is a "reference" vector, and one is an "example" vector. Both vectors always follow a couple rules:
* elements have positive values
* elements are in ascending order (but the values do not need to be contiguous)

So a reference vector might have 4 elements like this { 1, 5, 6, 18 }. Imagine that each element represents a processing step of some kind.
The way that an example vector gets used is that it is first created with 4 elements all set to zero (0). Then as each processing step is performed, the corresponding element gets changed from 0 to the correct value as specified in the reference vector. So it starts out as { 0, 0, 0, 0 } and after four processing stages, it has a final state of { 1, 5, 6, 18 }, matching the reference vector. Note that the processing steps do not need to happen in order, so the example vector could have an intermediate state that looks like { 1, 0, 0, 0 } but could just as easily have an intermediate state that looks like { 0, 5, 6, 0 }.

Suppose a reference vector is declared, and an example vector initialized to the needed number of zeros. Some number of processing steps are performed, so the example vector is either in an intermediate or final state, and then the reference vector CHANGES! Elements may have been added to the reference vector, removed from it, or both. The problem is, how do you update the example vector to make it match the reference vector, but preserve elements that already indicate a completed processing step (meaning you can't just wipe everything out and start over with a bunch of zeros).
