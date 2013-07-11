// $Id$

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include "util/Logger.h"
#include "util/Assert.h"

using namespace srch2::util;
using namespace srch2::instantsearch;
using namespace std;

const char* filename = "testlogger.txt";
const char* fnStdout = "teststdout.txt";
const char* fnStderr = "teststderr.txt";

typedef void (*logFunc)(const char* format, ...);

void testsetLogLevel() {
	for (int i = Logger::SRCH2_LOG_SILENT; i < Logger::SRCH2_LOG_DEBUG; i++) {
		Logger::setLogLevel((Logger::LogLevel) i);
		ASSERT(Logger::LogLevel(i) == Logger::getLogLevel());
	}
}

void logToFile(const char* filename, int level, const char* str) {
	FILE* fp = fopen(filename, "w");
	Logger::setOutputFile(fp);
	logFunc func = NULL;
	switch (level) {
	case Logger::SRCH2_LOG_ERROR:
		func = &(Logger::error);
		break;
	case Logger::SRCH2_LOG_WARN:
		func = &(Logger::warn);
		break;
	case Logger::SRCH2_LOG_INFO:
		func = &(Logger::info);
		break;
	case Logger::SRCH2_LOG_DEBUG:
		func = &(Logger::debug);
		break;
	default:
		func = &(Logger::console);
	}
	func(str);
	fclose(fp);
}

int testEqual(const char* filename, const char* teststr) {
	ifstream fs(filename);
	string line("");
	getline(fs, line);
	fs.close();
	return line.substr(line.find('\t') + 1).compare(teststr);
}

/**
 * Each higher priority level will print to file.
 * Each lower priority level will not print to file.
 *
 */
void testLevel(const char* filename, Logger::LogLevel curLevel) {
	char buffer[64];
	Logger::setLogLevel(curLevel);
	int level = Logger::SRCH2_LOG_SILENT + 1;
	for (; level <= curLevel; level++) {
		sprintf(buffer, "current level is %d", level);
		logToFile(filename, level, buffer);
		ASSERT(testEqual(filename, buffer) == 0);
	}
	for (; level <= Logger::SRCH2_LOG_DEBUG; level++) {
		sprintf(buffer, "current level is %d", level);
		logToFile(filename, level, buffer);
		ASSERT(testEqual(filename, "") == 0);
	}
}

/**
 * No matter what current level it is, the console will still write to stdout
 */
void testConsole(const char* filename) {
	char buffer[64];
	int level = Logger::SRCH2_LOG_SILENT + 1;
	for (; level <= Logger::SRCH2_LOG_DEBUG; level++) {
		FILE* originStdout = stdout;
		stdout = fopen(fnStdout, "w");

		Logger::setLogLevel((Logger::LogLevel) level);
		sprintf(buffer, "current level is %d", level);
		logToFile(filename, -1, buffer);
		ASSERT(testEqual(filename, buffer) == 0);

		fclose (stdout);
		ASSERT(testEqual(filename, buffer) == 0);
		stdout = originStdout;
	}
}

/**
 * No matter what current level it is, the error will still write to stderr
 */
void testError(const char* filename) {
	testLevel(filename, Logger::SRCH2_LOG_ERROR);

	char buffer[64];
	int level = Logger::SRCH2_LOG_SILENT + 1;
	for (; level <= Logger::SRCH2_LOG_DEBUG; level++) {
		FILE* originStderr = stderr;
		stderr = fopen(fnStderr, "w");

		Logger::setLogLevel((Logger::LogLevel) level);
		sprintf(buffer, "current level is %d", level);
		logToFile(filename, Logger::SRCH2_LOG_ERROR, buffer);
		ASSERT(testEqual(filename, buffer) == 0);
		fclose (stderr);
		ASSERT(testEqual(filename, buffer) == 0);
		stderr = originStderr;
	}
}

void testWarn(const char* filename) {
	testLevel(filename, Logger::SRCH2_LOG_WARN);
}

void testInfo(const char* filename) {
	testLevel(filename, Logger::SRCH2_LOG_INFO);
}

void testDebug(const char* filename) {
	testLevel(filename, Logger::SRCH2_LOG_DEBUG);
}

void testHugeMessage(const char* filename) {
	FILE* fp = fopen(filename, "w");
	Logger::setOutputFile(fp);
	Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);
	char buffer[Logger::kMaxLengthOfMessage * 2] = { 0 };
	const char* msg = "don't be afraid, this is the longest log message test";
	strcpy(buffer, msg);
	memset(buffer + strlen(msg), 'a', sizeof(buffer) - strlen(msg));
	buffer[sizeof(buffer) - 1] = 0;
	Logger::console("%s %s", buffer, "some extra");
	fclose(fp);

	ifstream fs(filename);
	string line("");
	getline(fs, line);
	string substr = line.substr(line.find('\t') + 1);
	ASSERT(line.length() <= Logger::kMaxLengthOfMessage);
	ASSERT(substr.compare(0, substr.length(), buffer, substr.length()) == 0);
	fs.close();
}

void testFormat(const char* filename){
	FILE* fp = fopen(filename, "w");
	Logger::setOutputFile(fp);
	Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);
	char buffer[Logger::kMaxLengthOfMessage] = { 0 };
	const char* message = "here is one string";
	float f = 1024.12f;
	int i = 42;
	const char* fmt = "This is a format :%s float should be 1024.120:%.3f int should be 00042 int:%05d";
	sprintf(buffer, fmt, message, f, i);
	Logger::console(fmt, message, f, i);
	fclose(fp);

	ifstream fs(filename);
	string line("");
	getline(fs, line);
	string substr = line.substr(line.find('\t') + 1);
	ASSERT(line.length() <= Logger::kMaxLengthOfMessage);
	ASSERT(substr.compare(buffer) == 0);
	fs.close();
}

int main() {
	testsetLogLevel();

	testConsole(filename);
	testError(filename);

	testWarn(filename);
	testInfo(filename);
	testDebug(filename);

	testHugeMessage(filename);
	testFormat(filename);
	remove(filename);
	remove(fnStderr);
	remove(fnStdout);
}
