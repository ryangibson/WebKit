/*
 * Copyright (C) 2007-2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DragController.h"

#include "HTMLAnchorElement.h"
#include "SVGAElement.h"

#if ENABLE(DRAG_SUPPORT)
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "DataTransfer.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DragActions.h"
#include "DragClient.h"
#include "DragData.h"
#include "DragImage.h"
#include "DragState.h"
#include "Editing.h"
#include "Editor.h"
#include "EditorClient.h"
#include "ElementAncestorIterator.h"
#include "EventHandler.h"
#include "File.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLAttachmentElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLParserIdioms.h"
#include "HTMLPlugInElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Image.h"
#include "ImageOrientation.h"
#include "MoveSelectionCommand.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PlatformKeyboardEvent.h"
#include "PluginDocument.h"
#include "PluginViewBase.h"
#include "Position.h"
#include "PromisedAttachmentInfo.h"
#include "RenderAttachment.h"
#include "RenderFileUploadControl.h"
#include "RenderImage.h"
#include "RenderView.h"
#include "ReplaceSelectionCommand.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleProperties.h"
#include "Text.h"
#include "TextEvent.h"
#include "VisiblePosition.h"
#include "markup.h"

#if ENABLE(DATA_INTERACTION)
#include "SelectionRect.h"
#endif

#include <wtf/RefPtr.h>
#include <wtf/SetForScope.h>
#endif

#if ENABLE(DATA_DETECTION)
#include "DataDetection.h"
#endif

namespace WebCore {

bool isDraggableLink(const Element& element)
{
    if (is<HTMLAnchorElement>(element)) {
        auto& anchorElement = downcast<HTMLAnchorElement>(element);
        if (!anchorElement.isLiveLink())
            return false;
#if ENABLE(DATA_DETECTION)
        return !DataDetection::isDataDetectorURL(anchorElement.href());
#else
        return true;
#endif
    }
    if (is<SVGAElement>(element))
        return element.isLink();
    return false;
}

#if ENABLE(DRAG_SUPPORT)
    
static PlatformMouseEvent createMouseEvent(const DragData& dragData)
{
    bool shiftKey = false;
    bool ctrlKey = false;
    bool altKey = false;
    bool metaKey = false;

    PlatformKeyboardEvent::getCurrentModifierState(shiftKey, ctrlKey, altKey, metaKey);

    return PlatformMouseEvent(dragData.clientPosition(), dragData.globalPosition(),
                              LeftButton, PlatformEvent::MouseMoved, 0, shiftKey, ctrlKey, altKey,
                              metaKey, WallTime::now(), ForceAtClick, NoTap);
}

DragController::DragController(Page& page, DragClient& client)
    : m_page(page)
    , m_client(client)
    , m_numberOfItemsToBeAccepted(0)
    , m_dragHandlingMethod(DragHandlingMethod::None)
    , m_dragDestinationAction(DragDestinationActionNone)
    , m_dragSourceAction(DragSourceActionNone)
    , m_didInitiateDrag(false)
    , m_sourceDragOperation(DragOperationNone)
{
}

DragController::~DragController()
{
    m_client.dragControllerDestroyed();
}

static RefPtr<DocumentFragment> documentFragmentFromDragData(const DragData& dragData, Frame& frame, Range& context, bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;

    Document& document = context.ownerDocument();
    if (dragData.containsCompatibleContent()) {
        if (auto fragment = frame.editor().webContentFromPasteboard(*Pasteboard::createForDragAndDrop(dragData), context, allowPlainText, chosePlainText))
            return fragment;

        if (dragData.containsURL(DragData::DoNotConvertFilenames)) {
            String title;
            String url = dragData.asURL(DragData::DoNotConvertFilenames, &title);
            if (!url.isEmpty()) {
                auto anchor = HTMLAnchorElement::create(document);
                anchor->setHref(url);
                if (title.isEmpty()) {
                    // Try the plain text first because the url might be normalized or escaped.
                    if (dragData.containsPlainText())
                        title = dragData.asPlainText();
                    if (title.isEmpty())
                        title = url;
                }
                anchor->appendChild(document.createTextNode(title));
                auto fragment = document.createDocumentFragment();
                fragment->appendChild(anchor);
                return WTFMove(fragment);
            }
        }
    }
    if (allowPlainText && dragData.containsPlainText()) {
        chosePlainText = true;
        return createFragmentFromText(context, dragData.asPlainText()).ptr();
    }

    return nullptr;
}

#if !PLATFORM(IOS_FAMILY)

DragOperation DragController::platformGenericDragOperation()
{
    return DragOperationMove;
}

#endif

bool DragController::dragIsMove(FrameSelection& selection, const DragData& dragData)
{
    const VisibleSelection& visibleSelection = selection.selection();
    return m_documentUnderMouse == m_dragInitiator && visibleSelection.isContentEditable() && visibleSelection.isRange() && !isCopyKeyDown(dragData);
}

void DragController::clearDragCaret()
{
    m_page.dragCaretController().clear();
}

void DragController::dragEnded()
{
    m_dragInitiator = nullptr;
    m_didInitiateDrag = false;
    m_documentUnderMouse = nullptr;
    clearDragCaret();
    
    m_client.dragEnded();
}

DragOperation DragController::dragEntered(const DragData& dragData)
{
    return dragEnteredOrUpdated(dragData);
}

void DragController::dragExited(const DragData& dragData)
{
    auto& mainFrame = m_page.mainFrame();
    if (mainFrame.view())
        mainFrame.eventHandler().cancelDragAndDrop(createMouseEvent(dragData), Pasteboard::createForDragAndDrop(dragData), dragData.draggingSourceOperationMask(), dragData.containsFiles());
    mouseMovedIntoDocument(nullptr);
    if (m_fileInputElementUnderMouse)
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
    m_fileInputElementUnderMouse = nullptr;
}

DragOperation DragController::dragUpdated(const DragData& dragData)
{
    return dragEnteredOrUpdated(dragData);
}

inline static bool dragIsHandledByDocument(DragController::DragHandlingMethod dragHandlingMethod)
{
    if (dragHandlingMethod == DragController::DragHandlingMethod::None)
        return false;

    if (dragHandlingMethod == DragController::DragHandlingMethod::PageLoad)
        return false;

    return true;
}

bool DragController::performDragOperation(const DragData& dragData)
{
    SetForScope<bool> isPerformingDrop(m_isPerformingDrop, true);
    TemporarySelectionChange ignoreSelectionChanges(m_page.focusController().focusedOrMainFrame(), std::nullopt, TemporarySelectionOption::IgnoreSelectionChanges);

    m_documentUnderMouse = m_page.mainFrame().documentAtPoint(dragData.clientPosition());

    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy = ShouldOpenExternalURLsPolicy::ShouldNotAllow;
    if (m_documentUnderMouse)
        shouldOpenExternalURLsPolicy = m_documentUnderMouse->shouldOpenExternalURLsPolicyToPropagate();

    if ((m_dragDestinationAction & DragDestinationActionDHTML) && dragIsHandledByDocument(m_dragHandlingMethod)) {
        m_client.willPerformDragDestinationAction(DragDestinationActionDHTML, dragData);
        Ref<Frame> mainFrame(m_page.mainFrame());
        bool preventedDefault = false;
        if (mainFrame->view())
            preventedDefault = mainFrame->eventHandler().performDragAndDrop(createMouseEvent(dragData), Pasteboard::createForDragAndDrop(dragData), dragData.draggingSourceOperationMask(), dragData.containsFiles());
        if (preventedDefault) {
            clearDragCaret();
            m_documentUnderMouse = nullptr;
            return true;
        }
    }

    if ((m_dragDestinationAction & DragDestinationActionEdit) && concludeEditDrag(dragData)) {
        m_client.didConcludeEditDrag();
        m_documentUnderMouse = nullptr;
        clearDragCaret();
        return true;
    }

    m_documentUnderMouse = nullptr;
    clearDragCaret();

    if (operationForLoad(dragData) == DragOperationNone)
        return false;

    auto urlString = dragData.asURL();
    if (urlString.isEmpty())
        return false;

    m_client.willPerformDragDestinationAction(DragDestinationActionLoad, dragData);
    m_page.mainFrame().loader().load(FrameLoadRequest(m_page.mainFrame(), { urlString }, shouldOpenExternalURLsPolicy));
    return true;
}

void DragController::mouseMovedIntoDocument(Document* newDocument)
{
    if (m_documentUnderMouse == newDocument)
        return;

    // If we were over another document clear the selection
    if (m_documentUnderMouse)
        clearDragCaret();
    m_documentUnderMouse = newDocument;
}

DragOperation DragController::dragEnteredOrUpdated(const DragData& dragData)
{
    mouseMovedIntoDocument(m_page.mainFrame().documentAtPoint(dragData.clientPosition()));

    m_dragDestinationAction = dragData.dragDestinationAction();
    if (m_dragDestinationAction == DragDestinationActionNone) {
        clearDragCaret(); // FIXME: Why not call mouseMovedIntoDocument(nullptr)?
        return DragOperationNone;
    }

    DragOperation dragOperation = DragOperationNone;
    m_dragHandlingMethod = tryDocumentDrag(dragData, m_dragDestinationAction, dragOperation);
    if (m_dragHandlingMethod == DragHandlingMethod::None && (m_dragDestinationAction & DragDestinationActionLoad)) {
        dragOperation = operationForLoad(dragData);
        if (dragOperation != DragOperationNone)
            m_dragHandlingMethod = DragHandlingMethod::PageLoad;
    } else if (m_dragHandlingMethod == DragHandlingMethod::SetColor)
        dragOperation = DragOperationCopy;

    updateSupportedTypeIdentifiersForDragHandlingMethod(m_dragHandlingMethod, dragData);
    return dragOperation;
}

static HTMLInputElement* asFileInput(Node& node)
{
    if (!is<HTMLInputElement>(node))
        return nullptr;

    auto* inputElement = &downcast<HTMLInputElement>(node);

    // If this is a button inside of the a file input, move up to the file input.
    if (inputElement->isTextButton() && is<ShadowRoot>(inputElement->treeScope().rootNode())) {
        auto& host = *downcast<ShadowRoot>(inputElement->treeScope().rootNode()).host();
        inputElement = is<HTMLInputElement>(host) ? &downcast<HTMLInputElement>(host) : nullptr;
    }

    return inputElement && inputElement->isFileUpload() ? inputElement : nullptr;
}

#if ENABLE(INPUT_TYPE_COLOR)
static bool isEnabledColorInput(Node& node, bool setToShadowAncestor)
{
    Node* candidate = setToShadowAncestor ? node.deprecatedShadowAncestorNode() : &node;
    if (is<HTMLInputElement>(*candidate)) {
        auto& input = downcast<HTMLInputElement>(*candidate);
        if (input.isColorControl() && !input.isDisabledFormControl())
            return true;
    }

    return false;
}
#endif

// This can return null if an empty document is loaded.
static Element* elementUnderMouse(Document* documentUnderMouse, const IntPoint& p)
{
    Frame* frame = documentUnderMouse->frame();
    float zoomFactor = frame ? frame->pageZoomFactor() : 1;
    LayoutPoint point(p.x() * zoomFactor, p.y() * zoomFactor);

    HitTestResult result(point);
    documentUnderMouse->renderView()->hitTest(HitTestRequest(), result);

    Node* node = result.innerNode();
    while (node && !is<Element>(*node))
        node = node->parentNode();
    if (node)
        node = node->deprecatedShadowAncestorNode();

    return downcast<Element>(node);
}

#if !ENABLE(DATA_INTERACTION)

void DragController::updateSupportedTypeIdentifiersForDragHandlingMethod(DragHandlingMethod, const DragData&) const
{
}

#endif

DragController::DragHandlingMethod DragController::tryDocumentDrag(const DragData& dragData, DragDestinationAction actionMask, DragOperation& dragOperation)
{
    if (!m_documentUnderMouse)
        return DragHandlingMethod::None;

    if (m_dragInitiator && !m_documentUnderMouse->securityOrigin().canReceiveDragData(m_dragInitiator->securityOrigin()))
        return DragHandlingMethod::None;

    bool isHandlingDrag = false;
    if (actionMask & DragDestinationActionDHTML) {
        isHandlingDrag = tryDHTMLDrag(dragData, dragOperation);
        // Do not continue if m_documentUnderMouse has been reset by tryDHTMLDrag.
        // tryDHTMLDrag fires dragenter event. The event listener that listens
        // to this event may create a nested message loop (open a modal dialog),
        // which could process dragleave event and reset m_documentUnderMouse in
        // dragExited.
        if (!m_documentUnderMouse)
            return DragHandlingMethod::None;
    }

    // It's unclear why this check is after tryDHTMLDrag.
    // We send drag events in tryDHTMLDrag and that may be the reason.
    RefPtr<FrameView> frameView = m_documentUnderMouse->view();
    if (!frameView)
        return DragHandlingMethod::None;

    if (isHandlingDrag) {
        clearDragCaret();
        m_numberOfItemsToBeAccepted = dragData.numberOfFiles();
        return DragHandlingMethod::NonDefault;
    }

    if ((actionMask & DragDestinationActionEdit) && canProcessDrag(dragData)) {
        if (dragData.containsColor()) {
            dragOperation = DragOperationGeneric;
            return DragHandlingMethod::SetColor;
        }

        IntPoint point = frameView->windowToContents(dragData.clientPosition());
        Element* element = elementUnderMouse(m_documentUnderMouse.get(), point);
        if (!element)
            return DragHandlingMethod::None;
        
        HTMLInputElement* elementAsFileInput = asFileInput(*element);
        if (m_fileInputElementUnderMouse != elementAsFileInput) {
            if (m_fileInputElementUnderMouse)
                m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
            m_fileInputElementUnderMouse = elementAsFileInput;
        }
        
        if (!m_fileInputElementUnderMouse)
            m_page.dragCaretController().setCaretPosition(m_documentUnderMouse->frame()->visiblePositionForPoint(point));
        else
            clearDragCaret();

        Frame* innerFrame = element->document().frame();
        dragOperation = dragIsMove(innerFrame->selection(), dragData) ? DragOperationMove : DragOperationCopy;
        m_numberOfItemsToBeAccepted = 0;

        unsigned numberOfFiles = dragData.numberOfFiles();
        if (m_fileInputElementUnderMouse) {
            if (m_fileInputElementUnderMouse->isDisabledFormControl())
                m_numberOfItemsToBeAccepted = 0;
            else if (m_fileInputElementUnderMouse->multiple())
                m_numberOfItemsToBeAccepted = numberOfFiles;
            else if (numberOfFiles > 1)
                m_numberOfItemsToBeAccepted = 0;
            else
                m_numberOfItemsToBeAccepted = 1;
            
            if (!m_numberOfItemsToBeAccepted)
                dragOperation = DragOperationNone;
            m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(m_numberOfItemsToBeAccepted);
        } else {
            // We are not over a file input element. The dragged item(s) will only
            // be loaded into the view the number of dragged items is 1.
            m_numberOfItemsToBeAccepted = numberOfFiles != 1 ? 0 : 1;
        }

        if (m_fileInputElementUnderMouse)
            return DragHandlingMethod::UploadFile;

        if (m_page.dragCaretController().isContentRichlyEditable())
            return DragHandlingMethod::EditRichText;

        return DragHandlingMethod::EditPlainText;
    }
    
    // We are not over an editable region. Make sure we're clearing any prior drag cursor.
    clearDragCaret();
    if (m_fileInputElementUnderMouse)
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
    m_fileInputElementUnderMouse = nullptr;
    return DragHandlingMethod::None;
}

DragSourceAction DragController::delegateDragSourceAction(const IntPoint& rootViewPoint)
{
    m_dragSourceAction = m_client.dragSourceActionMaskForPoint(rootViewPoint);
    return m_dragSourceAction;
}

DragOperation DragController::operationForLoad(const DragData& dragData)
{
    Document* document = m_page.mainFrame().documentAtPoint(dragData.clientPosition());

    bool pluginDocumentAcceptsDrags = false;

    if (is<PluginDocument>(document)) {
        const Widget* widget = downcast<PluginDocument>(*document).pluginWidget();
        const PluginViewBase* pluginView = is<PluginViewBase>(widget) ? downcast<PluginViewBase>(widget) : nullptr;

        if (pluginView)
            pluginDocumentAcceptsDrags = pluginView->shouldAllowNavigationFromDrags();
    }

    if (document && (m_didInitiateDrag || (is<PluginDocument>(*document) && !pluginDocumentAcceptsDrags) || document->hasEditableStyle()))
        return DragOperationNone;
    return dragOperation(dragData);
}

static bool setSelectionToDragCaret(Frame* frame, VisibleSelection& dragCaret, RefPtr<Range>& range, const IntPoint& point)
{
    Ref<Frame> protector(*frame);
    frame->selection().setSelection(dragCaret);
    if (frame->selection().selection().isNone()) {
        dragCaret = frame->visiblePositionForPoint(point);
        frame->selection().setSelection(dragCaret);
        range = dragCaret.toNormalizedRange();
    }
    return !frame->selection().isNone() && frame->selection().selection().isContentEditable();
}

bool DragController::dispatchTextInputEventFor(Frame* innerFrame, const DragData& dragData)
{
    ASSERT(m_page.dragCaretController().hasCaret());
    String text = m_page.dragCaretController().isContentRichlyEditable() ? emptyString() : dragData.asPlainText();
    Element* target = innerFrame->editor().findEventTargetFrom(m_page.dragCaretController().caretPosition());
    // FIXME: What guarantees target is not null?
    auto event = TextEvent::createForDrop(&innerFrame->windowProxy(), text);
    target->dispatchEvent(event);
    return !event->defaultPrevented();
}

bool DragController::concludeEditDrag(const DragData& dragData)
{
    RefPtr<HTMLInputElement> fileInput = m_fileInputElementUnderMouse;
    if (m_fileInputElementUnderMouse) {
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
        m_fileInputElementUnderMouse = nullptr;
    }

    if (!m_documentUnderMouse)
        return false;

    IntPoint point = m_documentUnderMouse->view()->windowToContents(dragData.clientPosition());
    Element* element = elementUnderMouse(m_documentUnderMouse.get(), point);
    if (!element)
        return false;
    RefPtr<Frame> innerFrame = element->document().frame();
    ASSERT(innerFrame);

    if (m_page.dragCaretController().hasCaret() && !dispatchTextInputEventFor(innerFrame.get(), dragData))
        return true;

    if (dragData.containsColor()) {
        Color color = dragData.asColor();
        if (!color.isValid())
            return false;
#if ENABLE(INPUT_TYPE_COLOR)
        if (isEnabledColorInput(*element, false)) {
            auto& input = downcast<HTMLInputElement>(*element);
            input.setValue(color.serialized(), DispatchInputAndChangeEvent);
            return true;
        }
#endif
        auto innerRange = innerFrame->selection().toNormalizedRange();
        if (!innerRange)
            return false;
        auto style = MutableStyleProperties::create();
        style->setProperty(CSSPropertyColor, color.serialized(), false);
        if (!innerFrame->editor().shouldApplyStyle(style.ptr(), innerRange.get()))
            return false;
        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);
        innerFrame->editor().applyStyle(style.ptr(), EditAction::SetColor);
        return true;
    }

    if (dragData.containsFiles() && fileInput) {
        // fileInput should be the element we hit tested for, unless it was made
        // display:none in a drop event handler.
        ASSERT(fileInput == element || !fileInput->renderer());
        if (fileInput->isDisabledFormControl())
            return false;

        return fileInput->receiveDroppedFiles(dragData);
    }

    if (!m_page.dragController().canProcessDrag(dragData))
        return false;

    VisibleSelection dragCaret = m_page.dragCaretController().caretPosition();
    RefPtr<Range> range = dragCaret.toNormalizedRange();
    RefPtr<Element> rootEditableElement = innerFrame->selection().selection().rootEditableElement();

    // For range to be null a WebKit client must have done something bad while
    // manually controlling drag behaviour
    if (!range)
        return false;

    ResourceCacheValidationSuppressor validationSuppressor(range->ownerDocument().cachedResourceLoader());
    auto& editor = innerFrame->editor();
    bool isMove = dragIsMove(innerFrame->selection(), dragData);
    if (isMove || dragCaret.isContentRichlyEditable()) {
        bool chosePlainText = false;
        RefPtr<DocumentFragment> fragment = documentFragmentFromDragData(dragData, *innerFrame, *range, true, chosePlainText);
        if (!fragment || !editor.shouldInsertFragment(*fragment, range.get(), EditorInsertAction::Dropped))
            return false;

        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);

        if (editor.client() && editor.client()->performTwoStepDrop(*fragment, *range, isMove))
            return true;

        if (isMove) {
            // NSTextView behavior is to always smart delete on moving a selection,
            // but only to smart insert if the selection granularity is word granularity.
            bool smartDelete = editor.smartInsertDeleteEnabled();
            bool smartInsert = smartDelete && innerFrame->selection().granularity() == WordGranularity && dragData.canSmartReplace();
            MoveSelectionCommand::create(fragment.releaseNonNull(), dragCaret.base(), smartInsert, smartDelete)->apply();
        } else {
            if (setSelectionToDragCaret(innerFrame.get(), dragCaret, range, point)) {
                OptionSet<ReplaceSelectionCommand::CommandOption> options { ReplaceSelectionCommand::SelectReplacement, ReplaceSelectionCommand::PreventNesting };
                if (dragData.canSmartReplace())
                    options.add(ReplaceSelectionCommand::SmartReplace);
                if (chosePlainText)
                    options.add(ReplaceSelectionCommand::MatchStyle);
                ReplaceSelectionCommand::create(*m_documentUnderMouse, fragment.releaseNonNull(), options, EditAction::InsertFromDrop)->apply();
            }
        }
    } else {
        String text = dragData.asPlainText();
        if (text.isEmpty() || !editor.shouldInsertText(text, range.get(), EditorInsertAction::Dropped))
            return false;

        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);
        RefPtr<DocumentFragment> fragment = createFragmentFromText(*range, text);
        if (!fragment)
            return false;

        if (editor.client() && editor.client()->performTwoStepDrop(*fragment, *range, isMove))
            return true;

        if (setSelectionToDragCaret(innerFrame.get(), dragCaret, range, point))
            ReplaceSelectionCommand::create(*m_documentUnderMouse, fragment.get(), { ReplaceSelectionCommand::SelectReplacement, ReplaceSelectionCommand::MatchStyle, ReplaceSelectionCommand::PreventNesting }, EditAction::InsertFromDrop)->apply();
    }

    if (rootEditableElement) {
        if (Frame* frame = rootEditableElement->document().frame())
            frame->eventHandler().updateDragStateAfterEditDragIfNeeded(*rootEditableElement);
    }

    return true;
}

bool DragController::canProcessDrag(const DragData& dragData)
{
    IntPoint point = m_page.mainFrame().view()->windowToContents(dragData.clientPosition());
    HitTestResult result = HitTestResult(point);
    if (!m_page.mainFrame().contentRenderer())
        return false;

    result = m_page.mainFrame().eventHandler().hitTestResultAtPoint(point, HitTestRequest::ReadOnly | HitTestRequest::Active);

    if (!result.innerNonSharedNode())
        return false;

    DragData::DraggingPurpose dragPurpose = DragData::DraggingPurpose::ForEditing;
    if (asFileInput(*result.innerNonSharedNode()))
        dragPurpose = DragData::DraggingPurpose::ForFileUpload;
#if ENABLE(INPUT_TYPE_COLOR)
    else if (isEnabledColorInput(*result.innerNonSharedNode(), true))
        dragPurpose = DragData::DraggingPurpose::ForColorControl;
#endif

    if (!dragData.containsCompatibleContent(dragPurpose))
        return false;

    if (dragPurpose == DragData::DraggingPurpose::ForFileUpload)
        return true;

#if ENABLE(INPUT_TYPE_COLOR)
    if (dragPurpose == DragData::DraggingPurpose::ForColorControl)
        return true;
#endif

    if (is<HTMLPlugInElement>(*result.innerNonSharedNode())) {
        if (!downcast<HTMLPlugInElement>(result.innerNonSharedNode())->canProcessDrag() && !result.innerNonSharedNode()->hasEditableStyle())
            return false;
    } else if (!result.innerNonSharedNode()->hasEditableStyle())
        return false;

    if (m_didInitiateDrag && m_documentUnderMouse == m_dragInitiator && result.isSelected())
        return false;

    return true;
}

static DragOperation defaultOperationForDrag(DragOperation srcOpMask)
{
    // This is designed to match IE's operation fallback for the case where
    // the page calls preventDefault() in a drag event but doesn't set dropEffect.
    if (srcOpMask == DragOperationEvery)
        return DragOperationCopy;
    if (srcOpMask == DragOperationNone)
        return DragOperationNone;
    if (srcOpMask & DragOperationMove)
        return DragOperationMove;
    if (srcOpMask & DragOperationGeneric)
        return DragController::platformGenericDragOperation();
    if (srcOpMask & DragOperationCopy)
        return DragOperationCopy;
    if (srcOpMask & DragOperationLink)
        return DragOperationLink;
    
    // FIXME: Does IE really return "generic" even if no operations were allowed by the source?
    return DragOperationGeneric;
}

bool DragController::tryDHTMLDrag(const DragData& dragData, DragOperation& operation)
{
    ASSERT(m_documentUnderMouse);
    Ref<Frame> mainFrame(m_page.mainFrame());
    RefPtr<FrameView> viewProtector = mainFrame->view();
    if (!viewProtector)
        return false;

    DragOperation sourceOperation = dragData.draggingSourceOperationMask();
    auto targetResponse = mainFrame->eventHandler().updateDragAndDrop(createMouseEvent(dragData), [&dragData]() { return Pasteboard::createForDragAndDrop(dragData); }, sourceOperation, dragData.containsFiles());
    if (!targetResponse.accept)
        return false;

    if (!targetResponse.operation)
        operation = defaultOperationForDrag(sourceOperation);
    else if (!(sourceOperation & targetResponse.operation.value())) // The element picked an operation which is not supported by the source
        operation = DragOperationNone;
    else
        operation = targetResponse.operation.value();

    return true;
}

static bool imageElementIsDraggable(const HTMLImageElement& image, const Frame& sourceFrame)
{
    if (sourceFrame.settings().loadsImagesAutomatically())
        return true;

    auto* renderer = image.renderer();
    if (!is<RenderImage>(renderer))
        return false;

    auto* cachedImage = downcast<RenderImage>(*renderer).cachedImage();
    return cachedImage && !cachedImage->errorOccurred() && cachedImage->imageForRenderer(renderer);
}

#if ENABLE(ATTACHMENT_ELEMENT)

static RefPtr<HTMLAttachmentElement> enclosingAttachmentElement(Element& element)
{
    if (is<HTMLAttachmentElement>(element))
        return downcast<HTMLAttachmentElement>(&element);

    if (is<HTMLAttachmentElement>(element.parentOrShadowHostElement()))
        return downcast<HTMLAttachmentElement>(element.parentOrShadowHostElement());

    return { };
}

#endif

Element* DragController::draggableElement(const Frame* sourceFrame, Element* startElement, const IntPoint& dragOrigin, DragState& state) const
{
    state.type = (sourceFrame->selection().contains(dragOrigin)) ? DragSourceActionSelection : DragSourceActionNone;
    if (!startElement)
        return nullptr;
#if ENABLE(ATTACHMENT_ELEMENT)
    if (auto attachment = enclosingAttachmentElement(*startElement)) {
        auto selection = sourceFrame->selection().selection();
        bool isSingleAttachmentSelection = selection.start() == Position(attachment.get(), Position::PositionIsBeforeAnchor) && selection.end() == Position(attachment.get(), Position::PositionIsAfterAnchor);
        bool isAttachmentElementInCurrentSelection = false;
        if (auto selectedRange = selection.toNormalizedRange()) {
            auto compareResult = selectedRange->compareNode(*attachment);
            isAttachmentElementInCurrentSelection = !compareResult.hasException() && compareResult.releaseReturnValue() == Range::NODE_INSIDE;
        }

        if (!isAttachmentElementInCurrentSelection || isSingleAttachmentSelection) {
            state.type = DragSourceActionAttachment;
            return attachment.get();
        }
    }
#endif

    for (auto* element = startElement; element; element = element->parentOrShadowHostElement()) {
        auto* renderer = element->renderer();
        if (!renderer)
            continue;

        UserDrag dragMode = renderer->style().userDrag();
        if ((m_dragSourceAction & DragSourceActionDHTML) && dragMode == UserDrag::Element) {
            state.type = static_cast<DragSourceAction>(state.type | DragSourceActionDHTML);
            return element;
        }
        if (dragMode == UserDrag::Auto) {
            if ((m_dragSourceAction & DragSourceActionImage)
                && is<HTMLImageElement>(*element)
                && imageElementIsDraggable(downcast<HTMLImageElement>(*element), *sourceFrame)) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionImage);
                return element;
            }
            if ((m_dragSourceAction & DragSourceActionLink) && isDraggableLink(*element)) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionLink);
                return element;
            }
#if ENABLE(ATTACHMENT_ELEMENT)
            if ((m_dragSourceAction & DragSourceActionAttachment)
                && is<HTMLAttachmentElement>(*element)
                && downcast<HTMLAttachmentElement>(*element).file()) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionAttachment);
                return element;
            }
#endif
#if ENABLE(INPUT_TYPE_COLOR)
            if ((m_dragSourceAction & DragSourceActionColor)
                && isEnabledColorInput(*element, false)) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionColor);
                return element;
            }
#endif
        }
    }

    // We either have nothing to drag or we have a selection and we're not over a draggable element.
    return (state.type & DragSourceActionSelection) ? startElement : nullptr;
}

static CachedImage* getCachedImage(Element& element)
{
    RenderObject* renderer = element.renderer();
    if (!is<RenderImage>(renderer))
        return nullptr;
    auto& image = downcast<RenderImage>(*renderer);
    return image.cachedImage();
}

static Image* getImage(Element& element)
{
    CachedImage* cachedImage = getCachedImage(element);
    // Don't use cachedImage->imageForRenderer() here as that may return BitmapImages for cached SVG Images.
    // Users of getImage() want access to the SVGImage, in order to figure out the filename extensions,
    // which would be empty when asking the cached BitmapImages.
    return (cachedImage && !cachedImage->errorOccurred()) ?
        cachedImage->image() : nullptr;
}

static void selectElement(Element& element)
{
    RefPtr<Range> range = element.document().createRange();
    range->selectNode(element);
    element.document().frame()->selection().setSelection(VisibleSelection(*range, DOWNSTREAM));
}

static IntPoint dragLocForDHTMLDrag(const IntPoint& mouseDraggedPoint, const IntPoint& dragOrigin, const IntPoint& dragImageOffset, bool isLinkImage)
{
    // dragImageOffset is the cursor position relative to the lower-left corner of the image.
#if PLATFORM(MAC)
    // We add in the Y dimension because we are a flipped view, so adding moves the image down.
    const int yOffset = dragImageOffset.y();
#else
    const int yOffset = -dragImageOffset.y();
#endif

    if (isLinkImage)
        return IntPoint(mouseDraggedPoint.x() - dragImageOffset.x(), mouseDraggedPoint.y() + yOffset);

    return IntPoint(dragOrigin.x() - dragImageOffset.x(), dragOrigin.y() + yOffset);
}

static FloatPoint dragImageAnchorPointForSelectionDrag(Frame& frame, const IntPoint& mouseDraggedPoint)
{
    IntRect draggingRect = enclosingIntRect(frame.selection().selectionBounds());

    float x = (mouseDraggedPoint.x() - draggingRect.x()) / (float)draggingRect.width();
    float y = (mouseDraggedPoint.y() - draggingRect.y()) / (float)draggingRect.height();

    return FloatPoint { x, y };
}

static IntPoint dragLocForSelectionDrag(Frame& src)
{
    IntRect draggingRect = enclosingIntRect(src.selection().selectionBounds());
    int xpos = draggingRect.maxX();
    xpos = draggingRect.x() < xpos ? draggingRect.x() : xpos;
    int ypos = draggingRect.maxY();
#if PLATFORM(COCOA)
    // Deal with flipped coordinates on Mac
    ypos = draggingRect.y() > ypos ? draggingRect.y() : ypos;
#else
    ypos = draggingRect.y() < ypos ? draggingRect.y() : ypos;
#endif
    return IntPoint(xpos, ypos);
}

bool DragController::startDrag(Frame& src, const DragState& state, DragOperation srcOp, const PlatformMouseEvent& dragEvent, const IntPoint& dragOrigin, HasNonDefaultPasteboardData hasData)
{
    if (!src.view() || !src.contentRenderer() || !state.source)
        return false;

    Ref<Frame> protector(src);
    HitTestResult hitTestResult = src.eventHandler().hitTestResultAtPoint(dragOrigin, HitTestRequest::ReadOnly | HitTestRequest::Active);

    bool sourceContainsHitNode = state.source->containsIncludingShadowDOM(hitTestResult.innerNode());
    if (!sourceContainsHitNode) {
        // The original node being dragged isn't under the drag origin anymore... maybe it was
        // hidden or moved out from under the cursor. Regardless, we don't want to start a drag on
        // something that's not actually under the drag origin.
        return false;
    }

    URL linkURL = hitTestResult.absoluteLinkURL();
    URL imageURL = hitTestResult.absoluteImageURL();

    IntPoint mouseDraggedPoint = src.view()->windowToContents(dragEvent.position());

    m_draggingImageURL = URL();
    m_sourceDragOperation = srcOp;

    DragImage dragImage;
    IntPoint dragLoc(0, 0);
    IntPoint dragImageOffset(0, 0);

    ASSERT(state.dataTransfer);

    DataTransfer& dataTransfer = *state.dataTransfer;
    if (state.type == DragSourceActionDHTML) {
        dragImage = DragImage { dataTransfer.createDragImage(dragImageOffset) };
        // We allow DHTML/JS to set the drag image, even if its a link, image or text we're dragging.
        // This is in the spirit of the IE API, which allows overriding of pasteboard data and DragOp.
        if (dragImage) {
            dragLoc = dragLocForDHTMLDrag(mouseDraggedPoint, dragOrigin, dragImageOffset, !linkURL.isEmpty());
            m_dragOffset = dragImageOffset;
        }
    }

    if (state.type == DragSourceActionSelection || !imageURL.isEmpty() || !linkURL.isEmpty()) {
        // Selection, image, and link drags receive a default set of allowed drag operations that
        // follows from:
        // http://trac.webkit.org/browser/trunk/WebKit/mac/WebView/WebHTMLView.mm?rev=48526#L3430
        m_sourceDragOperation = static_cast<DragOperation>(m_sourceDragOperation | DragOperationGeneric | DragOperationCopy);
    }

    ASSERT(state.source);
    Element& element = *state.source;

    bool mustUseLegacyDragClient = hasData == HasNonDefaultPasteboardData::Yes || m_client.useLegacyDragClient();

    IntRect dragImageBounds;
    Image* image = getImage(element);
    if (state.type == DragSourceActionSelection) {
        PasteboardWriterData pasteboardWriterData;

        if (hasData == HasNonDefaultPasteboardData::No) {
            if (src.selection().selection().isNone()) {
                // The page may have cleared out the selection in the dragstart handler, in which case we should bail
                // out of the drag, since there is no content to write to the pasteboard.
                return false;
            }

            // FIXME: This entire block is almost identical to the code in Editor::copy, and the code should be shared.
            RefPtr<Range> selectionRange = src.selection().toNormalizedRange();
            ASSERT(selectionRange);

            src.editor().willWriteSelectionToPasteboard(selectionRange.get());

            if (enclosingTextFormControl(src.selection().selection().start())) {
                if (mustUseLegacyDragClient)
                    dataTransfer.pasteboard().writePlainText(src.editor().selectedTextForDataTransfer(), Pasteboard::CannotSmartReplace);
                else {
                    PasteboardWriterData::PlainText plainText;
                    plainText.canSmartCopyOrDelete = false;
                    plainText.text = src.editor().selectedTextForDataTransfer();
                    pasteboardWriterData.setPlainText(WTFMove(plainText));
                }
            } else {
                if (mustUseLegacyDragClient) {
#if PLATFORM(COCOA) || PLATFORM(GTK)
                    src.editor().writeSelectionToPasteboard(dataTransfer.pasteboard());
#else
                    // FIXME: Convert all other platforms to match Mac and delete this.
                    dataTransfer.pasteboard().writeSelection(*selectionRange, src.editor().canSmartCopyOrDelete(), src, IncludeImageAltTextForDataTransfer);
#endif
                } else {
#if PLATFORM(COCOA)
                    src.editor().writeSelection(pasteboardWriterData);
#endif
                }
            }

            src.editor().didWriteSelectionToPasteboard();
        }
        m_client.willPerformDragSourceAction(DragSourceActionSelection, dragOrigin, dataTransfer);
        if (!dragImage) {
            TextIndicatorData textIndicator;
            dragImage = DragImage { dissolveDragImageToFraction(createDragImageForSelection(src, textIndicator), DragImageAlpha) };
            if (textIndicator.contentImage)
                dragImage.setIndicatorData(textIndicator);
            dragLoc = dragLocForSelectionDrag(src);
            m_dragOffset = IntPoint(dragOrigin.x() - dragLoc.x(), dragOrigin.y() - dragLoc.y());
        }

        if (!dragImage)
            return false;

        if (mustUseLegacyDragClient) {
            doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, { });
            return true;
        }

        DragItem dragItem;
        dragItem.imageAnchorPoint = dragImageAnchorPointForSelectionDrag(src, mouseDraggedPoint);
        dragItem.image = WTFMove(dragImage);
        dragItem.data = WTFMove(pasteboardWriterData);

        beginDrag(WTFMove(dragItem), src, dragOrigin, mouseDraggedPoint, dataTransfer, DragSourceActionSelection);

        return true;
    }

    if (!src.document()->securityOrigin().canDisplay(linkURL)) {
        src.document()->addConsoleMessage(MessageSource::Security, MessageLevel::Error, "Not allowed to drag local resource: " + linkURL.stringCenterEllipsizedToLength());
        return false;
    }

    if (!imageURL.isEmpty() && image && !image->isNull() && (m_dragSourceAction & DragSourceActionImage)) {
        // We shouldn't be starting a drag for an image that can't provide an extension.
        // This is an early detection for problems encountered later upon drop.
        ASSERT(!image->filenameExtension().isEmpty());

#if ENABLE(ATTACHMENT_ELEMENT)
        auto attachmentInfo = promisedAttachmentInfo(src, element);
#else
        PromisedAttachmentInfo attachmentInfo;
#endif

        if (hasData == HasNonDefaultPasteboardData::No) {
            m_draggingImageURL = imageURL;
            if (element.isContentRichlyEditable())
                selectElement(element);
            if (!attachmentInfo)
                declareAndWriteDragImage(dataTransfer, element, !linkURL.isEmpty() ? linkURL : imageURL, hitTestResult.altDisplayString());
        }

        m_client.willPerformDragSourceAction(DragSourceActionImage, dragOrigin, dataTransfer);

        if (!dragImage)
            doImageDrag(element, dragOrigin, hitTestResult.imageRect(), src, m_dragOffset, state, WTFMove(attachmentInfo));
        else {
            // DHTML defined drag image
            doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, WTFMove(attachmentInfo));
        }

        return true;
    }

    if (!linkURL.isEmpty() && (m_dragSourceAction & DragSourceActionLink)) {
        PasteboardWriterData pasteboardWriterData;

        String textContentWithSimplifiedWhiteSpace = hitTestResult.textContent().simplifyWhiteSpace();

        if (hasData == HasNonDefaultPasteboardData::No) {
            // Simplify whitespace so the title put on the dataTransfer resembles what the user sees
            // on the web page. This includes replacing newlines with spaces.
            if (mustUseLegacyDragClient)
                src.editor().copyURL(linkURL, textContentWithSimplifiedWhiteSpace, dataTransfer.pasteboard());
            else
                pasteboardWriterData.setURL(src.editor().pasteboardWriterURL(linkURL, textContentWithSimplifiedWhiteSpace));
        } else {
            // Make sure the pasteboard also contains trustworthy link data
            // but don't overwrite more general pasteboard types.
            PasteboardURL pasteboardURL;
            pasteboardURL.url = linkURL;
            pasteboardURL.title = hitTestResult.textContent();
            dataTransfer.pasteboard().writeTrustworthyWebURLsPboardType(pasteboardURL);
        }

        const VisibleSelection& sourceSelection = src.selection().selection();
        if (sourceSelection.isCaret() && sourceSelection.isContentEditable()) {
            // a user can initiate a drag on a link without having any text
            // selected.  In this case, we should expand the selection to
            // the enclosing anchor element
            Position pos = sourceSelection.base();
            Node* node = enclosingAnchorElement(pos);
            if (node)
                src.selection().setSelection(VisibleSelection::selectionFromContentsOfNode(node));
        }

        m_client.willPerformDragSourceAction(DragSourceActionLink, dragOrigin, dataTransfer);
        if (!dragImage) {
            TextIndicatorData textIndicator;
            dragImage = DragImage { createDragImageForLink(element, linkURL, textContentWithSimplifiedWhiteSpace, textIndicator, src.settings().fontRenderingMode(), m_page.deviceScaleFactor()) };
            if (dragImage) {
                m_dragOffset = dragOffsetForLinkDragImage(dragImage.get());
                dragLoc = IntPoint(dragOrigin.x() + m_dragOffset.x(), dragOrigin.y() + m_dragOffset.y());
                dragImage = DragImage { platformAdjustDragImageForDeviceScaleFactor(dragImage.get(), m_page.deviceScaleFactor()) };
                if (textIndicator.contentImage)
                    dragImage.setIndicatorData(textIndicator);
            }
        }

        if (mustUseLegacyDragClient) {
            doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, { });
            return true;
        }

        DragItem dragItem;
        dragItem.imageAnchorPoint = dragImage ? anchorPointForLinkDragImage(dragImage.get()) : FloatPoint();
        dragItem.image = WTFMove(dragImage);
        dragItem.data = WTFMove(pasteboardWriterData);

        beginDrag(WTFMove(dragItem), src, dragOrigin, mouseDraggedPoint, dataTransfer, DragSourceActionSelection);

        return true;
    }

#if ENABLE(ATTACHMENT_ELEMENT)
    if (is<HTMLAttachmentElement>(element) && m_dragSourceAction & DragSourceActionAttachment) {
        auto& attachment = downcast<HTMLAttachmentElement>(element);
        auto* attachmentRenderer = attachment.renderer();

        src.editor().setIgnoreSelectionChanges(true);
        auto previousSelection = src.selection().selection();
        selectElement(element);

        PromisedAttachmentInfo promisedAttachment;
        if (hasData == HasNonDefaultPasteboardData::No) {
            promisedAttachment = promisedAttachmentInfo(src, attachment);
            auto& editor = src.editor();
            if (!promisedAttachment && editor.client()) {
#if PLATFORM(COCOA)
                // Otherwise, if no file URL is specified, call out to the injected bundle to populate the pasteboard with data.
                editor.willWriteSelectionToPasteboard(src.selection().toNormalizedRange().get());
                editor.writeSelectionToPasteboard(dataTransfer.pasteboard());
                editor.didWriteSelectionToPasteboard();
#endif
            }
        }
        
        m_client.willPerformDragSourceAction(DragSourceActionAttachment, dragOrigin, dataTransfer);
        
        if (!dragImage) {
            TextIndicatorData textIndicator;
            if (attachmentRenderer)
                attachmentRenderer->setShouldDrawBorder(false);
            dragImage = DragImage { dissolveDragImageToFraction(createDragImageForSelection(src, textIndicator), DragImageAlpha) };
            if (attachmentRenderer)
                attachmentRenderer->setShouldDrawBorder(true);
            if (textIndicator.contentImage)
                dragImage.setIndicatorData(textIndicator);
            dragLoc = dragLocForSelectionDrag(src);
            m_dragOffset = IntPoint(dragOrigin.x() - dragLoc.x(), dragOrigin.y() - dragLoc.y());
        }
        doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, WTFMove(promisedAttachment));
        if (!element.isContentRichlyEditable())
            src.selection().setSelection(previousSelection);
        src.editor().setIgnoreSelectionChanges(false);
        return true;
    }
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    bool isColorControl = is<HTMLInputElement>(state.source) && downcast<HTMLInputElement>(*state.source).isColorControl();
    if (isColorControl && m_dragSourceAction & DragSourceActionColor) {
        auto& input = downcast<HTMLInputElement>(*state.source);
        auto color = input.valueAsColor();

        Path visiblePath;
        dragImage = DragImage { createDragImageForColor(color, input.boundsInRootViewSpace(), input.document().page()->pageScaleFactor(), visiblePath) };
        dragImage.setVisiblePath(visiblePath);
        dataTransfer.pasteboard().write(color);
        dragImageOffset = IntPoint { dragImageSize(dragImage.get()) };
        dragLoc = dragLocForDHTMLDrag(mouseDraggedPoint, dragOrigin, dragImageOffset, false);

        m_client.willPerformDragSourceAction(DragSourceActionColor, dragOrigin, dataTransfer);
        doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, { });
        return true;
    }
#endif

    if (state.type == DragSourceActionDHTML && dragImage) {
        ASSERT(m_dragSourceAction & DragSourceActionDHTML);
        m_client.willPerformDragSourceAction(DragSourceActionDHTML, dragOrigin, dataTransfer);
        doSystemDrag(WTFMove(dragImage), dragLoc, dragOrigin, src, state, { });
        return true;
    }

    return false;
}

void DragController::doImageDrag(Element& element, const IntPoint& dragOrigin, const IntRect& layoutRect, Frame& frame, IntPoint& dragImageOffset, const DragState& state, PromisedAttachmentInfo&& attachmentInfo)
{
    IntPoint mouseDownPoint = dragOrigin;
    DragImage dragImage;
    IntPoint scaledOrigin;

    if (!element.renderer())
        return;

    ImageOrientationDescription orientationDescription(element.renderer()->shouldRespectImageOrientation(), element.renderer()->style().imageOrientation());

    Image* image = getImage(element);
    if (image && !layoutRect.isEmpty() && shouldUseCachedImageForDragImage(*image) && (dragImage = DragImage { createDragImageFromImage(image, element.renderer() ? orientationDescription : ImageOrientationDescription()) })) {
        dragImage = DragImage { fitDragImageToMaxSize(dragImage.get(), layoutRect.size(), maxDragImageSize()) };
        IntSize fittedSize = dragImageSize(dragImage.get());

        dragImage = DragImage { platformAdjustDragImageForDeviceScaleFactor(dragImage.get(), m_page.deviceScaleFactor()) };
        dragImage = DragImage { dissolveDragImageToFraction(dragImage.get(), DragImageAlpha) };

        // Properly orient the drag image and orient it differently if it's smaller than the original.
        float scale = fittedSize.width() / (float)layoutRect.width();
        float dx = scale * (layoutRect.x() - mouseDownPoint.x());
        float originY = layoutRect.y();
#if PLATFORM(COCOA)
        // Compensate for accursed flipped coordinates in Cocoa.
        originY += layoutRect.height();
#endif
        float dy = scale * (originY - mouseDownPoint.y());
        scaledOrigin = IntPoint((int)(dx + 0.5), (int)(dy + 0.5));
    } else {
        if (CachedImage* cachedImage = getCachedImage(element)) {
            dragImage = DragImage { createDragImageIconForCachedImageFilename(cachedImage->response().suggestedFilename()) };
            if (dragImage) {
                dragImage = DragImage { platformAdjustDragImageForDeviceScaleFactor(dragImage.get(), m_page.deviceScaleFactor()) };
                scaledOrigin = IntPoint(DragIconRightInset - dragImageSize(dragImage.get()).width(), DragIconBottomInset);
            }
        }
    }

    if (!dragImage)
        return;

    dragImageOffset = mouseDownPoint + scaledOrigin;
    doSystemDrag(WTFMove(dragImage), dragImageOffset, dragOrigin, frame, state, WTFMove(attachmentInfo));
}

void DragController::beginDrag(DragItem dragItem, Frame& frame, const IntPoint& mouseDownPoint, const IntPoint& mouseDraggedPoint, DataTransfer& dataTransfer, DragSourceAction dragSourceAction)
{
    ASSERT(!m_client.useLegacyDragClient());

    m_didInitiateDrag = true;
    m_dragInitiator = frame.document();

    // Protect this frame and view, as a load may occur mid drag and attempt to unload this frame
    Ref<Frame> mainFrameProtector(m_page.mainFrame());
    RefPtr<FrameView> viewProtector = mainFrameProtector->view();

    auto mouseDownPointInRootViewCoordinates = viewProtector->rootViewToContents(frame.view()->contentsToRootView(mouseDownPoint));
    auto mouseDraggedPointInRootViewCoordinates = viewProtector->rootViewToContents(frame.view()->contentsToRootView(mouseDraggedPoint));

    m_client.beginDrag(WTFMove(dragItem), frame, mouseDownPointInRootViewCoordinates, mouseDraggedPointInRootViewCoordinates, dataTransfer, dragSourceAction);
}

void DragController::doSystemDrag(DragImage image, const IntPoint& dragLoc, const IntPoint& eventPos, Frame& frame, const DragState& state, PromisedAttachmentInfo&& promisedAttachmentInfo)
{
    m_didInitiateDrag = true;
    m_dragInitiator = frame.document();
    // Protect this frame and view, as a load may occur mid drag and attempt to unload this frame
    Ref<Frame> frameProtector(m_page.mainFrame());
    RefPtr<FrameView> viewProtector = frameProtector->view();

    DragItem item;
    item.image = WTFMove(image);
    item.sourceAction = state.type;
    item.promisedAttachmentInfo = WTFMove(promisedAttachmentInfo);

    auto eventPositionInRootViewCoordinates = frame.view()->contentsToRootView(eventPos);
    auto dragLocationInRootViewCoordinates = frame.view()->contentsToRootView(dragLoc);
    item.eventPositionInContentCoordinates = viewProtector->rootViewToContents(eventPositionInRootViewCoordinates);
    item.dragLocationInContentCoordinates = viewProtector->rootViewToContents(dragLocationInRootViewCoordinates);
    item.dragLocationInWindowCoordinates = viewProtector->contentsToWindow(item.dragLocationInContentCoordinates);
    if (auto element = state.source) {
        auto dataTransferImageElement = state.dataTransfer->dragImageElement();
        if (state.type == DragSourceActionDHTML) {
            // If the drag image has been customized, fall back to positioning the preview relative to the drag event location.
            IntSize dragPreviewSize;
            if (dataTransferImageElement)
                dragPreviewSize = dataTransferImageElement->boundsInRootViewSpace().size();
            else {
                dragPreviewSize = dragImageSize(item.image.get());
                if (auto* page = frame.page())
                    dragPreviewSize.scale(1 / page->deviceScaleFactor());
            }
            item.dragPreviewFrameInRootViewCoordinates = { dragLocationInRootViewCoordinates, WTFMove(dragPreviewSize) };
        } else {
            // We can position the preview using the bounds of the drag source element.
            item.dragPreviewFrameInRootViewCoordinates = element->boundsInRootViewSpace();
        }

        RefPtr<Element> link;
        if (element->isLink())
            link = element;
        else {
            for (auto& currentElement : elementLineage(element.get())) {
                if (currentElement.isLink()) {
                    link = &currentElement;
                    break;
                }
            }
        }
        if (link) {
            auto titleAttribute = link->attributeWithoutSynchronization(HTMLNames::titleAttr);
            item.title = titleAttribute.isEmpty() ? link->innerText() : titleAttribute.string();
            item.url = frame.document()->completeURL(stripLeadingAndTrailingHTMLSpaces(link->getAttribute(HTMLNames::hrefAttr)));
        }
    }
    m_client.startDrag(WTFMove(item), *state.dataTransfer, frameProtector.get());
    // DragClient::startDrag can cause our Page to dispear, deallocating |this|.
    if (!frameProtector->page())
        return;

    cleanupAfterSystemDrag();
}

// Manual drag caret manipulation
void DragController::placeDragCaret(const IntPoint& windowPoint)
{
    mouseMovedIntoDocument(m_page.mainFrame().documentAtPoint(windowPoint));
    if (!m_documentUnderMouse)
        return;
    Frame* frame = m_documentUnderMouse->frame();
    FrameView* frameView = frame->view();
    if (!frameView)
        return;
    IntPoint framePoint = frameView->windowToContents(windowPoint);

    m_page.dragCaretController().setCaretPosition(frame->visiblePositionForPoint(framePoint));
}

bool DragController::shouldUseCachedImageForDragImage(const Image& image) const
{
#if ENABLE(DATA_INTERACTION)
    UNUSED_PARAM(image);
    return true;
#else
    return image.size().height() * image.size().width() <= MaxOriginalImageArea;
#endif
}

#if !PLATFORM(COCOA)

String DragController::platformContentTypeForBlobType(const String& type) const
{
    return type;
}

#endif

#if ENABLE(ATTACHMENT_ELEMENT)

PromisedAttachmentInfo DragController::promisedAttachmentInfo(Frame& frame, Element& element)
{
    auto* client = frame.editor().client();
    if (!client || !client->supportsClientSideAttachmentData())
        return { };

    RefPtr<HTMLAttachmentElement> attachment;
    if (is<HTMLAttachmentElement>(element))
        attachment = &downcast<HTMLAttachmentElement>(element);
    else if (is<HTMLImageElement>(element))
        attachment = downcast<HTMLImageElement>(element).attachmentElement();

    if (!attachment)
        return { };

    Vector<String> additionalTypes;
    Vector<RefPtr<SharedBuffer>> additionalData;
#if PLATFORM(COCOA)
    frame.editor().getPasteboardTypesAndDataForAttachment(element, additionalTypes, additionalData);
#endif

    if (auto* file = attachment->file())
        return { file->url(), platformContentTypeForBlobType(file->type()), file->name(), { }, WTFMove(additionalTypes), WTFMove(additionalData) };

    return { { }, { }, { }, attachment->uniqueIdentifier(), WTFMove(additionalTypes), WTFMove(additionalData) };
}

#endif // ENABLE(ATTACHMENT_ELEMENT)

#endif // ENABLE(DRAG_SUPPORT)

} // namespace WebCore
