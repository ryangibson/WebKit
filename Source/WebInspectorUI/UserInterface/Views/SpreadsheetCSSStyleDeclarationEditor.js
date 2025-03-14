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

WI.SpreadsheetCSSStyleDeclarationEditor = class SpreadsheetCSSStyleDeclarationEditor extends WI.View
{
    constructor(delegate, style)
    {
        super();

        this.element.classList.add(WI.SpreadsheetCSSStyleDeclarationEditor.StyleClassName);

        this._delegate = delegate;
        this.style = style;
        this._propertyViews = [];

        this._focused = false;
        this._inlineSwatchActive = false;

        this._showsImplicitProperties = false;
        this._alwaysShowPropertyNames = new Set;
        this._propertyVisibilityMode = WI.SpreadsheetCSSStyleDeclarationEditor.PropertyVisibilityMode.ShowAll;
        this._hideFilterNonMatchingProperties = false;
        this._sortPropertiesByName = false;

        this._propertyPendingStartEditing = null;
        this._pendingAddBlankPropertyIndexOffset = NaN;
        this._filterText = null;

        this._anchorIndex = NaN;
        this._focusIndex = NaN;
    }

    // Public

    initialLayout()
    {
        if (!this._style || !this._style.editable)
            return;

        this.element.addEventListener("focus", () => { this.focused = true; }, true);
        this.element.addEventListener("blur", (event) => {
            let focusedElement = event.relatedTarget;
            if (focusedElement && this.element.contains(focusedElement))
                return;

            this.focused = false;
        }, true);

        this.element.addEventListener("keydown", this._handleKeyDown.bind(this));
    }

    layout()
    {
        // Prevent layout of properties when one of them is being edited. A full layout resets
        // the focus, text selection, and completion state <http://webkit.org/b/182619>.
        if (this.editing && !this._propertyPendingStartEditing && isNaN(this._pendingAddBlankPropertyIndexOffset))
            return;

        super.layout();

        this.element.removeChildren();

        let properties = this.propertiesToRender;
        this.element.classList.toggle("no-properties", !properties.length);

        // FIXME: Only re-layout properties that have been modified and preserve focus whenever possible.
        this._propertyViews = [];

        let propertyViewPendingStartEditing = null;
        for (let index = 0; index < properties.length; index++) {
            let property = properties[index];
            let propertyView = new WI.SpreadsheetStyleProperty(this, property);
            propertyView.index = index;
            this.element.append(propertyView.element);
            this._propertyViews.push(propertyView);

            if (property === this._propertyPendingStartEditing)
                propertyViewPendingStartEditing = propertyView;
        }

        if (propertyViewPendingStartEditing) {
            propertyViewPendingStartEditing.nameTextField.startEditing();
            this._propertyPendingStartEditing = null;
        }

        if (this._filterText)
            this.applyFilter(this._filterText);

        if (!isNaN(this._pendingAddBlankPropertyIndexOffset))
            this.addBlankProperty(this._propertyViews.length - 1 - this._pendingAddBlankPropertyIndexOffset);
    }

    detached()
    {
        this._inlineSwatchActive = false;
        this.focused = false;

        for (let propertyView of this._propertyViews)
            propertyView.detached();
    }

    hidden()
    {
        for (let propertyView of this._propertyViews)
            propertyView.hidden();
    }

    get style()
    {
        return this._style;
    }

    set style(style)
    {
        if (this._style === style)
            return;

        if (this._style)
            this._style.removeEventListener(WI.CSSStyleDeclaration.Event.PropertiesChanged, this._propertiesChanged, this);

        this._style = style || null;

        if (this._style)
            this._style.addEventListener(WI.CSSStyleDeclaration.Event.PropertiesChanged, this._propertiesChanged, this);

        this.needsLayout();
    }

    get editing()
    {
        return this._focused || this._inlineSwatchActive;
    }

    set focused(value)
    {
        this._focused = value;
        this._updateStyleLock();
    }

    set inlineSwatchActive(value)
    {
        this._inlineSwatchActive = value;
        this._updateStyleLock();
    }

    set showsImplicitProperties(value)
    {
        if (value === this._showsImplicitProperties)
            return;

        this._showsImplicitProperties = value;

        this.needsLayout();
    }

    set alwaysShowPropertyNames(propertyNames)
    {
        this._alwaysShowPropertyNames = new Set(propertyNames);

        this.needsLayout();
    }

    set propertyVisibilityMode(propertyVisibilityMode)
    {
        if (this._propertyVisibilityMode === propertyVisibilityMode)
            return;

        this._propertyVisibilityMode = propertyVisibilityMode;

        this.needsLayout();
    }

    set hideFilterNonMatchingProperties(value)
    {
        if (value === this._hideFilterNonMatchingProperties)
            return;

        this._hideFilterNonMatchingProperties = value;

        this.needsLayout();
    }

    set sortPropertiesByName(value)
    {
        if (value === this._sortPropertiesByName)
            return;

        this._sortPropertiesByName = value;
        this.needsLayout();
    }

    get propertiesToRender()
    {
        let properties = [];
        if (!this._style)
            return properties;

        if (this._style._styleSheetTextRange)
            properties = this._style.allVisibleProperties;
        else
            properties = this._style.allProperties;

        if (this._sortPropertiesByName)
            properties.sort((a, b) => a.name.extendedLocaleCompare(b.name));

        return properties.filter((property) => {
            if (!property.variable && this._propertyVisibilityMode === WI.SpreadsheetCSSStyleDeclarationEditor.PropertyVisibilityMode.HideNonVariables)
                return false;

            if (property.variable && this._propertyVisibilityMode === WI.SpreadsheetCSSStyleDeclarationEditor.PropertyVisibilityMode.HideVariables)
                return false;

            return !property.implicit || this._showsImplicitProperties || this._alwaysShowPropertyNames.has(property.canonicalName);
        });
    }

    get selectionRange()
    {
        let startIndex = Math.min(this._anchorIndex, this._focusIndex);
        let endIndex = Math.max(this._anchorIndex, this._focusIndex);
        return [startIndex, endIndex];
    }

    startEditingFirstProperty()
    {
        let firstEditableProperty = this._editablePropertyAfter(-1);
        if (firstEditableProperty)
            firstEditableProperty.nameTextField.startEditing();
        else {
            const appendAfterLast = -1;
            this.addBlankProperty(appendAfterLast);
        }
    }

    startEditingLastProperty()
    {
        let lastEditableProperty = this._editablePropertyBefore(this._propertyViews.length);
        if (lastEditableProperty)
            lastEditableProperty.valueTextField.startEditing();
        else {
            const appendAfterLast = -1;
            this.addBlankProperty(appendAfterLast);
        }
    }

    highlightProperty(property)
    {
        let propertiesMatch = (cssProperty) => {
            if (cssProperty.attached && !cssProperty.overridden) {
                if (cssProperty.canonicalName === property.canonicalName || hasMatchingLonghandProperty(cssProperty))
                    return true;
            }

            return false;
        };

        let hasMatchingLonghandProperty = (cssProperty) => {
            let cssProperties = cssProperty.relatedLonghandProperties;

            if (!cssProperties.length)
                return false;

            for (let property of cssProperties) {
                if (propertiesMatch(property))
                    return true;
            }

            return false;
        };

        for (let cssProperty of this._style.properties) {
            if (propertiesMatch(cssProperty)) {
                let propertyView = cssProperty.__propertyView;
                if (propertyView) {
                    propertyView.highlight();

                    if (cssProperty.editable)
                        propertyView.valueTextField.startEditing();
                }
                return true;
            }
        }

        return false;
    }

    addBlankProperty(index)
    {
        this._pendingAddBlankPropertyIndexOffset = NaN;

        if (index === -1) {
            // Append to the end.
            index = this._propertyViews.length;
        }

        this._propertyPendingStartEditing = this._style.newBlankProperty(index);
        this.needsLayout();
    }

    hasSelectedProperties()
    {
        return !isNaN(this._anchorIndex) && !isNaN(this._focusIndex);
    }

    selectProperties(anchorIndex, focusIndex)
    {
        console.assert(anchorIndex < this._propertyViews.length, `anchorIndex (${anchorIndex}) is greater than the last property index (${this._propertyViews.length})`);
        console.assert(focusIndex < this._propertyViews.length, `focusIndex (${focusIndex}) is greater than the last property index (${this._propertyViews.length})`);

        if (isNaN(anchorIndex) || isNaN(focusIndex)) {
            console.error(`Nothing to select. anchorIndex (${anchorIndex}) and focusIndex (${focusIndex}) must be numbers.`);
            this.deselectProperties();
            return;
        }

        this._anchorIndex = anchorIndex;
        this._focusIndex = focusIndex;

        let [startIndex, endIndex] = this.selectionRange;

        for (let i = 0; i < this._propertyViews.length; ++i) {
            let propertyView = this._propertyViews[i];
            let isSelected = i >= startIndex && i <= endIndex;
            propertyView.selected = isSelected;
        }

        this._suppressBlur = true;
        let property = this._propertyViews[focusIndex];
        property.element.focus();
        this._suppressBlur = false;
    }

    extendSelectedProperties(focusIndex)
    {
        this.selectProperties(this._anchorIndex, focusIndex);
    }

    deselectProperties()
    {
        for (let propertyView  of this._propertyViews)
            propertyView.selected = false;

        this._focused = false;
        this._anchorIndex = NaN;
        this._focusIndex = NaN;
    }

    applyFilter(filterText)
    {
        this._filterText = filterText;

        if (!this.didInitialLayout)
            return;

        let matches = false;
        for (let propertyView of this._propertyViews) {
            if (propertyView.applyFilter(this._filterText)) {
                matches = true;

                if (this._hideFilterNonMatchingProperties)
                    this.element.append(propertyView.element);
            } else if (this._hideFilterNonMatchingProperties)
                propertyView.element.remove();
        }

        this.dispatchEventToListeners(WI.SpreadsheetCSSStyleDeclarationEditor.Event.FilterApplied, {matches});
    }

    // SpreadsheetStyleProperty delegate

    spreadsheetStylePropertyBlur(event, property)
    {
        if (this._suppressBlur)
            return;

        this._delegate.spreadsheetCSSStyleDeclarationEditorPropertyBlur(event, property);
    }

    spreadsheetStylePropertyMouseEnter(event, property)
    {
        this._delegate.spreadsheetCSSStyleDeclarationEditorPropertyMouseEnter(event, property);
    }

    spreadsheetStylePropertyFocusMoved(propertyView, {direction, willRemoveProperty})
    {
        this._updatePropertiesStatus();

        if (!direction)
            return;

        let movedFromIndex = this._propertyViews.indexOf(propertyView);
        console.assert(movedFromIndex !== -1, "Property doesn't exist, focusing on a selector as a fallback.");
        if (movedFromIndex === -1) {
            if (this._style.selectorEditable)
                this._delegate.spreadsheetCSSStyleDeclarationEditorStartEditingRuleSelector();

            return;
        }

        if (direction === "forward") {
            // Move from the value to the next enabled property's name.
            let propertyView = this._editablePropertyAfter(movedFromIndex);
            if (propertyView)
                propertyView.nameTextField.startEditing();
            else {
                if (willRemoveProperty) {
                    const delta = 1;
                    this._delegate.spreadsheetCSSStyleDeclarationEditorStartEditingAdjacentRule(this, delta);
                } else {
                    const appendAfterLast = -1;
                    this.addBlankProperty(appendAfterLast);
                }
            }
        } else {
            let propertyView = this._editablePropertyBefore(movedFromIndex);
            if (propertyView) {
                // Move from the property's name to the previous enabled property's value.
                propertyView.valueTextField.startEditing();
            } else {
                // Move from the first property's name to the rule's selector.
                if (this._style.selectorEditable)
                    this._delegate.spreadsheetCSSStyleDeclarationEditorStartEditingRuleSelector();
                else {
                    const delta = -1;
                    this._delegate.spreadsheetCSSStyleDeclarationEditorStartEditingAdjacentRule(this, delta);
                }
            }
        }
    }

    spreadsheetStylePropertyAddBlankPropertySoon(propertyView, {index})
    {
        if (isNaN(index))
            index = this._propertyViews.length;
        this._pendingAddBlankPropertyIndexOffset = this._propertyViews.length - index;
    }

    spreadsheetStylePropertyCopy(event)
    {
        if (!this.hasSelectedProperties())
            return;

        let formattedProperties = [];
        let [startIndex, endIndex] = this.selectionRange;
        for (let i = startIndex; i <= endIndex; ++i) {
            let propertyView = this._propertyViews[i];
            formattedProperties.push(propertyView.property.formattedText);
        }

        event.clipboardData.setData("text/plain", formattedProperties.join("\n"));
        event.stop();
    }

    spreadsheetStylePropertyRemoved(propertyView)
    {
        this._propertyViews.remove(propertyView);

        for (let index = 0; index < this._propertyViews.length; index++)
            this._propertyViews[index].index = index;

        this._focused = false;
    }

    spreadsheetStylePropertyShowProperty(propertyView, property)
    {
        if (this._delegate.spreadsheetCSSStyleDeclarationEditorShowProperty)
            this._delegate.spreadsheetCSSStyleDeclarationEditorShowProperty(this, property);
    }

    stylePropertyInlineSwatchActivated()
    {
        this.inlineSwatchActive = true;
    }

    stylePropertyInlineSwatchDeactivated()
    {
        this.inlineSwatchActive = false;
    }

    // Private

    _handleKeyDown(event)
    {
        if (!this.hasSelectedProperties() || !this._propertyViews.length)
            return;

        if (event.key === "ArrowUp" || event.key === "ArrowDown") {
            let delta = event.key === "ArrowUp" ? -1 : 1;
            let focusIndex = Number.constrain(this._focusIndex + delta, 0, this._propertyViews.length - 1);

            if (event.shiftKey)
                this.extendSelectedProperties(focusIndex);
            else
                this.selectProperties(focusIndex, focusIndex);

            event.stop();
        } else if (event.key === "Tab" || event.key === "Enter") {
            let property = this._propertyViews[this._focusIndex];
            if (property && property.enabled) {
                event.stop();
                property.nameTextField.startEditing();
            }
        } else if (event.key === "Backspace") {
            let [startIndex, endIndex] = this.selectionRange;

            let propertyIndexToSelect = NaN;
            if (endIndex + 1 !== this._propertyViews.length)
                propertyIndexToSelect = startIndex;
            else if (startIndex > 0)
                propertyIndexToSelect = startIndex - 1;

            this.deselectProperties();

            for (let i = endIndex; i >= startIndex; i--)
                this._propertyViews[i].remove();

            if (!isNaN(propertyIndexToSelect))
                this.selectProperties(propertyIndexToSelect, propertyIndexToSelect);

            event.stop();

        } else if ((event.code === "Space" && !event.shiftKey && !event.metaKey && !event.ctrlKey) || (event.key === "/" && event.commandOrControlKey && !event.shiftKey)) {
            let [startIndex, endIndex] = this.selectionRange;

            // Toggle the first selected property and set this state to all selected properties.
            let disabled = this._propertyViews[startIndex].property.enabled;

            for (let i = endIndex; i >= startIndex; --i) {
                let propertyView = this._propertyViews[i];
                propertyView.property.commentOut(disabled);
                propertyView.update();
            }

            event.stop();

        } else if (event.key === "a" && event.commandOrControlKey) {

            this.selectProperties(0, this._propertyViews.length - 1);
            event.stop();

        } else if (event.key === "Esc")
            this.deselectProperties();
    }

    _editablePropertyAfter(propertyIndex)
    {
        for (let index = propertyIndex + 1; index < this._propertyViews.length; index++) {
            let property = this._propertyViews[index];
            if (property.enabled)
                return property;
        }

        return null;
    }

    _editablePropertyBefore(propertyIndex)
    {
        for (let index = propertyIndex - 1; index >= 0; index--) {
            let property = this._propertyViews[index];
            if (property.enabled)
                return property;
        }

        return null;
    }

    _propertiesChanged(event)
    {
        if (this.editing && isNaN(this._pendingAddBlankPropertyIndexOffset))
            this._updatePropertiesStatus();
        else
            this.needsLayout();
    }

    _updatePropertiesStatus()
    {
        for (let propertyView of this._propertyViews)
            propertyView.updateStatus();
    }

    _updateStyleLock()
    {
        if (!this._style)
            return;

        this._style.locked = this._focused || this._inlineSwatchActive;
    }
};

WI.SpreadsheetCSSStyleDeclarationEditor.Event = {
    FilterApplied: "spreadsheet-css-style-declaration-editor-filter-applied",
};

WI.SpreadsheetCSSStyleDeclarationEditor.StyleClassName = "spreadsheet-style-declaration-editor";

WI.SpreadsheetCSSStyleDeclarationEditor.PropertyVisibilityMode = {
    ShowAll: Symbol("variable-visibility-show-all"),
    HideVariables: Symbol("variable-visibility-hide-variables"),
    HideNonVariables: Symbol("variable-visibility-hide-non-variables"),
};
