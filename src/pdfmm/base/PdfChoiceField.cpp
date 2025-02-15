/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfChoiceField.h"

#include "PdfArray.h"

using namespace std;
using namespace mm;


PdChoiceField::PdChoiceField(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfField(fieldType, doc, widget, insertInAcroform)
{
}

PdChoiceField::PdChoiceField(PdfFieldType fieldType, PdfObject& obj, PdfAnnotation* widget)
    : PdfField(fieldType, obj, widget)
{
}

PdChoiceField::PdChoiceField(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect)
    : PdfField(fieldType, page, rect)
{
}

void PdChoiceField::InsertItem(const PdfString& value, nullable<const PdfString&> displayName)
{
    PdfObject objToAdd;
    if (displayName.has_value())
    {
        PdfArray array;
        array.Add(value);
        array.Add(*displayName);
        objToAdd = array;
    }
    else
    {
        objToAdd = value;
    }

    auto optObj = GetObject().GetDictionary().FindKey("Opt");
    if (optObj == nullptr)
        optObj = &GetObject().GetDictionary().AddKey("Opt", PdfArray());

    // TODO: Sorting
    optObj->GetArray().Add(objToAdd);

    // CHECK-ME: Should I set /V /I here?
}

void PdChoiceField::RemoveItem(unsigned index)
{
    auto optObj = GetObject().GetDictionary().FindKey("Opt");
    if (optObj == nullptr)
        return;

    auto& arr = optObj->GetArray();
    if (index >= arr.size())
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    arr.RemoveAt(index);
}

PdfString PdChoiceField::GetItem(unsigned index) const
{
    auto opt = GetObject().GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto& optArray = opt->GetArray();
    if (index >= optArray.GetSize())
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    auto& item = optArray[index];
    if (item.IsArray())
    {
        auto& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
        }
        else
            return itemArray.FindAt(0).GetString();
    }

    return item.GetString();
}

nullable<PdfString> PdChoiceField::GetItemDisplayText(int index) const
{
    auto* opt = GetObject().GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return { };

    auto& optArray = opt->GetArray();
    if (index < 0 || index >= static_cast<int>(optArray.size()))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
    }

    auto& item = optArray[index];
    if (item.IsArray())
    {
        auto& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
        }
        else
            return itemArray.FindAt(1).GetString();
    }

    return item.GetString();
}

size_t PdChoiceField::GetItemCount() const
{
    auto* opt = GetObject().GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return 0;

    return opt->GetArray().size();
}

void PdChoiceField::SetSelectedIndex(int index)
{
    AssertTerminalField();
    PdfString selected = this->GetItem(index);
    GetObject().GetDictionary().AddKey("V", selected);
}

int PdChoiceField::GetSelectedIndex() const
{
    AssertTerminalField();
    auto* valueObj = GetObject().GetDictionary().FindKey("V");
    if (valueObj == nullptr || !valueObj->IsString())
        return -1;

    auto& value = valueObj->GetString();
    auto* opt = GetObject().GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return -1;

    auto& optArray = opt->GetArray();
    for (unsigned i = 0; i < optArray.GetSize(); i++)
    {
        auto& found = optArray.FindAt(i);
        if (found.IsString())
        {
            if (found.GetString() == value)
                return i;
        }
        else if (found.IsArray())
        {
            auto& arr = found.GetArray();
            if (arr.FindAt(0).GetString() == value)
                return i;
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Choice field item has invaid data type");
        }
    }

    return -1;
}

bool PdChoiceField::IsComboBox() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

void PdChoiceField::SetSpellcheckingEnabled(bool spellCheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), !spellCheck);
}

bool PdChoiceField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), true);
}

void PdChoiceField::SetSorted(bool sorted)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Sort), sorted);
}

bool PdChoiceField::IsSorted() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Sort), false);
}

void PdChoiceField::SetMultiSelect(bool multi)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), multi);
}

bool PdChoiceField::IsMultiSelect() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), false);
}

void PdChoiceField::SetCommitOnSelectionChange(bool commit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), commit);
}

bool PdChoiceField::IsCommitOnSelectionChange() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), false);
}
