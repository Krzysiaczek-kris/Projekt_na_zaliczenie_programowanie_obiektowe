#include <iostream>
#include <thread>
#include "rapidcsv.h"

using namespace std;
using namespace rapidcsv;

template <typename T>
void printVector( const vector<T>& v ) {
	for ( int i = 0; i < v.size(); i++ ) {
		cout << v[i] << "\n";
	}
	cout << endl;
}

struct country {
	string name;
	vector<vector<int>> data; // year and values

	friend ostream& operator<<( ostream& os, const country& c ) {
		os << c.name;
		return os;
	}
};

void loaddata( vector<country>& v, map<string, int>& countries, const Document& doc ) {
	int line = 0;
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		vector<vector<int>> data;

		for ( int i = 0; i < it->second; i++ ) {
			vector<int> year;

			for ( int j = 1; j < doc.GetColumnCount(); j++ ) {
				year.push_back( doc.GetCell<int>( j, line ) );
			}

			data.push_back( year );
			line++;
		}
		country c = { it->first, data };
		v.push_back( c );
	}
}

void stats( const vector<country> v, const vector<string>& labels, const int country_it, int years, const string& desktop ) {
	//create new txt file
	ofstream file( desktop + v[country_it].name + "_" + to_string( years ) + ".txt" );

	file << "Country: " << v[country_it].name << endl;
	file << "Years: " << years << endl << endl;

	years -= v[country_it].data[0][0];
	int sum = 0;

	for ( int i = 1; i < labels.size(); i++ ) {
		sum += v[country_it].data[years][i];
	}

	for ( int i = 1; i < labels.size(); i++ ) {
		string first = labels[i] + ": " + to_string( v[country_it].data[years][i] );
		int spaces = 100 - first.length();
		double percent = static_cast<double>( v[country_it].data[years][i] ) / sum * 100;
		string second = to_string( percent ) + "%";
		file << first << string( spaces, ' ' ) << second << endl;
	}

	file << endl << "Total: " << sum << endl;

	file.close();
}

int main() {
	Document doc( "annual_deaths_by_causes.csv", LabelParams( 0 ) );
	map <string, int> countries;

	//reading countries
	for ( int i = 0; i < doc.GetRowCount(); i++ ) {
		countries[doc.GetCell<string>( "country", i )]++;
	}
	vector<string> labels;
	for ( int i = 1; i < doc.GetColumnCount(); i++ ) {
		labels.push_back( doc.GetColumnName( i ) );
	}
	printVector( labels );

	//display countries and number of entries
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		int spaces = 50 - it->first.length();
		cout << it->first << string( spaces, ' ' ) << "Entries: " << it->second << endl;
	}
	cout << "Countries: " << countries.size() << endl;
	cout << "Total entries: " << doc.GetRowCount() << endl;

	vector<country> countries_vec;

	thread loading( loaddata, ref( countries_vec ), ref( countries ), ref( doc ) );

	cout << "Enter Country name: " << endl;
	string country_name;
	cin >> country_name;
	for ( int i = 0; i < country_name.size(); i++ ) {
		if ( i == 0 || country_name[i - 1] == ' ' ) {
			country_name[i] = toupper( country_name[i] );
		}
		else {
			country_name[i] = tolower( country_name[i] );
		}
	}
	//find country
	int country_iterator = 0;
	while ( country_iterator < countries_vec.size() && countries_vec[country_iterator].name != country_name ) country_iterator++;
	cout << "\n\n";
	for ( int i = 0; i < countries_vec[country_iterator].data.size(); i++ ) {
		cout << countries_vec[country_iterator].data[i][0] << "   ";
		if ( ( i + 1 ) % 3 == 0 ) cout << endl;
	}
	cout << "Choose year(s):" << endl;
	int years;
	cin >> years;

	char* buff = nullptr;
	_dupenv_s( &buff, nullptr, "USERPROFILE" );
	string desktop = buff;
	desktop += "\\Desktop\\";
	loading.join();

	//Czy jak bede kopiowal wektor to bede mogl wlaczyc kilka watkow?
	stats( countries_vec, labels, country_iterator, years, desktop );

}