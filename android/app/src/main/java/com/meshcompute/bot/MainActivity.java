package com.meshcompute.bot;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // Konfiguration laden (hartcodiert oder aus shared preferences)
        String serverUrl = "wss://your-vps:8443";
        String regToken = "your-registration-token";
        String botId = "android-bot-1";
        
        Intent serviceIntent = new Intent(this, BotService.class);
        serviceIntent.putExtra("server_url", serverUrl);
        serviceIntent.putExtra("reg_token", regToken);
        serviceIntent.putExtra("bot_id", botId);
        startService(serviceIntent);
        
        finish(); // Activity beenden, Service läuft weiter
    }
}