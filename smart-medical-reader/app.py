# -*- coding: utf-8 -*-
"""
القارئ الطبي الذكي — Smart Medical Imaging Reader
===================================================
واجهة Streamlit عربية بالكامل (RTL) تعمل 100% محلياً بدون أي API خارجي.

التشغيل:
    streamlit run app.py
"""

import json
import os

import numpy as np
import streamlit as st
from PIL import Image

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "models")
NUTRITION_FILE = os.path.join(BASE_DIR, "nutrition_db.json")
IMG_SIZE = (224, 224)

# ══════════════════════════════════════════════════════════════
# إعدادات أنواع الفحوصات
# ══════════════════════════════════════════════════════════════
SCAN_TYPES = {
    "chest": {
        "label": "🫁 أشعة الصدر (X-ray)",
        "model_file": "chest_model.keras",
        "classes_file": "chest_classes.json",
        # اسم الفئة في الداتاسيت → (الاسم العربي, مفتاح التغذية, هل هي حالة مرضية؟)
        "class_map": {
            "NORMAL": ("سليم ✅", "healthy", False),
            "PNEUMONIA": ("التهاب رئوي 🦠", "pneumonia", True),
        },
    },
    "brain": {
        "label": "🧠 صور الدماغ (MRI)",
        "model_file": "brain_model.keras",
        "classes_file": "brain_classes.json",
        "class_map": {
            "notumor": ("سليم — لا يوجد ورم ✅", "healthy", False),
            "glioma": ("ورم دبقي (Glioma) ⚠️", "glioma", True),
            "meningioma": ("ورم سحائي (Meningioma) ⚠️", "meningioma", True),
            "pituitary": ("ورم الغدة النخامية (Pituitary) ⚠️", "pituitary", True),
        },
    },
    "bone": {
        "label": "🦴 أشعة العظام (X-ray)",
        "model_file": "bone_model.keras",
        "classes_file": "bone_classes.json",
        "class_map": {
            "not fractured": ("سليم — لا يوجد كسر ✅", "healthy", False),
            "fractured": ("كسر في العظم 🦴", "fracture", True),
        },
    },
}

# ══════════════════════════════════════════════════════════════
# إعداد الصفحة والتصميم (RTL + ألوان طبية)
# ══════════════════════════════════════════════════════════════
st.set_page_config(
    page_title="القارئ الطبي الذكي",
    page_icon="🩺",
    layout="centered",
)

st.markdown(
    """
    <style>
    @import url('https://fonts.googleapis.com/css2?family=Cairo:wght@400;600;700;900&display=swap');

    html, body, [class*="css"], .stApp {
        direction: rtl;
        font-family: 'Cairo', 'Segoe UI', Tahoma, sans-serif;
    }
    .stApp {
        background: linear-gradient(180deg, #f0f7ff 0%, #ffffff 40%);
    }
    h1, h2, h3, h4 { text-align: right; color: #0b4f8a; }
    p, li, label, .stMarkdown { text-align: right; }

    .main-header {
        background: linear-gradient(135deg, #0b4f8a 0%, #1976d2 100%);
        color: white;
        padding: 28px;
        border-radius: 16px;
        text-align: center;
        margin-bottom: 24px;
        box-shadow: 0 4px 14px rgba(11, 79, 138, 0.25);
    }
    .main-header h1 { color: white; text-align: center; margin: 0; }
    .main-header p { color: #dbeafe; text-align: center; margin: 8px 0 0; }

    .result-card {
        background: white;
        border: 2px solid #1976d2;
        border-radius: 14px;
        padding: 20px;
        margin: 14px 0;
        box-shadow: 0 2px 10px rgba(0,0,0,0.07);
    }
    .result-healthy { border-color: #2e7d32; background: #f1f8e9; }
    .result-sick    { border-color: #c62828; background: #fff5f5; }
    .result-urgent  { border-color: #b71c1c; background: #ffebee; }

    .nutrient-card {
        background: #f7fbff;
        border-right: 5px solid #1976d2;
        border-radius: 10px;
        padding: 14px 18px;
        margin: 10px 0;
    }
    .nutrient-card h4 { margin: 0 0 6px; color: #0b4f8a; }

    .avoid-card {
        background: #fff8f0;
        border-right: 5px solid #ef6c00;
        border-radius: 10px;
        padding: 14px 18px;
        margin: 10px 0;
    }

    .disclaimer {
        background: #fff3cd;
        border: 1px solid #ffc107;
        border-radius: 10px;
        padding: 14px;
        text-align: center;
        font-weight: 700;
        color: #7a5c00;
        margin-top: 30px;
    }
    .stProgress > div > div > div > div { background-color: #1976d2; }
    </style>
    """,
    unsafe_allow_html=True,
)

# ══════════════════════════════════════════════════════════════
# تحميل الموارد (مع تخزين مؤقت)
# ══════════════════════════════════════════════════════════════
@st.cache_resource(show_spinner="⏳ جاري تحميل الموديل ...")
def load_model(model_file: str):
    import tensorflow as tf  # استيراد متأخر لتسريع فتح الصفحة

    return tf.keras.models.load_model(os.path.join(MODELS_DIR, model_file))


@st.cache_data
def load_classes(classes_file: str):
    with open(os.path.join(MODELS_DIR, classes_file), encoding="utf-8") as f:
        return json.load(f)


@st.cache_data
def load_nutrition_db():
    with open(NUTRITION_FILE, encoding="utf-8") as f:
        return json.load(f)


def predict(model, class_names, image: Image.Image):
    """إرجاع (اسم الفئة، نسبة الثقة) — يدعم الثنائي والمتعدد"""
    img = image.convert("RGB").resize(IMG_SIZE)
    arr = np.asarray(img, dtype=np.float32)[np.newaxis, ...]
    preds = model.predict(arr, verbose=0)

    if preds.shape[-1] == 1:  # تصنيف ثنائي (sigmoid)
        p = float(preds[0][0])
        idx = 1 if p >= 0.5 else 0
        confidence = p if idx == 1 else 1.0 - p
    else:  # تصنيف متعدد (softmax)
        idx = int(np.argmax(preds[0]))
        confidence = float(preds[0][idx])

    return class_names[idx], confidence


def render_nutrition_report(report: dict):
    """عرض التقرير الغذائي من قاعدة البيانات المحلية"""
    st.markdown(f"### 🥗 {report['title']}")
    st.markdown(report["summary"])

    if report.get("urgent"):
        st.error(report["advice"])
    else:
        st.info(f"💡 {report['advice']}")

    st.markdown("#### الأطعمة المفيدة ولماذا:")
    for n in report["nutrients"]:
        foods = "، ".join(n["foods"]) if n["foods"] else "—"
        st.markdown(
            f"""
            <div class="nutrient-card">
                <h4>🔹 {n['name']}</h4>
                <p><b>الفائدة:</b> {n['benefit']}</p>
                <p><b>المصادر:</b> {foods}</p>
            </div>
            """,
            unsafe_allow_html=True,
        )

    if report.get("avoid"):
        avoid_items = "".join(f"<li>{a}</li>" for a in report["avoid"])
        st.markdown(
            f"""
            <div class="avoid-card">
                <h4>🚫 تجنَّب:</h4>
                <ul>{avoid_items}</ul>
            </div>
            """,
            unsafe_allow_html=True,
        )


# ══════════════════════════════════════════════════════════════
# الواجهة الرئيسية
# ══════════════════════════════════════════════════════════════
st.markdown(
    """
    <div class="main-header">
        <h1>🩺 القارئ الطبي الذكي</h1>
        <p>تحليل أولي للصور الطبية بالذكاء الاصطناعي — يعمل محلياً 100% بدون إنترنت</p>
    </div>
    """,
    unsafe_allow_html=True,
)

scan_key = st.selectbox(
    "🔍 اختر نوع الصورة الطبية:",
    options=list(SCAN_TYPES.keys()),
    format_func=lambda k: SCAN_TYPES[k]["label"],
)
scan_cfg = SCAN_TYPES[scan_key]

model_path = os.path.join(MODELS_DIR, scan_cfg["model_file"])
if not os.path.exists(model_path):
    st.warning(
        f"⚠️ موديل هذا الفحص غير موجود بعد ({scan_cfg['model_file']}).\n\n"
        f"شغِّل أولاً:\n"
        f"1. `python download_data.py`\n"
        f"2. `python train_{scan_key}.py`"
    )
    st.stop()

uploaded = st.file_uploader(
    "📤 ارفع الصورة الطبية (JPG / PNG):",
    type=["jpg", "jpeg", "png"],
)

if uploaded is not None:
    image = Image.open(uploaded)
    col1, col2 = st.columns([1, 1])
    with col1:
        st.image(image, caption="الصورة المرفوعة", use_container_width=True)

    model = load_model(scan_cfg["model_file"])
    class_names = load_classes(scan_cfg["classes_file"])

    with st.spinner("🔬 جاري تحليل الصورة ..."):
        raw_class, confidence = predict(model, class_names, image)

    class_map = scan_cfg["class_map"]
    # مطابقة اسم الفئة (غير حساس لحالة الأحرف)
    matched = next(
        (v for k, v in class_map.items() if k.lower() == raw_class.lower()),
        (raw_class, "healthy", False),
    )
    arabic_name, nutrition_key, is_sick = matched

    with col2:
        card_class = "result-sick" if is_sick else "result-healthy"
        nutrition_db = load_nutrition_db()
        report = nutrition_db.get(nutrition_key, nutrition_db["healthy"])
        if report.get("urgent"):
            card_class = "result-urgent"

        st.markdown(
            f"""
            <div class="result-card {card_class}">
                <h3>📋 نتيجة التحليل الأولي</h3>
                <h2>{arabic_name}</h2>
            </div>
            """,
            unsafe_allow_html=True,
        )
        st.markdown(f"**نسبة الثقة: {confidence * 100:.1f}%**")
        st.progress(min(confidence, 1.0))

    st.divider()

    if is_sick:
        render_nutrition_report(report)
    else:
        st.success("✅ النتيجة الأولية سليمة — حافظ على الفحوصات الدورية ونمط حياة صحي.")
        with st.expander("🥗 نصائح عامة للحفاظ على الصحة"):
            render_nutrition_report(nutrition_db["healthy"])

# ══════════════════════════════════════════════════════════════
# التحذير الثابت
# ══════════════════════════════════════════════════════════════
st.markdown(
    """
    <div class="disclaimer">
        ⚠️ أداة مساعدة أولية للفرز، ليست بديلاً عن التشخيص الطبي المتخصص.
        راجع طبيباً مختصاً دائماً لتأكيد أي نتيجة.
    </div>
    """,
    unsafe_allow_html=True,
)
