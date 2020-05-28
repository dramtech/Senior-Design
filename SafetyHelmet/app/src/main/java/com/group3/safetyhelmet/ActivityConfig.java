package com.group3.safetyhelmet;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;


import android.os.Bundle;
import android.widget.CompoundButton;
import android.widget.Switch;

import com.google.android.material.card.MaterialCardView;


public class ActivityConfig extends AppCompatActivity {

    private Switch tempSwitch;
    private String tempUnit = "F";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_config);

        Toolbar toolbar = findViewById(R.id.config_toolbar);
        toolbar.setTitle(R.string.config_toolbar_title);
        setSupportActionBar(toolbar);

        tempSwitch = (Switch) findViewById(R.id.tempSwitch);
        tempSwitch.setText(tempUnit);
        tempSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    tempSwitch.setText("C");
                    setTempUnit("C");
                } else {
                    tempSwitch.setText("F");
                    setTempUnit("F");
                }
            }
        });
    }

    public void setTempUnit(String s) {
        tempUnit = s;
    }

    public String getTempUnit() {
        return tempUnit;
    }

}
