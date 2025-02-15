/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

// All Standard14 fonts have glyphs that start with a
// white space (code 0x20, or 32)
static constexpr unsigned DEFAULT_STD14_FIRSTCHAR = 32;

PdfFontObject::PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) :
    PdfFont(obj, metrics, encoding) { }

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, PdfObject& descendantObj,
    const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding)
{
    (void)descendantObj;
    // TODO: MAke a virtual GetDescendantFontObject()
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, metrics, encoding));
}

unique_ptr<PdfFontObject> PdfFontObject::Create(PdfObject& obj, const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding)
{
    return unique_ptr<PdfFontObject>(new PdfFontObject(obj, metrics, encoding));
}

bool PdfFontObject::tryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    if (m_Metrics->IsStandard14FontMetrics() && !m_Encoding->HasParsedLimits())
    {
        gid = cid - DEFAULT_STD14_FIRSTCHAR;
    }
    else
    {
        // We just convert to a GID using /FirstChar
        gid = cid - m_Encoding->GetFirstChar().Code;
    }

    return true;
}

bool PdfFontObject::IsObjectLoaded() const
{
    return true;
}

PdfFontType PdfFontObject::GetType() const
{
    // TODO Just read the object from the object
    return PdfFontType::Unknown;
}
