#include <iostream>
#include <vector>
using namespace std;
int level = 0;
vector<int> reverseStackRecursively(vector<int> stack, int top) {
    // write code here
    if (top > 0)
    {
        int val = stack[top - 1];
        ++level;
        cout << "level="<<level<<",val="<<val<<"\n";
        stack = reverseStackRecursively(stack,top-1);
        --level;
        cout << "level="<< level << ",val=" << val << "\n";
        stack[level] = val;
    }
    return stack;
}

int main()
{
    vector<int> in = { 1,2,3,4,5,6,7,8,9,10,11 };
    vector<int> out;
    out=reverseStackRecursively(in,11);
	return 0;
}