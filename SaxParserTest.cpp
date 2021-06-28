#include "SaxFormatter.h"

#include <fstream>
#include <iostream>
#include <cstring>

int strcmpi(char const *a, char const *b);

using namespace std;

class XmlFileFormatter : public SaxFormatter
{
public:
    XmlFileFormatter(ofstream& file) : m_file(file) {
    }
protected:
    ofstream& m_file;
    void Output(const string szText) override { m_file << szText; }
};

int main(int argc, char* argv[])
{
    switch (argc)
    {
    case 1:
        cout << "Input file is not defined" << endl;
        return 1;
        break;
    case 2:
        cout << "Output file is not defined"  << endl;
        return 1;
    }

    ifstream input(argv[1]);
    if (input.fail())
    {
        cout << "Error open input file" << endl;
        return 1;
    }

    ofstream output(argv[2]);
    if (output.fail())
    {
        cout << "Error open input file" << endl;
        return 1;
    }

    try
    {
        XmlFileFormatter handler(output);
        SaxParser parser;
        parser.Parse(&input, &handler);
        std::cout << "Successful" << std::endl;
    }
    catch (SaxParserException& e)
    {
        std::cout << e.what() << " Line: " << e.GetLine() << " Column: " << e.GetColumn() << "  " << e.m_str << std::endl;
    }
    return 0;
}

