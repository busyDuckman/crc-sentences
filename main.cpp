/**
 * @file main.cpp
 * @author Warren Creemers
 * @date 24th Jan, 2019.
 * @version 0.9
 *
 * A tool to create "autological sentences" for testing/fun, ie: sentences that describe themselves.
 */
#define CRCPP_USE_CPP11
#include "3rd_party/CRC.h"

#include <iomanip>
#include <cstdint>
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <vector>
#include <chrono>

#include <ctype.h>
#include <cmath>
#include <bitset>

using std::chrono::duration_cast;
using std::chrono::milliseconds;

// 512 different strings for the same CRC
const int maxSentenceOperations = 0b100000000;

// The system will report a near miss if the crc is within +/- this error margin,
static const int nearMissDistance = 25;

// percentage complete counter
volatile int percentComplete = -1;

// Forward declarations, doxygen is in the definition.
std::string generateSentence(const int operation, const std::string & crcString);
std::string getInfoString(long i, int operation, long hash);
std::string createCRCString(int crcValue, bool upperCase);
void testSentences(const uint32_t start_inc, const uint32_t end_ex, bool reportPercentComplete);

/**
 * Generates random sentences and dumps them to stdout.
 * Uses threading.
 */
int main()
{
    // Enable thousands separators
    std::cout.imbue(std::locale(""));

    // Print a synopsis.
    std::cout << "A tool to create \"autological sentences\" for testing/fun, ie: sentences that describe themselves"
              << std::endl;
    std::cout << "\tSee source code to configure or make changes." << std::endl << std::endl;

    // Detect cpu concurrency
    int numThreads = std::thread::hardware_concurrency();
    if(numThreads == 0) {
        std::cerr << "Could not detect cpu cores properly, using 1 thread." << std::endl;
        numThreads = 1;
    }
    else {
        std::cout << "Detected ability to use " << numThreads << " threads." << std::endl;
        numThreads = std::max(numThreads-1, 1); // leave a thread spare if possible.
    }

    // Search bounds
    uint32_t start = 0;
    uint32_t length = 0xffffffff;
    uint32_t bucketSize = length / numThreads;

    // Threads
    std::vector<std::thread> threads;

    // Launch a separate thread for each part of the range being investigated
    for(int i=0; i<numThreads; i++)
    {
        uint32_t tStart = start + (i * bucketSize);
        uint32_t tEnd = std::min(tStart+bucketSize, start+length);

        // this first thread launched is going to give a percent complete feedback to the console.
        bool isReporterThread = i==0;
        
        // start the thread
        threads.emplace_back(testSentences, tStart, tEnd, isReporterThread);
    }

    // join all threads
    for(std::thread & t : threads) {
        if(t.joinable()) {
            t.join();
        }
    }

    // done.
    return 0;
}



/**
 * Generates a sentence.
 * @param operation A number controlling which type of sentance to create (>= 0 and < maxSentenceOperations)
 * @param crcString A string representing a CRC.
 * @return
 */
std::string generateSentence(const int operation, const std::string & crcString)
{
    // parse opCode
    int basicText = operation & 0b11;
    bool capitalFirstLetter =  (operation & 0b100) != 0;
    bool fullStop =  (operation & 0b1000) != 0;
    bool col =  (operation & 0b10000) != 0;
    int openingPhrase = (operation & 0b1100000) >> 5;
    bool appendLength = (operation & 0b10000000) != 0 ;

    // output stream
    std::ostringstream out;

    // Start sentance.
    switch(openingPhrase) {
        case 0:
            // no opening
            break;
        case 1:
            out << (capitalFirstLetter ? "B" : "b");
            out << "elieve it or not, ";
            capitalFirstLetter = false;
            break;
        case 2:
            out << (capitalFirstLetter ? "U" : "u");
            out << "seful for testing, ";
            capitalFirstLetter = false;
            break;
        case 3:
            out << (capitalFirstLetter ? "H" : "h");
            out << "andily, ";
            capitalFirstLetter = false;
            break;
    }

    // Sentence body
    switch(basicText) {
        case 0:
            out << (capitalFirstLetter ? "T" : "t");
            out << "his text has a CRC of";
            out << (col ? ": " : " ");
            out << crcString;
            break;
        case 1:
            out << (capitalFirstLetter ? "T" : "t");
            out << "his string has a CRC of";
            out << (col ? ": " : " ");
            out << crcString;
            break;
        case 2:
            out << (capitalFirstLetter ? "T" : "t");
            out << "his has a CRC of";
            out << (col ? ": " : " ");
            out << crcString;
            break;
        case 3:
            //nb: not using lower case 'i' for self
            out << (capitalFirstLetter ? "I " : "I happen to ");
            out << "have a CRC value of";
            out << (col ? ": " : " ");
            out << crcString;
            break;
    }

    // Append a length string
    if(appendLength) {
        out << " and a length of ";

        // calc string length including the fullstop, that may follow
        int strLen = out.str().length() + (fullStop ? 1 : 0);

        // calc string length including the number used to store the string length.
        int charsToStoreLen =  (int) log10((double) strLen) + 1;
        strLen += charsToStoreLen;

        // and then handle the longer strLen, incrementing charsToStoreLen
        int newCharsToStoreLen =  (int) log10((double) strLen) + 1;
        if(newCharsToStoreLen > charsToStoreLen) {
            strLen++;
        }

        // Done.
        out << std::dec << strLen;
    }

    // Add a fullstop
    if(fullStop) {
        out << ".";
    }

    // Done.
    return out.str();
}

/**
 * Generates and tests sentences for a given CRC value range.
 * @param start_inc Start index (inclusive)
 * @param end_ex End index (exclusive)
 * @param reportPercentComplete True if function should report its percent complete,
 */
void testSentences(const uint32_t start_inc, const uint32_t end_ex, bool reportPercentComplete)
{
    // get the start time
    auto startTime = std::chrono::high_resolution_clock::now();

    // loop through the integer range assigned to this thread
    for(uint32_t i=start_inc; i<end_ex; i++)
    {
        // report percentage complete
        if(reportPercentComplete && (i % 0xff) == 0) {
            int per = (int)((((double) i-start_inc) / (double)(end_ex - start_inc)) * 100.0);
            if(per != percentComplete) {
                percentComplete = per;
                std::cout << per << "% complete." <<std::endl;
            }
        }

        // loop uppercase / lowercase
        for(int c=0; c<2; c++)
        {
            // create the crc string
            std::string crcString = createCRCString(i, c == 1);

            // loop through different sentance types
            for (int operation = 0; operation < maxSentenceOperations; operation++)
            {
                // create a sentence and calculate its cCRC
                std::string sentence = generateSentence(operation, crcString);
                std::uint32_t crc = CRC::Calculate(sentence.c_str(), sentence.length(), CRC::CRC_32());

                // Check against actual crc.
                if (crc == i) {
                    std::cout << "--------------------------------------------" << std::endl;
                    std::cout << "HIT: " << getInfoString(i, operation, crc) << std::endl;
                    std::cout << sentence << std::endl;
                    std::cout << "--------------------------------------------" << std::endl;
                }
                else if (std::abs((long) crc - (long) i) < (nearMissDistance)) {
                    // We report near misses (within 100), because this allows us estimate likelihood of a hit over a given time.
                    std::cout << "NEAR MISS "<< getInfoString(i, operation, crc) << ": "<< sentence << std::endl;
                }
            }

            // exit loop early, in the event there are no letters (changing capitalisation has no effect)
            int numLetters = (int)std::count_if(crcString.begin(), crcString.end(), [](char c){return isalpha(c);});
            if (numLetters == 0) {
                break;
            }
        } // end for c

    } // end for i

    // report duration
    auto finishTime = std::chrono::high_resolution_clock::now();
    milliseconds diff = duration_cast<milliseconds>(finishTime - startTime);
    std::cout << "done: " << std::dec << start_inc << " to " << end_ex
              << " in " << diff.count() << "ms" << std::endl;
}

/**
 * Turns loop params in testCRCThread, into useful debug text.
 */
std::string getInfoString(long i, int operation, long hash)
{
    std::bitset<9> opBitset(operation);

    std::ostringstream out;
    out << "(i=" << i << ", op=" << opBitset << ", dist=" << std::dec << (hash - i) << ")";
    return out.str();
}

/**
 * Gets the string representation of a CRC string.
 *
 * @return 8 character, 0 padded hex string.
 */
std::string createCRCString(int crcValue, bool upperCase)
{
    std::ostringstream out;
    out << std::setw(8)       // set width to 8 chars
        << std::setfill('0')  // pad with 0's
        << std::hex           // convert to hex
        << crcValue;
    std::string crcString = out.str();

    // case conversion is done in place.
    if(upperCase) {
        for (auto & letter: crcString) letter = toupper(letter);
    }

    return crcString;
}



