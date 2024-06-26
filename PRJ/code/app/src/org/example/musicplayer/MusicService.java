/*
 * MusicService.java -
 * This source file contains the MusicService class responsible for service in Android application.
 * MusicService manages the data structures necessary for the music playback
 * application and communicates with the native library through JNI.
 *
 * Author : 20211584 Junyeong Jang
 */

package org.example.musicplayer;

import android.app.Service;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.IBinder;
import android.util.Log;

import java.util.LinkedList;
import java.util.Queue;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

import android.content.Context;
import java.io.File;

public class MusicService extends Service
{
    private MediaPlayer mp; /* media player obj */
    private Queue<Integer> queue = new LinkedList<Integer>(); /* music queue */
    private int userTop = 0; /* number of user-added music */
    private boolean isPaused = false; /* boolean to check if the music is paused */
    private int prevButtonId = -1; /* ID of the previously pressed button */
    private int durationTime = -1; /* duration time of the currently playing music */
    private int remainingTime = -1; /* remaining time of the currently playing music */
    private Thread worker; /* worker thread */

    /* native library methods */
    static
    {
        System.loadLibrary("recgraph");
    }
    private native void initializeGraph();
    private native void updateGraph(int prev, int curr);
    private native void recommendMusic();
    private native void saveGraph();
    private native int resetInput();
    private native void deviceOutput();
    private native void initDisplay();

    /* onBind */
    @Override
    public IBinder onBind(Intent intent)
    {
        return null;
    }

    /* onCreate */
    @Override
    public void onCreate()
    {
        super.onCreate();
        /* Initialize recgraph and output devices. */
        initializeGraph();
        initDisplay();
        /* Create a new media player. */
        mp = new MediaPlayer();
        /* Set a listener for when the music completes. */
        mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
        {
            @Override
            public void onCompletion(MediaPlayer mp)
            {
                /* When the music completes, remove the song from the queue
                 * and play the song at the front of the queue.
                 */
                queue.poll();
                userTop--;
                if (userTop < 0)
                    userTop = 0;
                playNextSong();
            }
        });

        /* Create a new worker thread. */
        worker = new Thread(new Runnable()
        {
            @Override
            public void run()
            {
                int prevValue = 0;
                while (true)
                {
                    /* When the reset button is pressed,
                     * shuffle the queue and play the song at the front of the queue.
                     */
                    int value = resetInput();
                    if (value == 1 && prevValue == 0)
                    {
                        List<Integer> q2list = new ArrayList<Integer>(queue);
                        Collections.shuffle(q2list);
                        queue = new LinkedList<Integer>(q2list);
                        userTop = queue.size();
                        playNextSong();
                    }
                    prevValue = value;
                    /* If the music is playing, update the duration and remaining time. */
                    if (mp.isPlaying())
                    {
                        durationTime = mp.getDuration() / 1000;
                        remainingTime = (mp.getDuration() - mp.getCurrentPosition()) / 1000;
                    }
                    else
                    {
                        durationTime = -1;
                        remainingTime = -1;
                    }
                    /* Pass the necessary information to the module and broadcast the queue status. */
                    deviceOutput();
                    broadcastQueueStatus();
                    /* Sleep for 0.1 seconds. */
                    try
                    {
                        Thread.sleep(100);
                    }
                    catch (InterruptedException e)
                    {
                        return;
                    }
                }
            }
        });
        /* Start the worker thread. */
        worker.start();
    }

    /* onStartCommand */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        /* Leave only as many elements in the queue as userTop. */
        if (userTop != 0)
            maintainUserTop();

        /* Get the command from the intent. */
        String command = intent.getStringExtra("command");
        if (command != null)
        {
            /* If the command is "play", start the music. */
            if (command.equals("play"))
            {
                if (isPaused)
                {
                    mp.start();
                    isPaused = false;
                }
                else
                    playNextSong();
            }
            /* If the command is "pause", pause the music. */
            else if (command.equals("pause"))
            {
                if (mp.isPlaying())
                {
                    mp.pause();
                    isPaused = true;
                }
            }
            /* If the command is "skip", skip to the next song. */
            else if (command.equals("skip"))
            {
                if (!queue.isEmpty())
                {
                    queue.poll();
                    userTop--;
                    if (userTop < 0)
                        userTop = 0;
                    isPaused = false;
                    playNextSong();
                }
            }
            /* If the command is "exit", destroy the service. */
            else if(command.equals("exit"))
                onDestroy();
        }
        else
        {
            /* If there is no command, get the button ID from the intent. */
            int buttonId = intent.getIntExtra("buttonId", -1);
            if (buttonId != -1 && queue.size() < 10)
            {
                /* Update the graph based on previously pressed and currently pressed buttons. */
                if (prevButtonId != -1)
                    updateGraph(prevButtonId, buttonId);
                /* Add the button ID to the queue. */
                queue.add(buttonId);
                userTop++;
                if (userTop > 10)
                    userTop = 10;
                prevButtonId = buttonId;
                /* If it is enqueued first into the empty queue, start playback. */
                if (!mp.isPlaying() && !isPaused)
                    playNextSong();
            }
        }
        /* Recommend music. */
        recommendMusic();
        return START_STICKY;
    }

    /* onDestroy */
    @Override
    public void onDestroy()
    {
        super.onDestroy();
        /* Save recgraph. */
        saveGraph();
        /* Interrupt the worker thread to stop. */
        if (worker != null)
            worker.interrupt();
        /* Stop and release the media player. */
        mp.stop();
        mp.release();
        /* Initialize the output devices. */
        initDisplay();
    }

    /* maintainUserTop - Method to leave only as many elements in the queue as userTop */
    private void maintainUserTop()
    {
        while (queue.size() > userTop)
            ((LinkedList<Integer>)queue).removeLast();
    }

    /* playNextSong - Method to play the song at the front of the queue */
    private void playNextSong()
    {
        mp.reset();
        if (!queue.isEmpty())
        {
            /* Get the song ID from the queue and load the music corresponding to the ID. */
            int songId = queue.peek();
            int resId = getResources().getIdentifier("music" + songId, "raw", getPackageName());
            mp.reset();
            mp = MediaPlayer.create(this, resId);
            mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
            {
                @Override
                public void onCompletion(MediaPlayer mp)
                {
                    queue.poll();
                    userTop--;
                    if (userTop < 0)
                        userTop = 0;
                    playNextSong();
                }
            });
            /* Start the music. */
            mp.start();
        }
    }

    /* getAppDataFilepath - Method to get the file path for the app data */
    public String getAppDataFilepath(String filename)
    {
        File file = new File(getFilesDir(), filename);
        return file.getAbsolutePath();
    }

    /* broadcastQueueStatus - Method to broadcast the queue status */
    private void broadcastQueueStatus()
    {
        Intent intent = new Intent("org.example.musicplayer.QUEUE_STATUS");
        intent.putExtra("queue", new ArrayList<Integer>(queue));
        intent.putExtra("userTop", userTop);
        sendBroadcast(intent);
    }
}