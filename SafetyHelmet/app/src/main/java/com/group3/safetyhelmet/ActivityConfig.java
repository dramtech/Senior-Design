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

    private Switch bluetoothSwitch;
    private String blueOnState = "OFF";

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
                    setTempUnit("C");
                    tempSwitch.setText(getTempUnit());
                    // TODO convert temperature to C and update temperature value
                } else {
                    setTempUnit("F");
                    tempSwitch.setText(getTempUnit());
                    // TODO convert temperature to F and update temperature value
                }
            }
        });

        bluetoothSwitch = (Switch) findViewById(R.id.bluetoothOnSwitch);
        bluetoothSwitch.setText(blueOnState);

        //TODO set Bluetooth switch to ON if already paired and connected
        bluetoothSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    setBlueOnState("ON");
                    bluetoothSwitch.setText(getBlueOnState());
                    //TODO implement bluetooth pairing procedure and update bluetoothConfigState to connected and set color to colorDeviceStatConnected
                    //TODO ask Bluetooth permissions
                    //TODO implement if( BluetoothIsNotConnected ) --> set switch to off
                } else {
                    setBlueOnState("OFF");
                    bluetoothSwitch.setText(getBlueOnState());
                    //TODO disconnect bluetooth here (after bluetooth is implemented)
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

    public void setBlueOnState(String state) {
        blueOnState = state;
    }

    public String getBlueOnState() {
        return blueOnState;
    }
}
