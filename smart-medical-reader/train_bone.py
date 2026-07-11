# -*- coding: utf-8 -*-
"""
تدريب موديل أشعة العظام (سليم / كسر)
======================================
الداتاسيت: bmadushanirodrigo/fracture-multi-region-x-ray-data
الهيكل: Bone_Fracture_Binary_Classification/train|val|test/fractured|not fractured

التشغيل:
    python train_bone.py
"""

from train_common import find_dir, load_dataset_root, train_and_save


def main():
    root = load_dataset_root("bone")
    train_dir = find_dir(root, ["train"])
    val_dir = find_dir(root, ["val", "validation"])
    test_dir = find_dir(root, ["test"])

    train_and_save(
        model_key="bone",
        train_dir=train_dir,
        val_dir=val_dir,
        test_dir=test_dir,
        num_classes=2,
        epochs=8,
    )


if __name__ == "__main__":
    main()
