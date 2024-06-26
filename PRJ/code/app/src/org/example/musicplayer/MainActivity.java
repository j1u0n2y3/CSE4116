/*
 * MainActivity.java -
 * This source file contains the MainActivity class responsible for activity in Android application.
 * MainActivity constructs the application screen and communicates
 * with the MusicService through intents and broadcast receiver.
 *
 * Author : 20211584 Junyeong Jang
 */

package org.example.musicplayer;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.view.ViewGroup.LayoutParams;

import java.util.LinkedList;
import java.util.Queue;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

public class MainActivity extends Activity
{
    private ArrayList<Integer> queue; /* music queue */
    private int userTop; /* number of user-inserted music */

    /* onCreate */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /* Register the receiver to receive broadcast intents with a matching action. */
        IntentFilter filter = new IntentFilter("org.example.musicplayer.QUEUE_STATUS");
        registerReceiver(receiver, filter);
        /* Start the service. */
        Intent intent = new Intent(MainActivity.this, MusicService.class);
        startService(intent);

        /* Music buttons (1 ~ 10) */
        for (int i = 1; i <= 10; i++)
        {
            int buttonId = getResources().getIdentifier("button" + i, "id", getPackageName());
            Button button = (Button)findViewById(buttonId);
            final int finalI = i;
            button.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    /* Include the music ID in the intent and pass it to the service. */
                    Intent intent = new Intent(MainActivity.this, MusicService.class);
                    intent.putExtra("buttonId", finalI);
                    startService(intent);
                }
            });
        }
        /* Play button */
        Button playButton = (Button)findViewById(R.id.playButton);
        playButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                /* Include the "play" command in the intent and pass it to the service. */
                Intent intent = new Intent(MainActivity.this, MusicService.class);
                intent.putExtra("command", "play");
                startService(intent);
            }
        });
        /* Pause button */
        Button pauseButton = (Button)findViewById(R.id.pauseButton);
        pauseButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                /* Include the "pause" command in the intent and pass it to the service. */
                Intent intent = new Intent(MainActivity.this, MusicService.class);
                intent.putExtra("command", "pause");
                startService(intent);
            }
        });
        /* Skip button */
        Button skipButton = (Button)findViewById(R.id.skipButton);
        skipButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                /* Include the "skip" command in the intent and pass it to the service. */
                Intent intent = new Intent(MainActivity.this, MusicService.class);
                intent.putExtra("command", "skip");
                startService(intent);
            }
        });
        /* Exit button */
        Button exitButton = (Button)findViewById(R.id.exitButton);
        exitButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                /* Finish the activity. */
                finish();
            }
        });
        /* Update the queue display. */
        updateQueueDisplay();
    }

    /* onDestroy */
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        /* Include the "exit" command in the intent and pass it to the service. */
        Intent intent = new Intent(MainActivity.this, MusicService.class);
        intent.putExtra("command", "exit");
        startService(intent);
        /* Unregister the receiver. */
        unregisterReceiver(receiver);
    }

    /* receiver - BroadcastReceiver to receive broadcast intents */
    private BroadcastReceiver receiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            /* Get the queue and the userTop from the intent. */
            queue = intent.getIntegerArrayListExtra("queue");
            userTop = intent.getIntExtra("userTop", 0);
            /* Update the queue display. */
            updateQueueDisplay();
        }
    };

    /* updateQueueDisplay - Method to update the queue display */
    private void updateQueueDisplay()
    {
        LinearLayout queueLayout = (LinearLayout)findViewById(R.id.queueLayout);
        /* Remove all views from the queueLayout. */
        queueLayout.removeAllViews();
        if (queue != null)
        {
            for (int i = 0; i < queue.size(); i++)
            {
                TextView textView = new TextView(this);
                textView.setText(String.valueOf(queue.get(i)));
                textView.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
                /* If the music is user-added, set the text color to holo_blue_dark. */
                if (i < userTop)
                    textView.setTextColor(getResources().getColor(android.R.color.holo_blue_dark));
                /* Otherwise, set the text color to black. */
                else
                    textView.setTextColor(getResources().getColor(android.R.color.black));
                queueLayout.addView(textView);
            }
        }
    }
}