#include <iostream>
#include <vector>

using namespace std;

void fillMatrix(vector<vector<char>>& matrix, int aCount, int bCount,
                int cCount) {
	int rows = matrix.size();
	int cols = matrix[0].size();

	// 初始化矩阵为空格
	for (auto& row : matrix)
		fill(row.begin(), row.end(), ' ');

	char currentChar = 'A'; // 假设 a 类为 'A', b 类为 'B', c 类为 'C'

	// 填充 a 类和 b 类
	int totalPlaced = 0;
	for (int col = 0; col < cols; ++col) {
		for (int row = 0; row < rows; ++row) {
			if (totalPlaced >= aCount + bCount)
				break;

			if (totalPlaced < aCount)
				matrix[row][col] = 'A';
			else
				matrix[row][col] = 'B';

			++totalPlaced;
		}
		if (totalPlaced >= aCount + bCount)
			break;
	}

	// 填充 c 类，从右往左，从上往下
	for (int col = cols - 1; col >= 0; --col) {
		for (int row = 0; row < rows; ++row) {
			if (matrix[row][col] != ' ')
				continue; // 已被填充
			if (cCount <= 0)
				return;

			matrix[row][col] = 'C';
			--cCount;
		}
	}
}

// 打印矩阵
void printMatrix(const vector<vector<char>>& matrix) {
	for (const auto& row : matrix) {
		for (char c : row)
			cout << c << " ";
		cout << endl;
	}
}

int main() {
	int rows = 3, cols = 4;
	vector<vector<char>> matrix(rows, vector<char>(cols));

	fillMatrix(matrix, 3, 2, 3);

	printMatrix(matrix);

	return 0;
}