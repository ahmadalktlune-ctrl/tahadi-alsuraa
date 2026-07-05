# 📘 دليل تشغيل البوت / السكربت — Full Run Guide

> نفس هذا الدليل متوفّر **داخل التطبيق** في تبويب «الدليل».
> This same guide is available **inside the app** under the “Guide” tab.

---

## 🇸🇦 بالعربية

كيف تحوّل الكود الذي أنشأته إلى بوت تلكرام أو واتساب يعمل فعلاً — على **اللابتوب** وعلى **الموبايل**.

### 1) تلكرام: إنشاء البوت والحصول على التوكن (مرة واحدة)
1. افتح **تلكرام** وابحث عن `@BotFather` (الرسمي بعلامة زرقاء).
2. اضغط **Start** ثم أرسل `/newbot`.
3. أرسل **اسم البوت** (أي اسم).
4. أرسل **معرّف البوت (username)** ويجب أن ينتهي بـ `bot` (مثل `my_store_bot`).
5. ستحصل على **التوكن** (سطر طويل مثل `123456789:AAH...xyz`). انسخه.
6. الصقه في خانة «توكن البوت» داخل التطبيق، ثم أنشئ الكود وحمّله.

> ⚠️ التوكن مثل كلمة السر — لا تنشره.

### 2) تلكرام على اللابتوب
**Python:**
1. ثبّت بايثون من `python.org/downloads` — على ويندوز علّم **«Add Python to PATH»**.
2. افتح نافذة الأوامر (cmd / Terminal).
3. ثبّت المكتبة: `pip install python-telegram-bot`
4. احفظ الكود باسم `bot.py`.
5. شغّله: `python bot.py`
6. لما تشوف `...is running` صار البوت شغّال — كلّمه بتلكرام وأرسل `/start`.

**JavaScript:**
1. ثبّت **Node.js** (LTS) من `nodejs.org`.
2. `npm install node-telegram-bot-api`
3. احفظ باسم `bot.js` ثم `node bot.js`

> 💡 البوت يبقى شغّالاً ما دامت النافذة مفتوحة. للتشغيل الدائم شوف القسم 7.

### 3) تلكرام على الموبايل (أندرويد)
**الأسهل — Pydroid 3 (Python):**
1. ثبّت **Pydroid 3** من متجر Play.
2. من القائمة اختر **Pip** واكتب `python-telegram-bot` واضغط Install.
3. الصق الكود في المحرّر واضغط **▶️**.

**متقدّم — Termux (Python أو JS):**
1. ثبّت **Termux** من **F-Droid**، ثم `pkg update`.
2. Python: `pkg install python` ثم `pip install python-telegram-bot`
3. JS: `pkg install nodejs` ثم `npm install node-telegram-bot-api`
4. شغّل: `python bot.py` أو `node bot.js`

> 📱 آيفون: صعب — استعمل لابتوب أو استضافة (القسم 7).

### 4) واتساب على اللابتوب (JavaScript فقط)
1. ثبّت **Node.js** من `nodejs.org`.
2. `npm install whatsapp-web.js qrcode-terminal`
3. احفظ باسم `bot.js` ثم `node bot.js`
4. سيظهر **رمز QR** في النافذة.
5. على الهاتف: **واتساب ← الإعدادات ← الأجهزة المرتبطة ← ربط جهاز** ثم امسح الرمز.
6. يتصل البوت ويبدأ الرد. اتركه شغّالاً.

> 💡 استعمل رقماً مخصّصاً للبوت وتجنّب الإرسال الجماعي حتى لا يُحظر.

### 5) واتساب على الموبايل (متقدّم)
1. **Termux** من F-Droid، ثم `pkg update`.
2. `pkg install nodejs`
3. `pkg install chromium`
4. `npm install whatsapp-web.js qrcode-terminal`
5. `node bot.js` وامسح رمز الـ QR.

> ⚠️ يستهلك بطارية وذاكرة — الأفضل اللابتوب أو الاستضافة.

### 6) تشغيل السكربتات
1. ثبّت بايثون أو Node.js.
2. ثبّت المكتبات المذكورة في صفحة الشرح (بعضها لا يحتاج).
3. شغّل: `python script.py` أو `node script.js`
4. سكربتات بايثون البسيطة تُجرّب فوراً داخل التطبيق بزر «تشغيل (Python)».

### 7) تشغيل دائم 24/7
عند إغلاق النافذة يتوقف البوت. لجعله دائماً:
- **Railway** / **Render**: ترفع ملفاتك وتشتغل سحابياً (للمبتدئين).
- **Replit**: كود مباشر في المتصفح.
- **VPS** (Hetzner / DigitalOcean): تحكّم كامل برسوم بسيطة.

ارفع ملف الكود + ملف المكتبات (`requirements.txt` لبايثون أو `package.json` لـ JS)، واضبط أمر التشغيل `python bot.py` أو `node bot.js`.

### 8) حل المشاكل الشائعة
- **«python»/«node» غير معروف:** لم يُثبّت أو نسيت «Add to PATH». أعد التثبيت وأعد فتح النافذة.
- **ModuleNotFoundError / Cannot find module:** نفّذ أمر `pip install` أو `npm install`.
- **البوت لا يرد:** النافذة مفتوحة؟ التوكن صحيح وكامل؟
- **Unauthorized:** انسخ التوكن من جديد من BotFather.
- **QR انتهى (واتساب):** أعد التشغيل وامسح بسرعة.
- **ffmpeg غير موجود (بوت التحميل):** ثبّته (ويندوز: ffmpeg.org، Termux: `pkg install ffmpeg`، ماك: `brew install ffmpeg`).

> ✅ ابدأ ببوت بسيط على اللابتوب، وبعدها انتقل للاستضافة والمميزات الأكبر.

---

## 🇬🇧 English

How to turn the generated code into a real Telegram or WhatsApp bot — on **laptop** and **phone**.

### 1) Telegram: create the bot & get the token (once)
1. Open **Telegram**, search `@BotFather` (official, blue check).
2. Press **Start**, send `/newbot`.
3. Send a **bot name** (anything).
4. Send a **username** ending in `bot` (e.g. `my_store_bot`).
5. You get a **token** (`123456789:AAH...xyz`). Copy it.
6. Paste it into the “Bot token” field in the app, then generate & download the code.

> ⚠️ The token is a password — never share it.

### 2) Telegram on a laptop
**Python:**
1. Install Python from `python.org/downloads` — on Windows tick **“Add Python to PATH”**.
2. Open a terminal (cmd / Terminal).
3. `pip install python-telegram-bot`
4. Save the code as `bot.py`.
5. `python bot.py`
6. When you see `...is running`, message the bot and send `/start`.

**JavaScript:**
1. Install **Node.js** (LTS) from `nodejs.org`.
2. `npm install node-telegram-bot-api`
3. Save as `bot.js`, then `node bot.js`

> 💡 The bot runs while the window is open. See section 7 for always-on.

### 3) Telegram on mobile (Android)
**Easiest — Pydroid 3 (Python):**
1. Install **Pydroid 3** from Play Store.
2. Menu → **Pip** → type `python-telegram-bot` → Install.
3. Paste the code, press **▶️**.

**Advanced — Termux (Python or JS):**
1. Install **Termux** from **F-Droid**, then `pkg update`.
2. Python: `pkg install python` then `pip install python-telegram-bot`
3. JS: `pkg install nodejs` then `npm install node-telegram-bot-api`
4. Run `python bot.py` or `node bot.js`

> 📱 iPhone: hard — use a laptop or hosting (section 7).

### 4) WhatsApp on a laptop (JavaScript only)
1. Install **Node.js** from `nodejs.org`.
2. `npm install whatsapp-web.js qrcode-terminal`
3. Save as `bot.js`, then `node bot.js`
4. A **QR code** appears in the terminal.
5. On your phone: **WhatsApp → Settings → Linked Devices → Link a Device**, scan the QR.
6. The bot connects and replies. Keep it running.

> 💡 Use a dedicated number and avoid spammy bulk sending.

### 5) WhatsApp on mobile (advanced)
1. **Termux** from F-Droid, `pkg update`.
2. `pkg install nodejs`
3. `pkg install chromium`
4. `npm install whatsapp-web.js qrcode-terminal`
5. `node bot.js`, scan the QR.

> ⚠️ Heavy on battery/memory — laptop or hosting is better.

### 6) Running scripts
1. Install Python or Node.js.
2. Install any libraries from the explanation page (some need none).
3. `python script.py` or `node script.js`
4. Simple Python scripts also run instantly in the app via “Run (Python)”.

### 7) Keeping it running 24/7
The bot stops when you close the window. To keep it always on:
- **Railway** / **Render**: upload files, runs in the cloud (beginner-friendly).
- **Replit**: code and run in the browser.
- **VPS** (Hetzner / DigitalOcean): full control, small monthly fee.

Upload the code + a dependencies file (`requirements.txt` for Python or `package.json` for JS), set the start command `python bot.py` or `node bot.js`.

### 8) Troubleshooting
- **“python”/“node” not recognized:** not installed / skipped “Add to PATH”. Reinstall, reopen terminal.
- **ModuleNotFoundError / Cannot find module:** run the `pip install` / `npm install` command.
- **Bot doesn’t reply:** window still open? token correct and complete?
- **Unauthorized:** re-copy the token from BotFather.
- **QR expired (WhatsApp):** restart and scan quickly.
- **ffmpeg not found (downloader bot):** install ffmpeg (Windows: ffmpeg.org, Termux: `pkg install ffmpeg`, Mac: `brew install ffmpeg`).

> ✅ Start with a simple bot on a laptop, then move to hosting and bigger features.
