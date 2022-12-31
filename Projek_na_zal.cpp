#include <iostream>
#include <thread>
#include <future>
#include <mutex>

#include "rapidcsv.h"
#include "stats.h"

using namespace std;
using namespace rapidcsv;

double aprox = 0.99;

mutex cout_mutex;

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
	string code;

	vector<vector<double>> data; // years and values

	friend ostream& operator<<( ostream& os, const country_Health& country_Health ) {
		os << country_Health.name;
		return os;
	}
};

struct result
{
	bool done;
	bool working;
	vector<double> res;
	double mean = 0.0;
	double stddev = 0.0;
	//int size = 0;
	int size_per_country = 0;
	int nans = 0;
	int ones = 0;
	int time = 0;
	vector<pair<string, int>> v;

	result(): done( false ), working( false ) {}

	friend ostream& operator<<( ostream& os, const result& r )
	{
		os << "Mean: " << r.mean << endl;
		os << "Stddev: " <<  r.stddev << endl;

		os << "size: " << r.res.size() << endl;
		os << "size(per country): " << r.size_per_country << endl;

		os << "Time taken: " << r.time << "ms" << endl;

		os << "Nans: " << r.nans << endl;
		os << "High values: " << r.ones << "\n\n\n";
		//cout_mutex.unlock();

		os << "Entries with corr over " << aprox << " per country:" << endl;
		//show 25 top results
		for ( int i = 0; i < 25; i++ ) {
			os << r.v[i].first << string( 50 - r.v[i].first.size(), ' ' ) << r.v[i].second << '\n';
		}
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

void loaddata_HDI( vector<country_HDI>& v, map<string, int> labels, const Document& doc ) {
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

void loaddata_health( vector<country_Health>& v, map<string, int>& countries, const Document& doc ) {
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

void quickstats_deaths( const country_deaths& c, const vector<string>& labels, int year, const string& desktop ) {
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

void death_health_stats( const vector<country_deaths>& countries_vec, const vector<country_Health>& v_Health, const vector<string>& labels_deaths, const vector<string>& labels_health, result& r ) {

		vector<double>stddevs;
		vector<double> all_results;
		map<string, int> countries_appearances;
		int ones = 0;
		int nans = 0;

		//time start
		auto start = chrono::high_resolution_clock::now();

		for ( int i = 0; i < labels_health.size(); i++ ) {
			for ( int j = 1; j < labels_deaths.size(); j++ ) { // first label is year, and we skip it
				vector<double> results;
				for ( auto v : countries_vec ) {
					//using code find same element in v_health
					auto it_Health = find_if( v_Health.begin(), v_Health.end(), [&v] ( const country_Health& c ) { return c.code == v.code; } );
					if ( it_Health == v_Health.end() ) { // no matching data
						continue;
					}
					int iter_Health = static_cast<int>( distance( v_Health.begin(), it_Health ) );

					vector<vector<int>> deaths = transposeMatrix( v.data ); // transpose matrix, to easily access values by years

					double res = spearman( deaths[j], v_Health[iter_Health].data[i] );
					//if res is nan, skip
					if ( _isnan( res ) ) {
						nans++;
						continue;
					}
					res = abs( res ); //abs value because we want to see how strong the correlation is, not if it's positive or negative
					if ( res > aprox ) {
						ones++;
						countries_appearances[v.name]++;
					}

					results.push_back( res );
					all_results.push_back( res );
				}

				stddevs.emplace_back( stddev( results ) );
			}
		}

		//sort map by second value
		vector<pair<string, int>> v;
		copy( countries_appearances.begin(), countries_appearances.end(), back_inserter( v ) );
		sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

		auto end = chrono::high_resolution_clock::now();

		r.res = all_results;
		r.mean = mean( all_results );
		r.stddev = mean( stddevs );
		r.size_per_country = all_results.size() / countries_appearances.size();
		r.nans = nans;
		r.ones = ones;
		r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
		r.v = v;
		r.done = true;
		r.working = false;
		

	lock_guard<mutex> lock( cout_mutex );
	cout << "\n\n\n Death and Health data: \n\n";
	cout << r;
	
}

void death_hdi_stats( const vector<country_deaths>& countries_vec, const vector<country_HDI>& v_HDI, const vector<string>& labels_deaths, const map <string, int>& labels_HDI, result& r ) {
	vector<double>stddevs;
	vector<double> all_results;
	map<string, int> countries_appearances;
	int ones = 0;
	int nans = 0;

	//time start
	auto start = chrono::high_resolution_clock::now();

	for ( auto it = labels_HDI.begin(); it != labels_HDI.end(); it++ ) {
		int temp = 0;
		for ( int j = 1; j < labels_deaths.size(); j++ ) { // first label is year, and we skip it
			vector<double> results;
			for ( auto v : countries_vec ) {
				//find same element in v_HDI
				auto it_HDI = find_if( v_HDI.begin(), v_HDI.end(), [&v] ( const country_HDI& c ) { return c.code == v.code; } );
				if ( it_HDI == v_HDI.end() ) {
					continue;
				}

				vector<vector<int>>deaths = transposeMatrix( v.data ); // transpose matrix, to easily access values by years
				double res = spearman( deaths[j], it_HDI->data[temp] );
				if ( _isnan( res ) ) {
					nans++;
					continue;
				}
				res = abs( res ); //abs value because we want to see how strong the correlation is, not if it's positive or negative
				if ( res > aprox ) {
					ones++;
					countries_appearances[v.name]++;
				}
				results.push_back( res );
				all_results.push_back( res );
			}
			stddevs.emplace_back( stddev( results ) );
		}
		temp++;
	}

	//sort map by second value
	vector<pair<string, int>> v;
	copy( countries_appearances.begin(), countries_appearances.end(), back_inserter( v ) );
	sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

	auto end = chrono::high_resolution_clock::now();

	r.res = all_results;
	r.mean = mean( all_results );
	r.stddev = mean( stddevs );
	r.size_per_country = all_results.size() / countries_appearances.size();
	r.nans = nans;
	r.ones = ones;
	r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
	r.v = v;
	r.done = true;
	r.working = false;

	lock_guard<mutex> lock( cout_mutex );
	cout << "\n\n\n Death and HDI data: \n\n";
	cout << r;

}

void health_hdi_stats( const vector<country_Health>& v_Health, const vector<country_HDI>& v_HDI, const vector<string>& labels_health, const map<string, int>& labels_HDI, result& r ) {
	vector<double> stddevs;
	vector<double> all_results;
	map<string, int> countries_appearances;
	int ones = 0;
	int nans = 0;

	//time start
	auto start = chrono::high_resolution_clock::now();

	for ( auto it = labels_HDI.begin(); it != labels_HDI.end(); it++ ) {
		int temp = 0;
	for ( int i = 0; i < labels_health.size(); i++ ) {
		vector<double> results;
		for ( auto v : v_Health ) {
			//find same element in v_HDI
			auto it_HDI = find_if( v_HDI.begin(), v_HDI.end(), [&v] ( const country_HDI& c ) { return c.code == v.code; } );
			if ( it_HDI == v_HDI.end() ) {
				continue;
			}

			double res = spearman( v.data[i], it_HDI->data[0] );
			if ( _isnan( res ) ) {
				nans++;
				continue;
			}
			res = abs( res ); //abs value because we want to see how strong the correlation is, not if it's positive or negative
			if ( res > aprox ) {
				ones++;
				countries_appearances[v.name]++;
			}
			results.push_back( res );
			all_results.push_back( res );
		}
		stddevs.emplace_back( stddev( results ) );
	}
	temp++;
	}

	//sort map by second value
	vector<pair<string, int>> v;
	copy( countries_appearances.begin(), countries_appearances.end(), back_inserter( v ) );
	sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

	auto end = chrono::high_resolution_clock::now();

	r.res = all_results;
	r.mean = mean( all_results );
	r.stddev = mean( stddevs );
	r.size_per_country = all_results.size() / countries_appearances.size();
	r.nans = nans;
	r.ones = ones;
	r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
	r.v = v;
	r.done = true;
	r.working = false;

	lock_guard<mutex> lock( cout_mutex );
	cout << "\n\n\n Health and HDI data: \n\n";
	cout << r;

}

void write_to_file( const vector<double>& v, const string& filename ) {
	ofstream file( filename );
	for ( auto d : v ) {
		file << d << endl;
	}
	file.close();
}


int main() {

	auto very_start = chrono::high_resolution_clock::now();
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

	vector<country_deaths> countries_vec;

	vector<thread> threads;
	threads.emplace_back( loaddata_deaths, ref( countries_vec ), ref( countries ), ref( doc_deaths ) );

	//display countries and number of entries
	for ( auto it = countries.begin(); it != countries.end(); it++ ) {
		int spaces = 50 - it->first.length();
		cout << it->first << string( spaces, ' ' ) << "Entries: " << it->second << endl;
	}

	cout << "Countries: " << countries.size() << endl;
	cout << "Above you can see the countries and the number of entries you have in the dataset." << endl;

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

	char* buff = nullptr;
	_dupenv_s( &buff, 0, "USERPROFILE" );
	string desktop = buff;
	desktop += "\\Desktop\\";

	
	cout << "\nWhat do you want to do with that data?" << endl;
	string input;
	cout << "1. See the correlation between the number of deaths and the health data - write cdhe" << endl;
	cout << "2. See the correlation between the number of deaths and the HDI data - write cdhd" << endl;
	cout << "3. See the correlation between the health data and the HDI data - write chh" << endl;
	cout << "4. All of the above - write all" << endl;
	cout << "5. See the statistics on the distribution of causes of death by country and year - write qsd" << endl;
	cout << "6. Settings - write settings" << endl;
	cout << "7. Exit - write exit" << endl;

	//cin >> input; //first input can be performed before the threads are finished
	//transform( input.begin(), input.end(), input.begin(), ::tolower );

	for ( auto& thread : threads ) {
		thread.join();
	}
	threads.clear();


	result result1, result2, result3;
	

	while ( input != "exit" ) {

		cin >> input;
		transform( input.begin(), input.end(), input.begin(), ::tolower );

		if ( input == "cdhe" || input == "1" ) {
			
			if ( result1.working ) continue;
			if ( result1.done )
			{
				cout << result1;
				continue;
			}
			result1.working = true;
			threads.emplace_back( death_health_stats, ref( countries_vec ), ref( v_Health ), ref( labels_deaths ), ref( labels_health ), ref( result1 ) );

		} else
		if ( input == "cdhd" || input == "2" )
		{
			if ( result2.working ) continue;
			if ( result2.done ) {
				cout << result2;
				continue;
			}
			result2.working = true;
			threads.emplace_back( death_hdi_stats, ref( countries_vec ), ref( v_HDI ), ref( labels_deaths ), ref( labels_HDI ), ref( result2 ) );

		} else
		if ( input == "cdhd" || input == "3" ) {
			if ( result3.working ) continue;
			if ( result3.done ) {
				cout << result3;
				continue;
			}
			result3.working = true;
			threads.emplace_back( health_hdi_stats, ref( v_Health ), ref( v_HDI ), ref( labels_health ), ref( labels_HDI ), ref( result3 ) );
		} else
		if ( input == "all" || input == "4" ) {

			if ( result1.done )
			{
				cout << "\n\n\nDeath and Health data: \n\n";
				cout << result1;
			}
			if ( result2.done ) {
				cout << "\n\n\nDeath and Health data: \n\n";
				cout << result2;
			}
			if ( result3.done ) {
				cout << "\n\n\nDeath and Health data: \n\n";
				cout << result3;
			}
			if ( !result1.working && !result1.done ) {
				result1.working == true;
				threads.emplace_back( death_health_stats, ref( countries_vec ), ref( v_Health ), ref( labels_deaths ), ref( labels_health ), ref( result1 ) );
			}
			if ( !result2.working && !result2.done ) {
				result2.working == true;
				threads.emplace_back( death_hdi_stats, ref( countries_vec ), ref( v_HDI ), ref( labels_deaths ), ref( labels_HDI ), ref( result2 ) );
			}
			if ( !result3.working && !result3.done ) {
				result3.working == true;
				threads.emplace_back( health_hdi_stats, ref( v_Health ), ref( v_HDI ), ref( labels_health ), ref( labels_HDI ), ref( result3 ) );
			}

		} else
		if ( input == "qsd" || input =="5" ) {
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
			quickstats_deaths( countries_vec[iter], labels_deaths, year, desktop );

		} else
		if ( input == "set" || input =="6" ) {

			cout << "PH" << endl;


		} else
		if ( input == "exit" || input == "7" ) {
			for ( auto& t : threads ) {
				if ( t.joinable() ) {
					cout << " Waiting for all threads to finish job" << endl;
					for ( auto& thread : threads ) {
						thread.join();
					}
					break;
				}
			}
			//cout << "Do you want to save your results?" << endl;
			
			
			auto very_end = chrono::high_resolution_clock::now();
			double time = chrono::duration_cast<chrono::milliseconds>( very_end - very_start ).count();
			time *= 0.0018;
			cout << "Assuming that 1,8 people die every second around: " << time << " people died while you were using this program." << endl;

			return 0;
			
		} else
		cout << "Wrong input!" << endl;

	}
	

	// Wait for all threads to finish
	for ( auto& thread : threads ) {
		thread.join();
	}
	//threads.clear();

	//write_to_file( result1, desktop + "result1.txt" );
	//write_to_file( result2, desktop + "result2.txt" );
	//write_to_file( result3, desktop + "result3.txt" );

	return 0;

}
