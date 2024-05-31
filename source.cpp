/*
To compile this program, you need to have libcurl installed. On Debian, you can install it with:

    sudo apt-get install libcurl4-openssl-dev

Then, you can compile the program with:

    g++ -o downloader source.cpp -lcurl

This will create an executable named 'downloader'.

To run the program, use the following syntax:

    ./downloader --url <URL> --destination_file <destination_file> [--attempts <number_of_attempts>]

Here's an example:

    ./downloader --url http://example.com/file.txt --destination_file /home/user/file.txt --attempts 3

This will download the file at 'http://example.com/file.txt' and save it to '/home/user/file.txt'. If the download fails, it will retry up to 3 times.
*/

#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <getopt.h>

// This function will be called by libcurl as soon as there is data received that needs to be saved
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream); // write directly to the file
    return written;
}

int main(int argc, char *argv[]) {
    CURL* curl;
    FILE* fp;
    CURLcode res;
    const char* url = NULL;
    const char* outfilename = NULL;
    int attempts = 1;

    // Define the long options
    static struct option long_options[] = {
        {"url", required_argument, 0, 'u'},
        {"destination_file", required_argument, 0, 'd'},
        {"attempts", required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    // Process the options
    while ((c = getopt_long(argc, argv, "u:d:a:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'u':
                url = optarg;
                break;
            case 'd':
                outfilename = optarg;
                break;
            case 'a':
                attempts = std::stoi(optarg);
                break;
            default:
                std::cerr << "Invalid option" << std::endl;
                return 1;
        }
    }

    // Initialize the curl session
    curl = curl_easy_init();
    if (curl) {
        // Open the file
        fp = fopen(outfilename,"wb");
        if(!fp) {
            std::cerr << "Could not open file: " << outfilename << std::endl;
            return 1;
        }
        // Set the URL to download
        curl_easy_setopt(curl, CURLOPT_URL, url);
        // Set the write function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        // Set the file pointer to pass to the write function
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        // Set the CURLOPT_FAILONERROR option
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        // Perform the request, retrying if necessary
        for(int i = 0; i < attempts; i++) {
            res = curl_easy_perform(curl);
            if(res == CURLE_OK) {
                break;
            } else {
                // If the request did not complete successfully, print the error information
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                if(res == CURLE_HTTP_RETURNED_ERROR) {
                    long response_code;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                    std::cerr << "HTTP response code: " << response_code << std::endl;
                }
                if(i < attempts - 1) {
                    std::cerr << "Retrying..." << std::endl;
                }
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return 0;
}