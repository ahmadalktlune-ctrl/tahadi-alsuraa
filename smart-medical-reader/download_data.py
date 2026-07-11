# -*- coding: utf-8 -*-
"""
تحميل الداتاسيتات الثلاثة من Kaggle عبر مكتبة kagglehub
==========================================================
1. أشعة الصدر  : paultimothymooney/chest-xray-pneumonia
2. صور الدماغ  : masoudnickparvar/brain-tumor-mri-dataset
3. أشعة العظام : bmadushanirodrigo/fracture-multi-region-x-ray-data

يحفظ السكربت مسارات التحميل في ملف data_paths.json
حتى تستخدمها سكربتات التدريب مباشرة.

التشغيل:
    python download_data.py
"""

import json
import os
import sys

try:
    import kagglehub
except ImportError:
    sys.exit(
        "مكتبة kagglehub غير مثبتة. ثبّتها أولاً:\n"
        "    pip install kagglehub"
    )

DATASETS = {
    "chest": "paultimothymooney/chest-xray-pneumonia",
    "brain": "masoudnickparvar/brain-tumor-mri-dataset",
    "bone": "bmadushanirodrigo/fracture-multi-region-x-ray-data",
}

PATHS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data_paths.json")


def main():
    paths = {}
    for name, dataset in DATASETS.items():
        print(f"\n{'=' * 60}")
        print(f"⬇️  جاري تحميل داتاسيت [{name}] : {dataset}")
        print("=" * 60)
        path = kagglehub.dataset_download(dataset)
        paths[name] = path
        print(f"✅ تم التحميل إلى: {path}")

    with open(PATHS_FILE, "w", encoding="utf-8") as f:
        json.dump(paths, f, ensure_ascii=False, indent=2)

    print(f"\n{'=' * 60}")
    print(f"✅ اكتمل تحميل الداتاسيتات الثلاثة.")
    print(f"📄 المسارات محفوظة في: {PATHS_FILE}")
    print("=" * 60)
    print("\nالخطوة التالية — درِّب الموديلات الثلاثة:")
    print("    python train_chest.py")
    print("    python train_brain.py")
    print("    python train_bone.py")


if __name__ == "__main__":
    main()
