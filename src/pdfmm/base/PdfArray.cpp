/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfArray.h"
#include "PdfOutputDevice.h"

using namespace mm;

PdfArray::PdfArray() { }

PdfArray::PdfArray(const PdfObject& var)
{
    add(var);
}

void PdfArray::RemoveAt(unsigned idx)
{
    // TODO: Set dirty only if really removed
    if (idx >= m_Objects.size())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    m_Objects.erase(m_Objects.begin() + idx);
    SetDirty();
}

const PdfObject& PdfArray::FindAt(unsigned idx) const
{
    return findAt(idx);
}

PdfObject& PdfArray::FindAt(unsigned idx)
{
    return findAt(idx);
}

unsigned PdfArray::GetSize() const
{
    return (unsigned)m_Objects.size();
}

PdfObject& PdfArray::Add(const PdfObject& obj)
{
    AssertMutable();
    auto& ret = add(obj);
    SetDirty();
    return ret;
}

void PdfArray::AddIndirect(const PdfObject* obj)
{
    AssertMutable();
    if (obj == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall not be null");

    if (IsIndirectReferenceAllowed(*obj))
        add(obj->GetIndirectReference());
    else
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");

    SetDirty();
}

PdfObject& PdfArray::AddIndirectSafe(const PdfObject& obj)
{
    PdfObject& ret = IsIndirectReferenceAllowed(obj)
        ? add(obj.GetIndirectReference())
        : add(obj);
    SetDirty();
    return ret;
}

void PdfArray::SetAt(const PdfObject& obj, unsigned idx)
{
    AssertMutable();
    if (IsIndirectReferenceAllowed(obj))
    {
        m_Objects.at(idx) = PdfObject(obj.GetIndirectReference());
        return;
    }

    m_Objects.at(idx) = obj;
}

void PdfArray::SetAtIndirect(const PdfObject& obj, unsigned idx)
{
    AssertMutable();
    m_Objects.at(idx) = obj;
}

PdfArrayIndirectIterator PdfArray::GetIndirectIterator()
{
    return PdfArrayIndirectIterator(*this);
}

const PdfArrayIndirectIterator PdfArray::GetIndirectIterator() const
{
    return PdfArrayIndirectIterator(const_cast<PdfArray&>(*this));
}

void PdfArray::Clear()
{
    clear();
}

void PdfArray::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    const PdfEncrypt* encrypt) const
{
    auto it = m_Objects.begin();

    int count = 1;

    if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
        device.Write("[ ");
    else
        device.Put('[');

    while (it != m_Objects.end())
    {
        it->GetVariant().Write(device, writeMode, encrypt);
        if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
        {
            device.Put((count % 10 == 0) ? '\n' : ' ');
        }

        it++;
        count++;
    }

    device.Put(']');
}

void PdfArray::ResetDirtyInternal()
{
    // Propagate state to all subclasses
    for (auto& obj : m_Objects)
        obj.ResetDirty();
}

void PdfArray::SetOwner(PdfObject* owner)
{
    PdfDataContainer::SetOwner(owner);
    auto document = owner->GetDocument();
    if (document != nullptr)
    {
        // Set owmership for all children
        for (auto& obj : m_Objects)
            obj.SetDocument(*document);
    }
}

PdfObject& PdfArray::add(const PdfObject& obj)
{
    return *insertAt(m_Objects.end(), obj);
}

PdfArray::iterator PdfArray::insertAt(const iterator& pos, const PdfObject& val)
{
    auto ret = m_Objects.insert(pos, val);
    ret->SetParent(this);
    auto document = GetObjectDocument();
    if (document != nullptr)
        ret->SetDocument(*document);

    return ret;
}

PdfObject& PdfArray::getAt(unsigned idx) const
{
    if (idx >= (unsigned)m_Objects.size())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    return const_cast<PdfArray&>(*this).m_Objects[idx];
}

PdfObject& PdfArray::findAt(unsigned idx) const
{
    if (idx >= (unsigned)m_Objects.size())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index is out of bounds");

    PdfObject& obj = const_cast<PdfArray&>(*this).m_Objects[idx];
    if (obj.IsReference())
        return GetIndirectObject(obj.GetReference());
    else
        return obj;
}

void PdfArray::push_back(const PdfObject& obj)
{
    AssertMutable();
    add(obj);
    SetDirty();
}

size_t PdfArray::size() const
{
    return m_Objects.size();
}

bool PdfArray::empty() const
{
    return m_Objects.empty();
}

void PdfArray::clear()
{
    AssertMutable();
    if (m_Objects.size() == 0)
        return;

    m_Objects.clear();
    SetDirty();
}

PdfArray::iterator PdfArray::insert(const iterator& pos, const PdfObject& obj)
{
    AssertMutable();
    auto ret = insertAt(pos, obj);
    SetDirty();
    return ret;
}

void PdfArray::erase(const iterator& pos)
{
    // TODO: Set dirty only if really removed
    AssertMutable();
    m_Objects.erase(pos);
    SetDirty();
}

void PdfArray::erase(const iterator& first, const iterator& last)
{
    // TODO: Set dirty only if really removed
    AssertMutable();
    m_Objects.erase(first, last);
    SetDirty();
}

void PdfArray::resize(size_t count, const PdfObject& val)
{
    AssertMutable();

    size_t currentSize = m_Objects.size();
    m_Objects.resize(count, val);
    auto document = GetObjectDocument();

    for (size_t i = currentSize; i < count; i++)
    {
        auto& obj = m_Objects[i];
        obj.SetParent(this);
        if (document != nullptr)
            obj.SetDocument(*document);
    }

    if (currentSize != count)
        SetDirty();
}

void PdfArray::reserve(size_type n)
{
    AssertMutable();
    m_Objects.reserve(n);
}

PdfObject& PdfArray::operator[](size_type idx)
{
    AssertMutable();
    return getAt((unsigned)idx);
}

const PdfObject& PdfArray::operator[](size_type idx) const
{
    return getAt((unsigned)idx);
}

PdfArray::iterator PdfArray::begin()
{
    AssertMutable();
    return m_Objects.begin();
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_Objects.begin();
}

PdfArray::iterator PdfArray::end()
{
    AssertMutable();
    return m_Objects.end();
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_Objects.end();
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    AssertMutable();
    return m_Objects.rbegin();
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_Objects.rbegin();
}

PdfArray::reverse_iterator PdfArray::rend()
{
    AssertMutable();
    return m_Objects.rend();
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_Objects.rend();
}

PdfObject& PdfArray::front()
{
    AssertMutable();
    return m_Objects.front();
}

const PdfObject& PdfArray::front() const
{
    return m_Objects.front();
}

PdfObject& PdfArray::back()
{
    AssertMutable();
    return m_Objects.back();
}

const PdfObject& PdfArray::back() const
{
    return m_Objects.back();
}

bool PdfArray::operator==(const PdfArray& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_Objects == rhs.m_Objects;
}

bool PdfArray::operator!=(const PdfArray& rhs) const
{
    if (this == &rhs)
        return false;

    // We don't check owner
    return m_Objects != rhs.m_Objects;
}

PdfArrayIndirectIterator::PdfArrayIndirectIterator(PdfArray& arr)
    : m_arr(&arr) { }

PdfArrayIndirectIterator::iterator PdfArrayIndirectIterator::begin()
{
    return iterator(m_arr->begin());
}

PdfArrayIndirectIterator::iterator PdfArrayIndirectIterator::end()
{
    return iterator(m_arr->end());
}

PdfArrayIndirectIterator::const_iterator PdfArrayIndirectIterator::begin() const
{
    return const_iterator(m_arr->begin());
}

PdfArrayIndirectIterator::const_iterator PdfArrayIndirectIterator::end() const
{
    return const_iterator(m_arr->end());
}
