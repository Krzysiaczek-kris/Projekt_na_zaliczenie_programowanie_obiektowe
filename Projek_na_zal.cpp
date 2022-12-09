#include <iostream>
#include <thread>
#include "rapidcsv.h"
#include "json.hpp"

using namespace std;
using namespace rapidcsv;
using json = nlohmann::json;

template <typename T>
void printVector( const vector<T>& v ) {
	for ( int i = 0; i < v.size(); i++ ) {
		cout << v[i] << "\n";
	}
	cout << endl;
}

struct country {
	string name;
	string code;
	vector<vector<int>> data; // years and values

	friend ostream& operator<<( ostream& os, const country& c ) {
		os << c.name;
		return os;
	}
};

struct country_HDI {
	string name;
	int rank;
	string hdicode;
	vector<vector<double>> data; // years and values

	friend ostream& operator<<( ostream& os, const country_HDI& HDI ) {
		os << HDI.name;
		return os;
	}
};

void loaddata_csv( vector<country>& v, map<string, int>& countries, const Document& doc ) {
	int line = 0;
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		vector<vector<int>> data;
		string code = doc.GetCell<string>( "code", line );
		for ( int i = 0; i < it->second; i++ ) {
			vector<int> year;

			for ( int j = 2; j < doc.GetColumnCount(); j++ ) {
				year.push_back( doc.GetCell<int>( j, line ) );
			}

			data.push_back( year );
			line++;
		}
		v.emplace_back( country{ it->first, code, data } );
	}
}
void loaddata_json( vector<country_HDI>& v_HDI, const json& HDI ) {
	for (const auto& i : HDI)
	{
		string name = i["country"];
		string hdicode = i["hdicode"];
		int rank = i["hdi_rank_2021"];

		vector<double> hdi_data;
		vector<double> le_data;
		vector<double> gnipc_data;

		for ( int j = 1990; j <= 2021; j++ ) {
			hdi_data.push_back(i["hdi_" + to_string( j )] );
			le_data.push_back(i["le_" + to_string( j )] );
			gnipc_data.push_back(i["gnipc_" + to_string( j )] );
		}
		vector<vector<double>> temp;
		temp.push_back( hdi_data );
		temp.push_back( le_data );
		temp.push_back( gnipc_data );

		v_HDI.emplace_back( country_HDI{ name, rank, hdicode, temp } );
	}
	cout << "done";

}

void quickstats_deaths( const country c, const vector<string>& labels, int year, const string& desktop ) {
	ofstream file( desktop + c.name + "_" + to_string( year ) + "_Death_Stats" + ".txt" );

	file << "Country: " << c.name << endl;
	file << "Years: " << year << endl << endl;

	year -= c.data[0][0];
	int sum = 0;

	for ( int i = 1; i < labels.size(); i++ ) {
		sum += c.data[year][i];
	}

	for ( int i = 1; i < labels.size(); i++ ) {
		string first = labels[i] + ": " + to_string( c.data[year][i] );
		int spaces = 100 - first.length();
		double percent = static_cast<double>( c.data[year][i] ) / sum * 100;
		string second = to_string( percent ) + "%";
		file << first << string( spaces, ' ' ) << second << endl;
	}

	file << endl << "Total: " << sum << endl << endl;

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
	for ( int i = 2; i < doc.GetColumnCount(); i++ ) {
		labels.push_back( doc.GetColumnName( i ) );
	}

	printVector( labels );

	vector<country> countries_vec;

	vector<thread> threads;
	threads.emplace_back( loaddata_csv, ref( countries_vec ), ref( countries ), ref( doc ) );

	ifstream f( "HDI.json" );
	json HDI = json::parse( f );

	vector<country_HDI> v_HDI;
	threads.emplace_back( loaddata_json, ref( v_HDI ), ref( HDI ) );

	//display countries and number of entries
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		int spaces = 50 - it->first.length();
		cout << it->first << string( spaces, ' ' ) << "Entries: " << it->second << endl;
	}

	cout << "Countries: " << countries.size() << endl;
	cout << "Total entries: " << doc.GetRowCount() << endl;

	cout << "Enter Country name:" << endl;
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
	auto it = countries.find( country_name );
	int iter = distance( countries.begin(), it );

	cout << "Choose year:" << endl;
	int year;
	cin >> year;

	char* buff = nullptr;
	_dupenv_s( &buff, nullptr, "USERPROFILE" );
	string desktop = buff;
	desktop += "\\Desktop\\";

	for ( auto& thread : threads ) {
		thread.join();
	}

	quickstats_deaths( countries_vec[iter], labels, year, desktop );

}