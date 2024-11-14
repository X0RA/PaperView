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

public:
    ElementManager(uint8_t *fb) {
        elementCount = 0;
        framebuffer = fb;
        memset(elements, 0, sizeof(elements));
    }

    ~ElementManager() {
        clearElements();
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

    void clearElements() {
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

    // Method to handle touch events if needed
    bool handleTouch(int16_t x, int16_t y) {
        // Iterate through elements in reverse order (top to bottom)
        for (int i = MAX_ELEMENTS - 1; i >= 0; i--) {
            if (elements[i]) {
                if (elements[i]->getType() == ElementType::BUTTON) {
                    ButtonElement *button = static_cast<ButtonElement *>(elements[i]);
                    if (button && button->isPointInside(x, y)) {
                        epd_poweron();
                        Serial.printf("Button %d touched\n", button->getId());

                        // Show touch state
                        // TODO: Make this change color or some shit
                        button->setTouched(true);
                        button->draw(framebuffer);
                        epd_draw_grayscale_image(epd_full_screen(), framebuffer);

                        // Execute callback and check if refresh needed
                        bool refresh = button->executeCallback(framebuffer);

                        // Reset touch state
                        button->setTouched(false);
                        button->draw(framebuffer);
                        epd_draw_grayscale_image(epd_full_screen(), framebuffer);

                        epd_poweroff();
                        return refresh;
                    }
                }
            }
        }
        return false;
    }
};

#endif // ELEMENT_MANAGER_H