/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCanvas.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfColor.h"
#include "PdfObjectStream.h"

using namespace std;
using namespace mm;

PdfCanvas::~PdfCanvas() { }

const PdfObject* PdfCanvas::GetContentsObject() const
{
    return getContentsObject();
}

PdfObject* PdfCanvas::GetContentsObject()
{
    return getContentsObject();
}

PdfObject* PdfCanvas::GetFromResources(const string_view& type, const string_view& key)
{
    return getFromResources(type, key);
}

const PdfObject* PdfCanvas::GetFromResources(const string_view& type, const string_view& key) const
{
    return getFromResources(type, key);
}

PdfResources* PdfCanvas::GetResources()
{
    return getResources();
}

const PdfResources* PdfCanvas::GetResources() const
{
    return getResources();
}

PdfObject* PdfCanvas::getFromResources(const string_view& type, const string_view& key) const
{
    auto resources = getResources();
    if (resources == nullptr)
        return nullptr;

    return resources->GetResource(type, key);

}

PdfArray PdfCanvas::GetProcSet()
{
    PdfArray procset;
    procset.Add(PdfName("PDF"));
    procset.Add(PdfName("Text"));
    procset.Add(PdfName("ImageB"));
    procset.Add(PdfName("ImageC"));
    procset.Add(PdfName("ImageI"));
    return procset;
}

void PdfCanvas::AddColorResource(const PdfColor& color)
{
    auto& resources = GetOrCreateResources();
    switch (color.GetColorSpace())
    {
        case PdfColorSpace::Separation:
        {
            string csPrefix("ColorSpace");
            string csName = color.GetName();
            string temp(csPrefix + csName);

            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey(csPrefix + csName))
            {
                // Build color-spaces for separation
                PdfObject* csp = color.BuildColorSpace(resources.GetDocument());

                AddResource(csPrefix + csName, csp->GetIndirectReference(), "ColorSpace");
            }
        }
        break;

        case PdfColorSpace::CieLab:
        {
            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey("ColorSpaceLab"))
            {
                // Build color-spaces for CIE-lab
                PdfObject* csp = color.BuildColorSpace(resources.GetDocument());

                AddResource("ColorSpaceCieLab", csp->GetIndirectReference(), "ColorSpace");
            }
        }
        break;

        case PdfColorSpace::DeviceGray:
        case PdfColorSpace::DeviceRGB:
        case PdfColorSpace::DeviceCMYK:
        case PdfColorSpace::Indexed:
            // No colorspace needed
        case PdfColorSpace::Unknown:
        default:
            break;
    }
}

void PdfCanvas::AddResource(const PdfName& identifier, const PdfReference& ref, const PdfName& name)
{
    if (name.IsNull() || identifier.IsNull())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto& resources = this->GetOrCreateResources();
    if (!resources.GetDictionary().HasKey(name))
        resources.GetDictionary().AddKey(name, PdfDictionary());

    if (resources.GetDictionary().GetKey(name)->GetDataType() == PdfDataType::Reference)
    {
        auto directObject = resources.GetDocument().GetObjects().GetObject(resources.GetDictionary().GetKey(name)->GetReference());
        if (directObject == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

        if (!directObject->GetDictionary().HasKey(identifier))
            directObject->GetDictionary().AddKey(identifier, ref);
    }
    else
    {
        if (!resources.GetDictionary().GetKey(name)->GetDictionary().HasKey(identifier))
            resources.GetDictionary().GetKey(name)->GetDictionary().AddKey(identifier, ref);
    }
}
