/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfMemDocument.h"

#include <algorithm>
#include <deque>

#include "PdfParser.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfImmediateWriter.h"
#include "PdfObject.h"
#include "PdfParserObject.h"
#include "PdfObjectStream.h"
#include "PdfIndirectObjectList.h"
#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNameTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPageCollection.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace mm;

PdfMemDocument::PdfMemDocument()
    : PdfMemDocument(false) { }

PdfMemDocument::PdfMemDocument(bool empty) :
    PdfDocument(empty),
    m_Version(PdfVersionDefault),
    m_InitialVersion(PdfVersionDefault),
    m_HasXRefStream(false),
    m_PrevXRefOffset(-1)
{
}

PdfMemDocument::PdfMemDocument(const shared_ptr<InputStreamDevice>& device, const string_view& password)
    : PdfMemDocument(true)
{
    if (device == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromDevice(device, password);
}

PdfMemDocument::PdfMemDocument(const PdfMemDocument& rhs) :
    PdfDocument(rhs),
    m_Version(rhs.m_Version),
    m_InitialVersion(rhs.m_InitialVersion),
    m_HasXRefStream(rhs.m_HasXRefStream),
    m_PrevXRefOffset(rhs.m_PrevXRefOffset)
{
    auto encryptObj = GetTrailer().GetDictionary().FindKey("Encrypt");
    if (encryptObj != nullptr)
        m_Encrypt = PdfEncrypt::CreatePdfEncrypt(*encryptObj);
}

PdfMemDocument::~PdfMemDocument()
{
    // NOTE: Don't call PdfMemDocument::Clear() here,
    // ~PdfDocument() will call its PdfDocument::Clear()
    clear();
}

void PdfMemDocument::Clear()
{
    // Do clear both locally defined variables and inherited ones
    clear();
    PdfDocument::Clear();
}

void PdfMemDocument::clear()
{
    m_HasXRefStream = false;
    m_PrevXRefOffset = -1;
    m_Encrypt = nullptr;
    m_device = nullptr;
}

void PdfMemDocument::initFromParser(PdfParser& parser)
{
    m_Version = parser.GetPdfVersion();
    m_InitialVersion = m_Version;
    m_HasXRefStream = parser.HasXRefStream();
    m_PrevXRefOffset = parser.GetXRefOffset();

    auto trailer = std::make_unique<PdfObject>(parser.GetTrailer());
    this->SetTrailer(std::move(trailer)); // Set immediately as trailer
                                // so that trailer has an owner

    if (PdfError::IsLoggingSeverityEnabled(PdfLogSeverity::Debug))
    {
        auto debug = GetTrailer().GetObject().GetVariant().ToString();
        debug.push_back('\n');
        mm::LogMessage(PdfLogSeverity::Debug, debug);
    }

    if (parser.IsEncrypted())
    {
        // All PdfParser instances have a pointer to a PdfEncrypt object.
        // So we have to take ownership of it (command the parser to give it).
        m_Encrypt = parser.TakeEncrypt();
    }

    Init();
}

void PdfMemDocument::Load(const string_view& filename, const string_view& password)
{
    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto device = std::make_shared<FileStreamDevice>(filename);
    LoadFromDevice(device, password);
}

void PdfMemDocument::LoadFromBuffer(const bufferview& buffer, const string_view& password)
{
    if (buffer.size() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto device = std::make_shared<SpanStreamDevice>(buffer);
    LoadFromDevice(device, password);
}

void PdfMemDocument::LoadFromDevice(const shared_ptr<InputStreamDevice>& device, const string_view& password)
{
    if (device == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    this->Clear();
    loadFromDevice(device, password);
}

void PdfMemDocument::loadFromDevice(const shared_ptr<InputStreamDevice>& device, const string_view& password)
{
    m_device = device;

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.Parse(*device, true);
    initFromParser(parser);
}

void PdfMemDocument::AddPdfExtension(const PdfName& ns, int64_t level)
{
    if (!this->HasPdfExtension(ns, level))
    {

        PdfObject* extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
        PdfDictionary newExtension;

        newExtension.AddKey("BaseVersion", PdfName(mm::GetPdfVersionName(m_Version)));
        newExtension.AddKey("ExtensionLevel", PdfVariant(level));

        if (extensions != nullptr && extensions->IsDictionary())
        {
            extensions->GetDictionary().AddKey(ns, newExtension);
        }
        else
        {
            PdfDictionary extensions;
            extensions.AddKey(ns, newExtension);
            this->GetCatalog().GetDictionary().AddKey("Extensions", extensions);
        }
    }
}

bool PdfMemDocument::HasPdfExtension(const PdfName& ns, int64_t level) const {

    auto extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
    if (extensions != nullptr)
    {
        auto extension = extensions->GetDictionary().FindKey(ns);
        if (extension != nullptr)
        {
            auto levelObj = extension->GetDictionary().FindKey("ExtensionLevel");
            if (levelObj != nullptr && levelObj->IsNumber() && levelObj->GetNumber() == level)
                return true;
        }
    }

    return false;
}

/** Return the list of all vendor-specific extensions to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
vector<PdfExtension> PdfMemDocument::GetPdfExtensions() const
{
    vector<PdfExtension> ret;
    auto extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
    if (extensions == nullptr)
        return ret;

    // Loop through all declared extensions
    for (auto& pair : extensions->GetDictionary())
    {
        auto bv = pair.second.GetDictionary().FindKey("BaseVersion");
        auto el = pair.second.GetDictionary().FindKey("ExtensionLevel");

        if (bv != nullptr && el != nullptr && bv->IsName() && el->IsNumber())
        {
            // Convert BaseVersion name to PdfVersion
            auto version = mm::GetPdfVersion(bv->GetName().GetString());
            if (version != PdfVersion::Unknown)
                ret.push_back(PdfExtension(pair.first.GetString(), version, el->GetNumber()));
        }
    }
    return ret;
}

/** Remove a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::RemovePdfExtension(const PdfName& ns, int64_t level)
{
    if (this->HasPdfExtension(ns, level))
        this->GetCatalog().GetDictionary().FindKey("Extensions")->GetDictionary().RemoveKey("ns");
}

void PdfMemDocument::Save(const string_view& filename, PdfSaveOptions options)
{
    FileStreamDevice device(filename, FileMode::Create);
    this->Save(device, options);
}

void PdfMemDocument::Save(OutputStreamDevice& device, PdfSaveOptions opts)
{
    beforeWrite(opts);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject());
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetSaveOptions(opts);

    if (m_Encrypt != nullptr)
        writer.SetEncrypted(*m_Encrypt);

    try
    {
        writer.Write(device);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME(e);
        throw e;
    }
}

void PdfMemDocument::SaveUpdate(const string_view& filename, PdfSaveOptions opts)
{
    FileStreamDevice device(filename, FileMode::Append);
    this->SaveUpdate(device, opts);
}

void PdfMemDocument::SaveUpdate(OutputStreamDevice& device, PdfSaveOptions opts)
{
    beforeWrite(opts);

    PdfWriter writer(this->GetObjects(), this->GetTrailer().GetObject());
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetSaveOptions(opts);
    writer.SetPrevXRefOffset(m_PrevXRefOffset);
    writer.SetUseXRefStream(m_HasXRefStream);
    writer.SetIncrementalUpdate(false);

    if (m_Encrypt != nullptr)
        writer.SetEncrypted(*m_Encrypt);

    if (m_InitialVersion < this->GetPdfVersion())
    {
        if (this->GetPdfVersion() < PdfVersion::V1_0 || this->GetPdfVersion() > PdfVersion::V1_7)
            PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        GetCatalog().GetDictionary().AddKey("Version", PdfName(mm::GetPdfVersionName(GetPdfVersion())));
    }

    try
    {
        device.Seek(0, SeekDirection::End);
        writer.Write(device);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME(e);
        throw e;
    }
}

void PdfMemDocument::beforeWrite(PdfSaveOptions opts)
{
    if ((opts & PdfSaveOptions::NoModifyDateUpdate) ==
        PdfSaveOptions::None)
    {
        GetMetadata().SetModifyDate(PdfDate(), true);
    }

    GetFontManager().EmbedFonts();
}

void PdfMemDocument::deletePages(unsigned atIndex, unsigned pageCount)
{
    for (unsigned i = 0; i < pageCount; i++)
        this->GetPages().DeletePage(atIndex);
}

const PdfMemDocument& PdfMemDocument::InsertPages(const PdfMemDocument& doc, unsigned atIndex, unsigned pageCount)
{
    /*
      This function works a bit different than one might expect.
      Rather than copying one page at a time - we copy the ENTIRE document
      and then delete the pages we aren't interested in.

      We do this because
      1) SIGNIFICANTLY simplifies the process
      2) Guarantees that shared objects aren't copied multiple times
      3) offers MUCH faster performance for the common cases

      HOWEVER: because pdfmm doesn't currently do any sort of "object garbage collection" during
      a Write() - we will end up with larger documents, since the data from unused pages
      will also be in there.
    */

    // calculate preliminary "left" and "right" page ranges to delete
    // then offset them based on where the pages were inserted
    // NOTE: some of this will change if/when we support insertion at locations
    //       OTHER than the end of the document!
    unsigned leftStartPage = 0;
    unsigned leftCount = atIndex;
    unsigned rightStartPage = atIndex + pageCount;
    unsigned rightCount = doc.GetPages().GetCount() - rightStartPage;
    unsigned pageOffset = this->GetPages().GetCount();

    leftStartPage += pageOffset;
    rightStartPage += pageOffset;

    // append in the whole document
    this->Append(doc);

    // delete
    if (rightCount > 0)
        this->deletePages(rightStartPage, rightCount);
    if (leftCount > 0)
        this->deletePages(leftStartPage, leftCount);

    return *this;
}

void PdfMemDocument::SetEncrypted(const string& userPassword, const string& ownerPassword,
    PdfPermissions protection, PdfEncryptAlgorithm algorithm,
    PdfKeyLength keyLength)
{
    m_Encrypt = PdfEncrypt::CreatePdfEncrypt(userPassword, ownerPassword, protection, algorithm, keyLength);
}

void PdfMemDocument::SetEncrypted(const PdfEncrypt& encrypt)
{
    m_Encrypt = PdfEncrypt::CreatePdfEncrypt(encrypt);
}

void PdfMemDocument::FreeObjectMemory(const PdfReference& ref, bool force)
{
    FreeObjectMemory(this->GetObjects().GetObject(ref), force);
}

void PdfMemDocument::FreeObjectMemory(PdfObject* obj, bool force)
{
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(obj);
    if (parserObject == nullptr)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle,
            "FreeObjectMemory works only on classes of type PdfParserObject");
    }

    parserObject->FreeObjectMemory(force);
}

const PdfEncrypt* PdfMemDocument::GetEncrypt() const
{
    return m_Encrypt.get();
}

void PdfMemDocument::SetPdfVersion(PdfVersion version)
{
    m_Version = version;
}

PdfVersion PdfMemDocument::GetPdfVersion() const
{
    return m_Version;
}
