# Telegram Media Downloader Bot (beta)

Supports downloading from Instagram (Reels and Posts), Youtube, Facebook share links (``facebook.com/share/v/...``), Twitter and Threads. You can add more from [yt-dlp](https://github.com/yt-dlp/yt-dlp) and [gallery-dl](https://github.com/mikf/gallery-dl) but it will require modifying the source code (appriciated).

# Running the bot

The bot is supposed to be run in Docker (see docker/Dockerfile). Before starting the container, you must create **bot_shared** folder on the Docker host machine with:

- **token.txt** file with your Telegram bot token code
- **www.instagram.com_cookies.txt** file (optional) to download instagram posts, you should login to your instagram account using a web browser (Chrome) and get this file using the [Get cookes.txt LOCALLY](https://chromewebstore.google.com/detail/get-cookiestxt-locally/cclelndahbckbenkjhflpdbgdldlbecc) Chrome extension.

and mount it to **/home/apprunner/bot_shared** folder in the container with both read&write access (docker/synology and docker/windows folder have some sample instruction how to do it in these environments).

The bot stores log files in bot_shared/logs (for example, logs/2025-11-18.txt) and writes stats.csv to the root directory of bot_shared.

# Developer Notes

Feel free to create pull requests and improve the bot: its code and features are still far from being perfect, the bot was written very quickly back in time. There could be quite a few bugs in this code (you are warned).

The bot itself is written in **C++/Qt**, can be built with cmake, for convenience Visual Studio 2022 Solution is provided.

The bot Docker container basically uses Ubuntu 24.04 with all build tools required to build the bot and yt-dlp. When starting the container, it downloads this repository, and runs *_build.sh* script, which in its turn does the following:

- builds the bot
- downloads yt-dlp source code and applies meta Threads support from a [pull request](https://github.com/yt-dlp/yt-dlp/pull/13512) (not currently merged).
- downloads gallery-dl binary
- starts the bot

# Dependencies (already in this repository)

1. Fork of the [TelegramBotAPI](https://github.com/Modersi/TelegramBotAPI) library with the following changes:
- fixed sendMediaGroup API

- fixed compilation errors in Visual Studio 2022

- fixed compilation errors in Linux
2. [spdlog](https://github.com/gabime/spdlog)

3. [BS_thread_pool](https://github.com/bshoshany/thread-pool)

4. [json.hpp](https://github.com/nlohmann/json)

# License

LICENSE (GNU General Public License v3.0)
