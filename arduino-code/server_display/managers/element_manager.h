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

    void processElements(JsonArray &jsonElements) {
        LOG_I("Processing %d elements", jsonElements.size());

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
                updateElement(newElement);
            } else {
                LOG_E("Failed to create or update element of type: %s", type);
                delete newElement;
            }
        }

        // Remove elements that weren't updated
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

    void updateElement(DrawElement *newElement) {
        // Find existing element with same ID
        DrawElement *existing = findElementById(newElement->getId());

        if (existing) {
            if (!existing->isEqual(*newElement)) {
                LOG_D("Updating element %d", existing->getId());
                // Find index of existing element
                for (size_t i = 0; i < MAX_ELEMENTS; i++) {
                    if (elements[i]->getId() == existing->getId()) {
                        existing->clearArea(framebuffer);
                        delete existing;
                        elements[i] = newElement;
                        newElement->draw(framebuffer); // Need to update this to just calculate the bounds as to not redraw on the screen which can cause issues
                        return;
                    }
                }
            } else {
                existing->setUpdated(true);
                delete newElement;
            }
        } else {
            storeElement(newElement);
        }
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