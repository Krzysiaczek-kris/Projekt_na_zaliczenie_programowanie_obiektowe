#include <iostream>
#include <thread>
#include <mutex>

#include "rapidcsv.h"
#include "stats.h"

using namespace std;
using namespace rapidcsv;

double aprox = 0.99;
int top_results = 25;

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

struct result {
	bool done;
	bool working;
	bool saved;
	vector<double> res;
	double mean = 0.0;
	double stddev = 0.0;
	//int size = 0;
	int size_per_country = 0;
	int nans = 0;
	int ones = 0;
	int time = 0;
	vector<pair<string, int>> v;

	result(): done( false ), working( false ), saved( false ) {}

	friend ostream& operator<<( ostream& os, const result& r ) {
		os << "Mean: " << r.mean << endl;
		os << "Stddev: " << r.stddev << endl;

		os << "size: " << r.res.size() << endl;
		os << "size(per country): " << r.size_per_country << endl;

		os << "Time taken: " << r.time << "ms" << endl;

		os << "Nans: " << r.nans << endl;
		os << "High values: " << r.ones << "\n\n\n";

		os << top_results << " entries with correlation over " << aprox << " per country:" << endl;

		for ( int i = 0; i < top_results; i++ ) {
			os << r.v[i].first << string( 50 - r.v[i].first.size(), ' ' ) << r.v[i].second << '\n';
		}
		return os;
	}
};

void loaddata_deaths( vector<country_deaths>& v, const map<string, int>& countries, const Document& doc ) {
	// Index of the current line in the csv file
	int line = 0;
	for (auto& countrie : countries)
	{
		// 2D vector to store the data for a country
		vector<vector<int>> data;
		// Country code
		string code = doc.GetCell<string>( "code", line );
		for ( int i = 0; i < countrie.second; i++ ) {
			// Vector to store the data for a year
			vector<int> year;

			for ( int j = 2; j < doc.GetColumnCount(); j++ ) {
				year.push_back( doc.GetCell<int>( j, line ) );
			}
			data.push_back( year );
			line++;
		}
		// Create a country_deaths object and add it to the vector
		v.emplace_back( country_deaths{countrie.first, code, data } );
	}
}

void loaddata_HDI( vector<country_HDI>& v, const map<string, int>& labels, const Document& doc ) {
	// loop through each row of the csv file
	for ( int i = 0; i < doc.GetRowCount(); i++ ) {
		// initialize variables to store information about the country
		int col = 5;
		string name = doc.GetCell<string>( "country", i );
		string code = doc.GetCell<string>( "iso3", i );
		string hdicode = doc.GetCell<string>( "hdicode", i );
		int rank = doc.GetCell<int>( "hdi_rank_2021", i );
		int gdi_group = doc.GetCell<int>( "gdi_group_2021", i );
		int gii_rank = doc.GetCell<int>( "gii_rank_2021", i );
		int rankdiff_hdi = doc.GetCell<int>( "rankdiff_hdi_phdi_2021", i );
		vector<vector<double>> data;
		// loop through each column of the csv file
		for (auto& label : labels)
		{
			vector<double> year;
			// loop through each year of data
			for ( int j = 0; j < label.second; j++ ) {
				year.push_back( doc.GetCell<double>( col, i ) );
				col++;
			}
			data.push_back( year );
		}
		// store the information about the country in the vector
		v.emplace_back( country_HDI{ name, code, rank, gdi_group, gii_rank, rankdiff_hdi, hdicode, data } );
	}
}

// This function reads in the data from the csv file.
// It uses the map to determine how many lines of data to read in for each country.
void loaddata_health( vector<country_Health>& v, const map<string, int>& countries, const Document& doc ) {
	// line keeps track of which line of data we are reading in.
	int line = 0;
	// For each country in the map
	for (auto& countrie : countries)
	{
		// Create a vector of vectors to store the data for each country
		vector<vector<double>> data;
		// Get the country code
		string code = doc.GetCell<string>( "Country Code", line );
		// For each year of data for the country
		for ( int i = 0; i < countrie.second; i++ ) {
			// Create a vector to store the data for a single year
			vector<double> year;
			// For each column in the data (the years)
			for ( int j = 4; j < doc.GetColumnCount(); j++ ) {
				// Read in the data for that year
				year.push_back( doc.GetCell<double>( j, line ) );
			}
			// Add the year data to the vector of data for the country
			data.push_back( year );
			// Move to the next line
			line++;
		}
		// Add the country data to the vector
		v.emplace_back( country_Health{countrie.first, code, data } );
	}
}

void quickstats_deaths( const country_deaths& c, const vector<string>& labels, int year, const string& desktop ) {
	// Opens file with name of country + year + "Death_Stats" + ".txt" on desktop
	ofstream file( desktop + c.name + "_" + to_string( year ) + "_Death_Stats.txt" );

	// Writes to the file
	file << "Country: " << c.name << endl;
	file << "Years: " << year << endl << endl;

	// Sets year to the index of the year in the data
	year -= c.data[0][0];

	const double sum = accumulate( c.data[year].begin(), c.data[year].end(), 0 );

	// Writes the death statistics to the file
	for ( int i = 1; i < labels.size(); i++ ) {
		string first = labels[i] + ": " + to_string( c.data[year][i] );
		double percent = static_cast<double>( c.data[year][i] ) / sum * 100;
		string second = to_string( percent ) + "%";
		file << first << string( 100 - first.length(), ' ' ) << second << endl;
	}

	// Writes the total to the file
	file << endl << "Total: " << sum << endl << endl;

	// Closes the file
	file.close();
}

void death_health_stats( const vector<country_deaths>& countries_vec, const vector<country_Health>& v_Health, const vector<string>& labels_deaths, const vector<string>& labels_health, result& r ) {
	vector<double>stddevs;
	vector<double> all_results;
	map<string, int> countries_appearances;
	int ones = 0;
	int nans = 0;

	//time start
	const auto start = chrono::high_resolution_clock::now();

	for ( int i = 0; i < labels_health.size(); i++ ) {
		for ( int j = 1; j < labels_deaths.size(); j++ ) { // first label is year, and we skip it
			vector<double> results;
			for ( auto v : countries_vec ) {
				//using code find same element in v_health
				auto it_Health = find_if( v_Health.begin(), v_Health.end(), [&v] ( const country_Health& c ) { return c.code == v.code; } );
				if ( it_Health == v_Health.end() ) { // no matching data
					continue;
				}
				const int iter_Health = static_cast<int>( distance( v_Health.begin(), it_Health ) ); // get index of matching element

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
					countries_appearances[v.name]++; //add country to map
				}

				results.push_back( res );
				all_results.push_back( res );
			}

			stddevs.emplace_back( stddev( results ) ); //add stddev to vector
		}
	}

	//sort map by second value
	vector<pair<string, int>> v( countries_appearances.begin(), countries_appearances.end() );
	sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

	const auto end = chrono::high_resolution_clock::now();

	r.mean = mean( all_results );
	r.stddev = mean( stddevs );
	r.size_per_country = all_results.size() / countries_appearances.size();
	r.nans = nans;
	r.ones = ones;
	all_results.shrink_to_fit();
	v.shrink_to_fit();
	r.res = std::move( all_results );
	r.v = std::move( v );
	r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
	r.done = true;
	r.working = false;

	//cout data and lock mutex
	lock_guard lock( cout_mutex );
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
	const auto start = chrono::high_resolution_clock::now();
	int temp = 0;
	for ( auto it = labels_HDI.begin(); it != labels_HDI.end(); it++ ) {
		
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
	vector<pair<string, int>> v( countries_appearances.begin(), countries_appearances.end() );
	sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

	const auto end = chrono::high_resolution_clock::now();
	
	r.mean = mean( all_results );
	r.stddev = mean( stddevs );
	r.size_per_country = all_results.size() / countries_appearances.size();
	r.nans = nans;
	r.ones = ones;
	all_results.shrink_to_fit();
	v.shrink_to_fit();
	r.res = std::move( all_results );
	r.v = std::move( v );
	r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
	r.done = true;
	r.working = false;

	//cout data and lock mutex
	lock_guard lock( cout_mutex );
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
	const auto start = chrono::high_resolution_clock::now();

	for ( auto it = labels_HDI.begin(); it != labels_HDI.end(); it++ ) {
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
	}

	//sort map by second value
	vector<pair<string, int>> v( countries_appearances.begin(), countries_appearances.end() );
	sort( v.begin(), v.end(), [] ( const pair<string, int>& a, const pair<string, int>& b ) { return a.second > b.second; } );

	const auto end = chrono::high_resolution_clock::now();

	
	r.mean = mean( all_results );
	r.stddev = mean( stddevs );
	r.size_per_country = all_results.size() / countries_appearances.size();
	r.nans = nans;
	r.ones = ones;
	all_results.shrink_to_fit();
	v.shrink_to_fit();
	r.v = std::move( v );
	r.res = std::move( all_results );
	r.time = chrono::duration_cast<chrono::milliseconds>( end - start ).count();
	r.done = true;
	r.working = false;

	//cout data and lock mutex
	lock_guard lock( cout_mutex );
	cout << "\n\n\n Health and HDI data: \n\n";
	cout << r;
}
// write vector with results to file
void write_to_file( const vector<double>& v, const string& filename ) {
	ofstream file( filename );
	for ( const auto d : v ) {
		file << d << endl;
	}
	file.close();
	cout << "File " << filename << " written successfully." << endl;
}

void write_info() {
	cout << "1. See the correlation between the number of deaths and the health data - write cdhe" << endl;
	cout << "2. See the correlation between the number of deaths and the HDI data - write cdhd" << endl;
	cout << "3. See the correlation between the health data and the HDI data - write chh" << endl;
	cout << "4. All of the above - write all" << endl;
	cout << "5. See the statistics on the distribution of causes of death by country and year - write qsd" << endl;
	cout << "6. Settings - write settings" << endl;
	cout << "7. Exit - write exit" << endl;
}

int main() {
	const auto very_start = chrono::high_resolution_clock::now();
	//DEATH DATA
	Document doc_deaths( "annual_deaths_by_causes.csv", LabelParams( 0 ) ); //create a document object
	map <string, int> countries; //create a map with string keys and int values
	//reading countries
	for ( int i = 0; i < doc_deaths.GetRowCount(); i++ ) { //iterate over each row in the dataset
		countries[doc_deaths.GetCell<string>( "country", i )]++; //increase the value of the corresponding key
	}

	vector<string> labels_deaths; //create a vector of strings
	for ( int i = 2; i < doc_deaths.GetColumnCount(); i++ ) { //iterate over each column in the dataset
		labels_deaths.push_back( doc_deaths.GetColumnName( i ) ); //add the column name to the vector
	}

	vector<country_deaths> countries_vec; //create a vector of country_deaths objects

	vector<thread> threads; //create a vector of threads
	threads.emplace_back( loaddata_deaths, ref( countries_vec ), ref( countries ), ref( doc_deaths ) ); //add a thread to the vector, the thread executes the function loaddata_deaths with the parameters countries_vec, countries, and doc_deaths

	//display countries and number of entries
	for ( auto& country : countries )
	{ //iterate over the map
		cout << country.first << string( 50 - country.first.length(), ' ' ) << "Entries: " << country.second << endl; //output the country name, spaces, and the number of entries
	}
	cout << "Countries: " << countries.size() << endl; //output the number of countries
	cout << "Above you can see the countries and the number of entries you have in the dataset." << endl; //output an explanation

	//HEALTH DATA -----------------------------------------------------------------------
	//this code loads the data from the health.csv file and stores it in a vector of type country_health
	//it is similar to the code above

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
	// this code loads the data from the HDI.csv file and stores it in a vector of type country_HDI
	// the labels_HDI map is used to store the names of the countries and the number of times they appear in the file
	// this is used later to determine the number of countries in the data set
	// the function loaddata_HDI() is called and passed the vector, map, and the document

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

	//get the desktop path
	char* buff = nullptr;
	_dupenv_s( &buff, 0, "USERPROFILE" );
	string desktop = buff;
	desktop += "\\Desktop\\";
	delete buff;

	string input;
	cout << "\nWhat do you want to do with that data?" << endl;

	write_info();

	//wait for the threads to finish
	for ( auto& thread : threads ) {
		thread.join();
	}
	threads.clear();

	//create the results objects
	result result1, result2, result3;

	//program loop
	while ( true ) {
		cin >> input;
		transform( input.begin(), input.end(), input.begin(), ::tolower ); //convert the input to lowercase

		if ( input == "cdhe" || input == "1" ) {
			if ( result1.working ) continue; //if the thread is already working, skip this iteration
			if ( result1.done ) {
				cout << result1; //output the results
				if ( !result1.saved ) {
					cout << "Do you want to save the results to a file? (y/n)" << endl;
					cin >> input;
					transform( input.begin(), input.end(), input.begin(), ::tolower );
					if ( input == "yes" || input == "y" ) {
						write_to_file( result1.res, desktop + "cdhe.txt" ); //write the results to a file
					}
				}
				continue;
			}
			result1.working = true; //set the working flag to true
			//create a thread and run the death_health_stats function with the parameters
			threads.emplace_back( death_health_stats, ref( countries_vec ), ref( v_Health ), ref( labels_deaths ), ref( labels_health ), ref( result1 ) );
		}
		else
			if ( input == "cdhd" || input == "2" ) {
				if ( result2.working ) continue;
				if ( result2.done ) {
					cout << result2;
					if ( !result2.saved ) {
						cout << "Do you want to save the results to a file? (y/n)" << endl;
						cin >> input;
						transform( input.begin(), input.end(), input.begin(), ::tolower );
						if ( input == "yes" || input == "y" ) {
							write_to_file( result2.res, desktop + "cdhd.txt" );
						}
					}
					continue;
				}
				result2.working = true;
				threads.emplace_back( death_hdi_stats, ref( countries_vec ), ref( v_HDI ), ref( labels_deaths ), ref( labels_HDI ), ref( result2 ) );
			}
			else
				if ( input == "cdhh" || input == "3" ) {
					if ( result3.working ) continue;
					if ( result3.done ) {
						if ( !result3.saved ) {
							cout << result3;
							if ( !result3.saved ) {
								cout << "Do you want to save the results to a file? (y/n)" << endl;
								cin >> input;
								transform( input.begin(), input.end(), input.begin(), ::tolower );
								if ( input == "yes" || input == "y" ) {
									write_to_file( result3.res, desktop + "chh.txt" );

								}
							}
						}
						continue;
					}
					result3.working = true;
					threads.emplace_back( health_hdi_stats, ref( v_Health ), ref( v_HDI ), ref( labels_health ), ref( labels_HDI ), ref( result3 ) );
				}
				else
					if ( input == "all" || input == "4" ) {
						if ( result1.done ) {
							cout << "\n\n\nDeath and Health data: \n\n";
							cout << result1;
						}
						if ( result2.done ) {
							cout << "\n\n\nDeath and HDI data: \n\n";
							cout << result2;
						}
						if ( result3.done ) {
							cout << "\n\n\nHealth and HDI data: \n\n";
							cout << result3;
						}
						if ( !result1.working && !result1.done ) {
							result1.working = true;
							threads.emplace_back( death_health_stats, ref( countries_vec ), ref( v_Health ), ref( labels_deaths ), ref( labels_health ), ref( result1 ) );
						}
						if ( !result2.working && !result2.done ) {
							result2.working = true;
							threads.emplace_back( death_hdi_stats, ref( countries_vec ), ref( v_HDI ), ref( labels_deaths ), ref( labels_HDI ), ref( result2 ) );
						}
						if ( !result3.working && !result3.done ) {
							result3.working = true;
							threads.emplace_back( health_hdi_stats, ref( v_Health ), ref( v_HDI ), ref( labels_health ), ref( labels_HDI ), ref( result3 ) );
						}
					}
					else
						if ( input == "qsd" || input == "5" ) {
							cout << "Enter Country name:" << endl;
							string country_name;
							cin >> country_name;
							//capitalize first letter of each word in country name
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
							for (auto& i : countries_vec[iter].data) {
								cout << i[0] << endl;
							}
							int year;
							cin >> year;
							quickstats_deaths( countries_vec[iter], labels_deaths, year, desktop );
						}
						else
							if ( input == "set" || input == "6" ) {
								system( "cls" ); //clear screen
								cout << "Current settings: " << endl;
								cout << "1. Listing " << top_results << " top results" << endl;
								cout << "2. High correlation value ( 0 - 1 ), now it is " << aprox << ", you need to change it before calculating results" << endl;
								cout << "3. Exit from settings" << endl;
								string input_settings;

								while ( true ) {
									cin >> input_settings;
									if ( input_settings == "1" ) {
										cout << "Enter new value for top results: " << endl;
										cin >> top_results;
										cout << "Value has been changed" << endl;
									}
									else
										if ( input_settings == "2" ) {
											cout << "Enter new high correlation value: " << endl;
											cin >> aprox;
											if ( aprox > 1 || aprox < 0 ) {
												cout << "Value is out of range, it has to be between 0 and 1" << endl;
											}
											else {
												cout << "Value has been changed" << endl;
											}
										}
										else
											if ( input_settings == "3" ) {
												system( "cls" );
												write_info();
												break;
											}
								}
								
							}
							else
								if ( input == "exit" || input == "7" ) {

									for ( auto& t : threads ) {
										t.detach();
									}
									threads.clear();

									if ( ( result1.done && !result1.saved ) || ( result2.done && !result2.saved ) || ( result3.done && !result3.saved ) ) { //if any of the results are done ask if user wants to save them
											cout << "Do you want to save all your results?" << endl;
											cin >> input;
											transform( input.begin(), input.end(), input.begin(), ::tolower );
											if ( input == "yes" || input == "y" ) {
												if ( result1.done && !result1.saved ) {
													write_to_file( result1.res, desktop + "cdhe.txt" );
													result1.saved = true;
												}
												if ( result2.done && !result2.saved ) {
													write_to_file( result2.res, desktop + "cdhd.txt" );
													result2.saved = true;
												}
												if ( result3.done && !result3.saved ) {
													write_to_file( result3.res, desktop + "chh.txt" );
													result3.saved = true;
												}
											}
									}

									const auto very_end = chrono::high_resolution_clock::now();
									double time = chrono::duration_cast<chrono::milliseconds>( very_end - very_start ).count();
									time *= 0.0018;
									cout << "Assuming that 1,8 people die every second around: " << time << " people died while you were using this program." << endl;

									return 0;
								}
								else
									if ( input == "help" || input == "h" ) write_info();
									else
										cout << "Wrong input!" << endl;
	}

	//for ( auto& thread : threads ) {
	//	thread.join();
	//}
	//threads.clear();

	//return 0;
}