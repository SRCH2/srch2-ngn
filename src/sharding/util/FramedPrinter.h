#ifndef __SRCH2_UTIL_FRAMED_PRINTER_H__
#define __SRCH2_UTIL_FRAMED_PRINTER_H__

#include <cstdio>
#include <string>
#include <iostream>
#include <vector>

using namespace std;
namespace srch2 {
namespace util {

class TableFormatPrinter{
public:
	struct Cell{
		vector<string> lines;
		void print(const unsigned lineNumber, const unsigned colWidth){
			cout << "|";
			unsigned printSize = 0;
			if(lineNumber >= lines.size()){
				printSize = 0;
			}else{
				printSize = lines.at(lineNumber).size();
			}
			if(printSize < colWidth-1){
				unsigned counter = 0;
				unsigned leftSpace = 0;
				// (colWidth-1 - printSize) / 2
				while(counter++ < leftSpace){
					cout << " ";
				}
				if(lineNumber < lines.size()){
					cout << lines.at(lineNumber) ;
				}
				counter = (colWidth-1 - printSize) - leftSpace;
				while(counter--){
					cout << " ";
				}
			}else{
				cout << lines.at(lineNumber) ;
			}

		}
	};
public:
	TableFormatPrinter(const string & tableName, const unsigned width, const vector<string> & columnHeaders,
			const vector<string> & rowLabels):tableName(tableName), columnHeaders(columnHeaders),
			rowLabels(rowLabels),width(width){
		init();
	}

	TableFormatPrinter(const string & tableName, const unsigned width, const unsigned numCol,
			const vector<string> & rowLabels):tableName(tableName),rowLabels(rowLabels),width(width){
		init(numCol);
	}
	TableFormatPrinter(const string & tableName, const unsigned width, const unsigned numCol,
			const unsigned numRow):tableName(tableName),width(width){
		init(numCol,numRow);
	}

	void printColumnHeaders(){
		unsigned cellWidth = (width-1) / (numCol+1);
		vector<Cell*> headerCells;
		headerCells.push_back(prepareCell(tableName, cellWidth));
		for(unsigned c = 0 ; c < numCol; ++c){
			Cell * cell = prepareCell(columnHeaders.at(c), cellWidth);
			headerCells.push_back(cell);
		}

		printLine(width);
		unsigned lineNumber = 0;
		bool more = true;
		while(more){
			more = false;
			for(unsigned c = 0 ; c < numCol+1; ++c){
				Cell * firstCell = headerCells.at(c);
				headerCells.at(c)->print(lineNumber, cellWidth);
				if(headerCells.at(c)->lines.size() > lineNumber+1){
					more = true;
				}
			}
			printEndOfRow();
			lineNumber++;
		}
		printDoubleLine(width);
	}
	void startFilling(){
		rowNumber = 0;
		rowCells.clear();
	}
	void printNextCell(const string & content){
		unsigned cellWidth = (width-1) / (numCol+1);
		Cell * cell = prepareCell(content, cellWidth);
		rowCells.push_back(cell);
		if(rowCells.size() == numCol){
			Cell * label = NULL;
			if(rowLabels.size() != 0){
				label = prepareCell(rowLabels.at(rowNumber++),cellWidth);
			}else{
				label = new Cell();
			}
			unsigned lineNumber = 0;
			bool more = true;
			while(more){
				more = false;
				label->print(lineNumber++,cellWidth);
				if(label->lines.size() > lineNumber){
					more = true;
				}
				for(unsigned rc = 0 ; rc < numCol; ++rc){
					rowCells.at(rc)->print(lineNumber-1, cellWidth);
					if(rowCells.at(rc)->lines.size() > lineNumber){
						more = true;
					}
				}
				printEndOfRow();
			}
			printLine(width);
			for(unsigned rc = 0 ; rc < numCol; ++rc){
				delete rowCells.at(rc);
			}
			rowCells.clear();
			return;
		}
	}
private:
	const string tableName;
	const vector<string> columnHeaders;
	const vector<string> rowLabels;
	const unsigned width;
	unsigned numCol;
	unsigned numRow;
	vector<Cell*> rowCells;
	int rowNumber;

	void printLine(const unsigned width){
		for(unsigned c = 0 ; c < width; ++c){
			cout << "_";
		}
		cout << endl;
	}

	void printDoubleLine(const unsigned width){
		for(unsigned c = 0 ; c < width; ++c){
			cout << "=";
		}
		cout << endl;
	}

	void printEndOfRow(){
		cout << "|" << endl;
	}

	void init(){
		numCol = columnHeaders.size();
		numRow = rowLabels.size();
	}
	void init(const unsigned numCol){
		this->numCol = numCol;
		numRow = rowLabels.size();
	}

	void init(const unsigned numCol, const unsigned numRow){
		this->numCol = numCol;
		this->numRow = numRow;
	}
	Cell * prepareCell(const string & contentArg, const unsigned width){
		Cell * cell = new Cell();
		string content = contentArg;
		vector<string> contents;
		// tokenization
		while(true){
			if(content.find('%') == string::npos){
				contents.push_back(content);
				break;
			}
			contents.push_back(content.substr(0,content.find('%')));
			content = content.substr(content.find('%')+1);
		}
		for(vector<string>::iterator contentItr = contents.begin();
				contentItr != contents.end(); ++contentItr){

			unsigned startIndex = 0;
			while(startIndex < contentItr->size()){
				if(startIndex+width-1 >= contentItr->size()){
					cell->lines.push_back(contentItr->substr(startIndex));
					break;
				}
				cell->lines.push_back(contentItr->substr(startIndex, width-1));
				startIndex += width-1;
			}
		}
		return cell;
	}

};
}
}

#endif // __SRCH2_UTIL_FRAMED_PRINTER_H__
