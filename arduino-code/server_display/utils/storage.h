#ifndef UTILS_STORAGE_H
#define UTILS_STORAGE_H

#include "../config.h"
#include "utilities.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

bool cardMounted = false;

void setupSD() {
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 4000000)) {
        LOG_E("Card Mount Failed");
        cardMounted = false;
        return;
    }
    LOG_I("Card mounted");
    LOG_D("Card size: %lluMB", SD.cardSize() / (1024 * 1024));
    cardMounted = true;
}

bool isCardMounted() {
    if (!cardMounted) {
        LOG_E("Card not mounted");
    }
    return cardMounted;
}

#pragma region Given file System Methods
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
    if (!cardMounted) {
        return;
    }
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path) {
    if (!cardMounted) {
        return;
    }
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %lu ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
    file.close();
}

void testSD() {
    if (!cardMounted) {
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    // testFileIO(SD, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}
#pragma endregion

/**
 * Saves a 4-bit grayscale image to the SD card with the following format:
 * - Header: 8 bytes (4 bytes width + 4 bytes height)
 * - Data: Raw 4-bit grayscale pixel data (packed 2 pixels per byte)
 *
 * @param filename Path relative to IMAGE_SD_PATH (e.g., "icons/weather/sun.bin")
 * @param img_buffer Raw 4-bit grayscale image data (2 pixels per byte)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return true if save successful, false otherwise
 */
bool saveImageBufferToSD(String filename, uint8_t *img_buffer, uint32_t width, uint32_t height) {
    if (!cardMounted)
        return false;

    // Make sure the images directory exists
    if (!SD.exists(IMAGE_SD_PATH)) {
        if (!SD.mkdir(IMAGE_SD_PATH)) {
            LOG_E("Failed to create images directory");
            return false;
        }
    }

    // If filename contains subdirectories (e.g. "folder/image.bin"), create the subdirectory structure
    // Example: For filename "icons/weather/sun.bin", creates "/images/icons/weather/" if it doesn't exist
    if (filename.indexOf("/") != -1) {
        String full_path = String(IMAGE_SD_PATH) + "/" + filename.substring(0, filename.lastIndexOf("/"));
        if (!SD.exists(full_path)) {
            if (!SD.mkdir(full_path)) {
                LOG_E("Failed to create subdirectory");
                return false;
            }
        }
    }

    String file_path = String(IMAGE_SD_PATH) + String("/") + filename;
    LOG_I("Saving image to SD card: %s", file_path.c_str());
    File file = SD.open(file_path, FILE_WRITE);

    if (!file) {
        LOG_E("Failed to open file for writing");
        return false;
    }

    size_t bytes_per_row = width / 2;
    size_t image_data_size = bytes_per_row * height;

    // Write dimensions
    if (file.write((uint8_t *)&width, 4) != 4) {
        LOG_E("Failed to write width");
        file.close();
        return false;
    }

    if (file.write((uint8_t *)&height, 4) != 4) {
        LOG_E("Failed to write height");
        file.close();
        return false;
    }

    // Write image data
    if (file.write(img_buffer, image_data_size) != image_data_size) {
        LOG_E("Failed to write image data");
        file.close();
        return false;
    }

    file.close();
    LOG_D("Successfully saved image to SD card");
    return true;
}

/**
 * Reads a 4-bit grayscale image from the SD card with the following format:
 * - Header: 8 bytes (4 bytes width + 4 bytes height)
 * - Data: Raw 4-bit grayscale pixel data (packed 2 pixels per byte)
 *
 * @param filename Path relative to IMAGE_SD_PATH (e.g., "icons/weather/sun.bin")
 * @param img_buffer Raw 4-bit grayscale image data (2 pixels per byte)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return true if read successful, false otherwise
 */
bool readImageBufferFromSD(String filename, uint8_t *img_buffer, uint32_t &width, uint32_t &height) {
    String file_path = String(IMAGE_SD_PATH) + String("/") + filename;
    LOG_D("Checking if image exists on SD card: %s", file_path.c_str());
    if (!SD.exists(file_path)) {
        LOG_D("Image not found on SD card: %s", file_path.c_str());
        return false;
    }

    File file = SD.open(file_path, FILE_READ);
    if (!file) {
        LOG_E("Failed to open image file");
        return false;
    }

    // Read dimensions
    if (file.read((uint8_t *)&width, 4) != 4 ||
        file.read((uint8_t *)&height, 4) != 4) {
        file.close();
        LOG_E("Failed to read image dimensions");
        return false;
    }

    // Read image data
    size_t bytes_per_row = width / 2;
    size_t image_data_size = bytes_per_row * height;
    if (file.read(img_buffer, image_data_size) != image_data_size) {
        file.close();
        LOG_E("Failed to read image data");
        return false;
    }

    file.close();
    LOG_D("Read image from SD card");
    return true;
}

#endif // UTILS_STORAGE_H