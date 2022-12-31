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
    double sum = 0.0;
    for ( int i = 0; i < v.size(); i++ ) {
        sum += v[i];
    }
    return sum / v.size();
}

template <typename T>
double stddev( const vector<T>& v ) {
    double mean_v = mean( v );
    double sum_squared_differences = 0.0;
    for ( int i = 0; i < v.size(); i++ ) {
        sum_squared_differences += pow( v[i] - mean_v, 2 );
    }
    return sqrt( sum_squared_differences / ( v.size() - 1 ) );
}

template <typename T1, typename T2>
double pearson( const vector<T1>& x, const vector<T2>& y ) {
    int n = min( x.size(), y.size() );
    double mean_x = mean( x );
    double mean_y = mean( y );
    double stddev_x = stddev( x );
    double stddev_y = stddev( y );
    double sum_xy = 0.0;
    for ( int i = 0; i < n; i++ ) {
        sum_xy += ( x[i] - mean_x ) * ( y[i] - mean_y );
    }
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
    sort( x_rank.begin(), x_rank.end(), [&] ( int i, int j ) { return x_truncated[i - 1] < x_truncated[j - 1]; } );
    sort( y_rank.begin(), y_rank.end(), [&] ( int i, int j ) { return y_truncated[i - 1] < y_truncated[j - 1]; } );

    // Calculate the difference between ranks for each element
    vector<double> diff( n );
    transform( x_rank.begin(), x_rank.end(), y_rank.begin(), diff.begin(), minus<double>() );

    // Calculate the sum of the squares of the differences
    double sum_diff_squared = inner_product( diff.begin(), diff.end(), diff.begin(), 0.0 );

    // Calculate the Spearman rank correlation coefficient
    return 1 - ( 6 * sum_diff_squared ) / ( n * ( n * n - 1 ) );
}


template <typename T1, typename T2>
double r_squared( const vector<T1>& x, const vector <T2>& y ) {
	return pow( pearson( x, y ), 2 );
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