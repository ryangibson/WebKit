/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <limits.h>
#include <unicode/utypes.h>
#include <wtf/Forward.h>
#include <wtf/Optional.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/ConversionMode.h>
#include <wtf/text/LChar.h>
#include <wtf/text/StringCommon.h>
#include <wtf/text/UTF8ConversionError.h>

// FIXME: Enabling the StringView lifetime checking causes the MSVC build to fail. Figure out why.
#if defined(NDEBUG) || COMPILER(MSVC)
#define CHECK_STRINGVIEW_LIFETIME 0
#else
#define CHECK_STRINGVIEW_LIFETIME 1
#endif

namespace WTF {

// StringView is a non-owning reference to a string, similar to the proposed std::string_view.

class StringView {
public:
    StringView();
#if CHECK_STRINGVIEW_LIFETIME
    ~StringView();
    StringView(StringView&&);
    StringView(const StringView&);
    StringView& operator=(StringView&&);
    StringView& operator=(const StringView&);
#endif

    StringView(const AtomicString&);
    StringView(const String&);
    StringView(const StringImpl&);
    StringView(const StringImpl*);
    StringView(const LChar*, unsigned length);
    StringView(const UChar*, unsigned length);
    StringView(const char*);

    static StringView empty();

    unsigned length() const;
    bool isEmpty() const;

    explicit operator bool() const;
    bool isNull() const;

    UChar operator[](unsigned index) const;

    class CodeUnits;
    CodeUnits codeUnits() const;

    class CodePoints;
    CodePoints codePoints() const;

    class GraphemeClusters;
    GraphemeClusters graphemeClusters() const;

    bool is8Bit() const;
    const LChar* characters8() const;
    const UChar* characters16() const;

    String toString() const;
    String toStringWithoutCopying() const;
    AtomicString toAtomicString() const;
    RefPtr<AtomicStringImpl> toExistingAtomicString() const;

#if USE(CF)
    // This function converts null strings to empty strings.
    WTF_EXPORT_PRIVATE RetainPtr<CFStringRef> createCFString() const;
    WTF_EXPORT_PRIVATE RetainPtr<CFStringRef> createCFStringWithoutCopying() const;
#endif

#ifdef __OBJC__
    // These functions convert null strings to empty strings.
    WTF_EXPORT_PRIVATE RetainPtr<NSString> createNSString() const;
    WTF_EXPORT_PRIVATE RetainPtr<NSString> createNSStringWithoutCopying() const;
#endif

    WTF_EXPORT_PRIVATE Expected<CString, UTF8ConversionError> tryGetUtf8(ConversionMode = LenientConversion) const;
    WTF_EXPORT_PRIVATE CString utf8(ConversionMode = LenientConversion) const;

    class UpconvertedCharacters;
    UpconvertedCharacters upconvertedCharacters() const;

    void getCharactersWithUpconvert(LChar*) const;
    void getCharactersWithUpconvert(UChar*) const;

    StringView substring(unsigned start, unsigned length = std::numeric_limits<unsigned>::max()) const;
    StringView left(unsigned len) const { return substring(0, len); }
    StringView right(unsigned len) const { return substring(length() - len, len); }

    template<typename MatchedCharacterPredicate>
    StringView stripLeadingAndTrailingMatchedCharacters(const MatchedCharacterPredicate&);

    class SplitResult;
    SplitResult split(UChar) const;
    SplitResult splitAllowingEmptyEntries(UChar) const;

    size_t find(UChar, unsigned start = 0) const;
    size_t find(CodeUnitMatchFunction, unsigned start = 0) const;

    WTF_EXPORT_PRIVATE size_t find(StringView, unsigned start) const;

    size_t reverseFind(UChar, unsigned index = UINT_MAX) const;

    WTF_EXPORT_PRIVATE size_t findIgnoringASCIICase(const StringView&) const;
    WTF_EXPORT_PRIVATE size_t findIgnoringASCIICase(const StringView&, unsigned startOffset) const;

    WTF_EXPORT_PRIVATE String convertToASCIILowercase() const;
    WTF_EXPORT_PRIVATE String convertToASCIIUppercase() const;

    bool contains(UChar) const;
    WTF_EXPORT_PRIVATE bool containsIgnoringASCIICase(const StringView&) const;
    WTF_EXPORT_PRIVATE bool containsIgnoringASCIICase(const StringView&, unsigned startOffset) const;

    WTF_EXPORT_PRIVATE bool startsWith(const StringView&) const;
    WTF_EXPORT_PRIVATE bool startsWithIgnoringASCIICase(const StringView&) const;

    WTF_EXPORT_PRIVATE bool endsWith(const StringView&) const;
    WTF_EXPORT_PRIVATE bool endsWithIgnoringASCIICase(const StringView&) const;

    int toInt() const;
    int toInt(bool& isValid) const;
    int toIntStrict(bool& isValid) const;
    std::optional<uint64_t> toUInt64Strict() const;
    float toFloat(bool& isValid) const;

    static void invalidate(const StringImpl&);

    struct UnderlyingString;

private:
    friend bool equal(StringView, StringView);

    void initialize(const LChar*, unsigned length);
    void initialize(const UChar*, unsigned length);

    template<typename CharacterType, typename MatchedCharacterPredicate>
    StringView stripLeadingAndTrailingMatchedCharacters(const CharacterType*, const MatchedCharacterPredicate&);

#if CHECK_STRINGVIEW_LIFETIME
    WTF_EXPORT_PRIVATE bool underlyingStringIsValid() const;
    WTF_EXPORT_PRIVATE void setUnderlyingString(const StringImpl*);
    WTF_EXPORT_PRIVATE void setUnderlyingString(const StringView&);
#else
    bool underlyingStringIsValid() const { return true; }
    void setUnderlyingString(const StringImpl*) { }
    void setUnderlyingString(const StringView&) { }
#endif
    void clear();

    const void* m_characters { nullptr };
    unsigned m_length { 0 };
    bool m_is8Bit { true };

#if CHECK_STRINGVIEW_LIFETIME
    void adoptUnderlyingString(UnderlyingString*);
    UnderlyingString* m_underlyingString { nullptr };
#endif
};

template<typename CharacterType, size_t inlineCapacity> void append(Vector<CharacterType, inlineCapacity>&, StringView);

bool equal(StringView, StringView);
bool equal(StringView, const LChar* b);

bool equalIgnoringASCIICase(StringView, StringView);
bool equalIgnoringASCIICase(StringView, const char*);

template<unsigned length> bool equalLettersIgnoringASCIICase(StringView, const char (&lowercaseLetters)[length]);

inline bool operator==(StringView a, StringView b) { return equal(a, b); }
inline bool operator==(StringView a, const LChar *b);
inline bool operator==(StringView a, const char *b) { return equal(a, reinterpret_cast<const LChar*>(b)); }
inline bool operator==(const char* a, StringView b) { return equal(b, a); }

inline bool operator!=(StringView a, StringView b) { return !equal(a, b); }
inline bool operator!=(StringView a, const LChar* b) { return !equal(a, b); }
inline bool operator!=(StringView a, const char* b) { return !equal(a, b); }
inline bool operator!=(const LChar*a, StringView b) { return !equal(b, a); }
inline bool operator!=(const char*a, StringView b) { return !equal(b, a); }

}

#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

namespace WTF {

inline StringView::StringView()
{
    // FIXME: It's peculiar that null strings are 16-bit and empty strings return 8-bit (according to the is8Bit function).
}

#if CHECK_STRINGVIEW_LIFETIME
inline StringView::~StringView()
{
    setUnderlyingString(nullptr);
}

inline StringView::StringView(StringView&& other)
    : m_characters(other.m_characters)
    , m_length(other.m_length)
    , m_is8Bit(other.m_is8Bit)
{
    ASSERT(other.underlyingStringIsValid());

    other.clear();

    setUnderlyingString(other);
    other.setUnderlyingString(nullptr);
}

inline StringView::StringView(const StringView& other)
    : m_characters(other.m_characters)
    , m_length(other.m_length)
    , m_is8Bit(other.m_is8Bit)
{
    ASSERT(other.underlyingStringIsValid());

    setUnderlyingString(other);
}

inline StringView& StringView::operator=(StringView&& other)
{
    ASSERT(other.underlyingStringIsValid());

    m_characters = other.m_characters;
    m_length = other.m_length;
    m_is8Bit = other.m_is8Bit;

    other.clear();

    setUnderlyingString(other);
    other.setUnderlyingString(nullptr);

    return *this;
}

inline StringView& StringView::operator=(const StringView& other)
{
    ASSERT(other.underlyingStringIsValid());

    m_characters = other.m_characters;
    m_length = other.m_length;
    m_is8Bit = other.m_is8Bit;

    setUnderlyingString(other);

    return *this;
}
#endif // CHECK_STRINGVIEW_LIFETIME

inline void StringView::initialize(const LChar* characters, unsigned length)
{
    m_characters = characters;
    m_length = length;
    m_is8Bit = true;
}

inline void StringView::initialize(const UChar* characters, unsigned length)
{
    m_characters = characters;
    m_length = length;
    m_is8Bit = false;
}

inline StringView::StringView(const LChar* characters, unsigned length)
{
    initialize(characters, length);
}

inline StringView::StringView(const UChar* characters, unsigned length)
{
    initialize(characters, length);
}

inline StringView::StringView(const char* characters)
{
    initialize(reinterpret_cast<const LChar*>(characters), strlen(characters));
}

inline StringView::StringView(const StringImpl& string)
{
    setUnderlyingString(&string);
    if (string.is8Bit())
        initialize(string.characters8(), string.length());
    else
        initialize(string.characters16(), string.length());
}

inline StringView::StringView(const StringImpl* string)
{
    if (!string)
        return;

    setUnderlyingString(string);
    if (string->is8Bit())
        initialize(string->characters8(), string->length());
    else
        initialize(string->characters16(), string->length());
}

inline StringView::StringView(const String& string)
{
    setUnderlyingString(string.impl());
    if (!string.impl()) {
        clear();
        return;
    }
    if (string.is8Bit()) {
        initialize(string.characters8(), string.length());
        return;
    }
    initialize(string.characters16(), string.length());
}

inline StringView::StringView(const AtomicString& atomicString)
    : StringView(atomicString.string())
{
}

inline void StringView::clear()
{
    m_characters = nullptr;
    m_length = 0;
    m_is8Bit = true;
}

inline StringView StringView::empty()
{
    return StringView(reinterpret_cast<const LChar*>(""), 0);
}

inline const LChar* StringView::characters8() const
{
    ASSERT(is8Bit());
    ASSERT(underlyingStringIsValid());
    return static_cast<const LChar*>(m_characters);
}

inline const UChar* StringView::characters16() const
{
    ASSERT(!is8Bit());
    ASSERT(underlyingStringIsValid());
    return static_cast<const UChar*>(m_characters);
}

class StringView::UpconvertedCharacters {
public:
    explicit UpconvertedCharacters(const StringView&);
    operator const UChar*() const { return m_characters; }
    const UChar* get() const { return m_characters; }
private:
    Vector<UChar, 32> m_upconvertedCharacters;
    const UChar* m_characters;
};

inline StringView::UpconvertedCharacters StringView::upconvertedCharacters() const
{
    return UpconvertedCharacters(*this);
}

inline bool StringView::isNull() const
{
    return !m_characters;
}

inline bool StringView::isEmpty() const
{
    return !length();
}

inline unsigned StringView::length() const
{
    return m_length;
}

inline StringView::operator bool() const
{
    return !isNull();
}

inline bool StringView::is8Bit() const
{
    return m_is8Bit;
}

inline StringView StringView::substring(unsigned start, unsigned length) const
{
    if (start >= this->length())
        return empty();
    unsigned maxLength = this->length() - start;

    if (length >= maxLength) {
        if (!start)
            return *this;
        length = maxLength;
    }

    if (is8Bit()) {
        StringView result(characters8() + start, length);
        result.setUnderlyingString(*this);
        return result;
    }
    StringView result(characters16() + start, length);
    result.setUnderlyingString(*this);
    return result;
}

inline UChar StringView::operator[](unsigned index) const
{
    ASSERT(index < length());
    if (is8Bit())
        return characters8()[index];
    return characters16()[index];
}

inline bool StringView::contains(UChar character) const
{
    return find(character) != notFound;
}

inline void StringView::getCharactersWithUpconvert(LChar* destination) const
{
    ASSERT(is8Bit());
    StringImpl::copyCharacters(destination, characters8(), m_length);
}

inline void StringView::getCharactersWithUpconvert(UChar* destination) const
{
    if (is8Bit()) {
        StringImpl::copyCharacters(destination, characters8(), m_length);
        return;
    }
    StringImpl::copyCharacters(destination, characters16(), m_length);
}

inline StringView::UpconvertedCharacters::UpconvertedCharacters(const StringView& string)
{
    if (!string.is8Bit()) {
        m_characters = string.characters16();
        return;
    }
    const LChar* characters8 = string.characters8();
    unsigned length = string.m_length;
    m_upconvertedCharacters.reserveInitialCapacity(length);
    for (unsigned i = 0; i < length; ++i)
        m_upconvertedCharacters.uncheckedAppend(characters8[i]);
    m_characters = m_upconvertedCharacters.data();
}

inline String StringView::toString() const
{
    if (is8Bit())
        return String(characters8(), m_length);
    return String(characters16(), m_length);
}

inline AtomicString StringView::toAtomicString() const
{
    if (is8Bit())
        return AtomicString(characters8(), m_length);
    return AtomicString(characters16(), m_length);
}

inline RefPtr<AtomicStringImpl> StringView::toExistingAtomicString() const
{
    if (is8Bit())
        return AtomicStringImpl::lookUp(characters8(), m_length);
    return AtomicStringImpl::lookUp(characters16(), m_length);
}

inline float StringView::toFloat(bool& isValid) const
{
    if (is8Bit())
        return charactersToFloat(characters8(), m_length, &isValid);
    return charactersToFloat(characters16(), m_length, &isValid);
}

inline int StringView::toInt() const
{
    bool isValid;
    return toInt(isValid);
}

inline int StringView::toInt(bool& isValid) const
{
    if (is8Bit())
        return charactersToInt(characters8(), m_length, &isValid);
    return charactersToInt(characters16(), m_length, &isValid);
}

inline int StringView::toIntStrict(bool& isValid) const
{
    if (is8Bit())
        return charactersToIntStrict(characters8(), m_length, &isValid);
    return charactersToIntStrict(characters16(), m_length, &isValid);
}

inline std::optional<uint64_t> StringView::toUInt64Strict() const
{
    bool isValid;
    uint64_t result = is8Bit() ? charactersToUInt64Strict(characters8(), m_length, &isValid) : charactersToUInt64Strict(characters16(), m_length, &isValid);
    return isValid ? std::make_optional(result) : std::nullopt;
}

inline String StringView::toStringWithoutCopying() const
{
    if (is8Bit())
        return StringImpl::createWithoutCopying(characters8(), m_length);
    return StringImpl::createWithoutCopying(characters16(), m_length);
}

inline size_t StringView::find(UChar character, unsigned start) const
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, character, start);
    return WTF::find(characters16(), m_length, character, start);
}

inline size_t StringView::find(CodeUnitMatchFunction matchFunction, unsigned start) const
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, matchFunction, start);
    return WTF::find(characters16(), m_length, matchFunction, start);
}

inline size_t StringView::reverseFind(UChar character, unsigned index) const
{
    if (is8Bit())
        return WTF::reverseFind(characters8(), m_length, character, index);
    return WTF::reverseFind(characters16(), m_length, character, index);
}

#if !CHECK_STRINGVIEW_LIFETIME
inline void StringView::invalidate(const StringImpl&)
{
}
#endif

template<typename StringType, typename> class StringTypeAdapter;

template<> class StringTypeAdapter<StringView, void> {
public:
    StringTypeAdapter(StringView string)
        : m_string(string)
    {
    }

    unsigned length() { return m_string.length(); }
    bool is8Bit() { return m_string.is8Bit(); }
    void writeTo(LChar* destination) { m_string.getCharactersWithUpconvert(destination); }
    void writeTo(UChar* destination) { m_string.getCharactersWithUpconvert(destination); }

    String toString() const { return m_string.toString(); }

private:
    StringView m_string;
};

template<typename CharacterType, size_t inlineCapacity> void append(Vector<CharacterType, inlineCapacity>& buffer, StringView string)
{
    unsigned oldSize = buffer.size();
    buffer.grow(oldSize + string.length());
    string.getCharactersWithUpconvert(buffer.data() + oldSize);
}

inline bool equal(StringView a, StringView b)
{
    if (a.m_characters == b.m_characters) {
        ASSERT(a.is8Bit() == b.is8Bit());
        return a.length() == b.length();
    }

    return equalCommon(a, b);
}

inline bool equal(StringView a, const LChar* b)
{
    if (!b)
        return !a.isEmpty();
    if (a.isEmpty())
        return !b;

    unsigned aLength = a.length();
    if (aLength != strlen(reinterpret_cast<const char*>(b)))
        return false;

    if (a.is8Bit())
        return equal(a.characters8(), b, aLength);
    return equal(a.characters16(), b, aLength);
}

inline bool equalIgnoringASCIICase(StringView a, StringView b)
{
    return equalIgnoringASCIICaseCommon(a, b);
}

inline bool equalIgnoringASCIICase(StringView a, const char* b)
{
    return equalIgnoringASCIICaseCommon(a, b);
}

class StringView::SplitResult {
public:
    SplitResult(StringView, UChar separator, bool allowEmptyEntries);

    class Iterator;
    Iterator begin() const;
    Iterator end() const;

private:
    StringView m_string;
    UChar m_separator;
    bool m_allowEmptyEntries;
};

class StringView::GraphemeClusters {
public:
    explicit GraphemeClusters(const StringView&);

    class Iterator;
    Iterator begin() const;
    Iterator end() const;

private:
    StringView m_stringView;
};

class StringView::CodePoints {
public:
    explicit CodePoints(const StringView&);

    class Iterator;
    Iterator begin() const;
    Iterator end() const;

private:
    StringView m_stringView;
};

class StringView::CodeUnits {
public:
    explicit CodeUnits(const StringView&);

    class Iterator;
    Iterator begin() const;
    Iterator end() const;

private:
    StringView m_stringView;
};

class StringView::SplitResult::Iterator {
public:
    StringView operator*() const;

    WTF_EXPORT_PRIVATE Iterator& operator++();

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;

private:
    enum PositionTag { AtEnd };
    Iterator(const SplitResult&);
    Iterator(const SplitResult&, PositionTag);

    WTF_EXPORT_PRIVATE void findNextSubstring();

    friend SplitResult;

    const SplitResult& m_result;
    unsigned m_position { 0 };
    unsigned m_length;
    bool m_isDone;
};

class StringView::GraphemeClusters::Iterator {
public:
    Iterator() = delete;
    WTF_EXPORT_PRIVATE Iterator(const StringView&, unsigned index);
    WTF_EXPORT_PRIVATE ~Iterator();

    Iterator(const Iterator&) = delete;
    WTF_EXPORT_PRIVATE Iterator(Iterator&&);
    Iterator& operator=(const Iterator&) = delete;
    Iterator& operator=(Iterator&&) = delete;

    WTF_EXPORT_PRIVATE StringView operator*() const;
    WTF_EXPORT_PRIVATE Iterator& operator++();

    WTF_EXPORT_PRIVATE bool operator==(const Iterator&) const;
    WTF_EXPORT_PRIVATE bool operator!=(const Iterator&) const;

private:
    class Impl;

    std::unique_ptr<Impl> m_impl;
};

class StringView::CodePoints::Iterator {
public:
    Iterator(const StringView&, unsigned index);

    UChar32 operator*() const;
    Iterator& operator++();

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
    Iterator& operator=(const Iterator&);

private:
    std::reference_wrapper<const StringView> m_stringView;
    std::optional<unsigned> m_nextCodePointOffset;
    UChar32 m_codePoint;
};

class StringView::CodeUnits::Iterator {
public:
    Iterator(const StringView&, unsigned index);

    UChar operator*() const;
    Iterator& operator++();

    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;

private:
    const StringView& m_stringView;
    unsigned m_index;
};

inline auto StringView::graphemeClusters() const -> GraphemeClusters
{
    return GraphemeClusters(*this);
}

inline auto StringView::codePoints() const -> CodePoints
{
    return CodePoints(*this);
}

inline auto StringView::codeUnits() const -> CodeUnits
{
    return CodeUnits(*this);
}

inline StringView::GraphemeClusters::GraphemeClusters(const StringView& stringView)
    : m_stringView(stringView)
{
}

inline auto StringView::GraphemeClusters::begin() const -> Iterator
{
    return Iterator(m_stringView, 0);
}

inline auto StringView::GraphemeClusters::end() const -> Iterator
{
    return Iterator(m_stringView, m_stringView.length());
}

inline StringView::CodePoints::CodePoints(const StringView& stringView)
    : m_stringView(stringView)
{
}

inline StringView::CodePoints::Iterator::Iterator(const StringView& stringView, unsigned index)
    : m_stringView(stringView)
    , m_nextCodePointOffset(index)
{
    operator++();
}

inline auto StringView::CodePoints::Iterator::operator++() -> Iterator&
{
    ASSERT(m_nextCodePointOffset);
    if (m_nextCodePointOffset.value() == m_stringView.get().length()) {
        m_nextCodePointOffset = std::nullopt;
        return *this;
    }
    if (m_stringView.get().is8Bit())
        m_codePoint = m_stringView.get().characters8()[m_nextCodePointOffset.value()++];
    else
        U16_NEXT(m_stringView.get().characters16(), m_nextCodePointOffset.value(), m_stringView.get().length(), m_codePoint);
    ASSERT(m_nextCodePointOffset.value() <= m_stringView.get().length());
    return *this;
}

inline auto StringView::CodePoints::Iterator::operator=(const Iterator& other) -> Iterator&
{
    m_stringView = other.m_stringView;
    m_nextCodePointOffset = other.m_nextCodePointOffset;
    m_codePoint = other.m_codePoint;
    return *this;
}

inline UChar32 StringView::CodePoints::Iterator::operator*() const
{
    ASSERT(m_nextCodePointOffset);
    return m_codePoint;
}

inline bool StringView::CodePoints::Iterator::operator==(const Iterator& other) const
{
    ASSERT(&m_stringView.get() == &other.m_stringView.get());
    return m_nextCodePointOffset == other.m_nextCodePointOffset;
}

inline bool StringView::CodePoints::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

inline auto StringView::CodePoints::begin() const -> Iterator
{
    return Iterator(m_stringView, 0);
}

inline auto StringView::CodePoints::end() const -> Iterator
{
    return Iterator(m_stringView, m_stringView.length());
}

inline StringView::CodeUnits::CodeUnits(const StringView& stringView)
    : m_stringView(stringView)
{
}

inline StringView::CodeUnits::Iterator::Iterator(const StringView& stringView, unsigned index)
    : m_stringView(stringView)
    , m_index(index)
{
}

inline auto StringView::CodeUnits::Iterator::operator++() -> Iterator&
{
    ++m_index;
    return *this;
}

inline UChar StringView::CodeUnits::Iterator::operator*() const
{
    return m_stringView[m_index];
}

inline bool StringView::CodeUnits::Iterator::operator==(const Iterator& other) const
{
    ASSERT(&m_stringView == &other.m_stringView);
    return m_index == other.m_index;
}

inline bool StringView::CodeUnits::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

inline auto StringView::CodeUnits::begin() const -> Iterator
{
    return Iterator(m_stringView, 0);
}

inline auto StringView::CodeUnits::end() const -> Iterator
{
    return Iterator(m_stringView, m_stringView.length());
}

inline auto StringView::split(UChar separator) const -> SplitResult
{
    return SplitResult { *this, separator, false };
}

inline auto StringView::splitAllowingEmptyEntries(UChar separator) const -> SplitResult
{
    return SplitResult { *this, separator, true };
}

inline StringView::SplitResult::SplitResult(StringView stringView, UChar separator, bool allowEmptyEntries)
    : m_string { stringView }
    , m_separator { separator }
    , m_allowEmptyEntries { allowEmptyEntries }
{
}

inline auto StringView::SplitResult::begin() const -> Iterator
{
    return Iterator { *this };
}

inline auto StringView::SplitResult::end() const -> Iterator
{
    return Iterator { *this, Iterator::AtEnd };
}

inline StringView::SplitResult::Iterator::Iterator(const SplitResult& result)
    : m_result { result }
    , m_isDone { result.m_string.isEmpty() && !result.m_allowEmptyEntries }
{
    findNextSubstring();
}

inline StringView::SplitResult::Iterator::Iterator(const SplitResult& result, PositionTag)
    : m_result { result }
    , m_position { result.m_string.length() }
    , m_isDone { true }
{
}

inline StringView StringView::SplitResult::Iterator::operator*() const
{
    ASSERT(m_position <= m_result.m_string.length() && !m_isDone);
    return m_result.m_string.substring(m_position, m_length);
}

inline bool StringView::SplitResult::Iterator::operator==(const Iterator& other) const
{
    ASSERT(&m_result == &other.m_result);
    return m_position == other.m_position && m_isDone == other.m_isDone;
}

inline bool StringView::SplitResult::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

template<typename CharacterType, typename MatchedCharacterPredicate>
inline StringView StringView::stripLeadingAndTrailingMatchedCharacters(const CharacterType* characters, const MatchedCharacterPredicate& predicate)
{
    if (!m_length)
        return *this;

    unsigned start = 0;
    unsigned end = m_length - 1;

    while (start <= end && predicate(characters[start]))
        ++start;

    if (start > end)
        return StringView::empty();

    while (end && predicate(characters[end]))
        --end;

    if (!start && end == m_length - 1)
        return *this;

    StringView result(characters + start, end + 1 - start);
    result.setUnderlyingString(*this);
    return result;
}

template<typename MatchedCharacterPredicate>
StringView StringView::stripLeadingAndTrailingMatchedCharacters(const MatchedCharacterPredicate& predicate)
{
    if (is8Bit())
        return stripLeadingAndTrailingMatchedCharacters<LChar>(characters8(), predicate);
    return stripLeadingAndTrailingMatchedCharacters<UChar>(characters16(), predicate);
}

template<unsigned length> inline bool equalLettersIgnoringASCIICase(StringView string, const char (&lowercaseLetters)[length])
{
    return equalLettersIgnoringASCIICaseCommon(string, lowercaseLetters);
}

} // namespace WTF

using WTF::append;
using WTF::equal;
using WTF::StringView;
