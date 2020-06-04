package com.group3.safetyhelmet;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.Toolbar;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

//    private Button setEmergContBtn;
    private Button setConfigBtn;
    private Button navBtn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Setting app the tool bar
        Toolbar myToolbar = (Toolbar) findViewById(R.id.main_toolbar);
        setSupportActionBar(myToolbar);

        // App theme always in night mode
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

//        setEmergContBtn = findViewById(R.id.setEmergContBtn);
//        setEmergContBtn.setOnClickListener(this);
        setConfigBtn = findViewById(R.id.setConfigBtn);
        setConfigBtn.setOnClickListener(this);
        navBtn = findViewById(R.id.navBtn);
        navBtn.setOnClickListener(this);

    }


    @Override
    public void onClick(View v) {
        switch (v.getId()) {
//            case R.id.setEmergContBtn:
//                Toast.makeText(this,"emerg clicked",Toast.LENGTH_SHORT).show();
//                    loadEmergContForm();
//                break;
            case R.id.setConfigBtn:
                Toast.makeText(this,"config clicked",Toast.LENGTH_SHORT).show();
                loadConfig();
                break;
            case R.id.navBtn:
                Toast.makeText(this,"config clicked",Toast.LENGTH_SHORT).show();
                loadNav();
            default:
                break;
        }
    }

    private void loadEmergContForm() {
        // TODO load a form to add emergency contact
    }

    private void loadConfig() {
        Intent intent = new Intent(this, ActivityConfig.class);
        startActivity(intent);
    }

    private void loadNav() {
        Intent intent = new Intent(this, ActivityNav.class);
        startActivity(intent);
    }
}
