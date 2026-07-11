# -*- coding: utf-8 -*-
"""
تدريب موديل أشعة الصدر (سليم / التهاب رئوي)
=============================================
الداتاسيت: paultimothymooney/chest-xray-pneumonia
الهيكل: chest_xray/train|val|test/NORMAL|PNEUMONIA

التشغيل:
    python train_chest.py
"""

from train_common import find_dir, load_dataset_root, train_and_save


def main():
    root = load_dataset_root("chest")
    train_dir = find_dir(root, ["train"])
    test_dir = find_dir(root, ["test"])

    # مجلد val في هذا الداتاسيت صغير جداً (16 صورة فقط)،
    # لذلك نستخدم مجلد test للتحقق أثناء التدريب أيضاً.
    train_and_save(
        model_key="chest",
        train_dir=train_dir,
        val_dir=test_dir,
        test_dir=test_dir,
        num_classes=2,
        epochs=8,
    )


if __name__ == "__main__":
    main()
