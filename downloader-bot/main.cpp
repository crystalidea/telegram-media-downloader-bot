#include "pch.h"
#include "bot.h"

#include <signal.h>

const static int RET_CODE_NORMAL = 0;
const static int RET_CODE_ERROR = 1;
const static int RET_CODE_RESTART = 2;

void SigTerm_Handler(int n)
{
    QTelegramDownloaderBot* bot = static_cast<QTelegramDownloaderBot*>(QCoreApplication::instance());

    auto logger = bot->getLogger();

    logger->info("SIGTERM received");
    
    QCoreApplication::quit(); // normal, exec returns 0
}

int main(int argc, char *argv[])
{
    int nReturnValue = RET_CODE_NORMAL;

    signal(SIGTERM, &SigTerm_Handler);

    int nMaxRetryCount = RESTART_MAX_COUNT;

    while (true)
    {
        QTelegramDownloaderBot bot(argc, argv);

        QObject::connect(&bot, &QTelegramDownloaderBot::fatalError, [](qint32 errorCode) {

            if (errorCode == 0) // Internet is disconnected
            {
                QCoreApplication::exit(RET_CODE_RESTART);
            }
            else
                QCoreApplication::exit(RET_CODE_ERROR); // exec returns 1
        });

        if (bot.start())
        {
            nReturnValue = bot.exec();
        }

        if (nReturnValue == RET_CODE_RESTART && nMaxRetryCount > 0)
        {
            QThread::currentThread()->sleep(RESTART_WAIT_SECONDS);

            nMaxRetryCount--;

            continue;
        }

        bot.getLogger()->info("Waiting for all threads to be stopped...");

        bot.stop(); // waits until all threads are finished

        bot.getLogger()->info("Bye");

        break;
    }

    return nReturnValue;
}
