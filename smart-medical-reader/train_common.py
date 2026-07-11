# -*- coding: utf-8 -*-
"""
الوظائف المشتركة لتدريب الموديلات الثلاثة
==========================================
Transfer Learning باستخدام MobileNetV2 (الطبقات الأساسية مجمَّدة)
مناسب للأجهزة بدون GPU قوي:
- تجميد كامل لجسم MobileNetV2 وتدريب رأس التصنيف فقط
- Data Augmentation خفيف
- Early Stopping لإيقاف التدريب مبكراً
"""

import json
import os

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

IMG_SIZE = (224, 224)
BATCH_SIZE = 32
SEED = 42

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODELS_DIR = os.path.join(BASE_DIR, "models")
PATHS_FILE = os.path.join(BASE_DIR, "data_paths.json")


def load_dataset_root(key: str) -> str:
    """يقرأ مسار الداتاسيت المحفوظ من download_data.py"""
    if not os.path.exists(PATHS_FILE):
        raise SystemExit(
            "❌ ملف data_paths.json غير موجود.\n"
            "   شغِّل أولاً: python download_data.py"
        )
    with open(PATHS_FILE, encoding="utf-8") as f:
        paths = json.load(f)
    if key not in paths:
        raise SystemExit(f"❌ مسار الداتاسيت '{key}' غير موجود في data_paths.json")
    return paths[key]


def find_dir(root: str, target_names) -> str:
    """
    يبحث داخل مجلد الداتاسيت عن مجلد باسم معيّن (مثل train أو Training)
    لأن هيكل مجلدات Kaggle قد يحتوي مستويات إضافية.
    """
    target_lower = {t.lower() for t in target_names}
    for dirpath, dirnames, _ in os.walk(root):
        base = os.path.basename(dirpath).lower()
        if base in target_lower:
            # نتأكد أنه يحتوي مجلدات فئات (وليس مجلداً فارغاً)
            if dirnames:
                return dirpath
    raise SystemExit(
        f"❌ لم يتم العثور على مجلد {target_names} داخل: {root}\n"
        "   تحقق من اكتمال تحميل الداتاسيت."
    )


def make_datasets(train_dir: str, val_dir: str, label_mode: str):
    """تحميل بيانات التدريب والتحقق من المجلدات"""
    train_ds = keras.utils.image_dataset_from_directory(
        train_dir,
        image_size=IMG_SIZE,
        batch_size=BATCH_SIZE,
        label_mode=label_mode,
        seed=SEED,
        shuffle=True,
    )
    val_ds = keras.utils.image_dataset_from_directory(
        val_dir,
        image_size=IMG_SIZE,
        batch_size=BATCH_SIZE,
        label_mode=label_mode,
        seed=SEED,
        shuffle=False,
    )
    class_names = train_ds.class_names
    autotune = tf.data.AUTOTUNE
    train_ds = train_ds.prefetch(autotune)
    val_ds = val_ds.prefetch(autotune)
    return train_ds, val_ds, class_names


def build_model(num_classes: int) -> keras.Model:
    """
    MobileNetV2 مجمَّد + رأس تصنيف خفيف.
    num_classes == 2  → مخرج sigmoid واحد (تصنيف ثنائي)
    num_classes  > 2  → مخرج softmax متعدد
    """
    data_augmentation = keras.Sequential(
        [
            layers.RandomFlip("horizontal"),
            layers.RandomRotation(0.05),
            layers.RandomZoom(0.1),
            layers.RandomContrast(0.1),
        ],
        name="augmentation",
    )

    base = keras.applications.MobileNetV2(
        input_shape=IMG_SIZE + (3,),
        include_top=False,
        weights="imagenet",
    )
    base.trainable = False  # تجميد كامل — تدريب خفيف بدون GPU

    inputs = keras.Input(shape=IMG_SIZE + (3,))
    x = data_augmentation(inputs)
    x = keras.applications.mobilenet_v2.preprocess_input(x)
    x = base(x, training=False)
    x = layers.GlobalAveragePooling2D()(x)
    x = layers.Dropout(0.3)(x)

    if num_classes == 2:
        outputs = layers.Dense(1, activation="sigmoid")(x)
        loss = "binary_crossentropy"
    else:
        outputs = layers.Dense(num_classes, activation="softmax")(x)
        loss = "categorical_crossentropy"

    model = keras.Model(inputs, outputs)
    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=1e-3),
        loss=loss,
        metrics=["accuracy"],
    )
    return model


def train_and_save(
    model_key: str,
    train_dir: str,
    val_dir: str,
    test_dir: str,
    num_classes: int,
    epochs: int = 8,
):
    """التدريب الكامل: بناء → تدريب → تقييم → حفظ"""
    label_mode = "binary" if num_classes == 2 else "categorical"

    print(f"\n📂 بيانات التدريب : {train_dir}")
    print(f"📂 بيانات التحقق : {val_dir}")
    print(f"📂 بيانات الاختبار: {test_dir}\n")

    train_ds, val_ds, class_names = make_datasets(train_dir, val_dir, label_mode)
    print(f"🏷️  الفئات: {class_names}")

    model = build_model(num_classes)
    model.summary()

    callbacks = [
        keras.callbacks.EarlyStopping(
            monitor="val_loss",
            patience=3,
            restore_best_weights=True,
            verbose=1,
        ),
        keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss",
            factor=0.5,
            patience=2,
            min_lr=1e-6,
            verbose=1,
        ),
    ]

    print("\n🚀 بدء التدريب ...")
    model.fit(
        train_ds,
        validation_data=val_ds,
        epochs=epochs,
        callbacks=callbacks,
    )

    # ── التقييم على بيانات الاختبار ──
    test_ds = keras.utils.image_dataset_from_directory(
        test_dir,
        image_size=IMG_SIZE,
        batch_size=BATCH_SIZE,
        label_mode=label_mode,
        shuffle=False,
    )
    test_loss, test_acc = model.evaluate(test_ds, verbose=0)

    # ── الحفظ ──
    os.makedirs(MODELS_DIR, exist_ok=True)
    model_path = os.path.join(MODELS_DIR, f"{model_key}_model.keras")
    model.save(model_path)

    classes_path = os.path.join(MODELS_DIR, f"{model_key}_classes.json")
    with open(classes_path, "w", encoding="utf-8") as f:
        json.dump(class_names, f, ensure_ascii=False, indent=2)

    print("\n" + "=" * 60)
    print(f"✅ اكتمل تدريب موديل [{model_key}]")
    print(f"🎯 دقة الموديل على بيانات الاختبار: {test_acc * 100:.2f}%")
    print(f"💾 الموديل محفوظ في : {model_path}")
    print(f"💾 أسماء الفئات في : {classes_path}")
    print("=" * 60)
    return test_acc
