/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SIMPLE_ENCODING_H
#define PDF_SIMPLE_ENCODING_H

#include "PdfEncodingMap.h"

namespace mm
{
    /**
     * A common base class for Pdf defined predefined encodings which are
     * known by name.
     *
     *  - WinAnsiEncoding
     *  - MacRomanEncoding
     *  - MacExpertEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     */
    class PDFMM_API PdfPredefinedEncoding : public PdfBuiltInEncoding
    {
    protected:
        PdfPredefinedEncoding(const PdfName& name);

    protected:
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    };

    /**
     * The WinAnsiEncoding is the default encoding in pdfmm for
     * contents on PDF pages.
     *
     * It is also called CP-1252 encoding.
     * This class may be used as base for derived encodings.
     *
     * \see PdfWin1250Encoding
     *
     * \see PdfFont::WinAnsiEncoding
     */
    class PDFMM_API PdfWinAnsiEncoding : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;
        friend class PdfWin1250Encoding;
        friend class PdfIso88592Encoding;

    private:
        PdfWinAnsiEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from WinAnsiEncoding to UTF16

    };

    /**
     * MacRomanEncoding 
     */
    class PDFMM_API PdfMacRomanEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacRomanEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacRomanEncoding to UTF16
    };

    /**
     * MacExpertEncoding
     */
    class PDFMM_API PdfMacExpertEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacExpertEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacExpertEncoding to UTF16
    };

    /**
     * StandardEncoding
     */
    class PDFMM_API PdfStandardEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfStandardEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from StandardEncoding to UTF16
    };

    /**
     * Symbol Encoding
     */
    class PDFMM_API PdfSymbolEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfSymbolEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from SymbolEncoding to UTF16
    };

    /**
     * ZapfDingbats encoding
     */
    class PDFMM_API PdfZapfDingbatsEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfZapfDingbatsEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from ZapfDingbatsEncoding to UTF16
    };
}

#endif // PDF_SIMPLE_ENCODING_H
