/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#include "ArgumentCoders.h"
#include <WebCore/AutocapitalizeTypes.h>
#include <WebCore/Autofill.h>
#include <WebCore/Color.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/InputMode.h>
#include <WebCore/IntRect.h>
#include <WebCore/URL.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

enum class InputType {
    None,
    ContentEditable,
    Text,
    Password,
    TextArea,
    Search,
    Email,
    URL,
    Phone,
    Number,
    NumberPad,
    Date,
    DateTime,
    DateTimeLocal,
    Month,
    Week,
    Time,
    Select,
    Drawing,
#if ENABLE(INPUT_TYPE_COLOR)
    Color
#endif
};

#if PLATFORM(IOS_FAMILY)
struct OptionItem {
    OptionItem() { }

    OptionItem(const OptionItem& item)
        : text(item.text)
        , isGroup(item.isGroup)
        , isSelected(item.isSelected)
        , disabled(item.disabled)
        , parentGroupID(item.parentGroupID)
    {
    }

    OptionItem(const String& text, bool isGroup, int parentID, bool selected, bool disabled)
        : text(text)
        , isGroup(isGroup)
        , isSelected(selected)
        , disabled(disabled)
        , parentGroupID(parentID)
    {
    }
    String text;
    bool isGroup { false };
    bool isSelected { false };
    bool disabled { false };
    int parentGroupID { 0 };

    void encode(IPC::Encoder&) const;
    static std::optional<OptionItem> decode(IPC::Decoder&);
};

struct AssistedNodeInformation {
    WebCore::IntRect elementRect;
    WebCore::IntRect selectionRect;
    double minimumScaleFactor { -INFINITY };
    double maximumScaleFactor { INFINITY };
    double maximumScaleFactorIgnoringAlwaysScalable { INFINITY };
    double nodeFontSize { 0 };
    bool hasNextNode { false };
    WebCore::IntRect nextNodeRect;
    bool hasPreviousNode { false };
    WebCore::IntRect previousNodeRect;
    bool isAutocorrect { false };
    bool isRTL { false };
    bool isMultiSelect { false };
    bool isReadOnly {false };
    bool allowsUserScaling { false };
    bool allowsUserScalingIgnoringAlwaysScalable { false };
    bool insideFixedPosition { false };
    AutocapitalizeType autocapitalizeType { AutocapitalizeTypeDefault };
    InputType elementType { InputType::None };
    WebCore::InputMode inputMode { WebCore::InputMode::Unspecified };
    String formAction;
    Vector<OptionItem> selectOptions;
    int selectedIndex { -1 };
    String value;
    double valueAsNumber { 0 };
    String title;
    bool acceptsAutofilledLoginCredentials { false };
    bool isAutofillableUsernameField { false };
    bool elementIsTransparent { false };
    WebCore::URL representingPageURL;
    WebCore::AutofillFieldName autofillFieldName { WebCore::AutofillFieldName::None };
    String placeholder;
    String label;
    String ariaLabel;
    WebCore::GraphicsLayer::EmbeddedViewID embeddedViewID;
#if ENABLE(DATALIST_ELEMENT)
    bool hasSuggestions { false };
#if ENABLE(INPUT_TYPE_COLOR)
    Vector<WebCore::Color> suggestedColors;
#endif
#endif

    uint64_t assistedNodeIdentifier { 0 };

    void encode(IPC::Encoder&) const;
    static bool decode(IPC::Decoder&, AssistedNodeInformation&);
};
#endif

}
