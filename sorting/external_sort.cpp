#include <cstdio>
#include <algorithm>
#include <iostream>
#include <vector>
#include <climits>

using namespace std;

vector<long> segment_vec;	// assume  that we can store 5 integers at most

int get_integer(FILE* fp)
{
	char buf[10];
	if(fgets(buf, 10, fp) == NULL)
		perror("fgets");
	return atoi(buf);
}

FILE* open_and_seek(const char* file, long pos)
{
	FILE* fp = fopen(file, "r+");
	if(fseek(fp, pos, SEEK_SET) < 0)
		perror("fseek");
	return fp;
}

void merge(const char* file, long seg1, long seg1_end, long seg2, long seg2_end)
{
	int n1, n2;
	FILE* wfp = open_and_seek(file, seg1);
	FILE* fp1 = open_and_seek(file, seg1);
	FILE* fp2 = open_and_seek(file, seg2);
	n1 = get_integer(fp1);
	n2 = get_integer(fp2);
	while(seg1 != seg1_end || seg2 != seg2_end){
		if(n1 < n2){
			fprintf(wfp, "%d\n", n1);
			seg1 = ftell(fp1);
			n1 = seg1 == seg1_end? INT_MAX : get_integer(fp1);
		}else{
			fprintf(wfp, "%d\n", n2);
			seg2 = ftell(fp2);
			n2 = seg2 == seg2_end? INT_MAX : get_integer(fp2);
		}
	}
	fclose(wfp);
	fclose(fp1);
	fclose(fp2);
}

void print_vec(const char* file, const char* vec_name, vector<long>& vec)
{
	FILE* fp = fopen(file, "r");
	cout<<vec_name<<endl;
	for(int i=0; i<vec.size(); i++){
		char buf[10];
		fseek(fp, vec[i], SEEK_SET);
		if(fgets(buf, 10, fp))
			cout<<buf;
	}
}

void print_file(const char* file)
{
	FILE* fp = fopen(file, "r");

	char buf[10];
	for(int i=0; i+1<segment_vec.size(); i++){
		fseek(fp, segment_vec[i], SEEK_SET);
                while(ftell(fp) != segment_vec[i+1]){
                    fgets(buf, 100, fp);
                    cout<<buf;
                }
                cout<<"\n";
	}
        fclose(fp);
}

void external_sort(const char* file)
{
	FILE* fp = fopen(file, "r+");
	char buf[10];
	vector<int> vec_for_sort;
	bool eof = feof(fp);
	while(!eof){
		long seg_begin = ftell(fp);
		segment_vec.push_back(seg_begin);
		for(int i=0; i<5; i++){
			if(!fgets(buf, 10, fp))
				break;
			vec_for_sort.push_back(atoi(buf));
		}
		eof = feof(fp);		// get eof before fseek, cause fseek could clear eof flag.
		sort(vec_for_sort.begin(), vec_for_sort.end());
		long seg_end = ftell(fp);
		if(fseek(fp, seg_begin, SEEK_SET) < 0)
			perror("fseek");
		for(int i=0; i< vec_for_sort.size(); i++)
			fprintf(fp, "%d\n", vec_for_sort[i]);
		if(fseek(fp, seg_end, SEEK_SET) < 0)
			perror("fseek2");
		if(eof)
			segment_vec.push_back(seg_end);		// push into the eof position 
		vec_for_sort.clear();
	}
	fclose(fp);
        /*
        cout<<"before merge"<<endl;
        print_file(file);
        */

	//print_vec(file, "segment_vec", segment_vec);
        while(segment_vec.size() > 2){
            vector<long> tmp_vec;
            for(int i=0; i<segment_vec.size(); i+=2){
                if(i+2 < segment_vec.size()){
                    merge(file, segment_vec[i], segment_vec[i+1], segment_vec[i+1], segment_vec[i+2]);
                    tmp_vec.push_back(segment_vec[i]);
                }else{
                    tmp_vec.push_back(segment_vec[i]);
                    if(i+1 < segment_vec.size())
                        tmp_vec.push_back(segment_vec[i+1]);
                }
            }
            swap(segment_vec, tmp_vec);
            /*
            cout<<"after merge"<<endl;
            print_file(file);
            */
            //print_vec(file, "tmp_vec", segment_vec);
        }
}

int main()
{
	external_sort("a.txt");
	return 0;
}
