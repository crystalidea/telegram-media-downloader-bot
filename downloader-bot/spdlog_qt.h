#pragma once

#ifdef QT_VERSION

    // 1: define qdebug_sink

    #include <string>
    #include <string_view>

    #include <spdlog/details/null_mutex.h>
    #include <spdlog/sinks/base_sink.h>
    #include <spdlog/logger.h>

    #include <spdlog/fmt/bundled/format.h> // must be included for QString output

    namespace spdlog {
        namespace sinks {

            template<typename Mutex>
            class qdebug_sink : public base_sink<Mutex>
            {
            public:
                explicit qdebug_sink()
                {
                    _defaultHandler = qInstallMessageHandler(noNewlineOutput);
                }

            protected:

                void sink_it_(const details::log_msg& msg) override
                {
                    memory_buf_t formatted;
                    base_sink<Mutex>::formatter_->format(msg, formatted);
                    std::string fmt_msg = fmt::to_string(formatted);

                    switch (msg.level)
                    {
                    case level::critical:
                        qCritical() << fmt_msg.c_str(); break;
                    case level::warn:
                        qWarning() << fmt_msg.c_str(); break;
                    default:
                        qDebug() << fmt_msg.c_str(); break;
                    }
                }

                void flush_() override {}

            private:

                static void noNewlineOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
                {
                    _defaultHandler(type, context, msg.trimmed());
                }

                void noMessageOutput(QtMsgType, const QMessageLogContext&, const QString&) {}

                static QtMessageHandler _defaultHandler;
            };

            template <typename T>
            QtMessageHandler qdebug_sink<T>::_defaultHandler = nullptr;

            using qdebug_sink_mt = qdebug_sink<std::mutex>;
            using qdebug_sink_st = qdebug_sink<details::null_mutex>;

        } // namespace sinks
    } // namespace spdlog

    // 2 - define QString output support (https://stackoverflow.com/questions/58898948/extend-spdlog-for-custom-type)
    // more: https://github.com/fmtlib/fmt/pull/3680/files
    
    template<>
    struct fmt::formatter<QString> : fmt::formatter<std::string_view> {
        template <typename FormatContext>
        auto format(const QString& input, FormatContext& ctx) const -> decltype(ctx.out()) {
            const auto utf8 = input.toUtf8();
            return fmt::formatter<std::string_view>::format(
                std::string_view{utf8.constData(), static_cast<std::size_t>(utf8.size())}, ctx);
        }
    };

    #define log_traceQ(q) log_trace("{}", QString(q) )
    #define log_infoQ(q) log_info("{}", QString(q))
    #define log_errQ(q) log_err("{}", QString(q))
    #define log_warnQ(q) log_warn("{}", QString(q))

    using spdlogger = std::shared_ptr<spdlog::logger>;

    #ifdef QT_VERSION
        #define SPDLOG_WARN_NOT_SET() else qWarning("WARN: %s:%d - spdlog not set yet", __FILE__, __LINE__)
        #define SPDLOG_WARN_ZERO() else qWarning("WARN: %s:%d spdlog is NULL", __FILE__, __LINE__)
    #else
        #define SPDLOG_WARN_NOT_SET() else printf("WARN: %s:%d - spdlog not set yet", __FILE__, __LINE__)
        #define SPDLOG_WARN_ZERO() else printf("WARN: %s:%d - spdlog is NULL", __FILE__, __LINE__)
    #endif

#endif // QT_VERSION