# 📱 تحويل Script Bot إلى تطبيق أندرويد (APK) ونشره على Google Play

> ملف `script-bot.html` مكتفٍ ذاتياً ويعمل بدون إنترنت، لذا يمكن تغليفه كتطبيق أندرويد بسهولة عبر **Capacitor**.

---

## 🇸🇦 الطريقة (بالعربي)

### المتطلبات
- تثبيت [Node.js](https://nodejs.org) (نسخة 18 أو أحدث).
- تثبيت [Android Studio](https://developer.android.com/studio) (يجلب Android SDK و JDK).

### الخطوات

```bash
# 1) أنشئ مجلد مشروع جديد وادخل إليه
mkdir script-bot-app && cd script-bot-app

# 2) هيّئ مشروع Capacitor
npm init -y
npm install @capacitor/core @capacitor/cli @capacitor/android
npx cap init "Script Bot" com.scriptbot.app --web-dir=www

# 3) ضع ملف التطبيق داخل مجلد www باسم index.html
mkdir www
#   انسخ script-bot.html إلى www/index.html

# 4) أضف منصة أندرويد
npx cap add android

# 5) انسخ الملفات وافتح المشروع في Android Studio
npx cap copy
npx cap open android
```

### إنشاء APK
- داخل Android Studio: من القائمة **Build → Build Bundle(s) / APK(s) → Build APK(s)**.
- سيظهر ملف APK جاهز للتثبيت والتجربة على الهاتف.

### النشر على Google Play
1. سجّل حساب مطوّر في [Google Play Console](https://play.google.com/console) (رسوم لمرة واحدة).
2. أنشئ **App Bundle موقّع**: `Build → Generate Signed Bundle / APK → Android App Bundle` (اصنع مفتاح توقيع واحفظه جيداً).
3. أنشئ تطبيقاً جديداً في Play Console، ارفع ملف `.aab`، عبّئ الأيقونة والوصف ولقطات الشاشة، ثم أرسله للمراجعة.

> 💡 **الأيقونة:** ضع أيقونتك في `android/app/src/main/res/`. أسهل طريقة: انقر يميناً على مجلد `res` في Android Studio ثم `New → Image Asset`.

---

## 🇬🇧 Steps (English)

### Requirements
- [Node.js](https://nodejs.org) 18+
- [Android Studio](https://developer.android.com/studio) (includes Android SDK + JDK)

### Wrap the app with Capacitor

```bash
mkdir script-bot-app && cd script-bot-app
npm init -y
npm install @capacitor/core @capacitor/cli @capacitor/android
npx cap init "Script Bot" com.scriptbot.app --web-dir=www

mkdir www
# copy script-bot.html -> www/index.html

npx cap add android
npx cap copy
npx cap open android
```

### Build the APK
In Android Studio: **Build → Build Bundle(s) / APK(s) → Build APK(s)**.

### Publish on Google Play
1. Create a [Play Console](https://play.google.com/console) developer account (one-time fee).
2. Generate a **signed App Bundle**: `Build → Generate Signed Bundle / APK → Android App Bundle` (create and safely store a keystore).
3. In Play Console, create a new app, upload the `.aab`, fill in the icon/description/screenshots, and submit for review.

> 💡 **App icon:** In Android Studio right-click `res → New → Image Asset` to generate all icon sizes.

---

## ملاحظة / Note
الأكواد التي يولّدها التطبيق (بوتات تلكرام/واتساب وسكربتات) تُشغَّل على جهاز الكمبيوتر أو خادم عبر Python/Node.js — التطبيق نفسه فقط هو الذي يتحول إلى APK.

The code the app *generates* (Telegram/WhatsApp bots & scripts) runs on a computer or server via Python/Node.js — only the app itself is packaged as the APK.
