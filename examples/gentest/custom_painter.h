//
// Created by mgr on 6/1/22.
//

#ifndef PDFMM_CUSTOM_PAINTER_H
#define PDFMM_CUSTOM_PAINTER_H

#include <pdfmm/pdfmm.h>

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

  void InsertText(const std::string_view& str, double x, double y, double fontSize);
  void InsertLine(double startX, double startY, double endX, double endY);
  void InsertRect(double x, double y, double x2, double y2, bool drawLeftEdge = true);
  void InsertImage(const std::string_view& imagePath, double posX, double posY);

  void Terminate();
  int WriteDocumentToFile(const char *filepath);

  double GetPageHeight() const;
  double GetPageWidth() const;

  // example/sample specific methods
  void OutputTableColHeaders(double fontSize, float rowTop = -1.0f);
  void OutputTableRowValues(const std::string *valueTexts, double fontSize);
  void OutputTableOuterLines();

  // generic setter methods
  void SetTotalCols(const int totalCols);
  void SetFirstColumnStart(float value);
  void SetTopStart(float value);
  void SetColWidths(float *values);
  void SetTableRowHeight(float value);
  void SetMaxImageHeightPerRow(float maxImageHeightPerRow);
  void SetImageColumnIndex(int imageColumnIndex);
  void SetImagesFolder(const char* imagesFolder);
  void SetMaxImageWidthPerRow(float maxImageHeightPerRow);
  void SetTableRowTopPadding(float value);
  void SetHeadingTexts(string *headingTexts);

private:
  // fields only used within this class
  PdfMemDocument document;
  PdfPainter painter;
  PdfFont* font;
  std::vector<PdfPage*> pages;
  float currentTableRowOffset;

  float headerHeight;

  // fields with getters
  double m_pageHeight;
  double m_pageWidth;

  // fields with setters
  int m_totalCols;
  float m_firstColumnStart;
  float m_topStart;
  float* m_colWidths;
  float m_tableRowHeight;
  float m_maxImageHeightPerRow;
  int m_imageColumnIndex;
  std::string m_imagesFolder;
  float m_maxImageWidthPerRow;
  float m_tableRowTopPadding;
  std::string *headingTexts;

  // private methods
  void startNextPage();

};


#endif //PDFMM_CUSTOM_PAINTER_H
