package com.group3.safetyhelmet;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.os.Bundle;

public class ActivityNav extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_nav);

        Toolbar toolbar = findViewById(R.id.nav_toolbar);
        toolbar.setTitle(R.string.nav_toolbar_title);
        setSupportActionBar(toolbar);

        // TODO add navigation functionality to this activity
        // set elevation of cards to 4dp in xml layout
    }
}
