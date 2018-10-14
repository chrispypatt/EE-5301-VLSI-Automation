#include <string>
#include <vector> 

using namespace std;

#include "helpers.h" 

/*Helper functions!*/

//Modified function from 
//https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
vector<string> split(string s, string delimiter) {
	size_t start = 0, end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((end = s.find (delimiter, start)) != string::npos) {
        token = s.substr (start, end - start);
        start = end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (start));
    return res;
}

bool stringContains(string str,string token){
	if (str.find(token) != string::npos){
		return true;
	}
	return false;
}

vector<double>	vectorStr2Dbl(vector<string> strVector){
	vector<double> values(strVector.size());
	transform(strVector.begin(), strVector.end(), values.begin(), [](const string& val){
		return stod(val);
	});
	return values;
}