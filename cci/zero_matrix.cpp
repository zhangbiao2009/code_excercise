#include <iostream>
#include <vector>

using namespace std;

class MatrixZeroer{
	const vector<vector<int>>& _matrix;
	vector<bool> _row_zeroed;
	vector<bool> _col_zeroed;

	void zero_row(vector<vector<int>>& result, int row){
		for(int j=0; j<result[row].size(); j++)
			result[row][j] = 0;
	}

	void zero_col(vector<vector<int>>& result, int col){
		for(int i=0; i<result.size(); i++)
			result[i][col] = 0;
	}

	void zero_row_col(vector<vector<int>>& result, int row, int col)
	{
		if(!_row_zeroed[row]){
			zero_row(result, row);
			_row_zeroed[row] = true;
		}
		if(!_col_zeroed[col]){
			zero_col(result, col);
			_col_zeroed[col]  = true;
		}
	}

	public:
	MatrixZeroer(const vector<vector<int>>& matrix)
		: _matrix(matrix), _row_zeroed(_matrix.size())
	{
		if(_matrix.size() == 0){
			_col_zeroed = vector<bool>();
		}
		else{
			_col_zeroed = vector<bool>(_matrix[0].size());
		}
	}

	vector<vector<int>> zero()
	{
		if(_matrix.size() == 0 || _matrix[0].size() == 0)
			return _matrix;

		vector<vector<int>> result(_matrix.size());
		for(int i=0; i<result.size(); i++){
			result[i] = _matrix[i];
		}

		for(int i=0; i<_matrix.size(); i++){
			for(int j=0; j<_matrix[i].size(); j++){
				if(_matrix[i][j] == 0){
					zero_row_col(result, i, j);
					break;
				}
			}
		}

		return result;
	}
};

vector<vector<int>> zero_matrix(vector<vector<int>> matrix)
{
	MatrixZeroer mz(matrix);
	return mz.zero();
}

int main()
{
	vector<vector<int>> matrix = {
		{1, 1, 1, 1},
		{2, 0, 2, 2},
		{3, 3, 0, 3},
	};

	auto result = zero_matrix(matrix);
	for(int i=0; i<result.size(); i++){
		for(int j=0; j<result[i].size(); j++){
			cout<< result[i][j]<< '\t';
		}
		cout<<endl;
	}
}
