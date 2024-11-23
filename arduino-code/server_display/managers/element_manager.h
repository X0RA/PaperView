#ifndef ELEMENT_MANAGER_H
#define ELEMENT_MANAGER_H

#include <vector>
#include "../config.h"

// Elements
#include "../elements/button_element.h"
#include "../elements/element.h"
#include "../elements/image_element.h"
#include "../elements/text_element.h"

#include "../config.h"

class ElementManager {
private:
    DrawElement *elements[MAX_ELEMENTS];
    size_t elementCount;
    uint8_t *framebuffer;

    TaskHandle_t touchResponseTaskHandle;

    struct TouchData {
        DrawElement *element;
        uint8_t *framebuffer;
    };

    static void touchResponseTaskWrapper(void *parameter) {
        TouchData *data = (TouchData *)parameter;
        data->element->setTouched(true);
        data->element->executeCallback(data->framebuffer);
        delete data;
        vTaskDelete(NULL);
    }

    struct ElementUpdate {
        DrawElement *element;
        RefreshType refresh_type;
        bool needs_clear;
        bool needs_draw;
    };

    std::vector<ElementUpdate> update_queue;

public:
    ElementManager(uint8_t *fb) {
        elementCount = 0;
        framebuffer = fb;
        memset(elements, 0, sizeof(elements));
    }

    ~ElementManager() {
        clearElements();
    }

    /**
     * @brief Render the touched elements to the framebuffer
     * @return Whether any elements were touched
     */
    bool renderTouched() {
        bool elementsTouched = false;
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && elements[i]->isTouched()) {
                elements[i]->drawTouched(framebuffer);
                elements[i]->setTouched(false);
                elementsTouched = true;
            }
        }
        return elementsTouched;
    }

    bool processElements(JsonArray &jsonElements) {
        LOG_I("Processing %d elements", jsonElements.size());

        bool elementsChanged = false;

        // Mark all existing elements as not updated
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                elements[i]->setUpdated(false);
            }
        }

        // Process new elements
        for (JsonObject element : jsonElements) {
            const char *type = element["type"] | "";
            DrawElement *newElement = nullptr;

            LOG_D("Processing element of type: %s", type);

            if (strcmp(type, "text") == 0) {
                newElement = new TextElement();
            } else if (strcmp(type, "button") == 0) {
                newElement = new ButtonElement();
            } else if (strcmp(type, "image") == 0) {
                newElement = new ImageElement();
            }

            if (newElement && newElement->updateFromJson(element)) {
                elementsChanged |= updateElement(newElement);
            } else {
                LOG_E("Failed to create or update element of type: %s", type);
                delete newElement;
            }
        }

        // Remove elements that weren't updated
        removeUnusedElements();

        // Redraw all active elements if there were changes
        if (elementsChanged) {
            LOG_D("Redrawing all active elements");
            redrawElements();
        }

        return elementsChanged;
    }

private:
    DrawElement *findElementById(uint16_t id) {
        if (elementCount == 0) {
            return nullptr;
        }

        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && elements[i]->getId() == id) {
                return elements[i];
            }
        }
        return nullptr;
    }

    bool storeElement(DrawElement *element) {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (!elements[i]) {
                elements[i] = element;
                elementCount++;
                return true;
            }
        }

        LOG_E("No free slots for element storage!");
        delete element;
        return false;
    }

    void removeElement(size_t index) {
        if (elements[index]) {
            queueElementUpdate(elements[index], ELEMENT_REFRESH_PARTIAL, true, false);
            delete elements[index];
            elements[index] = nullptr;
            elementCount--;
        }
    }

    void removeUnusedElements() {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && !elements[i]->isUpdated()) {
                removeElement(i);
            }
        }
    }

    void redrawElements() {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                elements[i]->draw(framebuffer);
            }
        }
    }

    void clearElements() { // NOTE: returned bool will refresh display if true

        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                delete elements[i];
                elements[i] = nullptr;
            }
        }
        elementCount = 0;
    }

    /**
     * @brief Update an element if it exists and is different
     * @return Whether the element was updated
     */
    bool updateElement(DrawElement *newElement) {
        DrawElement *existing = findElementById(newElement->getId());

        if (existing) {
            if (!existing->isEqual(*newElement)) {
                LOG_D("Updating element %d", existing->getId());
                for (size_t i = 0; i < MAX_ELEMENTS; i++) {
                    if (elements[i]->getId() == existing->getId()) {
                        // Queue the clear operation with appropriate refresh type
                        queueElementUpdate(existing, ELEMENT_REFRESH_PARTIAL, true, false);

                        // Replace the element
                        delete existing;
                        elements[i] = newElement;

                        // Queue the draw operation
                        queueElementUpdate(newElement, NO_REFRESH, false, true);
                        return true;
                    }
                }
            } else {
                existing->setUpdated(true);
                delete newElement;
                return false;
            }
        } else {
            if (storeElement(newElement)) {
                // Queue new element draw
                queueElementUpdate(newElement, ELEMENT_REFRESH_FAST, false, true);
                return true;
            }
        }
        return false;
    }

    void queueElementUpdate(DrawElement *element, RefreshType refresh_type, bool needs_clear = true, bool needs_draw = true) {
        update_queue.push_back({.element = element,
                                .refresh_type = refresh_type,
                                .needs_clear = needs_clear,
                                .needs_draw = needs_draw});
    }

public:
    size_t getElementCount() const {
        return elementCount;
    }

    bool isFull() const {
        return elementCount >= MAX_ELEMENTS;
    }

    DrawElement *getElement(size_t index) {
        if (index < MAX_ELEMENTS) {
            return elements[index];
        }
        return nullptr;
    }

    // Method to handle touch events
    bool handleTouch(int16_t x, int16_t y) {
        for (int i = MAX_ELEMENTS - 1; i >= 0; i--) {
            if (elements[i] && elements[i]->isPointInside(x, y)) {
                Serial.printf("Button %d touched\n", elements[i]->getId());

                TouchData *data = new TouchData{elements[i], framebuffer};

                // Create new task for handling touch response and return immediately
                BaseType_t taskCreated = xTaskCreate(
                    touchResponseTaskWrapper,
                    "TouchResponse",
                    4096,
                    data,
                    1,
                    &touchResponseTaskHandle);

                LOG_D("Touch response task %s", taskCreated == pdPASS ? "created successfully" : "failed to create");

                // Return true regardless of whether task creation succeeded
                // This ensures the display refresh happens immediately
                return true;
            }
        }
        return false;
    }

    void loop() {
        // Process any queued element updates
        if (!update_queue.empty()) {
            auto update = update_queue.front();
            update_queue.erase(update_queue.begin());

            if (update.needs_clear) {
                update.element->setRefreshType(update.refresh_type);
                update.element->clearArea(framebuffer);
            }

            if (update.needs_draw) {
                update.element->draw(framebuffer);
            }
        }

        // Process any touched elements
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && elements[i]->isTouched()) {
                elements[i]->drawTouched(framebuffer);
                elements[i]->setTouched(false);
            }
        }
    }
};

#endif // ELEMENT_MANAGER_H