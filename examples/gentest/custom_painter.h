//
// Created by mgr on 6/1/22.
//

#ifndef PDFMM_CUSTOM_PAINTER_H
#define PDFMM_CUSTOM_PAINTER_H

#include <pdfmm/pdfmm.h>
#include <sstream>

// All pdfmm classes are member of the pdfmm namespace.
using namespace std;
using namespace mm;



class PdfGenerationException : public std::exception
{

public:
  PdfGenerationException() = delete;

  explicit PdfGenerationException(std::string errorCode, std::string message)
      : code_(std::move(errorCode)), message_(std::move(message))
  {
  }

  const char *what() const noexcept override
  {
    return code_.c_str();
  }

  const std::string &getMessage() const
  {
    return message_;
  }

private:
  std::string code_;
  std::string message_;

};


class CustomPainter
{
public:

  // generic methods
  CustomPainter();

  void AddNewPage();

  void InsertText(const std::string_view& str, double x, double y, double fontSize, bool isHeading = false);
  void InsertTextRightAligned(const std::string_view& str, double x, double y, double textWidth, double fontSize, bool isHeading = false);
  void InsertLine(double startX, double startY, double endX, double endY);
  void InsertRect(double x, double y, double x2, double y2, bool drawLeftEdge = true);
  void InsertImage(const std::string_view& imagePath, double posX, double posY);

  void Terminate();
  int WriteDocumentToFile(const char *filepath);

  double GetPageHeight() const;
  double GetPageHeightInches() const;
  double GetPageWidth() const;
  double GetPageWidthInches() const;

  // generic setter methods
  //void SetFirstColumnStart(const float &value);
  //void SetTopStart(const float &value);
  void SetTitle(const string &value);
  void SetAfterTitleTextLeft(const string &value);
  void SetAfterTitleTextRight(const string &value);
  void SetAddDateTime(const bool &value);
  void SetAddPageNumber(const bool &value);
  void SetHorizontalPageMargin(const float &value);
  void SetVerticalPageMargin(const float &value);

  // example/sample specific methods

  // table methods
  void OutputTableColHeaders(const double &fontSize, float rowTop = -1.0f);
  void OutputTableRowValues(const std::string *valueTexts, const double &fontSize);
  void OutputTableOuterLines();
  void SetTableTotalCols(const int &value);
  void SetTableColWidths(float *values);
  void SetTableRowHeight(const float &value);
  void SetTableMaxImageHeightPerRow(const float &value);
  void SetTableImageColumnIndex(const int &value);
  void SetTableImagesFolder(const char* value);
  void SetTableMaxImageWidthPerRow(const float &value);
  void SetTableRowTopPadding(const float &value);
  void SetTableHeadingTexts(string *value);
  void SetTableCellLeftPadding(const float &value);

private:
  // fields only used within this class
  PdfMemDocument document;
  PdfPainter painter;
  PdfFont* normalFont;
  PdfFont* boldFont;
  std::vector<PdfPage*> pages;
  float currentVerticalWriteOffset;
  int currentPageNumber;
  bool firstPageAdded;

  // fields with getters - generic fields
  double m_pageHeight;
  double m_pageWidth;

  // fields with setters - generic fields
  //float m_firstColumnStart;
  //float m_topStart;
  std::string m_title;
  std::string m_afterTitleTextLeft;
  std::string m_afterTitleTextRight;
  bool m_addDateTime;
  bool m_addPageNumber;
  float m_horizontalPageMargin;
  float m_verticalPageMargin;

  // fields with setters - example/sample specific fields - table fields
  float m_tableHeaderHeight;
  int m_tableTotalCols;
  float* m_tableColWidths;
  float m_tableRowHeight;
  float m_tableMaxImageHeightPerRow;
  int m_tableImageColumnIndex;
  std::string m_tableImagesFolder;
  float m_tableMaxImageWidthPerRow;
  float m_tableRowTopPadding;
  std::string *m_tableHeadingTexts;
  float m_tableCellLeftPadding;
  float m_tableHeaderFontSize;

  // private methods
  void startNextPage();

  // private utility methods
  static string getCurrentFormattedDateOrTime(const char* format)
  {
    time_t rawtime;
    struct tm *timeinfo;
    int sz = 23;
    char* ybuffer = (char*) calloc(sz, sizeof(char));
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(ybuffer, sz, format, timeinfo);
    return string(ybuffer);
  }

  static string getDoubleFormattedString(const double x, const int decDigits) {
    stringstream ss;
    ss << fixed;
    ss.precision(decDigits); // set # places after decimal
    ss << x;
    return ss.str();
  }


};


#endif //PDFMM_CUSTOM_PAINTER_H
