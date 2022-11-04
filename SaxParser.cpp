// SaxParser.cpp: implementation of the SaxParser class.
//
//////////////////////////////////////////////////////////////////////

//Some parts of encoding conversion code were copied out from 'tinyxml' component

#include "SaxParser.h"
#include <list>
#include <cstring>
#include <cassert>

using namespace std;

const int utf8ByteTable[256] = 
{
	//	0	1	2	3	4	5	6	7	8	9	a	b	c	d	e	f
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x00
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x10 
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x20
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x30
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x40
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x50
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x60
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x70	End of ASCII range
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x80 0x80 to 0xc1 invalid
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x90 
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0xa0 
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0xb0 
		1,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// 0xc0 0xc2 to 0xdf 2 byte
		2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// 0xd0
		3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	// 0xe0 0xe0 to 0xef 3 byte
		4,	4,	4,	4,	4,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	// 0xf0 0xf0 to 0xf4 4 byte, 0xf5 and higher invalid
};

void ConvertUTF32ToUTF8( unsigned long input, string& output)
{
	const unsigned long BYTE_MASK = 0xBF;
	const unsigned long BYTE_MARK = 0x80;
	const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

	int length;

	if (input < 0x80) 
		length = 1;
	else if ( input < 0x800 )
		length = 2;
	else if ( input < 0x10000 )
		length = 3;
	else if ( input < 0x200000 )
		length = 4;
	else
		{ length = 0; return; }	// This code won't covert this correctly anyway.

	output.resize(length);

	// Scary scary fall throughs.
	switch (length) 
	{
		case 4:
			//--output; 
			output[3] = (char)((input | BYTE_MARK) & BYTE_MASK); 
			input >>= 6;
		case 3:
			//--output; 
			output[2] = (char)((input | BYTE_MARK) & BYTE_MASK); 
			input >>= 6;
		case 2:
			//--output; 
			output[1] = (char)((input | BYTE_MARK) & BYTE_MASK); 
			input >>= 6;
		case 1:
			//--output; 
			output[0] = (char)(input | FIRST_BYTE_MARK[length]);
	}
}



struct SAttribute
{
	string m_name;
	string m_value;

	void Clear() {m_name.erase(); m_value.erase();}
};

static const char* g_cszErrorList[]=
{
	"No error",	//0
	"Document is empty",			//1
	"Invalid document format",		//2
	"Invalid instance definition",	//3
	"Value is too long",			//4
	"Invalid processing instruction name", //5
	"Name 'xml' is reserved and must content only lower-case symbols", //6
	"Missing closing tag",					//7
	"Invalid declaration description",		//8
	"Invalid version number",				//9
	"Encoding not supported",				//10
	"Invalid element name",					//11
	"Closing element does not match to open one",//12
	"Invalid comment declaration",			//13
	"Invalid CDATA declaration",			//14
	"Empty entity",							//15
	"Invalid symbol within entity",			//16
	"Invalid attribute name",				//17
	"Invalid attribute description",		//18
	"Document must not content data after root node",//19
	"Unknown entity",						//20
	"Root node is not closed",				//21
	"DTD is not supported",					//22
	"End of file encountered",				//23
	"Section CDATA is not closed",			//24
	"Missing semicolumn",					//25
	"Missing quote",						//26
	"The text before root node is illegal",	//27
	"The text after root node is illegal",	//28
	"Whitespace after element symbol open is illegal",//29
	"Whitespace after element symbol close is illegal",//30
	"Whitespace after processing instruction close is illegal", //31
	"Missing closing tag or commentary does not closed",//32
	"Two-byte encodings are not supported",				//33
	"Entity before document open root is illegal",		//34
	"CDATA section before document open root is illegal",//35
	"Declaration was not closed or invalid syntax", //36
	"Processing instruction was not closed",		//37
	"Duplicate attribute name", //38
    "Input data error"          //39
};

const int g_ciCountError = std::extent <decltype(g_cszErrorList)>::value;

const char* SaxParserException::what() const noexcept
{
    return g_cszErrorList[m_nCode];
}

struct SEntity
{
	string m_szEntity;
	char m_value;
};

const SEntity g_ListEntity[]= 
{
	{ "amp", '&'  },
	{ "lt",  '<'  },
	{ "gt",  '>'  },
	{ "quot",'\"' },
	//{ "apos",'\'' }
};

const unsigned int g_ciCountEntity = std::extent <decltype(g_ListEntity)>::value;

int findEntity(char c) {
    for (int i=0; i<g_ciCountEntity; i++)
        if (g_ListEntity[i].m_value==c) return i;
    return -1;
}

string ToXML(string str) {
	size_t lenOut = str.length();
    for (size_t i=0; i<str.length(); i++) {
        int k = findEntity(str[i]);
        if (k>=0) lenOut+= g_ListEntity[k].m_szEntity.length()+1;
    }
    string outStr;
    outStr.resize(lenOut);
    size_t i2=0;
    for (int i=0; i<str.length(); i++) {
        int k = findEntity(str[i]);
        if (k>=0) {
            outStr[i2] = '&';
            i2++;
            for (int j=0; j<g_ListEntity[k].m_szEntity.length(); j++)
                outStr[i2+j] = g_ListEntity[k].m_szEntity[j];
            i2 += g_ListEntity[k].m_szEntity.length();
            outStr[i2] = ';';
            i2++;
        }
        else {
            outStr[i2] = str[i];
            i2++;
        }
    }
    return outStr;
}

inline bool IsLetter(char c)
{
	if (((unsigned char)c)<127)
		return isalpha(c) ? true : false;
	return 
		true;
}

inline bool IsDigit(char c)
{
	return isdigit((unsigned char)c) ? true : false;
}

inline bool IsHexDigit(char c)
{
	return isxdigit((unsigned char)c) ? true : false;
}

inline bool IsPrintable(char c)
{
	return isprint((unsigned char)c) ? true : false;
}

inline bool IsControl(char c)
{
	return iscntrl((unsigned char)c) ? true : false;
}

inline bool IsSpace(char c)
{
	return (isspace((unsigned char)c) || c == '\n' || c == '\r' || c=='\t') ? true : false;
}

inline unsigned int GetIncreaseColumn(char c)
{
	switch(c)
	{
	case ' ':
	case '\t':
		return 1;
		break;
	default:
		if (IsPrintable(c))
			return 1;
	}
	return 0;
}

bool IsFirstNameValid(char c)
{
	if (c=='_') return true;
	if (IsLetter(c)) return true;
	return false;
}

bool IsCharNameValid(char c)
{
	if (c=='.' || c=='_' || c=='-' || c==':') return true;
	if (IsLetter(c) || IsDigit(c)) return true;
	return false;
}

int strcmpi_case_insensitive(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

bool IsXML(const string& str, bool& bError)
{
    bError = false;
	if (str.size()<3) return false;
	if (strcmpi_case_insensitive(str.c_str(),"xml")==0)
	{
        if (str[0] == 'X' || str[1] == 'M' || str[2] == 'L')
        {
            bError = true;
            return false;
        }
		return true;
	}
	return false;
}

void TrimRight(string& str, XSPHandler* pHandler)
{
	unsigned int pos=(unsigned int )str.size();
	if (pos==0) return;
	while (true)
	{
		if (!(IsSpace(str[pos-1]) || IsControl(str[pos-1])))
			break;
		else
			pHandler->OnNotLeadingChar(str[pos-1]);
		pos--;
		if (pos==0)
			break;
	}
	str.resize(pos);
}

//////////////////////////////////////////////////////////////////////////
//SPBuffer class
//////////////////////////////////////////////////////////////////////////

SaxParser::SPBuffer::SPBuffer()
{
	m_Data[0]='0'; m_Data[1]='\0'; m_Data[2]='0'; m_Data[3]='\0';
	m_nCount=0; m_nPos=0;
}

inline void SaxParser::SPBuffer::Put(char c)
{
	assert(m_nCount<4);
	m_Data[m_nCount]=c;
	m_nCount++; 
}

char SaxParser::SPBuffer::Extract()
{
	assert(m_nCount>0);
	m_nPos++;
	char c=m_Data[m_nPos-1];
	if (m_nPos==m_nCount)
	{
		m_nPos=0; m_nCount=0;
	}
	return c;
}

inline char SaxParser::SPBuffer::Get(unsigned int pos) const
{
	assert(pos<m_nCount);
	return m_Data[pos];
}

inline void SaxParser::SPBuffer::PutBack(char c)
{
	assert(m_nCount<4);
	if (IsEmpty())
		Put(c);
	else
	{
		if (m_nPos>0)
		{
			m_nPos--;
			m_Data[m_nPos]=c;
		}
		else
		{
			for (int i=2; i>=0; i--)
				m_Data[i+1]=m_Data[i];
			m_Data[0]=c;
			m_nCount++;
		}
	}
}

inline unsigned char SaxParser::SPBuffer::Current() const
{
	assert(!IsEmpty());
	return m_Data[m_nPos];
}

inline bool SaxParser::SPBuffer::IsEmpty() const
{
	return m_nCount==0 ? true : false;
}

char SaxParser::SPBuffer::operator[] (unsigned int i) const
{
	return Get(i);
}

inline const string SaxParser::SPBuffer::GetData() const
{
	assert(m_nCount>0);
	return m_Data;
}

inline void SaxParser::SPBuffer::Clear()
{
	m_nCount=0; m_nPos=0;
}


//////////////////////////////////////////////////////////////////////
// SaxParser class
//////////////////////////////////////////////////////////////////////

SaxParser::SaxParser()
{
    m_pHandler = NULL; m_pStream = NULL; m_nLimit = 0;
	m_bReadHeader=false; m_nEncoding=SPENC_UNKNOWN;
}

char SaxParser::ReadChar()
{
    char c; m_pStream->get(c);
    if (m_pStream->bad())
        ThrowException(SPE_INPUT_DATA_ERROR, "Can't read stream");
    return c;
}

void SaxParser::Parse(std::istream* pStream, XSPHandler* pHandler, int encoding)
{
	m_pStream=pStream; m_pHandler=pHandler; m_nEncoding=encoding;
    m_nLine = 0; m_nColumn = 0; m_bReadHeader = false;  m_bEmpty = true;
    m_buffer.Clear();

	enum TEState
	{
		st_begin=0,		// initial state
		st_ready,		// is ready to enter
		st_text,		// enter of text
		st_processing,	// XML processing instruction
		st_open,		// enter of open tag 
		st_analys,		// analysis of type after '!'
		st_element,		// enter of element
		st_close,		// Close of element
		st_remark,		// enter of remark
		st_cdata,		// enter of CDATA
		st_dtd,			// enter of DTD definition
		st_entity,		// enter of entity (value after the ampersand)
		st_finish,		// enter finished
		st_finish_extra, //enter of additional data that could appear after the document has been closed 
	};
	TEState state=st_begin;

	string temp;
	string accum;
	bool bOpen=false;
	pHandler->OnDocumentBegin();

	ReadSignature();
		
	while (!IsEOF())
	{
		char c=GetChar();

		switch(state)
		{

		case st_begin:
			{
				if (::IsSpace(c))
				{
					m_pHandler->OnNotLeadingChar(c);
					continue;
				}
				switch(c)
				{
				case '<':
					m_pHandler->OnOpenTag();
					state=st_open;
					break;
				default:
					ThrowException(SPE_INVALID_FORMAT, "not < in st_begin");
					break;
				}
					
			}
			break;

		case st_open:
			{
				if (::IsSpace(c))
                    ThrowException(SPE_WHITESPASE_OPEN, "space");
				switch(c)
				{
				case '?':
					state=st_processing;
					EnterProcessing();
					state=st_ready;
					break;
				case '!':
					state=st_analys;
					break;
				case '/':
					state=st_close;
					EnterClosingElement();
                    if (m_StackItems.size() == 0)
						state=st_finish;
					else
						state=st_ready;
					break;
				default:
					state=st_element;
					EnterOpenElement(c);
					bOpen=true;
                    if (m_StackItems.size() == 0)
						state=st_finish;
					else
						state=st_ready;
					break;
				}
			}
			break;

		case st_analys:
			{
				switch(c)
				{
				case '-':
					state=st_remark;
					EnterComment();
					state=st_ready;
					break;
				case '[':
					if (!bOpen)
                        ThrowException(SPE_CDATA_DOC_OPEN, "[");
					state=st_cdata;
					EnterCDATA();
					state=st_ready;
					break;
				default:
					if (c)
					{
						state=st_dtd;
						EnterDTD(c);
						state=st_ready;
					}
					else
                        ThrowException(SPE_INVALID_INSTANCE);
					break;
				}
			}
			break;

		case st_ready:
			{
				if (::IsSpace(c) || ::IsControl(c))
				{
					m_pHandler->OnNotLeadingChar(c);
					continue;
				}
				switch(c)
				{
				case '<':
					m_pHandler->OnOpenTag();
					state=st_open;
					break;
				case '&':
					if (!bOpen)
                        ThrowException(SPE_ENTITY_DOC_OPEN);
					state=st_entity;
					EnterEntity(&temp);
					accum=accum+temp;
					state=st_text;
					break;
				default:
					if (!bOpen)
                        ThrowException(SPE_TEXT_BEFORE_ROOT);
					accum+=c;
					state=st_text;
					break;
				}
			}
			break;

		case st_text:
			switch(c)
			{
			case '<':
				TrimRight(accum,m_pHandler);
				m_pHandler->OnText(accum);
				m_pHandler->OnOpenTag();
				accum.erase();
				state=st_open;
				break;
			case '&':
				state=st_entity;
				EnterEntity(&temp);
				accum=accum+temp;
				state=st_text;
				break;
			default:
				accum+=c;
				if (m_nLimit!=0 && accum.size()>m_nLimit)
                    ThrowException(SPE_TOO_BIG_VALUE,accum);
			}
			break;
			
			case st_finish:
				if (::IsSpace(c))
				{
					m_pHandler->OnNotLeadingChar(c);
					continue;
				}
				if (c!='<')
                    ThrowException(SPE_TEXT_AFTER_ROOT);
				m_pHandler->OnOpenTag();
				state=st_finish_extra;
				break;

			case st_finish_extra:
				switch(c)
				{
				case '?':
					EnterProcessing();
					state=st_finish;
					break;
				case'!':
					EnterComment();
					state=st_finish;
					break;
				default:
                    ThrowException(SPE_TEXT_AFTER_ROOT);
				}
		}
	}

	switch(state)
	{
	case st_begin:
        ThrowException(SPE_EMPTY);
		break;
	case st_ready:
        if (m_StackItems.size() != 0)
            ThrowException(SPE_ROOT_CLOSE);
		break;
	case st_text:
        ThrowException(SPE_TEXT_AFTER_ROOT);
		break;
	}

	m_pHandler->OnDocumentEnd();
}

void SaxParser::EnterProcessing()
{
	string accum; char store = '\0';
	enum TEState {st_begin,st_first_part,st_enter,st_check_end,st_end};
	TEState state=st_begin;

	try
	{

	while (true)
	{
		unsigned char c=GetChar();

		switch(state)
		{
		case st_begin:
			if (IsFirstNameValid(c))
			{
				accum+=c;
				state=st_first_part;
			}
			else
                ThrowException(SPE_PROCESSING_NAME);
			break;

		case st_first_part:
			if (::IsSpace(c))
			{
                bool bError;
				if (IsXML(accum,bError))
				{
					EnterDeclaration();
					return;
				}
				else
				{
                    if (bError)
                        ThrowException(SPE_RESERVED_NAME);
					accum+=c;
					state=st_enter;
				}
				break;
			}
			switch(c)
			{
			case '?':
				m_pHandler->OnProcessing(accum);
				state=st_end;
				break;
			default:
				if (IsCharNameValid(c))
					accum+=c;
				else
                    ThrowException(SPE_PROCESSING_NAME);
			}
			break;

		case st_enter:
			switch(c)
			{
			case '?':
				store=c;
				state=st_check_end;
				break;
			case '>':
                ThrowException(SPE_PROCESSING_CLOSE);
				break;
			default:
				accum+=c;
				break;
			}
			break;

		case st_end:
			if (::IsSpace(c))
                ThrowException(SPE_WHITESPACE_PROCESS);
			if (c!='>')
                ThrowException(SPE_MISSING_CLOSING);
			m_pHandler->OnCloseTag();
			break;

		case st_check_end:
			if (c=='>')
			{
				m_pHandler->OnProcessing(accum);
				m_pHandler->OnCloseTag();
				return;
			}
			else
			{
				accum+=store; accum+=c;
				state=st_enter;
			}
			break;
		}
	}

	}
    catch (SaxParserException& e)
    {
        RethrowException(e, SPE_EOF, SPE_MISSING_CLOSING);
    }
}

void SaxParser::EnterDeclaration()
{
	SAttribute version,encoding,sdecl;
	typedef enum {st_version,st_end_version,st_end_encoding,st_standalone,st_end,st_finish} TEState;
	TEState state=st_version;
	bool bProcess=true;
	
	try
	{

	while (bProcess)
	{
		char c=GetChar();

		switch(state)
		{
		case st_version:
			if (::IsSpace(c))
				continue;
			EnterAttribute(&version,c);
			if (version.m_name!="version")
				ThrowException(SPE_INVALID_DECL);
			if (version.m_value!="1.0")
                ThrowException(SPE_VERSION);
			state=st_end_version;
			SkipWhiteSpace();
			break;

		case st_end_version:
			switch(c)
			{
			case 'e':
				EnterAttribute(&encoding,c);
				if (encoding.m_name!="encoding")
                    ThrowException(SPE_INVALID_DECL);
				SkipWhiteSpace();
				m_pHandler->OnEncoding(encoding.m_value);
				state=st_end_encoding;
				break;
			case 's':
				EnterAttribute(&sdecl,c);
				if (sdecl.m_name!="standalone")
                    ThrowException(SPE_INVALID_DECL);
				if (!(sdecl.m_value!="yes" || sdecl.m_value!="no"))
                    ThrowException(SPE_INVALID_DECL);
				SkipWhiteSpace();
				state=st_end;
				break;
			case '?':
				state=st_finish;
				break;
			case '>':
                ThrowException(SPE_PROCESSING_CLOSE);
			default:
                ThrowException(SPE_INVALID_DECL);
			}
			break;

		case st_end_encoding:
			switch (c)
			{
			case 's':
				EnterAttribute(&sdecl,c);
				if (sdecl.m_name!="standalone")
                    ThrowException(SPE_INVALID_DECL);
				if (!(sdecl.m_value!="yes" || sdecl.m_value!="no"))
                    ThrowException(SPE_INVALID_DECL);
				SkipWhiteSpace();
				state=st_end;
				break;
			case '?':
				state=st_finish;
				break;
			case '>':
                ThrowException(SPE_PROCESSING_CLOSE);
				break;
			default:
                ThrowException(SPE_INVALID_DECL);
			}
			break;

		case st_end:
			switch(c)
			{
			case '?':
				state=st_finish;
				break;
			case '>':
                ThrowException(SPE_PROCESSING_CLOSE);
			default:
                ThrowException(SPE_INVALID_DECL);
			}
			break;

		case st_finish:	
			if (::IsSpace(c))
                ThrowException(SPE_WHITESPACE_PROCESS);
			if (c!='>')
                ThrowException(SPE_MISSING_CLOSING);
			bProcess=false;
			break;
		}
	}
	
	string szEncoding; string szStandalone;
	if (encoding.m_value.size()!=0) szEncoding=encoding.m_value;
	if (sdecl.m_value.size()!=0) szStandalone=sdecl.m_value;
	m_pHandler->OnDeclaration(version.m_value,szEncoding,szStandalone);
	m_pHandler->OnCloseTag();

	if (strcmpi_case_insensitive(encoding.m_value.c_str(),"UTF-8")==0)
		m_nEncoding=SPENC_UTF_8;
	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_MISSING_CLOSING);
	}
	
}

void SaxParser::EnterOpenElement(char c)
{
	if (!::IsFirstNameValid(c))
        ThrowException(SPE_ELEMENT_NAME);
	std::list<std::string> listAttrNames;
	string accum; accum=c;
	
	enum TEState {st_name=0,st_end_name,st_end_attr,st_single};
	TEState state=st_name;

	try
	{
	
	while (true)
	{
		char cc=GetChar();
		switch(state)
		{
		case st_name:
			if (IsSpace(cc))
			{
				SkipWhiteSpace();
				m_pHandler->OnElementBegin(accum);
                m_StackItems.push_back(accum);
				state=st_end_name;
			}
			else
			{
				switch(cc)
				{
				case '>':
					m_pHandler->OnElementBegin(accum);
					m_pHandler->OnCloseTag();
                    m_StackItems.push_back(accum);
					return;
					break;
				case '/':
					m_pHandler->OnElementBegin(accum);
					m_pHandler->OnCloseSingleElement(accum);
					state=st_single;
					break;
				default:
					if (!::IsCharNameValid(cc))
                        ThrowException(SPE_ELEMENT_NAME);
					else
						accum+=cc;
				}
			}
			break;

		case st_end_name:
			if (cc=='/')
			{
				m_pHandler->OnCloseSingleElement(accum);
                m_StackItems.pop_back();
				state=st_single;
			}
			else
			{
				SAttribute attr;
				EnterAttribute(&attr,cc);
				for (auto i=listAttrNames.begin(); i!=listAttrNames.end(); i++)
				{
					if ((*i) == attr.m_name)
                        ThrowException(SPE_DUBLICATE_ATTRIBUTE);
				}
				listAttrNames.push_back(attr.m_name);
				m_pHandler->OnAttribute(attr.m_name,attr.m_value);
				SkipWhiteSpace();
				state=st_end_attr;
			}
			break;
			
		case st_end_attr:
			switch(cc)
			{
			case '/':
				m_pHandler->OnCloseSingleElement(accum);
                m_StackItems.pop_back();
				state=st_single;
				break;
			case '>':
				m_pHandler->OnCloseTag();
				return;
				break;
			default:
				SAttribute attr;
				EnterAttribute(&attr,cc);
                for(auto i = listAttrNames.begin(); i != listAttrNames.end(); i++)
				{
					if ((*i)==attr.m_name)
                        ThrowException(SPE_DUBLICATE_ATTRIBUTE);
				}
				listAttrNames.push_back(attr.m_name);
				m_pHandler->OnAttribute(attr.m_name,attr.m_value);
				SkipWhiteSpace();
				state=st_end_attr;
			}
			break;

		case st_single:
			if (::IsSpace(cc))
                ThrowException(SPE_WHITESPASE_CLOSE);
			if (cc!='>')
                ThrowException(SPE_MISSING_CLOSING);
			m_pHandler->OnCloseTag();
			return;
			break;
		}
	}

	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_MISSING_CLOSING);
	}

}

void SaxParser::EnterClosingElement()
{
    if (m_StackItems.size() == 0)
        ThrowException(SPE_MATCH);

	typedef enum {st_begin,st_name,st_end} TEState;
	string accum;
	TEState state=st_begin;

	try
	{

	while (true)
	{
		char c=GetChar();
		switch(state)
		{
		case st_begin:
			if (::IsSpace(c))
                ThrowException(SPE_WHITESPASE_CLOSE);
			if (!::IsFirstNameValid(c))
                ThrowException(SPE_ELEMENT_NAME);
			accum+=c;
			state=st_name;
			break;

		case st_name:
			if (::IsSpace(c))
			{
				SkipWhiteSpace();
				state=st_end;
				continue;
			}
			switch(c)
			{
			case '>':
                if (m_StackItems.back() != accum)
                    ThrowException(SPE_MATCH);
				m_pHandler->OnElementEnd(accum);
				m_pHandler->OnCloseTag();
                m_StackItems.pop_back();
				return;
				break;
			default:
				if (!::IsCharNameValid(c))
                    ThrowException(SPE_ELEMENT_NAME);
				accum+=c;
		
			}
			break;

		case st_end:
			if (c!='>')
                ThrowException(SPE_MISSING_CLOSING);
            if (m_StackItems.back() != accum)
                ThrowException(SPE_MATCH);
			m_pHandler->OnElementEnd(accum);
			m_pHandler->OnCloseTag();
            m_StackItems.pop_back();
			return;
			break;
		}
	}

	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_MISSING_CLOSING);
	}
	
}

void SaxParser::EnterComment()
{
	if (GetChar()!='-')
        ThrowException(SPE_COMMENT);

	try
	{

	string accum;
	typedef enum {st_enter,st_check_end,st_finish} TEState;
	TEState state=st_enter;
	
	while (true)
	{
		char c=GetChar();

		switch(state)
		{
		case st_enter:
			if (c=='-')
				state=st_check_end;
			else
				accum+=c;
			break;
			
		case st_check_end:
			if (c=='-')
			{
				state=st_finish;
			}
			else
			{
				accum+='-'; accum+=c;
				state=st_enter;
			}
			break;

		case st_finish:
			if (c!='>')
                ThrowException(SPE_MISSING_CLOSING);
			m_pHandler->OnComment(accum);
			m_pHandler->OnCloseTag();
			return;
		}
	}
	
	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_COMMENT_CLOSE);
	}
}

void SaxParser::EnterCDATA()
{
	string accum;

	try
	{

		for (unsigned int i=0; i<6; i++)
			accum+=GetChar();
	}
	catch (SaxParserException& e)
	{
        RethrowException(e,SPE_EOF,SPE_CDATA);
	}
	if (accum != "CDATA[")
        ThrowException(SPE_CDATA);
	
	accum.erase();

	try
	{

	typedef enum {st_enter, st_check1,st_check2} TEState;
	TEState state=st_enter;

	while (true)
	{
		char c=GetChar();
		switch(state)
		{
		case st_enter:
			if (c==']')
				state=st_check1;
			else
				accum+=c;
			break;

		case st_check1:
			if (c==']')
				state=st_check2;
			else
			{
				accum+=']'; accum+=c;
				state=st_enter;
			}
			break;

		case st_check2:
			if (c=='>')
			{
				m_pHandler->OnCDATA(accum);
				m_pHandler->OnCloseTag();
				return;
			}
			else
			{
				accum+="]]"+c; 
				state=st_enter;
			}
			break;
		}
	}
	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_CDATA_CLOSE);
	}
	
}

void SaxParser::EnterDTD(char /*c*/)
{
    ThrowException(SPE_DTD_SUPPORT);
}

void SaxParser::EnterEntity(void* pValue)
{
	string& retStr=*(string*)pValue;
	retStr.erase();
	string accum;
	typedef enum{st_enter,st_analys,st_dec,st_hex,st_text} TEState;
	TEState state=st_enter;
	char c;

	try
	{

	while ((c=GetChar())!=';')
	{
		switch(state)
		{
		case st_enter:
			if (c=='#')
				state=st_analys;
			else
			{
				accum+=c;
				state=st_text;
			}
			break;

		case st_analys:
			if (c=='x')
				state=st_hex;
			else
			{
				if (!::IsDigit(c))
                    ThrowException(SPE_REF_SYMBOL);
				else
				{
					accum+=c;
					state=st_dec;
				}
			}
			break;

		case st_dec:
			if (!::IsDigit(c))
                ThrowException(SPE_REF_SYMBOL);
			else
				accum+=c;
			break;

		case st_hex:
			if (!::IsHexDigit(c))
                ThrowException(SPE_REF_SYMBOL);
			else
				accum+=c;
			break;

		case st_text:
			accum+=c;
			break;
		}
	}

	unsigned long ucs = 0;
	unsigned int mult = 1;
	int i; 
	
	switch(state)
	{
	case st_hex:
		for (i=(int)accum.size()-1; i>=0; i--)
		{
			char q = accum[i];
			if ( q >= '0' && q <= '9' )
				ucs += mult * (q - '0');
			else if ( q >= 'a' && q <= 'f' )
				ucs += mult * (q - 'a' + 10);
			else if ( q >= 'A' && q <= 'F' )
				ucs += mult * (q - 'A' + 10 );
			mult *= 16;
		}
		retStr=(char)ucs;
		break;

	case st_dec:
		for (i=(int)accum.size()-1; i>=0; i--)
		{
			char q = accum[i];
			ucs += mult * (q - '0');
			mult *= 10;
		}
		retStr=(char)ucs;
		break;

	case st_text:
		{
			bool bFound=false;
			for (i=0; i<g_ciCountEntity; i++)
			{
				if (accum == g_ListEntity[i].m_szEntity)
				{
					retStr=g_ListEntity[i].m_value;
					bFound=true;
					break;
				}
			}
			if (!bFound)
                ThrowException(SPE_UNKNOWN_ENTITY, accum);
		}
	}

	if ( (state==st_hex || state==st_dec) && m_nEncoding==SPENC_UTF_8 )
		ConvertUTF32ToUTF8(ucs,retStr);

	}
	catch (SaxParserException& e)
	{
        RethrowException(e, SPE_EOF, SPE_MISSING_SEMI);
	}
	
}

void SaxParser::EnterAttribute(void* pAttr, char c)
{
	SAttribute* pAttrData=(SAttribute*)pAttr;
	if (!::IsFirstNameValid(c))
        ThrowException(SPE_ATTR_NAME);
	pAttrData->Clear();
	pAttrData->m_name=c;
	typedef enum {st_name,st_end_name,st_begin_value,st_value} TEState;
	TEState state=st_name;
	char cOpen = '\0'; string temp;

	try
	{

	while (true)
	{
		c=GetChar();

		switch(state)
		{
		case st_name:
			if (::IsSpace(c))
			{
				SkipWhiteSpace();
				state=st_end_name;
				continue;
			}
			switch(c)
			{
			case '=':
				SkipWhiteSpace();
				state=st_begin_value;
				break;
			default:
				if (!::IsCharNameValid(c))
                    ThrowException(SPE_ATTR_NAME);
				pAttrData->m_name+=c;
			}
			break;

		case st_end_name:
			if (c!='=')
                ThrowException(SPE_ATTR_DESCR);
			SkipWhiteSpace();
			state=st_begin_value;
			break;

		case st_begin_value:
			if (c=='\'' || c=='"')
			{
				state=st_value;
				cOpen=c;
			}
			else
                ThrowException(SPE_ATTR_DESCR);
			break;

		case st_value:
			if (c==cOpen)
				return;
			switch (c)
			{
			case '&':
				EnterEntity(&temp);
				pAttrData->m_value+=temp;
				break;
			default:
				pAttrData->m_value+=c;
			}
		}
	}

	}
	catch (SaxParserException& e)
	{
        unsigned int code = e.GetCode();
        if (e.GetCode() == SPE_EOF)
        {
            switch (state)
            {
            case st_value:
                code = SPE_MISSING_QUOTE;
                break;
            default:
                code = SPE_ATTR_DESCR;
                break;
            }
        }
        RethrowException(e, SPE_EOF, code);
	}
}

void SaxParser::SkipWhiteSpace()
{
	if (IsEOF()) return;
	while (!m_buffer.IsEmpty())
	{
        if (::IsSpace(m_buffer.Current()))
			m_buffer.Extract();
		else 
			return;
	}
    if (!m_buffer.IsEmpty())
		return;

	while (!IsEOF())
	{
		char c=GetChar();

		if (!::IsSpace(c))
		{
			m_buffer.PutBack(c);
			return;
		}
	}
}

bool SaxParser::IsEOF()
{
	if (!m_buffer.IsEmpty()) 
		return false;
	if (m_pStream->eof())
		return true;

    char c = ReadChar(); 
    if (m_pStream->fail())
    {
        return true;
    }
    else
    {
        m_buffer.Put(c);
        return false;
    }
}

char SaxParser::GetChar()
{
	if (!m_buffer.IsEmpty() && !m_bReadHeader)
		return m_buffer.Extract();

	if (m_pStream->eof())
	{
		if (m_bEmpty)
            ThrowException(SPE_EMPTY);
		else
            ThrowException(SPE_EOF);
	}
	
	char c = ReadChar();
	if (m_bReadHeader) return c;

	if (::IsControl(c) && c!='\n' && c!='\t')
		return c;

	switch (c) 
	{
	
	case '\n':
		IncLine();
		break;

	case '\t':
		IncColumn();
		break;

	default:
		if (m_bEmpty && c!=' ')
			m_bEmpty=false;
		if ( m_nEncoding== SPENC_UTF_8 )
		{
			// Eat the 1 to 4 byte utf8 character.
			int step = utf8ByteTable[static_cast<unsigned char>(c)];
			if ( step == 0 )
				step = 1;		// Error case from bad encoding, but handle gracefully.
			for (int i=1; i<step; i++)
			{
				if (m_pStream->eof())
                    ThrowException(SPE_EOF);
				m_buffer.Put(ReadChar());
			}
		}
		IncColumn(); 
		break;
	}
	return c;
}

	
void SaxParser::ReadSignature()
{
	if (m_nEncoding!=SPENC_UNKNOWN) return;
	m_bReadHeader=true;
	
	for(unsigned int i=0; i<4; i++)
		m_buffer.Put(GetChar());

	/*

	00 00 FE FF  UCS-4, big-endian machine (1234 order) 
	FF FE 00 00  UCS-4, little-endian machine (4321 order) 
	00 00 FF FE  UCS-4, unusual octet order (2143) 
	FE FF 00 00  UCS-4, unusual octet order (3412) 
	FE FF ## ##  UTF-16, big-endian 
	FF FE ## ##  UTF-16, little-endian 
	EF BB BF  UTF-8 

	*/

	const unsigned char ar16[4][4]=
	{
		0x00,0x00,0xFE,0xFF,
		0xEF,0xFE,0x00,0x00,
		0x00,0x00,0xFF,0xFE,
		0xFE,0xFF,0x00,0x00
	};
	for (unsigned int i=0; i<4; i++)
		if (memcmp(&ar16[i],m_buffer.GetData().c_str(),4)==0)
            ThrowException(SPE_ENCODING32);
	if ((m_buffer.Get(0)==0xFE && m_buffer.Get(1)==0xFF) || (m_buffer.Get(0)==0xFF && m_buffer.Get(1)==0xFE))
        ThrowException(SPE_ENCODING32);
		
	if ( m_buffer.Get(0)==0xEF && m_buffer.Get(1)==0xBB && m_buffer.Get(2)==0xBF)
	{
		//UTF-8 With signature, signature must be reset
		m_buffer.Extract(); m_buffer.Extract(); m_buffer.Extract();
		m_bReadHeader=false;
		if (m_buffer.Current()=='\n')
			m_nLine=1;
		else
			m_nColumn+=GetIncreaseColumn(m_buffer.Current());
		m_nEncoding=SPENC_UTF_8;
		return;
	}

	m_nEncoding=SPENC_LEGACY; //We don't know exact what kind of encoding is using
	m_bReadHeader=false;
}

inline void SaxParser::IncLine()
{
    m_nLine++; m_nColumn = 0;
}

inline unsigned int SaxParser::GetLine() const
{
    return m_nLine + 1;
}

inline unsigned int SaxParser::GetColumn() const
{
    if (m_nColumn == 0)
        return 1;
    else
        return m_nColumn;
}


inline void SaxParser::IncColumn()
{
    m_nColumn++;
}

void SaxParser::SetLimitValue(unsigned int nLimit)
{
    m_nLimit = nLimit;
}

unsigned int SaxParser::GetLimitValue() const
{
    return m_nLimit;
}

inline void SaxParser::ThrowException(unsigned int code) {
    ThrowException(code, "");
}

inline void SaxParser::ThrowException(unsigned int code, std::string str)
{
    throw SaxParserException(code, GetLine(), GetColumn(), str);
}

inline void SaxParser::RethrowException(SaxParserException& e, unsigned int nCheckCode, unsigned int nSubstituteCode)
{
    if (e.GetCode() == nCheckCode)
        e.m_nCode = nSubstituteCode;
    throw e;
}
