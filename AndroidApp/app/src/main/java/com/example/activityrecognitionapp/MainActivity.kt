package com.example.activityrecognitionapp

import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.android.volley.DefaultRetryPolicy
import com.android.volley.Request
import com.android.volley.RequestQueue
import com.android.volley.toolbox.JsonArrayRequest
import com.android.volley.toolbox.JsonObjectRequest
import com.android.volley.toolbox.Volley

// Data class for History records
data class ActivityRecord(
    val activity: String,
    val confidence: Double,
    val timestamp: String
)

class MainActivity : ComponentActivity() {

    private lateinit var requestQueue: RequestQueue

    // 🌟 BACKUP PLAN: If 10.0.2.2 fails, change this to your laptop's IPv4 (e.g., "http://192.168.1.101:5000/api/activities")
    private val BASE_URL = "http://192.168.1.101:5000/api/activities"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        requestQueue = Volley.newRequestQueue(this)

        setContent {
            MaterialTheme {
                var showHistory by remember { mutableStateOf(false) }

                if (showHistory) {
                    HistoryScreen(
                        onBackClick = { showHistory = false },
                        fetchHistory = { onSuccess, onError -> fetchHistoryData(onSuccess, onError) }
                    )
                } else {
                    MainScreen(
                        onRefreshClick = { onSuccess, onError -> fetchLatestActivity(onSuccess, onError) },
                        onHistoryClick = { showHistory = true }
                    )
                }
            }
        }
    }

    @Composable
    fun MainScreen(
        onRefreshClick: (onSuccess: (String, String) -> Unit, onError: (String) -> Unit) -> Unit,
        onHistoryClick: () -> Unit
    ) {
        var currentActivity by remember { mutableStateOf("Waiting for prediction...") }
        var confidenceText by remember { mutableStateOf("Tap Refresh to start") }

        Column(
            modifier = Modifier.fillMaxSize().padding(24.dp),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(text = currentActivity, style = MaterialTheme.typography.headlineMedium)
            Spacer(modifier = Modifier.height(8.dp))
            Text(text = confidenceText, style = MaterialTheme.typography.bodyLarge)

            Spacer(modifier = Modifier.height(32.dp))

            Button(onClick = {
                currentActivity = "Fetching..."
                confidenceText = ""
                onRefreshClick(
                    { activity, conf ->
                        currentActivity = activity
                        confidenceText = conf
                    },
                    { error ->
                        currentActivity = "Error"
                        confidenceText = error
                    }
                )
            }) {
                Text("Refresh Prediction")
            }

            Spacer(modifier = Modifier.height(16.dp))

            Button(onClick = onHistoryClick) {
                Text("View History")
            }
        }
    }

    @Composable
    fun HistoryScreen(
        onBackClick: () -> Unit,
        fetchHistory: (onSuccess: (List<ActivityRecord>) -> Unit, onError: (String) -> Unit) -> Unit
    ) {
        var historyList by remember { mutableStateOf<List<ActivityRecord>>(emptyList()) }
        var errorText by remember { mutableStateOf("") }

        LaunchedEffect(Unit) {
            fetchHistory(
                { list -> historyList = list },
                { err -> errorText = err }
            )
        }

        Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Activity History", style = MaterialTheme.typography.headlineSmall)
                Button(onClick = onBackClick) { Text("Back") }
            }

            Spacer(modifier = Modifier.height(16.dp))

            if (errorText.isNotEmpty()) {
                Text(text = errorText, color = MaterialTheme.colorScheme.error)
            } else if (historyList.isEmpty()) {
                Text("No history found. Try predicting something first!")
            } else {
                LazyColumn {
                    items(historyList) { record ->
                        Card(modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                            Column(modifier = Modifier.padding(12.dp)) {
                                Text(text = record.activity, style = MaterialTheme.typography.titleMedium)
                                Text(text = "Confidence: ${(record.confidence * 100).toInt()}%", style = MaterialTheme.typography.bodyMedium)
                                Text(text = record.timestamp, style = MaterialTheme.typography.bodySmall)
                            }
                        }
                    }
                }
            }
        }
    }

    // Fetch the LATEST prediction (Reverted back to /latest)
    private fun fetchLatestActivity(
        onSuccess: (String, String) -> Unit,
        onError: (String) -> Unit
    ) {
        val url = "$BASE_URL/latest"

        val request = JsonObjectRequest(
            Request.Method.GET, url, null,
            { response ->
                Log.d("API_DEBUG", "Response received: $response")
                // Parse the actual activity and confidence
                val activity = response.optString("activity", "Unknown")
                val confidence = response.optDouble("confidence", 0.0)
                onSuccess(activity, "Confidence: ${(confidence * 100).toInt()}%")
            },
            { error ->
                Log.e("API_DEBUG", "Error: ${error.toString()}")
                onError("Failed: ${error.toString()}")
            }
        )

        // 15-second timeout
        request.retryPolicy = DefaultRetryPolicy(
            15000,
            3,
            DefaultRetryPolicy.DEFAULT_BACKOFF_MULT
        )
        Log.d("API_DEBUG", "Request started for URL: $url")
        requestQueue.add(request)
    }

    // Fetch the HISTORY records
    private fun fetchHistoryData(
        onSuccess: (List<ActivityRecord>) -> Unit,
        onError: (String) -> Unit
    ) {
        val url = "$BASE_URL/history"
        val request = JsonArrayRequest(
            Request.Method.GET, url, null,
            { response ->
                val list = mutableListOf<ActivityRecord>()
                for (i in 0 until response.length()) {
                    val obj = response.getJSONObject(i)
                    list.add(
                        ActivityRecord(
                            activity = obj.optString("activity"),
                            confidence = obj.optDouble("confidence"),
                            timestamp = obj.optString("timestamp")
                        )
                    )
                }
                onSuccess(list)
            },
            { error -> onError("Failed to load history: ${error.toString()}") }
        )
        requestQueue.add(request)
    }
}