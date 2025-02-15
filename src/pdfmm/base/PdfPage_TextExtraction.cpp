/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include <regex>
#include <deque>
#include <stack>

#include "PdfDocument.h"
#include "PdfTextState.h"
#include "PdfMath.h"
#include "PdfXObjectForm.h"
#include "PdfContentsReader.h"
#include "PdfFont.h"

#include <pdfmm/private/outstringstream.h>

using namespace std;
using namespace cmn;
using namespace mm;

constexpr double SAME_LINE_THRESHOLD = 0.01;
constexpr double SPACE_SEPARATION_EPSILON = 0.0000001;
#define ASSERT(condition, message, ...) if (!condition)\
    mm::LogMessage(PdfLogSeverity::Warning, message, ##__VA_ARGS__);

static constexpr float NaN = numeric_limits<float>::quiet_NaN();

// 5.2 Text State Parameters and Operators
// 5.3 Text Objects
struct TextState
{
    Matrix T_rm;  // Current T_rm
    Matrix CTM;   // Current CTM
    Matrix T_m;   // Current T_m
    Matrix T_lm;  // Current T_lm
    double T_l = 0;             // Leading text Tl
    PdfTextState PdfState;
    double SpaceSize = 0;
    void RecomputeT_rm();

    double GetCharWidth(char32_t ch) const;
    double GetStringWidth(const string_view& str) const;        // utf8 string
    double GetStringWidth(const PdfString& encodedStr) const;   // pdf encoded
};

class StatefulString
{
public:
    StatefulString(const string_view &str, double length, const TextState &state);
public:
    StatefulString GetTrimmedBegin() const;
    StatefulString GetTrimmedEnd() const;
public:
    const string String;
    const TextState State;
    Vector2 Pos;
    const double Length;
    const bool IsWhiteSpace;
};

struct EntryOptions
{
    bool IgnoreCase;
    bool TrimSpaces;
    bool TokenizeWords;
};

using StringChunk = list<StatefulString>;
using StringChunkPtr = unique_ptr<StringChunk>;
using StringChunkList = list<StringChunkPtr>;
using StringChunkListPtr = unique_ptr<StringChunkList>;

class TextStateStack
{
private:
    stack<TextState> m_states;
    TextState *m_current;
public:
    TextState * const & Current;
public:
    TextStateStack();
    void Push();
    void Pop(unsigned popCount = 1);
    unsigned GetSize() const;
private:
    void push(const TextState &state);
};

struct XObjectState
{
    const PdfXObjectForm* Form;
    unsigned TextStateIndex;
};

struct ExtractionContext
{
public:
    ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
        const EntryOptions &options, const nullable<PdfRect> &clipRect);
public:
    void BeginText();
    void EndText();
    void Tf_Operator(const PdfName &fontname, double fontsize);
    void cm_Operator(double a, double b, double c, double d, double e, double f);
    void Tm_Operator(double a, double b, double c, double d, double e, double f);
    void TdTD_Operator(double tx, double ty);
    void AdvanceSpace(double ty);
    void TStar_Operator();
public:
    void PushString(const StatefulString &str, bool pushchunk = false);
    void TryPushChunk();
    void TryAddLastEntry();
private:
    bool areChunksSplitted();
    void pushChunk();
    void addEntry();
    void tryAddEntry();
    const PdfCanvas& getActualCanvas();
private:
    const PdfPage& m_page;
public:
    const int PageIndex;
    const string Pattern;
    const EntryOptions Options;
    const nullable<PdfRect> ClipRect;
    unique_ptr<Matrix> Rotation;
    vector<PdfTextEntry> &Entries;
    StringChunkPtr Chunk = std::make_unique<StringChunk>();
    StringChunkList Chunks;
    TextStateStack States;
    vector<XObjectState> XObjectStateIndices;
    double CurrentEntryT_rm_y = NaN;    // Tracks line changing
    double PrevChunkT_rm_x = 0;         // Tracks space separation
    bool BlockOpen = false;
};

static EntryOptions FromFlags(PdfTextExtractFlags flags);
static bool DecodeString(const PdfString &str, TextState &state, string &decoded, double &length);
static bool AreEqual(double lhs, double rhs);
static bool IsWhiteSpaceChunk(const StringChunk &chunk);
static void SplitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk);
static void SplitStringBySpaces(vector<StatefulString> &separatedStrings, const StatefulString &string);
static void TrimSpacesBegin(StringChunk &chunk);
static void TrimSpacesEnd(StringChunk &chunk);
static double GetStringWidth(const string_view &str, const TextState &state);
static void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, const EntryOptions &options, const nullable<PdfRect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &strings,
    const string_view &pattern, bool ignoreCase, bool trimSpaces, const nullable<PdfRect> &clipRect,
    int pageIndex, const Matrix* rotation);
static void read(const PdfVariantStack& stack, double &tx, double &ty);
static void read(const PdfVariantStack& stack, double &a, double &b, double &c, double &d, double &e, double &f);

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const PdfTextExtractParams& params) const
{
    ExtractTextTo(entries, { }, params);
}

void PdfPage::ExtractTextTo(vector<PdfTextEntry>& entries, const string_view& pattern,
    const PdfTextExtractParams& params) const
{
    ExtractionContext context(entries, *this, pattern, FromFlags(params.Flags), params.ClipRect);

    // Look FIGURE 4.1 Graphics objects
    PdfContentsReader reader(*this);
    PdfContent content;
    while (reader.TryReadNext(content))
    {
        switch (content.Type)
        {
            case PdfContentType::Operator:
            {
                if ((content.Warnings & PdfContentWarnings::InvalidOperator)
                    != PdfContentWarnings::None)
                {
                    // Ignore invalid operators
                    continue;
                }

                // T_l TL: Set the text leading, T_l
                switch (content.Operator)
                {
                    case PdfOperator::TL:
                    {
                        context.States.Current->T_l = content.Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::cm:
                    {
                        double a, b, c, d, e, f;
                        read(content.Stack, a, b, c, d, e, f);
                        context.cm_Operator(a, b, c, d, e, f);
                        break;
                    }
                    // t_x t_y Td     : Move to the start of the next line
                    // t_x t_y TD     : Move to the start of the next line
                    // a b c d e f Tm : Set the text matrix, T_m , and the text line matrix, T_lm
                    case PdfOperator::Td:
                    case PdfOperator::TD:
                    case PdfOperator::Tm:
                    {
                        if (content.Operator == PdfOperator::Td || content.Operator == PdfOperator::TD)
                        {
                            double tx, ty;
                            read(content.Stack, tx, ty);
                            context.TdTD_Operator(tx, ty);

                            if (content.Operator == PdfOperator::TD)
                                context.States.Current->T_l = -ty;
                        }
                        else if (content.Operator == PdfOperator::Tm)
                        {
                            double a, b, c, d, e, f;
                            read(content.Stack, a, b, c, d, e, f);
                            context.Tm_Operator(a, b, c, d, e, f);
                        }
                        else
                        {
                            throw runtime_error("Invalid flow");
                        }

                        break;
                    }
                    // T*: Move to the start of the next line
                    case PdfOperator::T_Star:
                    {
                        // NOTE: Errata for the PDF Reference, sixth edition, version 1.7
                        // Section 5.3, Text Objects:
                        // This operator has the same effect as the code
                        //    0 -Tl Td
                        context.TStar_Operator();
                        break;
                    }
                    // BT: Begin a text object
                    case PdfOperator::BT:
                    {
                        context.BeginText();
                        break;
                    }
                    // ET: End a text object
                    case PdfOperator::ET:
                    {
                        context.EndText();
                        break;
                    }
                    // font size Tf : Set the text font, T_f
                    case PdfOperator::Tf:
                    {
                        double fontSize = content.Stack[0].GetReal();
                        auto& fontName = content.Stack[1].GetName();
                        context.Tf_Operator(fontName, fontSize);
                        break;
                    }
                    // string Tj : Show a text string
                    // string '  : Move to the next line and show a text string
                    // a_w a_c " : Move to the next line and show a text string,
                    //             using a_w as the word spacing and a_c as the
                    //             character spacing
                    case PdfOperator::Tj:
                    case PdfOperator::Quote:
                    case PdfOperator::DoubleQuote:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto str = content.Stack[0].GetString();
                        if (content.Operator == PdfOperator::DoubleQuote)
                        {
                            // Operator " arguments: aw ac string "
                            context.States.Current->PdfState.CharSpacing = content.Stack[1].GetReal();
                            context.States.Current->PdfState.WordSpacing = content.Stack[2].GetReal();
                        }

                        string decoded;
                        double length;
                        if (DecodeString(str, *context.States.Current, decoded, length)
                            && decoded.length() != 0)
                        {
                            context.PushString({ decoded, length, *context.States.Current }, true);
                        }

                        if (content.Operator == PdfOperator::Quote
                            || content.Operator == PdfOperator::DoubleQuote)
                        {
                            context.TStar_Operator();
                        }

                        break;
                    }
                    // array TJ : Show one or more text strings
                    case PdfOperator::TJ:
                    {
                        ASSERT(context.BlockOpen, "No text block open");

                        auto& array = content.Stack[0].GetArray();
                        for (unsigned i = 0; i < array.GetSize(); i++)
                        {
                            auto& obj = array[i];
                            if (obj.IsString())
                            {
                                string decoded;
                                double length;
                                if (DecodeString(obj.GetString(), *context.States.Current, decoded, length)
                                    && decoded.length() != 0)
                                {
                                    context.PushString({ decoded, length, *context.States.Current });
                                }
                            }
                            else if (obj.IsNumberOrReal())
                            {
                                // pg. 408, Pdf Reference 1.7: "The number is expressed in thousandths of a unit
                                // of text space. [...] This amount is subtracted from from the current horizontal or
                                // vertical coordinate, depending on the writing mode"
                                // It must be scaled by the font size
                                double space = (-obj.GetReal() / 1000) * context.States.Current->PdfState.FontSize;
                                context.AdvanceSpace(space);
                            }
                            else
                            {
                                mm::LogMessage(PdfLogSeverity::Warning, "Invalid array object type {}", obj.GetDataTypeString());
                            }
                        }

                        context.TryPushChunk();
                        break;
                    }
                    // Tc : word spacing
                    case PdfOperator::Tc:
                    {
                        context.States.Current->PdfState.CharSpacing = content.Stack[0].GetReal();
                        break;
                    }
                    case PdfOperator::Tw:
                    {
                        context.States.Current->PdfState.WordSpacing = content.Stack[0].GetReal();
                        break;
                    }
                    // q : Save the current graphics state
                    case PdfOperator::q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        context.States.Push();
                        break;
                    }
                    // Q : Restore the graphics state by removing
                    // the most recently saved state from the stack
                    case PdfOperator::Q:
                    {
                        ASSERT(!context.BlockOpen, "Text block must be not open");
                        context.States.Pop();
                        break;
                    }
                    default:
                    {
                        // Ignore all the other operators
                        break;
                    }
                }

                break;
            }
            case PdfContentType::ImageDictionary:
            case PdfContentType::ImageData:
            {
                // Ignore image data token
                break;
            }
            case PdfContentType::DoXObject:
            {
                if (content.XObject->GetType() == PdfXObjectType::Form)
                {
                    context.XObjectStateIndices.push_back({
                        (const PdfXObjectForm*)content.XObject.get(),
                        context.States.GetSize()
                    });
                    context.States.Push();
                }

                break;
            }
            case PdfContentType::EndXObjectForm:
            {
                PDFMM_ASSERT(context.XObjectStateIndices.size() != 0);
                context.States.Pop(context.States.GetSize() - context.XObjectStateIndices.back().TextStateIndex);
                context.XObjectStateIndices.pop_back();
                break;
            }
            default:
            {
                throw runtime_error("Unsupported PdfContentType");
            }
        }
    }

    // After finishing processing tokens, one entry may still
    // be inside the chunks
    context.TryAddLastEntry();
}

void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    const EntryOptions &options, const nullable<PdfRect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (options.TokenizeWords)
    {
        // Split lines into chunks separated by at char space
        // NOTE: It doesn't trim empty strings, leading and trailing,
        // white characters yet!
        vector<StringChunkListPtr> batches;
        vector<StringChunkPtr> previousWhiteChunks;
        vector<StringChunkPtr> sepratedChunks;
        auto currentBatch = std::make_unique<StringChunkList>();
        while (true)
        {
            if (chunks.size() == 0)
            {
                // Chunks analysis finished. Try to push last batch
                if (currentBatch->size() != 0)
                    batches.push_back(std::move(currentBatch));

                break;
            }

            auto chunk = std::move(chunks.front());
            chunks.pop_front();

            SplitChunkBySpaces(sepratedChunks, *chunk);
            for (auto &sepratedChunk : sepratedChunks)
            {
                if (IsWhiteSpaceChunk(*sepratedChunk))
                {
                    // A white space chunk is separating words. Try to push a batch
                    if (currentBatch->size() != 0)
                    {
                        batches.push_back(std::move(currentBatch));
                        currentBatch = std::make_unique<StringChunkList>();
                    }

                    previousWhiteChunks.push_back(std::move(sepratedChunk));
                }
                else
                {
                    // Reinsert previous white space chunks, they won't be trimmed yet
                    for (auto &whiteChunk : previousWhiteChunks)
                        currentBatch->push_back(std::move(whiteChunk));

                    previousWhiteChunks.clear();
                    currentBatch->push_back(std::move(sepratedChunk));
                }
            }
        }

        for (auto& batch : batches)
        {
            addEntry(textEntries, *batch, pattern, options.IgnoreCase,
                options.TrimSpaces, clipRect, pageIndex, rotation);
        }
    }
    else
    {
        addEntry(textEntries, chunks, pattern, options.IgnoreCase,
            options.TrimSpaces, clipRect, pageIndex, rotation);
    }
}

void addEntry(vector<PdfTextEntry> &textEntries, StringChunkList &chunks, const string_view &pattern,
    bool ignoreCase, bool trimSpaces, const nullable<PdfRect> &clipRect, int pageIndex, const Matrix* rotation)
{
    if (trimSpaces)
    {
        // Trim spaces at the begin of the string
        while (true)
        {
            if (chunks.size() == 0)
                return;

            auto &front = chunks.front();
            if (IsWhiteSpaceChunk(*front))
            {
                chunks.pop_front();
                continue;
            }

            TrimSpacesBegin(*front);
            break;
        }

        // Trim spaces at the end of the string
        while (true)
        {
            auto &back = chunks.back();
            if (IsWhiteSpaceChunk(*back))
            {
                chunks.pop_back();
                continue;
            }

            TrimSpacesEnd(*back);
            break;
        }
    }

    PDFMM_ASSERT(chunks.size() != 0);
    auto &firstChunk = *chunks.front();
    PDFMM_ASSERT(firstChunk.size() != 0);
    auto &firstStr = firstChunk.front();
    if (clipRect.has_value() && !clipRect->Contains(firstStr.Pos.X, firstStr.Pos.Y))
    {
        chunks.clear();
        return;
    }

    outstringstream stream;
    for (auto &chunk : chunks)
    {
        for (auto &str : *chunk)
            stream << str.String;
    }

    string str = stream.take_str();
    if (pattern.length() != 0)
    {
        auto flags = regex_constants::ECMAScript;
        if (ignoreCase)
            flags |= regex_constants::icase;

        // regex_search returns true when a sub-part of the string
        // matches the regex
        regex pieces_regex((string)pattern, flags);
        if (!std::regex_search(str, pieces_regex))
        {
            chunks.clear();
            return;
        }
    }

    // Rotate to canonical frame
    if (rotation == nullptr)
    {
        textEntries.push_back({ str, pageIndex, firstStr.Pos.X, firstStr.Pos.Y, firstStr.Pos.X, firstStr.Pos.Y });
    }
    else
    {
        Vector2 rawp(firstStr.Pos.X, firstStr.Pos.Y);
        auto p_1 = rawp * (*rotation);
        textEntries.push_back({ str, pageIndex, p_1.X, p_1.Y, firstStr.Pos.X, firstStr.Pos.Y });
    }

    chunks.clear();
}

void read(const PdfVariantStack& tokens, double & tx, double & ty)
{
    ty = tokens[0].GetReal();
    tx = tokens[1].GetReal();
}

void read(const PdfVariantStack& tokens, double & a, double & b, double & c, double & d, double & e, double & f)
{
    f = tokens[0].GetReal();
    e = tokens[1].GetReal();
    d = tokens[2].GetReal();
    c = tokens[3].GetReal();
    b = tokens[4].GetReal();
    a = tokens[5].GetReal();
}

bool DecodeString(const PdfString &str, TextState &state, string &decoded, double &length)
{
    if (state.PdfState.Font == nullptr)
    {
        length = 0;
        if (!str.IsHex())
        {
            // As a fallback try to retrieve the raw string
            decoded = str.GetString();
            return true;
        }

        return false;
    }

    decoded = state.PdfState.Font->GetEncoding().ConvertToUtf8(str);
    length = state.GetStringWidth(str);
    return true;
}

StatefulString::StatefulString(const string_view &str, double length, const TextState &state)
    : String((string)str), State(state), Pos(state.T_rm.GetTranslation()),
      Length(length), IsWhiteSpace(utls::IsStringEmptyOrWhiteSpace(str)) { }

StatefulString StatefulString::GetTrimmedBegin() const
{
    int i = 0;
    auto &str = String;
    for (; i < (int)str.length(); i++)
    {
        if (!std::isspace((unsigned char)str[i]))
            break;
    }

    // First, advance the x position by finding the space string size with the current font
    auto state = State;
    double spacestrWidth = 0;
    if (i != 0)
    {
        auto spacestr = str.substr(0, i);
        spacestrWidth = GetStringWidth(spacestr, state);
        state.T_m.Apply<Tx>(spacestrWidth);
        state.RecomputeT_rm();
    }

    // After, rewrite the string without spaces
    string trimmedStr = str.substr(i);
    return StatefulString(str.substr(i), Length - spacestrWidth, state);
}

StatefulString StatefulString::GetTrimmedEnd() const
{
    string trimmedStr = utls::TrimSpacesEnd(String);
    return StatefulString(trimmedStr, GetStringWidth(trimmedStr, State), State);
}

TextStateStack::TextStateStack()
    : m_current(nullptr), Current(m_current)
{
    push({ });
}

void TextStateStack::Push()
{
    push(m_states.top());
}

void TextStateStack::Pop(unsigned popCount)
{
    if (popCount >= m_states.size())
        throw runtime_error("Can't pop out all the states in the stack");

    for (unsigned i = 0; i < popCount; i++)
        m_states.pop();

    m_current = &m_states.top();
}

unsigned TextStateStack::GetSize() const
{
    return (unsigned)m_states.size();
}

void TextStateStack::push(const TextState & state)
{
    m_states.push(state);
    m_current = &m_states.top();
}

ExtractionContext::ExtractionContext(vector<PdfTextEntry> &entries, const PdfPage &page, const string_view &pattern,
    const EntryOptions & options, const nullable<PdfRect>& clipRect) :
    m_page(page),
    PageIndex(page.GetPageNumber() - 1),
    Pattern(pattern),
    Options(options),
    ClipRect(clipRect),
    Entries(entries)
{
    // Determine page rotation transformation
    double teta;
    if (page.HasRotation(teta))
        Rotation = std::make_unique<Matrix>(mm::GetFrameRotationTransform(page.GetRect(), teta));
}

void ExtractionContext::BeginText()
{
    ASSERT(!BlockOpen, "Text block already open");

    // NOTE: BT doesn't reset font
    BlockOpen = true;
}

void ExtractionContext::EndText()
{
    ASSERT(BlockOpen, "No text block open");
    States.Current->T_m = Matrix();
    States.Current->T_lm = Matrix();
    States.Current->RecomputeT_rm();
    BlockOpen = false;
}

void ExtractionContext::Tf_Operator(const PdfName &fontname, double fontsize)
{
    auto fontObj = getActualCanvas().GetFromResources("Font", fontname);
    auto &doc = m_page.GetDocument();
    double spacesize = 0;
    States.Current->PdfState.FontSize = fontsize;
    if (fontObj == nullptr || (States.Current->PdfState.Font = doc.GetFontManager().GetLoadedFont(*fontObj)) == nullptr)
    {
        mm::LogMessage(PdfLogSeverity::Warning, "Unable to find font object {}", fontname.GetString());
    }
    else
    {
        spacesize = States.Current->GetCharWidth(U' ');
    }

    States.Current->SpaceSize = spacesize;
    if (spacesize == 0)
    {
        mm::LogMessage(PdfLogSeverity::Warning, "Unable to provide a space size, setting default font size");
        States.Current->SpaceSize = fontsize;
    }
}

void ExtractionContext::cm_Operator(double a, double b, double c, double d, double e, double f)
{
    // TABLE 4.7: "cm" Modify the current transformation
    // matrix (CTM) by concatenating the specified matrix
    Matrix cm = Matrix::FromCoefficients(a, b, c, d, e, f);
    States.Current->CTM = cm * States.Current->CTM;
    States.Current->RecomputeT_rm();
}

void ExtractionContext::Tm_Operator(double a, double b, double c, double d, double e, double f)
{
    States.Current->T_lm = Matrix::FromCoefficients(a, b, c, d, e, f);
    States.Current->T_m = States.Current->T_lm;
    States.Current->RecomputeT_rm();
}

void ExtractionContext::TdTD_Operator(double tx, double ty)
{
    // 5.5 Text-positioning operators, Td/TD
    Matrix tdm = Matrix::CreateTranslation({ tx, ty });
    States.Current->T_lm = tdm * States.Current->T_lm;
    States.Current->T_m = States.Current->T_lm;
    States.Current->RecomputeT_rm();
}

void ExtractionContext::TStar_Operator()
{
    States.Current->T_lm.Apply<Ty>(-States.Current->T_l);
    States.Current->T_m = States.Current->T_lm;
    States.Current->RecomputeT_rm();
}

void ExtractionContext::AdvanceSpace(double tx)
{
    States.Current->T_m.Apply<Tx>(tx);
    States.Current->RecomputeT_rm();

    // FIXME: This is spurious, the only tryAddEntry in PushString should be enough
    // but that can't be track properly advances in x because of unreliable gliph
    // length determination. Fix that first, and following can be removed
    if (States.Current->SpaceSize > 0 && (Chunks.size() > 0 || Chunk->size() > 0))
    {
        if (tx > States.Current->SpaceSize)
        {
            TryPushChunk();
            addEntry();
        }
    }

}

void ExtractionContext::PushString(const StatefulString &str, bool pushchunk)
{
    PDFMM_ASSERT(str.String.length() != 0);
    if (std::isnan(CurrentEntryT_rm_y))
    {
        // Initalize tracking for line
        CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    }

    tryAddEntry();

    // Set current line tracking
    CurrentEntryT_rm_y = States.Current->T_rm.Get<Ty>();
    Chunk->push_back(str);
    if (pushchunk)
        pushChunk();

    States.Current->T_m.Apply<Tx>(str.Length);
    States.Current->RecomputeT_rm();
    PrevChunkT_rm_x = States.Current->T_rm.Get<Tx>();
}

void ExtractionContext::TryPushChunk()
{
    if (Chunk->size() != 0)
        pushChunk();
}

void ExtractionContext::pushChunk()
{
    Chunks.push_back(std::move(Chunk));
    Chunk = std::make_unique<StringChunk>();
}

void ExtractionContext::TryAddLastEntry()
{
    if (Chunks.size() > 0)
        addEntry();
}

const PdfCanvas& ExtractionContext::getActualCanvas()
{
    if (XObjectStateIndices.size() == 0)
        return m_page;

    return *XObjectStateIndices.back().Form;
}

void ExtractionContext::addEntry()
{
    ::addEntry(Entries, Chunks, Pattern, Options, ClipRect, PageIndex, Rotation.get());
}

void ExtractionContext::tryAddEntry()
{
    PDFMM_ASSERT(Chunk != nullptr);
    if (Chunks.size() > 0 || Chunk->size() > 0)
    {
        if (!AreEqual(States.Current->T_rm.Get<Ty>(), CurrentEntryT_rm_y)
            || (Options.TokenizeWords && areChunksSplitted()))
        {
            // Current entry is not on same line or it's separated by space
            TryPushChunk();
            addEntry();
        }
    }
}

bool ExtractionContext::areChunksSplitted()
{
    // NOTE: This is a simple euristic that assumes that all string
    // chunks must be separated by 10 spaces to split them
    // This is because we are very bad at computing string widths.
    // TODO: to improve the situation:
    // 1) Apply the T_rm transformation to the string widths;
    // 2) Handle the word spacing Tw state.
    return std::abs((States.Current->T_rm.Get<Tx>()) - PrevChunkT_rm_x) + SPACE_SEPARATION_EPSILON
        >= States.Current->SpaceSize;
}

// Separate chunk words by spaces
void SplitChunkBySpaces(vector<StringChunkPtr> &splittedChunks, const StringChunk &chunk)
{
    PDFMM_ASSERT(chunk.size() != 0);
    splittedChunks.clear();

    vector<StatefulString> separatedStrings;
    for (auto &str : chunk)
    {
        auto separatedChunk = std::make_unique<StringChunk>();
        bool previousWhiteSpace = true;
        SplitStringBySpaces(separatedStrings, str);
        for (auto &separatedStr : separatedStrings)
        {
            if (separatedChunk->size() != 0 && separatedStr.IsWhiteSpace != previousWhiteSpace)
            {
                splittedChunks.push_back(std::move(separatedChunk));
                separatedChunk = std::make_unique<StringChunk>();
            }

            separatedChunk->push_back(separatedStr);
            previousWhiteSpace = separatedStr.IsWhiteSpace;
        }

        // Push back last chunk, if present
        if (separatedChunk->size() != 0)
            splittedChunks.push_back(std::move(separatedChunk));
    }
}

// Separate string words by spaces
void SplitStringBySpaces(vector<StatefulString> &separatedStrings, const StatefulString &str)
{
    PDFMM_ASSERT(str.String.length() != 0);
    separatedStrings.clear();

    string separatedStr;
    auto state = str.State;

    auto pushString = [&]() {
        double length = GetStringWidth(separatedStr, state);
        separatedStrings.push_back({ separatedStr , length, state });
        separatedStr.clear();
        state.T_m.Apply<Tx>(length);
        state.RecomputeT_rm();
    };

    bool isPreviousWhiteSpace = true;
    for (int i = 0; i < (int)str.String.size(); i++)
    {
        char ch = str.String[i];
        bool isCurrentWhiteSpace = std::isspace((unsigned char)ch) != 0;
        if (separatedStr.length() != 0 && isCurrentWhiteSpace != isPreviousWhiteSpace)
            pushString();

        separatedStr.push_back(ch);
        isPreviousWhiteSpace = isCurrentWhiteSpace;
    }

    // Push back last string, if present
    if (separatedStr.length() != 0)
        pushString();
}

void TrimSpacesBegin(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &front = chunk.front();
        if (!front.IsWhiteSpace)
        {
            auto trimmed = front.GetTrimmedBegin();

            chunk.pop_front();
            chunk.push_front(trimmed);
            break;
        }

        chunk.pop_front();
    }
}

void TrimSpacesEnd(StringChunk &chunk)
{
    while (true)
    {
        if (chunk.size() == 0)
            break;

        auto &back = chunk.back();
        if (!back.IsWhiteSpace)
        {
            auto trimmed = back.GetTrimmedEnd();
            chunk.pop_back();
            chunk.push_back(trimmed);
            break;
        }

        chunk.pop_back();
    }
}

double GetStringWidth(const string_view &str, const TextState &state)
{
    if (state.PdfState.Font == nullptr)
        return 0;

    return state.GetStringWidth(str);
}

bool IsWhiteSpaceChunk(const StringChunk &chunk)
{
    for (auto &str : chunk)
    {
        if (!str.IsWhiteSpace)
            return false;
    }

    return true;
}

bool AreEqual(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < SAME_LINE_THRESHOLD;
}

void TextState::RecomputeT_rm()
{
    T_rm = T_m * CTM;
}

double TextState::GetCharWidth(char32_t ch) const
{
    return PdfState.Font->GetCharWidth(ch, PdfState);
}

double TextState::GetStringWidth(const string_view& str) const
{
    return PdfState.Font->GetStringWidth(str, PdfState);
}

double TextState::GetStringWidth(const PdfString& encodedStr) const
{
    return PdfState.Font->GetStringWidth(encodedStr, PdfState);
}

EntryOptions FromFlags(PdfTextExtractFlags flags)
{
    EntryOptions ret;
    ret.IgnoreCase = (flags & PdfTextExtractFlags::IgnoreCase) != PdfTextExtractFlags::None;
    ret.TokenizeWords = (flags & PdfTextExtractFlags::TokenizeWords) != PdfTextExtractFlags::None;
    ret.TrimSpaces = (flags & PdfTextExtractFlags::KeepWhiteTokens) == PdfTextExtractFlags::None || ret.TokenizeWords;
    return ret;
}
