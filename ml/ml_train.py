# ml_train.py — обучение модели классификации тревог
# ---------------------------------------------------
# Загружает данные из sensor_data.csv,
# обучает модель предсказания vibration_alarm,
# сохраняет модель в файл для последующего использования.

import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report
import joblib

# --- Загрузка данных ---
df = pd.read_csv("data/sensor_data.csv", parse_dates=["ts"])
df.set_index("ts", inplace=True)

# --- Признаки и целевая переменная ---
X = df[["vibration", "temperature", "pressure"]]
y = df["vibration_alarm"].astype(int)

# --- Разделение на обучающую и тестовую выборки ---
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# --- Обучение модели ---
model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

# --- Оценка качества ---
y_pred = model.predict(X_test)
print("📊 Отчёт по классификации:")
print(classification_report(y_test, y_pred))

# --- Сохранение модели ---
joblib.dump(model, "ml/model_rf_alarm.pkl")
print("✅ Модель сохранена: ml/model_rf_alarm.pkl")
