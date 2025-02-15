/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_MAP_FACTORY_H
#define PDF_ENCODING_MAP_FACTORY_H

#include "PdfEncodingMap.h"

namespace mm {

/** This factory creates a PdfEncodingMap
 */
class PDFMM_API PdfEncodingMapFactory final
{
    friend class PdfEncoding;
    friend class PdfEncodingFactory;
    friend class PdfDifferenceEncoding;

public:
    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     *
     *  \see Win1250EncodingInstance
     */
    static PdfEncodingMapConstPtr WinAnsiEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static PdfEncodingMapConstPtr MacRomanEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfEncodingMapConstPtr MacExpertEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Horizontal IndentityEncoding
     *
     *  \returns global instance of Horizontal IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesHorizontalIdentityEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Vertical IndentityEncoding
     *
     *  \returns global instance of Vertical IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesVerticalIdentityEncodingInstance();

    /** Return the encoding map for the given standard font type or nullptr for unknown
     */
    static PdfEncodingMapConstPtr GetStandard14FontEncodingMap(PdfStandard14FontType stdFont);

private:
    PdfEncodingMapFactory() = delete;

    // The following encodings are for internal use only

    /** Singleton method which returns a global instance
     *  of StandardEncoding.
     *
     *  \returns global instance of StandardEncoding
     */
    static PdfEncodingMapConstPtr StandardEncodingInstance();

    /** Singleton method which returns a global instance
     *  of SymbolEncoding.
     *
     *  \returns global instance of SymbolEncoding
     */
    static PdfEncodingMapConstPtr SymbolEncodingInstance();

    /** Singleton method which returns a global instance
     *  of ZapfDingbatsEncoding.
     *
     *  \returns global instance of ZapfDingbatsEncoding
     */
    static PdfEncodingMapConstPtr ZapfDingbatsEncodingInstance();

    static PdfEncodingMapConstPtr GetNullEncodingMap();
};

}

#endif // PDF_ENCODING_MAP_FACTORY_H
