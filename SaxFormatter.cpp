#include "SaxFormatter.h"

SaxFormatter::SaxFormatter()
{
	m_nIndentValue=0; m_bValue=false; m_bOpen=false;
	m_IndentString="\t";
}

void SaxFormatter::DoIndent()
{
	for (unsigned int i=0; i<m_nIndentValue; i++)
	{
		Output(m_IndentString);
	}
}


void SaxFormatter::OnElementBegin(const std::string szName)
{
	if (m_bOpen)
		Output(">");
	DoBreakLine();
	DoIndent();
	Output("<"); Output(ToXML(szName));
	m_bOpen=true;
	m_bValue=false;
	Incr();
}

void SaxFormatter::OnCloseSingleElement(const std::string szName)
{
	Output("/>");
	Decr();
	m_bOpen=false;
}

void SaxFormatter::OnElementEnd(const std::string szName)
{
	if (m_bOpen)
		Output(">");
	if (!m_bValue && !m_bOpen)
	{
		DoBreakLine();
		Decr();
		DoIndent();
	}
	else
	{
		Decr();
	}
	Output("</"); Output(ToXML(szName)); Output(">");
	m_bOpen=false;
	m_bValue=false;
}

void SaxFormatter::OnAttribute(const std::string szName, const std::string szValue)
{
	Output(" "); Output(ToXML(szName)); Output("=");
	Output("\""); Output(ToXML(szValue)); Output("\"");
}

void SaxFormatter::OnCDATA(const std::string szValue)
{
	if (m_bOpen)
		Output(">");
	DoBreakLine();
	DoIndent();
	Output("<![CDATA[ "); Output(ToXML(szValue)); Output(" ]]>");
	m_bValue=false;
	m_bOpen=false;
}

void SaxFormatter::OnComment(const std::string szText)
{
	if (m_bOpen)
		Output(">");
	DoBreakLine();
	DoIndent();
	Output("<!-- "); Output(ToXML(szText)); Output(" -->");
	m_bValue=false;
	m_bOpen=false;
}

void SaxFormatter::OnDeclaration(const std::string szVersion, const std::string szEncoding, const std::string szStandAlone)
{
	DoIndent();
	Output("<?xml ");
	if (!szVersion.empty())
		{Output("version=\""); Output(ToXML(szVersion)); Output("\"");}
	if (!szEncoding.empty())
		{Output(" encoding=\""); Output(ToXML(szEncoding)); Output("\"");}
	if (!szStandAlone.empty())
		{Output("standalone=\""); Output(ToXML(szStandAlone)); Output("\"");}
	Output(" ?>"); 
}

void SaxFormatter::OnProcessing(const std::string szValue)
{
	if (m_bOpen)
		Output(">");
	DoBreakLine();
	DoIndent();
	Output("<? "); Output(ToXML(szValue)); Output(" ?>");
	DoBreakLine();
}

void SaxFormatter::OnText(const std::string szValue)
{
	if (m_bOpen)
	{
		Output(">");
		m_bValue=true;
	}
	else
	{
		DoIndent();
	}
	Output(ToXML(szValue));
	m_bOpen=false;
}

void SaxFormatter::SetIndentAsTab()
{
    m_IndentString = "\t";
}

void SaxFormatter::SetIndentAsBlank(int n)
{
    m_IndentString = "";
    for (int i=0; i<n; i++)
        m_IndentString += ' ';
}

inline void SaxFormatter::DoBreakLine()
{
    Output("\n");
}

inline void SaxFormatter::Incr()
{
    m_nIndentValue++;
}

inline void SaxFormatter::Decr()
{
    m_nIndentValue--;
}
