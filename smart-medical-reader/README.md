# 🩺 القارئ الطبي الذكي — Smart Medical Imaging Reader

نظام ذكاء اصطناعي يعمل **100% محلياً (Offline)** بدون أي API خارجي، يحلل الصور الطبية ويعطي **تشخيصاً أولياً + توصيات غذائية بالعربي**.

## ✨ ماذا يفعل النظام؟

| نوع الفحص | الفئات |
|-----------|--------|
| 🫁 أشعة الصدر (X-ray) | سليم / التهاب رئوي |
| 🧠 صور الدماغ (MRI) | سليم / ورم دبقي / ورم سحائي / ورم الغدة النخامية |
| 🦴 أشعة العظام (X-ray) | سليم / كسر |

- **التقنية:** Transfer Learning باستخدام MobileNetV2 مع TensorFlow/Keras (خفيف — يعمل بدون GPU).
- **الواجهة:** Streamlit بالعربي الكامل واتجاه RTL.
- **التوصيات الغذائية:** قاعدة بيانات محلية `nutrition_db.json` (بدون أي API).

---

## 🚀 خطوات التشغيل بالترتيب

### 1️⃣ تثبيت المكتبات
```bash
pip install -r requirements.txt
```

### 2️⃣ تحميل الداتاسيتات (مرة واحدة فقط — يحتاج إنترنت)
```bash
python download_data.py
```
> ملاحظة: مكتبة `kagglehub` قد تطلب تسجيل دخول Kaggle أول مرة.
> ضع ملف `kaggle.json` في `~/.kaggle/` أو سجّل عبر `kagglehub.login()`.
> بعد التحميل، كل شيء يعمل بدون إنترنت.

### 3️⃣ تدريب الموديلات الثلاثة
```bash
python train_chest.py    # موديل أشعة الصدر
python train_brain.py    # موديل صور الدماغ
python train_bone.py     # موديل أشعة العظام
```
كل سكربت:
- يستخدم MobileNetV2 مجمَّد الطبقات (تدريب خفيف بدون GPU)
- Data Augmentation + Early Stopping
- يطبع **دقة الموديل على بيانات الاختبار** في النهاية
- يحفظ الموديل في `models/*.keras`

### 4️⃣ تشغيل الواجهة
```bash
streamlit run app.py
```
تفتح الواجهة تلقائياً في المتصفح على `http://localhost:8501`

---

## 📁 هيكل المشروع

```
smart-medical-reader/
├── download_data.py      # تحميل الداتاسيتات الثلاثة من Kaggle
├── train_common.py       # الكود المشترك للتدريب (MobileNetV2)
├── train_chest.py        # تدريب موديل الصدر
├── train_brain.py        # تدريب موديل الدماغ
├── train_bone.py         # تدريب موديل العظام
├── nutrition_db.json     # قاعدة البيانات الغذائية (عربي، محلية)
├── app.py                # واجهة Streamlit (عربي، RTL)
├── requirements.txt      # المكتبات المطلوبة
├── models/               # الموديلات المدربة (.keras) + أسماء الفئات
└── README.md
```

## 📊 الداتاسيتات المستخدمة (Kaggle)

1. **الصدر:** [paultimothymooney/chest-xray-pneumonia](https://www.kaggle.com/datasets/paultimothymooney/chest-xray-pneumonia)
2. **الدماغ:** [masoudnickparvar/brain-tumor-mri-dataset](https://www.kaggle.com/datasets/masoudnickparvar/brain-tumor-mri-dataset)
3. **العظام:** [bmadushanirodrigo/fracture-multi-region-x-ray-data](https://www.kaggle.com/datasets/bmadushanirodrigo/fracture-multi-region-x-ray-data)

## 🥗 التوصيات الغذائية

- **الكسور** ← كالسيوم وفيتامين D (حليب، لبن، سردين، بيض)
- **الالتهاب الرئوي** ← فيتامين C وزنك وسوائل (حمضيات، عدس، لحوم)
- **أورام الدماغ** ← توصية عاجلة بمراجعة اختصاص أورام فقط + أغذية مضادات أكسدة عامة (بدون أي ادعاءات علاجية)

## ⚠️ إخلاء مسؤولية

> **أداة مساعدة أولية للفرز، ليست بديلاً عن التشخيص الطبي المتخصص.**
> راجع طبيباً مختصاً دائماً لتأكيد أي نتيجة.
