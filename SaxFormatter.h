#pragma once 

#include "SaxParser.h"

class SaxFormatter: public XSPHandler
{
public:
	SaxFormatter();

	void OnElementBegin(const std::string szName) override;
    void OnElementEnd(const  std::string szName) override;
    void OnCloseSingleElement(const  std::string szName) override;
    void OnAttribute(const std::string szName, const std::string szValue) override;
    void OnText(const std::string szValue) override;
    void OnCDATA(const std::string szValue) override;
    void OnComment(const std::string szText) override;
    void OnDeclaration(const std::string szVersion, const std::string szEncoding, const std::string szStandAlone) override;
    void OnProcessing(const std::string szValue) override;

	void SetIndentAsTab(); //Set indent as tabulation
	void SetIndentAsBlank(int n); //Set indent as whitespace
protected:
	virtual void Output(const std::string szText) {}

private:
	std::string m_IndentString;
	unsigned int m_nIndentValue;
	bool m_bValue;
	bool m_bOpen;
	void Incr();
	void Decr();
	void DoIndent();
	void DoBreakLine();
};
