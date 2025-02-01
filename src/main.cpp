#include <iostream>
#include <chrono>
#include <iomanip>
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/cocos.hpp>  // For file dialog utilities

#include "DailyGeode.hpp"

using namespace geode::prelude;

static auto IMPORT_PICK_OPTIONS = file::FilePickOptions{
    std::nullopt,
    {
        {
            "Image Files",
            { "*.png", "*.jpg", "*.jpeg", "*.bmp" }
        }
    }
};

static auto EXPORT_PICK_OPTIONS = file::FilePickOptions{
    std::nullopt,
    {
        {
            "Output Folder",
            { "*" }
        }
    }
};

class $modify(MyMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto myButton = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::create(
                CCSprite::createWithSpriteFrameName("geode.loader/geode-logo-outline.png"),
                CircleBaseColor::Green,
                CircleBaseSize::MediumAlt
            ),
            this,
            menu_selector(MyMenuLayer::onMyButton)
        );

        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(myButton);
        myButton->setID("my-button"_spr);
        menu->updateLayout();

        return true;
    }

    void onMyButton(CCObject*) {
        // Define the config here
        PostConfig config;

        // Define an alias for the event type expected by the listener.
        using EventType = Task<Result<std::filesystem::path>>::Event;

        // Create an event listener for selecting the image file.
        EventListener<Task<Result<std::filesystem::path>>> imageListener;
        imageListener.bind([this, &config](EventType* ev) {
            if (auto result = ev->getValue()) {
                if (result->isOk()) {
                    auto imagePath = result->unwrap();
                    // Declare local variables for image dimensions.
                    int width = 0, height = 0, channels = 0;
                    config.image = imagePath;  // Set the image path in config
                    DailyGeodePostCreator::loadImage(imagePath.string(), width, height, channels);
                    log::debug("Image selected: {}", imagePath);
                } else {
                    FLAlertLayer::create("Error", result->unwrapErr(), "OK")->show();
                }
            }
        });
        // Set the filter to the task returned by file::pick.
        imageListener.setFilter(file::pick(file::PickMode::OpenFile, IMPORT_PICK_OPTIONS));

        // Create an event listener for selecting the output folder.
        EventListener<Task<Result<std::filesystem::path>>> outputListener;
        outputListener.bind([this, &config](EventType* ev) {
            if (auto result = ev->getValue()) {
                if (result->isOk()) {
                    auto outputPath = result->unwrap();
                    config.output = outputPath.string();  // Set the output path in config
                    log::debug("Output directory selected: {}", outputPath);
                } else {
                    FLAlertLayer::create("Error", result->unwrapErr(), "OK")->show();
                }
            }
        });
        outputListener.setFilter(file::pick(file::PickMode::OpenFolder, EXPORT_PICK_OPTIONS));

        // Get current date and time for the post
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_time_t); // Safe version on Windows

        config.day = now_tm.tm_mday;
        config.month = now_tm.tm_mon + 1;
        config.year = now_tm.tm_year + 1900;
        config.caption = "This is the caption for the post!";
        config.headerColor = "078c51"; // Example header color

        // Generate the post using the correct class name.
        DailyGeodePostCreator::generatePost(config);
    }
};
