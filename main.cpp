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

// We need some fine defs for the URL segments
#define LOTTO_URI_PREFFIX "http://pcso-lotto-results-and-statistics.webnatin.com/"
#define LOTTO_URI_SUFFIX  "results.asp"

/**
 * This function gives back a timestamp...
 */
std::string timestamp(std::string format) {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];
	time (&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, format.c_str(), timeinfo);
	std::string timestamp(buffer);
	return timestamp;
}

/**
 * This function is for debug output...
 */
void debug(std::string message) {
	std::cout << timestamp("%d-%m-%Y %I:%M:%S") << " | " << message << std::endl;
}

/**
 * This is for quick replace of string...
 */
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
void get_results(std::string type, int color) {
	// We need some colors!!!
	const std::string colors[] = {
		"\x1b[0m",
		"\x1b[31;1m",
		"\x1b[32;1m",
		"\x1b[33;1m",
		"\x1b[34;1m",
		"\x1b[35;1m",
		"\x1b[36;1m",
		"\x1b[37;1m",
		"\x1b[38;1m",
	};

	// We need a file name...
	std::string file_name("results/" + type + ".csv");

	// Wee need a result URL...
	std::string result_url(LOTTO_URI_PREFFIX + type + LOTTO_URI_SUFFIX);

	// We need to get a handle on the file...
	std::ofstream file(file_name);

	// Check if we have opened the file...
	if (!file.is_open()) {
		debug("Error opening \"" + file_name + "\"");
		return;
	}

	// We need a curl request instance
	curlpp::Easy request;

	// Specify the URL
	request.setOpt(curlpp::options::Url(result_url));

	// Specify some headers
	std::list<std::string> headers;
	headers.push_back(HEADER_ACCEPT);
	headers.push_back(HEADER_USER_AGENT);
	request.setOpt(new curlpp::options::HttpHeader(headers));
	request.setOpt(new curlpp::options::FollowLocation(true));
	request.setOpt(new curlpp::options::Encoding("gzip"));

	// we need a string stream...
	std::ostringstream str_stream;

	// Configure curlpp to use stream
	request.setOpt(new curlpp::options::WriteStream(&str_stream));

	// Collect response
	request.perform();

	// Where do we store the body?
	std::string body = str_stream.str();

	// Parse HTML and create a DOM tree
	xmlDoc *doc = htmlReadDoc((xmlChar*)body.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	// Encapsulate raw libxml document in a libxml++ wrapper
	auto *r = xmlDocGetRootElement(doc);
	auto *root = new xmlpp::Element(r);

	// get the rows
	auto games   = root->find("//table/tr[position() > 1]/td[1]/text()");
	auto dates   = root->find("//table/tr[position() > 1]/td[3]/text()");
	auto results = root->find("//table/tr[position() > 1]/td[4]/b/text()");
	auto amounts = root->find("//table/tr[position() > 1]/td[5]/text()");
	auto winners = root->find("//table/tr[position() > 1]/td[6]/text()");

	// Added the header to the file...
	file << "\"Game\",\"Date\",\"Result\",\"Amount\",\"Winners\"" << std::endl;

	// loop the results
	for (int i = 0; i < games.size(); ++i) {
		// Now add the results for the result
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(games[i])->get_content())   << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(dates[i])->get_content())   << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(results[i])->get_content()) << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(amounts[i])->get_content()) << "\",";
		file << "\"" << trim((std::string) dynamic_cast<xmlpp::ContentNode*>(winners[i])->get_content()) << "\"";
		file << std::endl;

		// Add our magical DOT...
		std::cout << colors[color];
		std::cout << "●";
	}

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

	// Where do we store our threads? LOL!
	std::thread threads[result_types_count];

	// Start our threads!
	for (int i = 0; i < result_types_count; ++i) {
		threads[i] = std::thread(std::bind(get_results, result_types[i], i));
	}

	// What did we get from the threads doc?
	for (int i = 0; i < result_types_count; ++i) {
		// We need to get back to the main thread!
		threads[i].join();
	}

	// Wow!
	return 0;
}
