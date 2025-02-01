#include <iostream>
#include <chrono>
#include <iomanip>
#include <memory>
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

// Shared state for file picks
struct PostState {
    PostConfig config;
    bool imageSelected = false;
    bool outputSelected = false;
};

// Helper function: if both picks are complete, generate the post.
void tryGeneratePost(std::shared_ptr<PostState> state) {
    if (state->imageSelected && state->outputSelected) {
        // Get current date and time
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;

        // Use platform-specific localtime function
        #if defined(GEODE_IS_WINDOWS)
            localtime_s(&now_tm, &now_time_t); // Safe on Windows
        #elif defined(GEODE_IS_MACOS) || defined(GEODE_IS_ANDROID)
            localtime_r(&now_time_t, &now_tm);  // Safe on macOS/Android
        #endif

        state->config.day = now_tm.tm_mday;
        state->config.month = now_tm.tm_mon + 1;
        state->config.year = now_tm.tm_year + 1900;
        state->config.caption = "This is the caption for the post!";
        state->config.headerColor = "078c51"; // Example header color

        DailyGeodePostCreator::generatePost(state->config);
    }
}

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
        // Create shared state to track file pick completion
        auto state = std::make_shared<PostState>();

        // --- Create an event listener for the image file ---
        {
            using EventType = Task<Result<std::filesystem::path>>::Event;
            EventListener<Task<Result<std::filesystem::path>>> imageListener;
            imageListener.bind([state](EventType* ev) {
                if (auto result = ev->getValue()) {
                    if (result->isOk()) {
                        auto imagePath = result->unwrap();
                        state->config.image = imagePath;  // Set image path
                        log::debug("Image selected: {}", imagePath);
                        state->imageSelected = true;
                    } else {
                        FLAlertLayer::create("Error", result->unwrapErr(), "OK")->show();
                    }
                }
                tryGeneratePost(state);
            });
            imageListener.setFilter(file::pick(file::PickMode::OpenFile, IMPORT_PICK_OPTIONS));
        }

        // --- Create an event listener for the output folder ---
        {
            using EventType = Task<Result<std::filesystem::path>>::Event;
            EventListener<Task<Result<std::filesystem::path>>> outputListener;
            outputListener.bind([state](EventType* ev) {
                if (auto result = ev->getValue()) {
                    if (result->isOk()) {
                        auto outputPath = result->unwrap();
                        state->config.output = outputPath.string();  // Set output path
                        log::debug("Output directory selected: {}", outputPath);
                        state->outputSelected = true;
                    } else {
                        FLAlertLayer::create("Error", result->unwrapErr(), "OK")->show();
                    }
                }
                tryGeneratePost(state);
            });
            outputListener.setFilter(file::pick(file::PickMode::OpenFolder, EXPORT_PICK_OPTIONS));
        }
    }
};
