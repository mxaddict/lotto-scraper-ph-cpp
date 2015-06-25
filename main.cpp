// BOOST!
#include <boost/algorithm/string.hpp>

// Load up the libxml
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml++/libxml++.h>

// We need curl? YES!
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

// Emmmm..... Strings and io and f streams?... my favorite...
#include <iostream>
#include <string>
#include <fstream>

// Back to the threads
#include <thread>

// This is just for sakes
#define HEADER_ACCEPT     "Accept: text/html"
#define HEADER_USER_AGENT "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/43.0.2357.125 Safari/537.36"

// We need some colors!!!
#define CLR_0 "\x1b[0m"
#define CLR_1 "\x1b[31;1m"
#define CLR_2 "\x1b[32;1m"
#define CLR_3 "\x1b[33;1m"
#define CLR_4 "\x1b[34;1m"
#define CLR_5 "\x1b[35;1m"
#define CLR_6 "\x1b[36;1m"
#define CLR_7 "\x1b[37;1m"
#define CLR_8 "\x1b[38;1m"

// We need some fine defs for the URL segments
#define LOTTO_URI_PREFFIX "http://pcso-lotto-results-and-statistics.webnatin.com/"
#define LOTTO_URI_SUFFIX  "results.asp"

void replace(std::string& str, const std::string& from, const std::string& to) {
	// get the first pos
	size_t start_pos = str.find(from);

	// loop until we have removed all
	while (start_pos != std::string::npos) {
		// remove it
		str.replace(start_pos, from.length(), to);

		// try to find another
		start_pos = str.find(from);
	}
}

/**
 * We need an alias to boost's string trim...
 */
std::string trim(std::string string) {
	replace(string, "\u00A0"," ");   // Replaces NBSPs normal space...
	boost::algorithm::trim(string);  // Trims normal white space...
	return string;                   // Give it back man...
}

/**
 * This function actually parses the results then stores them all in a CSV
 * that you can open in a spreadsheet :)
 */
void get_results(std::string type, std::string color) {
	// We need a file name...
	std::string file_name("results/" + type + ".csv");

	// We need to get a handle on the file...
	std::ofstream file(file_name);

	// Check if we have opened the file...
	if (!file.is_open()) {
		std::cout << "Error opening \"" << file_name << "\"" << std::endl;
		return;
	}

	// Added the header to the file...
	file << "\"Game\",\"Date\",\"Result\",\"Amount\",\"Winners\"" << std::endl;

	// We need a curl request instance
	curlpp::Easy request;

	// Specify the URL
	request.setOpt(curlpp::options::Url(LOTTO_URI_PREFFIX + type + LOTTO_URI_SUFFIX));

	// Specify some headers
	std::list<std::string> headers;
	headers.push_back(HEADER_ACCEPT);
	headers.push_back(HEADER_USER_AGENT);
	request.setOpt(new curlpp::options::HttpHeader(headers));
	request.setOpt(new curlpp::options::FollowLocation(true));

	// Configure curlpp to use stream
	std::ostringstream responseStream;
	curlpp::options::WriteStream streamWriter(&responseStream);
	request.setOpt(streamWriter);

	// Collect response
	request.perform();
	std::string re = responseStream.str();

	// Parse HTML and create a DOM tree
	xmlDoc* doc = htmlReadDoc((xmlChar*)re.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	// Encapsulate raw libxml document in a libxml++ wrapper
	auto *r = xmlDocGetRootElement(doc);
	auto *root = new xmlpp::Element(r);

	// get the rows
	auto games   = root->find("//table/tr[position() > 1]/td[1]/text()");
	auto dates   = root->find("//table/tr[position() > 1]/td[3]/text()");
	auto results = root->find("//table/tr[position() > 1]/td[4]/b/text()");
	auto amounts = root->find("//table/tr[position() > 1]/td[5]/text()");
	auto winners = root->find("//table/tr[position() > 1]/td[6]/text()");

	std::cout << color << "\n" ;
	std::cout << "games"   << ": " << games.size()    << ", ";
	std::cout << "dates"   << ": " << dates.size()    << ", ";
	std::cout << "results" << ": " << results.size()  << ", ";
	std::cout << "amounts" << ": " << amounts.size()  << ", ";
	std::cout << "winners" << ": " << winners.size();
	std::cout << std::endl;

	// loop the results
	for (int i = 0; i < games.size(); ++i) {
		// Now add the results for the result
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(games[i])->get_content())   << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(dates[i])->get_content())   << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(results[i])->get_content()) << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(amounts[i])->get_content()) << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(winners[i])->get_content()) << "\"";
		file << std::endl;
		std::cout << "●";
	}
	std::cout << CLR_0 ;

	// House keeping. Can we come in? :P
	delete root;
	xmlFreeDoc(doc);
	file.close();
}

/**
 * This is MAIN, if you don't know it, go study CPP
 */
int main(int argc, char *argv[])
{
	// How many result types do we run?
	int result_types_count = 8;

	// What type of results do we need?
	std::string result_types[] = {
		"2-d",
		"3-d",
		"4-d",
		"6-42",
		"6-45",
		"6-49",
		"6-55",
		"6-d",
	};

	// Packing up colors
	std::string colors[] = {CLR_1,CLR_2,CLR_3,CLR_4,CLR_5,CLR_6,CLR_7,CLR_8};

	// Where do we store our threads? LOL!
	std::thread threads[result_types_count];

	// Start our threads!
	for (int i = 0; i < result_types_count; ++i) {
		threads[i] = std::thread(std::bind(get_results, result_types[i], colors[i]));
	}

	// What did we get from the threads doc?
	for (int i = 0; i < result_types_count; ++i) {
		// We need to get back to the main thread!
		threads[i].join();
	}

	// Wow!
	return 0;
}
