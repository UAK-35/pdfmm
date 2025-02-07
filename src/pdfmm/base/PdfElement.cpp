/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfElement.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfObject.h"

#include "PdfStreamedDocument.h"

using namespace std;
using namespace mm;

PdfElement::PdfElement(PdfObject& obj)
    : m_Object(&obj)
{
    if (obj.GetDocument() == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

PdfElement::PdfElement(PdfObject& obj, PdfDataType expectedDataType)
    : m_Object(&obj)
{
    if (obj.GetDocument() == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (obj.GetDataType() != expectedDataType)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
}

PdfElement::~PdfElement() { }

PdfDocument& PdfElement::GetDocument() const
{
    return *m_Object->GetDocument();
}

PdfDictionaryElement::PdfDictionaryElement(PdfDocument& parent, const string_view& type,
    const string_view& subtype)
    : PdfElement(*parent.GetObjects().CreateDictionaryObject(type, subtype),
        PdfDataType::Dictionary)
{
}

PdfDictionaryElement::PdfDictionaryElement(PdfObject& obj)
    : PdfElement(obj, PdfDataType::Dictionary)
{
}

PdfDictionary& PdfDictionaryElement::GetDictionary()
{
    return GetObject().GetDictionary();
}

const PdfDictionary& PdfDictionaryElement::GetDictionary() const
{
    return GetObject().GetDictionary();
}

PdfArrayElement::PdfArrayElement(PdfDocument& parent)
    : PdfElement(*parent.GetObjects().CreateObject(PdfArray()),
        PdfDataType::Array)
{
}

PdfArrayElement::PdfArrayElement(PdfObject& obj)
    : PdfElement(obj, PdfDataType::Array)
{
}

PdfArray& PdfArrayElement::GetArray()
{
    return GetObject().GetArray();
}

const PdfArray& PdfArrayElement::GetArray() const
{
    return GetObject().GetArray();
}
