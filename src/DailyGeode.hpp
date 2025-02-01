#ifndef DAILY_GEODE_POST_CREATOR_H
#define DAILY_GEODE_POST_CREATOR_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <Geode/Geode.hpp>
#include <Geode/loader/Loader.hpp>

using namespace geode::prelude;

struct PostConfig {
    uint32_t width = 2560;
    float stretch = 1.5;
    uint32_t padding = 50;
    uint32_t extraPadding = 50;
    uint32_t logoPadding = 30;
    uint32_t logoExtra = 10;
    uint32_t linePadding = 36;
    uint32_t lineThickness = 20;
    std::filesystem::path headerFont = std::filesystem::path(Mod::get()->getResourcesDir() / "FuturaNowHeadlineBold.ttf");
    float headerFontSize = 48.0;
    std::filesystem::path captionFont = std::filesystem::path(Mod::get()->getResourcesDir() / "PrimaSerifBold.otf");
    float captionFontSize = 80.0;
    std::string brand = "The Daily Geode";
    uint8_t day;
    uint8_t month;
    uint16_t year;
    std::string headerColor = "078c51";
    std::filesystem::path logo = std::filesystem::path(Mod::get()->getResourcesDir() / "GeodeLogo.png");
    std::optional<std::string> link;
    std::optional<std::filesystem::path> image;
    std::string caption;
    std::filesystem::path output = "./output";
};

class DailyGeodePostCreator {
public:
    // Helper function to load an image using STB
    static std::pair<std::vector<unsigned char>, std::vector<unsigned char>> loadImages(
        const std::optional<std::filesystem::path>& imagePath,
        const std::optional<std::string>& link,
        const std::filesystem::path& logoPath)
    {
        std::vector<unsigned char> image, logo;

        if (imagePath) {
            int width, height, channels;
            image = loadImage(imagePath->string(), width, height, channels);
        } else if (link) {
            log::warn("Loading images from URLs is not implemented.");
        }

        // Declare temporary variables for logo loading.
        int width = 0, height = 0, channels = 0;
        logo = loadImage(logoPath.string(), width, height, channels);

        return {image, logo};
    }

    // Helper function to load an image with STB.
    static std::vector<unsigned char> loadImage(const std::string& path, int& width, int& height, int& channels) {
        std::vector<unsigned char> data;
        unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (image) {
            data.assign(image, image + (width * height * channels));
            stbi_image_free(image);
        } else {
            log::error("Failed to load image from {}", path);
        }

        return data;
    }

    static std::string createFormattedDate(uint8_t day, uint8_t month, uint16_t year) {
        std::ostringstream dateStream;
        dateStream << (month < 10 ? "0" : "") << static_cast<int>(month) << "/"
                   << static_cast<int>(day) << "/" << year;
        return dateStream.str();
    }

    static std::vector<std::string> calculateCaptionLines(const std::string& caption, float fontSize, int maxWidth) {
        std::vector<std::string> lines;
        std::istringstream stream(caption);
        std::string word, line;
        while (stream >> word) {
            std::string newLine = line.empty() ? word : line + " " + word;
            if (newLine.length() * fontSize > maxWidth) {
                lines.push_back(line);
                line = word; // Start a new line with the current word
            } else {
                line = newLine;
            }
        }
        if (!line.empty()) {
            lines.push_back(line); // Add the last line
        }
        return lines;
    }

    static void generatePost(const PostConfig& config) {
        auto [image, logo] = loadImages(config.image, config.link, config.logo);
        if (image.empty() || logo.empty()) {
            log::error("Error loading images.");
            return;
        }

        // Combine the image and logo as needed. Here we simply save the main image.
        std::filesystem::create_directories(config.output); // Ensure the output directory exists
        std::string outputPath = config.output.string() + "/post.png";

        if (stbi_write_png(outputPath.c_str(), config.width, config.width, 3, image.data(), config.width * 3)) {
            log::info("Post image saved successfully to {}", outputPath);
        } else {
            log::error("Failed to save post image to {}", outputPath);
        }

        std::string formattedDate = createFormattedDate(config.day, config.month, config.year);
        log::info("Generated post for date: {}", formattedDate);
    }
};

#endif // DAILY_GEODE_POST_CREATOR_H
