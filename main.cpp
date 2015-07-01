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
#include <cstdlib>

// Back to the threads
#include <thread>

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
void get_results(std::string result_url, std::ofstream* file, std::string color) {
	// we need a string stream...
	std::ostringstream response_stream;

	// We need this magic snip to do cleanup
	curlpp::Cleanup cleanup;

	// We need a curl request instance
	curlpp::Easy request;

	// We need some details about the request!
	request.setOpt<curlpp::options::Url>(result_url);
	request.setOpt<curlpp::options::Encoding>("gzip, deflate");
	request.setOpt<curlpp::options::WriteStream>(&response_stream);

	// Run the request
	request.perform();

	// Parse HTML and create a DOM tree
	xmlDoc *xml_document = htmlReadDoc((xmlChar*)response_stream.str().c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

	// Encapsulate raw libxml document in a libxml++ wrapper
	auto *xml_root = xmlDocGetRootElement(xml_document);
	auto *xmlpp_root = new xmlpp::Element(xml_root);

	// get the rows
	auto results        = xmlpp_root->find("//table/tr[position() > 1]/td/b/text()");
	auto result_details = xmlpp_root->find("//table/tr[position() > 1]/td/text()");

	// loop the results
	for (int i = 0; i < results.size(); i++) {
		// we need a string stream...
		std::ostringstream result_stream;

		// Now add the results for the result
		result_stream << "\"" << trim(dynamic_cast<xmlpp::ContentNode*>(result_details[i*7+0])->get_content()) << "\",";
		result_stream << "\"" << trim(dynamic_cast<xmlpp::ContentNode*>(result_details[i*7+2])->get_content()) << "\",";
		result_stream << "\"" << trim(dynamic_cast<xmlpp::ContentNode*>(result_details[i*7+4])->get_content()) << "\",";
		result_stream << "\"" << trim(dynamic_cast<xmlpp::ContentNode*>(result_details[i*7+5])->get_content()) << "\",";
		result_stream << "\"" << trim(dynamic_cast<xmlpp::ContentNode*>(result_details[i*7+6])->get_content()) << "\",";
		result_stream << std::endl;

		// Write our result line to the file stream
		*file  << result_stream.str().c_str();

		// Add our magical DOT...
		std::cout << color << "●";
	}

	// House keeping. Can we come in? :P
	delete xmlpp_root;
	xmlFreeDoc(xml_document);
}

/**
 * This is MAIN, if you don't know it, go study CPP
 */
int main(int argc, char *argv[])
{
	// What type of results do we need?
	const std::string result_types[] = {
		"2-d",
		"3-d",
		"4-d",
		"6-42",
		"6-45",
		"6-49",
		"6-55",
		"6-d",
	};

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

	// We need some fine consts for the URL segments
	const std::string url_preffix = "http://pcso-lotto-results-and-statistics.webnatin.com/";
	const std::string url_suffix  = "results.asp";

	// How many result types do we run?
	const int result_types_count = sizeof(result_types) / sizeof(std::string);

	// Where do we store our threads? LOL!
	std::thread threads[result_types_count];

	// Where to put the results?
	const std::string file_name("results/all.csv");

	// Open the file handle to the results file...
	std::ofstream file(file_name);

	// Check if we have opened the file...
	if (!file.is_open()) {
		debug("Error opening \"" + file_name + "\"");
		return EXIT_FAILURE;
	}

	// Added the header to the file...
	file << "\"Game\",\"Date\",\"Result\",\"Amount\",\"Winners\"" << std::endl;

	// Start our threads!
	for (int i = 0; i < result_types_count; ++i) {
		threads[i] = std::thread(std::bind(get_results, url_preffix + result_types[i] + url_suffix, &file, colors[i]));
	}

	// What did we get from the threads doc?
	for (int i = 0; i < result_types_count; ++i) {
		// We need to get back to the main thread!
		threads[i].join();
	}

	// Close the file handle
	file.close();

	// Add a newline at the end of the run... cause it breaks anything running
	// after our script... LOL
	std::cout << std::endl << colors[0];

	// Wow!
	return EXIT_SUCCESS;
}
