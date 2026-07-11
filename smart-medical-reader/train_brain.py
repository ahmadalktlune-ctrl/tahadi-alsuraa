# -*- coding: utf-8 -*-
"""
تدريب موديل صور الدماغ (MRI)
==============================
الفئات: glioma / meningioma / notumor / pituitary
الداتاسيت: masoudnickparvar/brain-tumor-mri-dataset
الهيكل: Training|Testing / glioma|meningioma|notumor|pituitary

التشغيل:
    python train_brain.py
"""

from train_common import find_dir, load_dataset_root, train_and_save


def main():
    root = load_dataset_root("brain")
    train_dir = find_dir(root, ["Training", "train"])
    test_dir = find_dir(root, ["Testing", "test"])

    train_and_save(
        model_key="brain",
        train_dir=train_dir,
        val_dir=test_dir,
        test_dir=test_dir,
        num_classes=4,
        epochs=8,
    )


if __name__ == "__main__":
    main()
