cmake_minimum_required(VERSION 3.16)
project(downloader-bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build)

find_package(Qt6 REQUIRED Core)
find_package(Qt6 REQUIRED COMPONENTS Network)

qt_add_executable(downloader-bot bot_config.cpp bot_logger.cpp bot_status.cpp main.cpp bot.cpp helpers.cpp 
		  video_downloader.cpp gallery_downloader.cpp stream_info.cpp video_info.cpp video_info_parser.cpp)

target_link_libraries(downloader-bot PRIVATE Qt6::Core PRIVATE Qt6::Network PRIVATE TelegramBotAPI)
target_precompile_headers(downloader-bot PUBLIC <QtCore> "spdlog_qt.h")

include_directories(../TelegramBotAPI 3rdParty)
