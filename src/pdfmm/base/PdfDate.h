/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DATE_H
#define PDF_DATE_H

#include <chrono>

#include "PdfDeclarations.h"
#include "PdfString.h"

namespace mm {

/** This class is a date datatype as specified in the PDF
 *  reference. You can easily convert from Unix time_t to
 *  the PDF time representation and back. Dates like these
 *  are used for example in the PDF info dictionary for the
 *  creation time and date of the PDF file.
 *
 *  PdfDate objects are immutable.
 *
 *  From the PDF reference:
 *
 *  PDF defines a standard date format, which closely follows
 *  that of the international standard ASN.1 (Abstract Syntax
 *  Notation One), defined in ISO/IEC 8824 (see the Bibliography).
 *  A date is a string of the form
 *  (D:YYYYMMDDHHmmSSOHH'mm')
 */
class PDFMM_API PdfDate final
{
public:
    /** Create a PdfDate object with the current date and time.
     */
    PdfDate();

    /** Create a PdfDate with a specified date and time
     *  \param t the date and time of this object
     *
     *  \see IsValid()
     */
    PdfDate(const std::chrono::seconds& secondsFromEpoch, const nullable<std::chrono::minutes>& offsetFromUTC);

    /** Create a PdfDate with a specified date and time
     *  \param dateStr the date and time of this object
     *         in PDF format. It has to be a string of
     *         in the format "D:YYYYMMDDHHmmSSOHH'mm'"
     */
    static PdfDate Parse(const std::string_view& dateStr);
    static bool TryParse(const std::string_view& dateStr, PdfDate& date);

    /** Create a PdfDate with a specified date and time
     *  \param dateStr the date and time of this object
     *         in W3C format. It has to be a string of
     *         the format "YYYY-MM-DDTHH:mm:SSOHH:mm"
     */
    static PdfDate ParseW3C(const std::string_view& dateStr);
    static bool TryParseW3C(const std::string_view& dateStr, PdfDate& date);

    /** \returns the date and time of this PdfDate in
     *  seconds since epoch.
     */
    const std::chrono::seconds& GetSecondsFromEpoch() const { return m_SecondsFromEpoch; }

    const nullable<std::chrono::minutes>& GetMinutesFromUtc() const { return m_MinutesFromUtc; }

    /** The value returned by this function can be used in any PdfObject
     *  where a date is needed
     */
    PdfString ToString() const;

    /** The value returned is a W3C compliant date representation
     */
    PdfString ToStringW3C() const;

    bool operator==(const PdfDate& rhs) const;
    bool operator!=(const PdfDate& rhs) const;

private:
    /** Creates the internal string representation from
     *  a time_t value and writes it to m_szDate.
     */
    PdfString createStringRepresentation(bool w3cstring) const;

private:
    std::chrono::seconds m_SecondsFromEpoch;
    nullable<std::chrono::minutes> m_MinutesFromUtc;
};

};

#endif // PDF_DATE_H
