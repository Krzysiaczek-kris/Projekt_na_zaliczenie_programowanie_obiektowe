#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace std;

template <typename T>
void printVector( const vector<T>& v ) {
    for ( int i = 0; i < v.size(); i++ ) {
        cout << v[i] << "\n";
    }
    cout << endl;
}
template <typename T>
void printMatrix( const vector<vector<T>>& v ) {
	for ( int i = 0; i < v.size(); i++ ) {
		for ( int j = 0; j < v[i].size(); j++ ) {
			cout << v[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

template <typename T>
inline double mean( const vector<T>& v ) {
    return accumulate( v.begin(), v.end(), 0.0 ) / v.size();
}

template <typename T>
inline double stddev( const vector<T>& v ) {
    return sqrt( inner_product( v.begin(), v.end(), v.begin(), 0.0 ) / ( v.size() - 1 ) );
}

// Calculate the Pearson correlation coefficient of two vectors
template <typename T1, typename T2>
double pearson( const vector<T1>& x, const vector<T2>& y ) {
    // Get the length of the shortest vector
    int n = min( x.size(), y.size() );
    // Calculate the standard deviation of each vector
    double stddev_x = stddev( x );
    double stddev_y = stddev( y );
	// Calculate the sum of the products of the vectors
	double sum_xy = inner_product( x.begin(), x.begin() + n, y.begin(), 0.0 );
    // Return the Pearson correlation coefficient
    return sum_xy / ( n * stddev_x * stddev_y );
}

template <typename T1, typename T2>
double spearman( const vector<T1>& x, const vector<T2>& y ) {

    int n = min( x.size(), y.size() );
	vector<T1> x_truncated( x.end() - n, x.end() ); // we assume that data is most likely to be missing from the beginning, because at the end we have the most recent data
    vector<T2> y_truncated( y.end() - n, y.end() );

    // Calculate the rank of each element in the two vectors
    vector<int> x_rank( n );
    vector<int> y_rank( n );
    iota( x_rank.begin(), x_rank.end(), 1 );
    iota( y_rank.begin(), y_rank.end(), 1 );
    sort( x_rank.begin(), x_rank.end(), [&] ( const int i, const int j ) { return x_truncated[i - 1] < x_truncated[j - 1]; } );
    sort( y_rank.begin(), y_rank.end(), [&] ( const int i, const int j ) { return y_truncated[i - 1] < y_truncated[j - 1]; } );

    // Calculate the difference between ranks for each element
    vector<double> diff( n );
    transform( x_rank.begin(), x_rank.end(), y_rank.begin(), diff.begin(), minus() );


    // Calculate the sum of the squares of the differences
    double sum_diff_squared = inner_product( diff.begin(), diff.end(), diff.begin(), 0.0 );

    // Calculate the Spearman rank correlation coefficient
    return 1 - ( 6 * sum_diff_squared ) / ( n * ( n * n - 1 ) );
}

template <typename T>
vector<vector<T>> transposeMatrix( vector<vector<T>>& matrix ) {
    int rows = matrix.size();
    int cols = matrix[0].size();
    vector<vector<T>> transposed( cols, vector<T>( rows ) );
    for ( int i = 0; i < rows; i++ ) {
        for ( int j = 0; j < cols; j++ ) {
            transposed[j][i] = matrix[i][j];
        }
    }
    return transposed;
}