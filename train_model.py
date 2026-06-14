import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score
import joblib

df = pd.read_csv("dataset.csv")

print(df.head())
#input to ai
X = df.drop("label", axis=1)
#answer
y = df["label"]


X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)


#scaling into same range
scaler = StandardScaler()


X_train = scaler.fit_transform(X_train)

X_test = scaler.transform(X_test)

#train model
model = RandomForestClassifier(
    n_estimators=10,
    max_depth=5,
    random_state=42
)


model.fit(
    X_train,
    y_train
)

predictions = model.predict(X_test)

#check accuracy
accuracy = accuracy_score(
    y_test,
    predictions
)


print(
    "Accuracy:",
    accuracy
)

#model file
joblib.dump(
    model,
    "sound_seismic_model.pkl"
)


joblib.dump(
    scaler,
    "scaler.pkl"
)

from micromlgen import port


c_code = port(model)



with open(
    "model.h",
    "w"
) as file:

    file.write(c_code)


print("C++ model exported")

# --- COPY AND PASTE THIS FIXED BLOCK AT THE BOTTOM OF YOUR PYTHON SCRIPT ---
print("\n--- COPY THESE INTO ARDUINO ---")
all_means = list(scaler.mean_)
all_stds = list(scaler.scale_)

# Split the 20 features into 10 seismic and 10 sound
print("Seismic Means:", [round(x, 4) for x in all_means[:10]])
print("Seismic Stds:",  [round(x, 4) for x in all_stds[:10]])
print("Sound Means:",   [round(x, 4) for x in all_means[10:]])
print("Sound Stds:",    [round(x, 4) for x in all_stds[10:]])