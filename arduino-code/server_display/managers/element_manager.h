#ifndef ELEMENT_MANAGER_H
#define ELEMENT_MANAGER_H

#include <vector>
#include "../config.h"
#include "../elements/button_element.h"
#include "../elements/element.h"
#include "../elements/image_element.h"
#include "../elements/text_element.h"

class ElementManager {
private:
    DrawElement *elements[MAX_ELEMENTS];
    size_t elementCount;
    uint8_t *framebuffer;

    struct ElementAction {
        DrawElement *element;
        bool needs_clear;
        bool needs_draw;
        RefreshType refresh_type;
    };

    std::vector<ElementAction> action_queue;

public:
    ElementManager(uint8_t *fb) : framebuffer(fb), elementCount(0) {
        memset(elements, 0, sizeof(elements));
    }

    ~ElementManager() {
        clearAllElements();
    }

    /**
     * @brief Process a JSON array of elements, updating the element manager's state
     * This function handles:
     * - Creating new elements from JSON
     * - Updating existing elements if they've changed
     * - Removing elements that are no longer present
     * - Queueing appropriate draw/clear actions for changed elements
     *
     * @param jsonElements JsonArray containing element definitions
     */
    void processElements(JsonArray &jsonElements) {
        if (!jsonElements.isNull()) {
            std::vector<uint16_t> processedIds;

            // Iterate through each element in the JSON array
            for (JsonObject elementJson : jsonElements) {
                if (elementJson.isNull())
                    continue;

                // Extract the element type and ID from JSON
                const char *typeStr = elementJson["type"] | "";
                uint16_t id = elementJson["id"] | 0;

                if (id == 0)
                    continue;
                processedIds.push_back(id);

                // Create a new element instance based on the type
                DrawElement *newElement = createElementFromType(typeStr);
                if (!newElement || !newElement->updateFromJson(elementJson)) {
                    if (newElement)
                        delete newElement;
                    continue;
                }

                // Check if an element with this ID already exists
                DrawElement *existingElement = findElementById(id);

                if (existingElement) {
                    // CASE: Element has changed
                    if (!existingElement->isEqual(*newElement)) {
                        existingElement->setRefreshType(ELEMENT_REFRESH_PARTIAL);
                        existingElement->clearArea(framebuffer);

                        // Get index before deleting
                        size_t index = getElementIndex(existingElement);

                        // Delete existing element
                        delete existingElement;

                        // Store new element in same slot
                        elements[index] = newElement;

                        // Queue draw of new element
                        queueAction(newElement, false, true, NO_REFRESH);
                    } else {
                        delete newElement;
                    }
                } else {
                    // Case: Element does not exist ( i.e. new element )
                    if (storeElement(newElement)) {
                        queueAction(newElement, false, true, NO_REFRESH);
                    } else {
                        delete newElement;
                    }
                }
            }

            // Clean up elements that weren't in the new JSON array
            for (size_t i = 0; i < MAX_ELEMENTS; i++) {
                if (!elements[i])
                    continue;

                // Check if this element's ID was processed
                bool elementFound = false;
                for (uint16_t id : processedIds) {
                    if (elements[i]->getId() == id) {
                        elementFound = true;
                        break;
                    }
                }

                // If element wasn't in new JSON, remove it
                if (!elementFound) {
                    elements[i]->setRefreshType(ELEMENT_REFRESH_COMPLETE);
                    elements[i]->clearArea(framebuffer);
                    elements[i] = nullptr;
                    elementCount--;
                }
            }
        }
    }

    bool handleTouch(int16_t x, int16_t y) {
        // Process elements from top to bottom (reverse order)
        for (int i = MAX_ELEMENTS - 1; i >= 0; i--) {
            if (!elements[i])
                continue;

            if (elements[i]->isPointInside(x, y)) {
                elements[i]->setTouched(true);
                elements[i]->executeCallback(framebuffer);
                return true;
            }
        }
        return false;
    }

    void loop() {
        if (action_queue.empty())
            return;

        bool action_performed = false;

        // Process all queued actions
        while (!action_queue.empty()) {
            ElementAction action = action_queue.front();
            if (action.element != nullptr) {
                if (action.needs_clear) {
                    action.element->setRefreshType(action.refresh_type);
                    action.element->clearArea(framebuffer);
                }

                if (action.needs_draw) {
                    action.element->draw(framebuffer);
                    action_performed = true;
                }
            }
            action_queue.erase(action_queue.begin());
        }
        if (action_performed && framebuffer != nullptr) {
            draw_framebuffer(framebuffer);
        }
    }

private:
    DrawElement *createElementFromType(const char *typeStr) {
        if (strcmp(typeStr, "text") == 0)
            return new TextElement();
        if (strcmp(typeStr, "button") == 0)
            return new ButtonElement();
        if (strcmp(typeStr, "image") == 0)
            return new ImageElement();
        return nullptr;
    }

    DrawElement *findElementById(uint16_t id) {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && elements[i]->getId() == id) {
                return elements[i];
            }
        }
        return nullptr;
    }

    size_t getElementIndex(DrawElement *element) {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] == element)
                return i;
        }
        return MAX_ELEMENTS;
    }

    bool storeElement(DrawElement *element) {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (!elements[i]) {
                elements[i] = element;
                elementCount++;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Queue an action to be processed later.
     * @param element The element to perform the action on.
     * @param needs_clear Whether to clear the element.
     * @param needs_draw Whether to draw the element.
     * @param refresh_type The refresh type to be set on the element.
     */
    void queueAction(DrawElement *element, bool needs_clear, bool needs_draw, RefreshType refresh_type) {
        action_queue.push_back({.element = element,
                                .needs_clear = needs_clear,
                                .needs_draw = needs_draw,
                                .refresh_type = refresh_type});
    }

    void clearAllElements() {
        // Process any remaining actions
        while (!action_queue.empty()) {
            action_queue.erase(action_queue.begin());
        }

        refresh_display(DISPLAY_REFRESH_PARTIAL, framebuffer);
        set_background(framebuffer);

        // Delete all elements
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                delete elements[i];
                elements[i] = nullptr;
            }
        }
        elementCount = 0;
    }
};

#endif // ELEMENT_MANAGER_H