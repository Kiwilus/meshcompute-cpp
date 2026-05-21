#include <jni.h>
#include <thread>
#include "bot_handler.h"

static std::unique_ptr<BotHandler> g_bot;

extern "C" JNIEXPORT void JNICALL
Java_com_meshcompute_bot_BotService_startBotNative(
        JNIEnv* env, jobject /* this */,
        jstring serverUrl, jstring regToken, jstring botId) {

    const char* url = env->GetStringUTFChars(serverUrl, nullptr);
    const char* token = env->GetStringUTFChars(regToken, nullptr);
    const char* id = env->GetStringUTFChars(botId, nullptr);

    g_bot = std::make_unique<BotHandler>(url, token, id);
    std::thread([] {
        try {
            g_bot->start();
        } catch (...) {
            // Logging oder Neustartversuch
        }
    }).detach();

    env->ReleaseStringUTFChars(serverUrl, url);
    env->ReleaseStringUTFChars(regToken, token);
    env->ReleaseStringUTFChars(botId, id);
}