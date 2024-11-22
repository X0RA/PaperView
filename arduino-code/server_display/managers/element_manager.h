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

    // Add new task handle
    TaskHandle_t touchResponseTaskHandle;

    // Add struct to pass data to task
    struct TouchData {
        DrawElement *element;
        uint8_t *framebuffer;
    };

    // Add static wrapper for the task
    static void touchResponseTaskWrapper(void *parameter) {
        TouchData *data = (TouchData *)parameter;
        data->element->setTouched(true);
        data->element->executeCallback(data->framebuffer);
        delete data;
        vTaskDelete(NULL);
    }

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

    /**
     * @brief Process the elements from the JSON
     * @param jsonElements The JSON array of elements
     * @return Whether the display needs to be refreshed
     */
    bool processElements(JsonArray &jsonElements) {
        LOG_I("Processing %d elements", jsonElements.size());

        // Reset updated flag for all elements at start of update cycle
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                elements[i]->setActive(false);
                elements[i]->setUpdated(false);
            }
        }

        bool refreshDisplay = false;

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
                refreshDisplay |= updateElement(newElement);
            } else {
                LOG_E("Element from JSON was not updated: %s", type);
                delete newElement;
            }
        }

        // Remove unused elements
        removeUnusedElements();

        // Redraw all active elements
        LOG_D("Redrawing all active elements");
        redrawElements();
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
            elements[index]->clearArea(framebuffer);
            delete elements[index];
            elements[index] = nullptr;
            elementCount--;
        }
    }

    void removeUnusedElements() {
        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i] && !elements[i]->isActive()) {
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

    // NOTE: returned bool will refresh display if true
    void clearElements() {

        for (size_t i = 0; i < MAX_ELEMENTS; i++) {
            if (elements[i]) {
                delete elements[i];
                elements[i] = nullptr;
            }
        }
        elementCount = 0;
    }

    /**
     * @brief Update an element (checks )
     * @param newElement The new element to update
     * @return Boolean indicating whether the element was updated
     */
    bool updateElement(DrawElement *newElement) {
        // Find existing element with same ID
        DrawElement *existing = findElementById(newElement->getId());

        if (existing) {
            // Check if the existing element needs to be updated
            if (!existing->isEqual(*newElement)) {
                LOG_D("Updating element %d", existing->getId());

                // Find index of existing element and update it
                for (size_t i = 0; i < MAX_ELEMENTS; i++) {
                    if (elements[i]->getId() == existing->getId()) {
                        // Clear the old element from the display
                        existing->clearArea(framebuffer, true);
                        delete existing;

                        // Replace with new element
                        elements[i] = newElement;
                        newElement->setActive(true);
                        newElement->setUpdated(true);
                        newElement->setChanged(true);

                        // Calculate properties
                        newElement->calculateProperties();

                        // Return true to trigger display refresh
                        return true;
                    }
                }
                // element is equal
            } else {
                existing->setActive(true);
                existing->setUpdated(true);
                existing->setChanged(false);
                delete newElement;
            }
        } else {
            // This is a new element, store it
            newElement->setActive(true);
            newElement->setUpdated(true);
            newElement->setChanged(true);
            storeElement(newElement);
        }

        // Return false if no refresh needed
        return false;
    }

public:
    size_t getElementCount() const {
        return elementCount;
    }

    bool isFull() const {
        return elementCount >= MAX_ELEMENTS;
    }

    /**
     * @brief Get an element by index
     * @param index The index of the element
     * @return The element at the index or nullptr if the index is out of bounds
     */
    DrawElement *getElement(size_t index) {
        if (index < MAX_ELEMENTS) {
            return elements[index];
        }
        return nullptr;
    }

    /**
     * @brief Handle touch events
     * @param x The x coordinate of the touch
     * @param y The y coordinate of the touch
     * @return Whether a touch was detected
     */
    bool handleTouch(int16_t x, int16_t y) {
        for (int i = MAX_ELEMENTS - 1; i >= 0; i--) {
            if (elements[i] && elements[i]->isPointInside(x, y)) {
                Serial.printf("Button %d touched\n", elements[i]->getId());

                // Create data structure for task
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
};

#endif // ELEMENT_MANAGER_H