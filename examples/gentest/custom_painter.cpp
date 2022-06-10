//
// Created by mgr on 6/1/22.
//

#include <iostream>
#include "custom_painter.h"

CustomPainter::CustomPainter()
{
  currentPageNumber = 1;
  firstPageAdded = false;

  m_tableMaxImageHeightPerRow = 0.0f;
  m_tableImageColumnIndex = -1;
  //topStart = 11.55f;
  m_verticalPageMargin = 0.05f;
  m_tableHeaderHeight = 0.25f;
  m_horizontalPageMargin = 0.05f;
  m_tableCellLeftPadding = 0.05f;
}

void CustomPainter::AddNewPage()
{
  PdfPage *page = document.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
  m_pageHeight = page->GetRect().GetHeight();
  m_pageWidth = page->GetRect().GetWidth();
  painter.SetCanvas(page);

  currentVerticalWriteOffset = (float) GetPageHeightInches() - m_verticalPageMargin;
  float firstColumnStart = m_horizontalPageMargin;
  float charWidthArial16 = 0.056f; //0.049f; // 0.0486842105263f for font-size = 16 for 1 char
  float charWidthArial11 = 0.077f; // 0.072f for font-size = 11.04 for 1 char

  PdfFontManager& fontManager = document.GetFontManager();
  normalFont = fontManager.GetFont("Arial");
  if (normalFont == nullptr)
    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

  boldFont = fontManager.GetStandard14Font(PdfStandard14FontType::HelveticaBold);
//  boldFont = fontManager.GetFont("Arial Black");
  if (boldFont == nullptr)
    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

  pages.emplace_back(page);

  auto pageWidth = (float) GetPageWidthInches();

  // add title if provided
  if (!m_title.empty())
  {
    float titleWidth = (float) m_title.size() * charWidthArial16;
    InsertText(m_title, (pageWidth / 2) - titleWidth, currentVerticalWriteOffset, 16.0f, true);
    currentVerticalWriteOffset -= 0.2f;
  }

  // add texts after title if provided
  if (!(m_afterTitleTextLeft.empty() && m_afterTitleTextRight.empty()))
  {
    if (!m_afterTitleTextLeft.empty())
    {
      InsertText(m_afterTitleTextLeft, firstColumnStart, currentVerticalWriteOffset, 11.04f);
    }
    if (!m_afterTitleTextRight.empty())
    {
      float textWidth = (float) m_afterTitleTextRight.size() * charWidthArial11;
      float pdfRightEnd = pageWidth - m_horizontalPageMargin;
      float labelStartX = pdfRightEnd - textWidth;
//      InsertTextRightAligned(m_afterTitleTextRight, labelStartX, currentVerticalWriteOffset, textWidth, 11.04f);
      InsertText(m_afterTitleTextRight, labelStartX, currentVerticalWriteOffset, 11.04f);
      //InsertLine(labelStartX, currentVerticalWriteOffset, labelStartX + textWidth, currentVerticalWriteOffset);
    }
    currentVerticalWriteOffset -= 0.2f;
  }

  // add date or page-number if required/asked
  if (m_addDateTime)
  {
    string labelText = "Date/Time: ";
    InsertText(labelText, firstColumnStart, currentVerticalWriteOffset, 11.04f);
    float labelWidth = (float) labelText.size() * charWidthArial11;
    InsertText(getCurrentFormattedDateOrTime("%d-%b-%Y / %H:%M:%S"), firstColumnStart + labelWidth, currentVerticalWriteOffset, 11.04f);
  }
  if (m_addPageNumber)
  {
    string labelText = "Page: ";
    float labelWidth = (float) labelText.size() * charWidthArial11;
    int maxValueSize = 3; // 3 characters
    float valueWidth = (float) maxValueSize * charWidthArial11;
    float pdfRightEnd = (float) pageWidth - m_horizontalPageMargin;
    float textWidth = labelWidth + valueWidth;
    float labelStartX = pdfRightEnd - textWidth;
    InsertText(labelText, labelStartX, currentVerticalWriteOffset, 11.04f);
    string valueZeroPadded = string(maxValueSize - 1, '0').append(to_string(currentPageNumber));
    InsertText(valueZeroPadded, labelStartX + labelWidth, currentVerticalWriteOffset, 11.04f);
    //InsertLine(labelStartX, currentVerticalWriteOffset, labelStartX + textWidth, currentVerticalWriteOffset);
  }
  if (m_addDateTime || m_addPageNumber)
    currentVerticalWriteOffset -= 0.1f;
  if (!firstPageAdded)
    firstPageAdded = true;
}

void CustomPainter::InsertText(const string_view &str, double x, double y, double fontSize, bool isHeading)
{
  if (isHeading)
  {
    painter.GetTextState().SetFont(boldFont, fontSize);
    painter.DrawText(str, x * 72, y * 72);
  }
  else
  {
    painter.GetTextState().SetFont(normalFont, fontSize);
    painter.DrawText(str, x * 72, y * 72);
  }
}

void CustomPainter::InsertTextRightAligned(const string_view &str, double x, double y, double textWidth, double fontSize, bool isHeading)
{
  if (isHeading)
  {
    painter.GetTextState().SetFont(boldFont, fontSize);
    painter.DrawTextAligned(str, x * 72, y * 72, textWidth, PdfHorizontalAlignment::Right);
  }
  else
  {
    painter.GetTextState().SetFont(normalFont, fontSize);
    painter.DrawTextAligned(str, x * 72, y * 72, textWidth, PdfHorizontalAlignment::Right);
  }
}

void CustomPainter::InsertLine(double startX, double startY, double endX, double endY)
{
  painter.DrawLine(startX * 72, startY * 72, endX * 72, endY * 72);
}

void CustomPainter::InsertRect(double x1, double y1, double x2, double y2, bool drawLeftEdge)
{
  InsertLine(x1, y1, x2, y1);
  InsertLine(x2, y1, x2, y2);
  InsertLine(x2, y2, x1, y2);
  if (drawLeftEdge)
    InsertLine(x1, y2, x1, y1);
}

void CustomPainter::InsertImage(const std::string_view &imagePath, double posX, double posY)
{
  double adjustmentForLettersWithHangingPart = 1.0f;
  float imageTopPadding = 0.05f;

  PdfImage pdfImage(document);
  pdfImage.LoadFromFile(imagePath);
  unsigned int oWidth = pdfImage.GetWidth();
  unsigned int oHeight = pdfImage.GetHeight();

  double scaleX = 1.0f;
  double scaleY = 1.0f;
  float finalImageHeight = oHeight;
  double maxWidth = m_tableMaxImageWidthPerRow * 72;
  double maxHeight = (m_tableMaxImageHeightPerRow - 0.2) * 72;
  if (oWidth > maxWidth)
  {
    scaleX = maxWidth / oWidth;
    scaleY = scaleX; // maintain aspect ratio
    double scaleXHeight = oHeight * scaleY;
    finalImageHeight = scaleXHeight;
    if (scaleXHeight > maxHeight)
    {
      scaleY = maxHeight / scaleXHeight;
      scaleY = (scaleXHeight * scaleY) / oHeight;
      scaleX = scaleY; // maintain aspect ratio
      finalImageHeight = oHeight * scaleY;
    }
  }

  posY = currentVerticalWriteOffset - (finalImageHeight / 72) - imageTopPadding;

  posX *= 72;
  posY *= 72;
  painter.DrawImage(pdfImage, posX, posY - adjustmentForLettersWithHangingPart, scaleX, scaleY);
}

void CustomPainter::Terminate()
{
  painter.FinishDrawing();
}

int CustomPainter::WriteDocumentToFile(const char *filepath)
{
  Terminate();
  document.GetInfo().SetCreator(PdfString("pdfmm"));
  document.GetInfo().SetAuthor(PdfString("Umar Ali Khan"));
  document.GetInfo().SetTitle(PdfString("Custom Report"));
  document.GetInfo().SetSubject(PdfString("Sample Report"));
  document.GetInfo().SetKeywords(PdfString("Results;Report;"));
  document.Save(filepath);
  return 0;
}

double CustomPainter::GetPageHeight() const
{
  return m_pageHeight;
}

double CustomPainter::GetPageHeightInches() const
{
  return m_pageHeight / 72;
}

double CustomPainter::GetPageWidth() const
{
  return m_pageWidth;
}

double CustomPainter::GetPageWidthInches() const
{
  return m_pageWidth / 72;
}

void CustomPainter::OutputTableColHeaders(const double &fontSize, float rowTop)
{
  float firstColumnStart = m_horizontalPageMargin;
  m_tableHeaderFontSize = (float) fontSize;
  if (m_tableHeadingTexts == nullptr) {
    throw PdfGenerationException("NO-HEADERS-PROVIDED", "Set headings/headers first...");
  }

  if (rowTop == -1.0f)
  {
    rowTop = currentVerticalWriteOffset;
  }

  auto pageWidth = (float) GetPageWidthInches();
  float runningColStart = 0.0f;
  float previousColStart = 0.0f;
  float bottomPadding = 0.07f;
  float rowTop00 = rowTop - m_tableHeaderHeight + bottomPadding;
  float previousWidth = 0.0f;
  float currentWidth = 0.0f;
  float runningColEnd = 0.0f;
  bool isFirstColumn = false;
  bool isLastColumn = false;
  for (int i = 0; i < m_tableTotalCols; ++i)
  {
    isFirstColumn = i == 0;
    isLastColumn = i == (m_tableTotalCols - 1);
    currentWidth = isLastColumn ? (pageWidth - runningColEnd - m_horizontalPageMargin) : m_tableColWidths[i];
    runningColStart = (isFirstColumn ? firstColumnStart : previousColStart) + previousWidth;
    runningColEnd = runningColStart + currentWidth;
    InsertLine(runningColStart, rowTop, runningColEnd, rowTop);
    InsertText(m_tableHeadingTexts[i], runningColStart + m_tableCellLeftPadding, rowTop00, fontSize, true);
    InsertLine(runningColStart, rowTop - m_tableHeaderHeight, runningColEnd, rowTop - m_tableHeaderHeight);
    //cout << "col-end - " << i << "; w=" << getDoubleFormattedString(currentWidth, 4) << " + rcs=" << getDoubleFormattedString(runningColStart, 4) << " => rce=" << getDoubleFormattedString(runningColEnd, 4) << endl;
    previousColStart = runningColStart;
    previousWidth = currentWidth;
  }
  currentVerticalWriteOffset = rowTop - m_tableHeaderHeight;
}

void CustomPainter::startNextPage()
{
  // output outer lines for completed previous page
  OutputTableOuterLines();

  // increment page number
  currentPageNumber++;

  // start next page
  AddNewPage();
  OutputTableColHeaders(m_tableHeaderFontSize);
}

void CustomPainter::OutputTableRowValues(const std::string *valueTexts, const double &fontSize)
{
  float firstColumnStart = m_horizontalPageMargin;
  auto pageWidth = (float) GetPageWidthInches();
  float runningColStart = 0.0f;
  float previousColStart = 0.0f;
  float bottomPadding = 0.07f;
  float rowTop00 = currentVerticalWriteOffset - m_tableRowHeight + bottomPadding;
  float previousWidth = 0.0f;
  float currentWidth = 0.0f;
  float runningColEnd = 0.0f;
  bool isFirstColumn = false;
  bool isLastColumn = false;
  float rowHeight = m_tableImageColumnIndex > -1 ? max(m_tableRowHeight, m_tableMaxImageHeightPerRow) : m_tableRowHeight;
  for (int i = 0; i < m_tableTotalCols; ++i)
  {
    isFirstColumn = i == 0;
    isLastColumn = i == (m_tableTotalCols - 1);
    currentWidth = isLastColumn ? (pageWidth - runningColEnd - m_horizontalPageMargin) : m_tableColWidths[i];
    runningColStart = (isFirstColumn ? firstColumnStart : previousColStart) + previousWidth;
    runningColEnd = runningColStart + currentWidth;
    if (i == m_tableImageColumnIndex) // image column
    {
      string imageFullPath = m_tableImagesFolder.empty() ? valueTexts[i] : m_tableImagesFolder + "/" + valueTexts[i];
      InsertImage(imageFullPath, runningColStart + m_tableCellLeftPadding, rowTop00);
    } else {
      InsertText(valueTexts[i], runningColStart + m_tableCellLeftPadding, rowTop00, fontSize);
    }
    double bottomLineOffset = currentVerticalWriteOffset - rowHeight;
    InsertLine(runningColStart, bottomLineOffset, runningColEnd, bottomLineOffset);
    previousColStart = runningColStart;
    previousWidth = currentWidth;
  }
  currentVerticalWriteOffset -= rowHeight;
  if (currentVerticalWriteOffset - rowHeight < 0.25)
    startNextPage();
}

void CustomPainter::OutputTableOuterLines()
{
  float topStart = (float) GetPageHeightInches() - m_verticalPageMargin;
  float firstColumnStart = m_horizontalPageMargin;
  auto pageWidth = (float) GetPageWidthInches();
  float runningColStart = 0.0f;
  float previousColStart = firstColumnStart;
  float currentWidth = 0.0f;
  float runningColEnd = 0.0f;
  bool isLastColumn = false;

  if (!m_title.empty())
    topStart -= 0.2f;
  if (!(m_afterTitleTextLeft.empty() && m_afterTitleTextRight.empty()))
    topStart -= 0.2f;
  if (m_addDateTime || m_addPageNumber)
    topStart -= 0.1f;
  runningColStart = previousColStart + currentWidth;
  runningColEnd = runningColStart;
  InsertLine(runningColStart, currentVerticalWriteOffset, runningColEnd, topStart);
  for (int i = 0; i < m_tableTotalCols; ++i)
  {
    isLastColumn = i == (m_tableTotalCols - 1);
    currentWidth = isLastColumn ? (pageWidth - runningColEnd - m_horizontalPageMargin) : m_tableColWidths[i];
    runningColStart = previousColStart + currentWidth;
    runningColEnd = runningColStart;
    InsertLine(runningColStart, currentVerticalWriteOffset, runningColEnd, topStart);
    previousColStart = runningColStart;
  }
}

void CustomPainter::SetTableTotalCols(const int &value)
{
  m_tableTotalCols = value;
}

//void CustomPainter::SetFirstColumnStart(const float &value)
//{
//  if (firstPageAdded)
//    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
//  firstColumnStart = value;
//}

//void CustomPainter::SetTopStart(const float &value)
//{
//  if (firstPageAdded)
//    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
//  topStart = value;
//}

void CustomPainter::SetTableColWidths(float *values)
{
  m_tableColWidths = values;
}

void CustomPainter::SetTableRowHeight(const float &value)
{
  m_tableRowHeight = value;
}

void CustomPainter::SetTableMaxImageHeightPerRow(const float &value)
{
  m_tableMaxImageHeightPerRow = value;
}

void CustomPainter::SetTableImageColumnIndex(const int &value)
{
  m_tableImageColumnIndex = value;
}

void CustomPainter::SetTableImagesFolder(const char *value)
{
  m_tableImagesFolder = string(value);
}

void CustomPainter::SetTableMaxImageWidthPerRow(const float &value)
{
  m_tableMaxImageWidthPerRow = value;
}

void CustomPainter::SetTableRowTopPadding(const float &value)
{
  m_tableRowTopPadding = value;
}

void CustomPainter::SetTableHeadingTexts(string *value)
{
  m_tableHeadingTexts = value;
}

void CustomPainter::SetTitle(const string &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_title = value;
}

void CustomPainter::SetAddDateTime(const bool &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_addDateTime = value;
}

void CustomPainter::SetAddPageNumber(const bool &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_addPageNumber = value;
}

void CustomPainter::SetAfterTitleTextLeft(const string &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_afterTitleTextLeft = value;
}

void CustomPainter::SetAfterTitleTextRight(const string &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_afterTitleTextRight = value;
}

void CustomPainter::SetHorizontalPageMargin(const float &value)
{
  if (firstPageAdded)
    throw PdfGenerationException("LOGICALLY-INCORRECT-METHOD-CALL", "Call this method before AddNewPage() method");
  m_horizontalPageMargin = value;
}

void CustomPainter::SetTableCellLeftPadding(const float &value)
{
  m_tableCellLeftPadding = value;
}

void CustomPainter::SetVerticalPageMargin(const float &value)
{
  m_verticalPageMargin = value;
}
