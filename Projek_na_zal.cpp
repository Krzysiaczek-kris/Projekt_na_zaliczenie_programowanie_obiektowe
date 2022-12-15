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

struct country_deaths {
	string name;
	string code;
	vector<vector<int>> data; // years and values

	friend ostream& operator<<( ostream& os, const country_deaths& c ) {
		os << c.name;
		return os;
	}
};

struct country_HDI {
	string name;
	string code;
	int rank;
	int gdi_group;
	int gii_rank;
	int rankdiff_hdi;
	string hdicode;
	vector<vector<double>> data; // years and values

	friend ostream& operator<<( ostream& os, const country_HDI& HDI ) {
		os << HDI.name;
		return os;
	}
};

struct country_Health {
	string name;
	string Healthcode;

	vector<vector<double>> data; // years and values

	friend ostream& operator<<( ostream& os, const country_Health& country_Health ) {
		os << country_Health.name;
		return os;
	}
};

void loaddata_deaths( vector<country_deaths>& v, map<string, int>& countries, const Document& doc ) {
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
		v.emplace_back( country_deaths{ it->first, code, data } );
	}
}

void loaddata_HDI( vector<country_HDI>& v, map<string, int> labels, const Document& doc )
{
	for ( int i = 0; i < doc.GetRowCount(); i++ ) {
		int col = 5;
		string name = doc.GetCell<string>( "country", i );
		string code = doc.GetCell<string>( "iso3", i );
		string hdicode = doc.GetCell<string>( "hdicode", i );
		int rank = doc.GetCell<int>( "hdi_rank_2021", i );
		int gdi_group = doc.GetCell<int>( "gdi_group_2021", i );
		int gii_rank = doc.GetCell<int>( "gii_rank_2021", i );
		int rankdiff_hdi = doc.GetCell<int>( "rankdiff_hdi_phdi_2021", i );
		vector<vector<double>> data;
		for ( auto it = labels.begin(); it != labels.end(); it++ ) {
			vector<double> year;
			for ( int j = 0; j < it->second; j++ ) {
				year.push_back( doc.GetCell<double>( col, i ) );
				col++;
			}
			data.push_back( year );
		}
		v.emplace_back( country_HDI{ name, code, rank, gdi_group, gii_rank, rankdiff_hdi, hdicode, data } );
	}

}

void loaddata_health( vector<country_Health>& v, map<string, int>& countries, const Document& doc )
{
	int line = 0;
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		vector<vector<double>> data;
		string code = doc.GetCell<string>( "Country Code", line );
		for ( int i = 0; i < it->second; i++ ) {
			vector<double> year;
			for ( int j = 4; j < doc.GetColumnCount(); j++ ) {
				year.push_back( doc.GetCell<double>( j, line ) );
			}
			data.push_back( year );
			line++;
		}
		v.emplace_back( country_Health{ it->first, code, data } );
	}
}



void quickstats_deaths( const country_deaths c, const vector<string>& labels, int year, const string& desktop ) {
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

	//DEATH DATA 
	Document doc_deaths( "annual_deaths_by_causes.csv", LabelParams( 0 ) );
	map <string, int> countries;
	//reading countries
	for ( int i = 0; i < doc_deaths.GetRowCount(); i++ ) {
		countries[doc_deaths.GetCell<string>( "country", i )]++;
	}

	vector<string> labels_deaths;
	for ( int i = 2; i < doc_deaths.GetColumnCount(); i++ ) {
		labels_deaths.push_back( doc_deaths.GetColumnName( i ) );
	}
	//print types of deaths
	printVector( labels_deaths );

	vector<country_deaths> countries_vec;

	vector<thread> threads;
	threads.emplace_back( loaddata_deaths, ref( countries_vec ), ref( countries ), ref( doc_deaths ) );

	//display countries and number of entries
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		int spaces = 50 - it->first.length();
		cout << it->first << string( spaces, ' ' ) << "Entries: " << it->second << endl;
	}

	cout << "Countries: " << countries.size() << endl;

	//HEALTH DATA -----------------------------------------------------------------------
	Document doc_health( "health.csv", LabelParams( 0 ) );
	vector<country_Health> v_Health;
	map <string, int> countries_health;

	for ( int i = 0; i < doc_health.GetRowCount(); i++ ) {
		countries_health[doc_health.GetCell<string>( "Country Name", i )]++;
	}

	vector<string> labels_health;
	for ( int i = 0; i < countries_health.begin()->second; i++ ) {
		labels_health.push_back( doc_health.GetCell<string>( "Series Name", i ) );
	}


	threads.emplace_back( loaddata_health, ref( v_Health ), ref( countries_health ), ref( doc_health ) );

	//HDI DATA ---------------------------------------------------------------------------
	Document doc_HDI( "HDI.csv", LabelParams( 0 ) );
	vector<country_HDI> v_HDI;
	map <string, int> labels_HDI;

	for ( int i = 5; i < doc_HDI.GetColumnCount(); i++ ) {
		if ( doc_HDI.GetColumnName( i ) == "gdi_group_2021" || doc_HDI.GetColumnName( i ) == "gii_rank_2021" || doc_HDI.GetColumnName( i ) == "rankdiff_hdi_phdi_2021" ) {
			continue;
		}
		string temp = doc_HDI.GetColumnName( i );
		temp.erase( temp.end() - 5, temp.end() );
		labels_HDI[temp]++;
	}
	
	threads.emplace_back( loaddata_HDI, ref( v_HDI ), ref( labels_HDI ), ref( doc_HDI ) );
	//REST OF THE PROGRAM ----------------------------------------------------------------

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
	int iter = static_cast<int>( distance( countries.begin(), it ) );

	cout << "Choose year:" << endl;
	int year;
	cin >> year;

	char* buff = nullptr;
	_dupenv_s( &buff, 0, "USERPROFILE" );
	string desktop = buff;
	desktop += "\\Desktop\\";

	for ( auto& thread : threads ) {
		thread.join();
	}

	quickstats_deaths( countries_vec[iter], labels_deaths, year, desktop );



}