package com.meshcompute.bot;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;

public class BotService extends Service {
    private static final String CHANNEL_ID = "MeshBotChannel";
    
    @Override
    public void onCreate() {
        super.onCreate();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID, "MeshBot", NotificationManager.IMPORTANCE_LOW);
            NotificationManager manager = getSystemService(NotificationManager.class);
            manager.createNotificationChannel(channel);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String url = intent.getStringExtra("server_url");
        String token = intent.getStringExtra("reg_token");
        String botId = intent.getStringExtra("bot_id");
        
        // Foreground service notification
        Notification notification = new Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("MeshBot running")
            .setContentText("Connected to mesh network")
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .build();
        startForeground(1, notification);
        
        // Start native bot in background thread
        new Thread(() -> startBotNative(url, token, botId)).start();
        return START_STICKY;
    }
    
    private native void startBotNative(String url, String token, String botId);
    
    static {
        System.loadLibrary("bot-core");
    }
    
    @Override
    public IBinder onBind(Intent intent) { return null; }
}