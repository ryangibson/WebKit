/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include <wtf/Optional.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

enum class PasteboardItemPresentationStyle {
    Unspecified,
    Inline,
    Attachment
};

struct PasteboardItemInfo {
    String pathForFileUpload;
    String contentTypeForFileUpload;
    String suggestedFileName;
    bool isNonTextType { false };
    PasteboardItemPresentationStyle preferredPresentationStyle { PasteboardItemPresentationStyle::Unspecified };

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<PasteboardItemInfo> decode(Decoder&);
};

template<class Encoder>
void PasteboardItemInfo::encode(Encoder& encoder) const
{
    encoder << pathForFileUpload << contentTypeForFileUpload << suggestedFileName << isNonTextType;
    encoder.encodeEnum(preferredPresentationStyle);
}

template<class Decoder>
std::optional<PasteboardItemInfo> PasteboardItemInfo::decode(Decoder& decoder)
{
    PasteboardItemInfo result;
    if (!decoder.decode(result.pathForFileUpload))
        return std::nullopt;

    if (!decoder.decode(result.contentTypeForFileUpload))
        return std::nullopt;

    if (!decoder.decode(result.suggestedFileName))
        return std::nullopt;

    if (!decoder.decode(result.isNonTextType))
        return std::nullopt;

    if (!decoder.decodeEnum(result.preferredPresentationStyle))
        return std::nullopt;

    return WTFMove(result);
}

}

namespace WTF {

template<> struct EnumTraits<WebCore::PasteboardItemPresentationStyle> {
    using values = EnumValues<
        WebCore::PasteboardItemPresentationStyle,
        WebCore::PasteboardItemPresentationStyle::Unspecified,
        WebCore::PasteboardItemPresentationStyle::Inline,
        WebCore::PasteboardItemPresentationStyle::Attachment
    >;
};

} // namespace WTF
