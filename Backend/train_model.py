import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report
import joblib

# 1. Load the dataset
csv_file = "run-or-walk/run-or-walk/data.csv" 

try:
    print("Loading data...")
    df = pd.read_csv(csv_file)
except FileNotFoundError:
    print(f"Error: Could not find '{csv_file}'. Check the folder path.")
    exit()

# 2. Preprocess the data (Fixed column names to match your CSV headers)
# We are using all 6 axes as requested by the PDF
features = ['acceleration_x', 'acceleration_y', 'acceleration_z', 'gyro_x', 'gyro_y', 'gyro_z']
target = 'activity'

X = df[features]
y = df[target]

# 3. Train-Test Split
print("Splitting data...")
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 4. Train a Lightweight Model (Random Forest)
print("Training Random Forest model...")
model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

# 5. Evaluate the Model
print("Evaluating model...")
y_pred = model.predict(X_test)
accuracy = accuracy_score(y_test, y_pred)
print(f"Model Accuracy: {accuracy * 100:.2f}%")
print("\nClassification Report:")
print(classification_report(y_test, y_pred))

# 6. Save the Model
joblib.dump(model, 'activity_model.pkl')
print("Model successfully saved as 'activity_model.pkl'!")