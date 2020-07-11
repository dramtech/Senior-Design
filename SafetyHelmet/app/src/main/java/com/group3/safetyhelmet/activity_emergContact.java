package com.group3.safetyhelmet;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.tasks.OnSuccessListener;

import java.util.List;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class activity_emergContact extends AppCompatActivity {
    private static final String TAG = "activityContact";

    private EditText contactFirstName;
    private EditText contactLastName;
    private EditText contactPhoneNum;
    private EditText messageEditText;
    private Button saveBtn;
    private Button editBtn;
    private SharedPreferences sp;
    private Storage storage;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Establish shared preference access.
        sp = getSharedPreferences("contactInfo", Context.MODE_PRIVATE);
        storage = new Storage(sp);

        setContentView(R.layout.activity_emerg_contact);

        Toolbar toolbar = findViewById(R.id.nav_toolbar);
        toolbar.setTitle(R.string.emergency_cont_title);
        setSupportActionBar(toolbar);

        contactFirstName = (EditText) findViewById(R.id.emergContName);
        contactLastName = (EditText) findViewById(R.id.emergContLastName);
        contactPhoneNum = (EditText) findViewById(R.id.emergContNumber);
        saveBtn = (Button) findViewById(R.id.saveButton);
        editBtn = (Button) findViewById(R.id.editButton);
        messageEditText = (EditText) findViewById(R.id.emergMessage);

        saveBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                // Retrieve contact info from view.
                String firstName = contactFirstName.getText().toString();
                String lastName = contactLastName.getText().toString();
                String phoneNum = contactPhoneNum.getText().toString();
                String personalMessage = messageEditText.getText().toString();

                // Check if contact info entered is valid.
                if (firstName == null || lastName == null ||
                        firstName.length() == 0 || lastName.length() == 0 || phoneNum.length() != 10) {
                    Log.d(TAG, "Error: User inputted invalid contact information.");
                    return;
                }

                // Save emergency contact in Shared Preferences.
                storage.add("ContactFirstName1", firstName);
                storage.add("ContactLastName1", lastName);
                storage.add("ContactPhoneNum1", phoneNum);
                storage.add("PersonalMessage1", personalMessage);

                // Disable edit text fields.
                contactFirstName.setEnabled(false);
                contactLastName.setEnabled(false);
                contactPhoneNum.setEnabled(false);
                messageEditText.setEnabled(false);

                // Let user know contact info has been added.
                Toast toast = Toast.makeText(getApplicationContext(), R.string.toast_contact_added, Toast.LENGTH_SHORT);
                toast.show();

                String fullName = firstName += " " + lastName;
                MainActivity.updateEmergContactInfoView(fullName, phoneNum);

                Log.d(TAG, storage.getValue("ContactFirstName1"));
                Log.d(TAG, storage.getValue("ContactLastName1"));
                Log.d(TAG, storage.getValue("ContactPhoneNum1"));
                Log.d(TAG, storage.getValue("PersonalMessage1"));
            }
        });

        editBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                // Enable edit text fields.
                contactFirstName.setEnabled(true);
                contactLastName.setEnabled(true);
                contactPhoneNum.setEnabled(true);
                messageEditText.setEnabled(true);
            }
        });
    }
}