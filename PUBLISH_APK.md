# 📱 دليل النشر الكامل — من ملف اللعبة إلى سوق Google Play

> كل شي بالكود **جاهز ومخلّص**: حذف الحساب ✅، إخفاء أزرار الدفع ✅، ربط AdMob ✅،
> صفحة الخصوصية ✅، مشروع Capacitor ✅. الباقي خطوات تنفيذية على جهازك وحساباتك —
> اتبعها بالترتيب وما راح يبقى شي.

---

## المرحلة 0: المتطلبات (مرة واحدة)

| الأداة | من وين |
|---|---|
| Node.js 18+ | <https://nodejs.org> |
| Android Studio | <https://developer.android.com/studio> |
| حساب مطوّر Google Play | <https://play.google.com/console> — رسوم 25$ مرة واحدة |
| حساب AdMob | <https://apps.admob.com> — مجاني |

---

## المرحلة 1: بناء الـ APK (على جهازك)

```bash
cd game-app
npm install                 # يثبّت Capacitor وإضافة AdMob
npm run copy-game           # ينسخ gameSECURE_v26-1.html إلى www/index.html
npx cap add android         # ينشئ مشروع الأندرويد (أول مرة فقط)
npm run sync                # ينسخ اللعبة والإضافات لمشروع الأندرويد
npx cap open android        # يفتح Android Studio
```

### إعداد AdMob داخل مشروع الأندرويد (مرة واحدة)

افتح `game-app/android/app/src/main/AndroidManifest.xml` وأضف داخل وسم `<application>`:

```xml
<meta-data
    android:name="com.google.android.gms.ads.APPLICATION_ID"
    android:value="ca-app-pub-3940256099942544~3347511713"/>
```

> ⚠️ هذا **معرف تطبيق تجريبي** من Google. بعد إنشاء تطبيقك في AdMob بدّله
> بمعرفك الحقيقي (الصيغة `ca-app-pub-XXXX~YYYY` بعلامة **~**).

### الأيقونة وشاشة البداية

في Android Studio: كليك يمين على مجلد `res` ← **New → Image Asset** وارفع أيقونتك
(1024×1024). لشاشة البداية استخدم نفس الطريقة مع `Launch Screen`.

### تجربة أول APK

**Build → Build Bundle(s) / APK(s) → Build APK(s)** — يطلع ملف APK ثبّته على موبايلك وجرّب:
- تسجيل دخول وإنشاء حساب ✅
- إعلان المكافأة بالمتجر (راح يظهر **إعلان تجريبي** حقيقي من Google) ✅
- شراء فئة بالجواهر ✅
- حذف الحساب من الإعدادات ✅

---

## المرحلة 2: Firebase (5 دقائق — إلزامية)

1. افتح [Firebase Console](https://console.firebase.google.com) ← مشروعك ← **Firestore Database → Rules**.
2. انسخ قواعد الأمان الموجودة داخل ملف اللعبة (ابحث في الملف عن `FIRESTORE SECURITY RULES`) والصقها ← **Publish**.
   - القواعد محدّثة وتشمل السماح بحذف الحساب.
3. **Authentication → Settings → Authorized domains**: تأكد أن نطاقك موجود (للتطبيق ما يحتاج إضافة).

---

## المرحلة 3: AdMob (قبل النشر النهائي)

1. في [AdMob](https://apps.admob.com): **Apps → Add app** (اخترها "غير منشورة بعد"، أندرويد).
2. أنشئ وحدتين إعلانيتين:
   - **Rewarded** (إعلان مكافأة) → انسخ معرفها
   - **Interstitial** (إعلان بيني) → انسخ معرفها
3. افتح `gameSECURE_v26-1.html` وابحث عن `ADMOB_IDS_REAL` وحط المعرفين.
4. ابحث عن `ADMOB_TESTING` وخلّيها `false`.
5. بدّل معرف التطبيق في `AndroidManifest.xml` بمعرفك الحقيقي (خطوة المرحلة 1).
6. أعد `npm run sync` وابنِ من جديد.

> 🚫 **لا تضغط على إعلاناتك الحقيقية أبداً** ولا تخلي أصدقاءك يسوونها — Google تحظر حساب AdMob بسرعة.

---

## المرحلة 4: صفحة الخصوصية على الإنترنت (5 دقائق)

Google Play **يشترط رابط ويب** لسياسة الخصوصية. الصفحة جاهزة بالريبو: `privacy-policy.html`.

1. من صفحة الريبو على GitHub: **Settings → Pages**.
2. Source: اختر `Deploy from a branch` ← الفرع `main` ← المجلد `/ (root)` ← **Save**.
3. بعد دقيقة يصير عندك رابط بصيغة:
   `https://ahmadalktlune-ctrl.github.io/tahadi-alsuraa/privacy-policy.html`
4. هذا الرابط تحطه في Play Console (الخطوة الجاية) — وهو نفسه يغطي شرط
   "رابط حذف الحساب" لأن الصفحة تشرح طريقة الحذف من داخل التطبيق.

---

## المرحلة 5: النسخة الموقّعة للنشر (AAB)

في Android Studio:

1. **Build → Generate Signed Bundle / APK → Android App Bundle**.
2. **Create new keystore**: اختر مكان الملف وكلمات المرور.
   - 🔑 **احفظ ملف الـ keystore وكلمات المرور بمكان آمن (نسختين!)** — إذا ضاع
     ما تكدر تحدّث تطبيقك أبداً.
3. اختر `release` ← **Finish** — يطلع ملف `.aab` هذا اللي يترفع للسوق.

---

## المرحلة 6: Google Play Console — إنشاء التطبيق ونشره

1. **Create app**: الاسم "هل أنت أذكى؟"، اللغة عربية، تطبيق مجاني، لعبة.
2. ارفع ملف الـ `.aab` في **Production → Create new release**
   (أو **Internal testing** أولاً إذا تريد تجرب — أنصح فيها).
3. عبّئ **Store listing**: وصف قصير وطويل + لقطات شاشة (خذها من موبايلك) + أيقونة 512×512 + صورة Feature ‏1024×500.

### استمارة Data Safety (أجوبتها الجاهزة حسب كود اللعبة)

| السؤال | الجواب |
|---|---|
| هل يجمع التطبيق بيانات؟ | **نعم** |
| Personal info → Email address | نعم — لإدارة الحساب — مشفّرة بالنقل — **يمكن للمستخدم طلب حذفها** ✅ |
| Personal info → Name (اسم العرض) | نعم — لعرض الملف واللوحة — يمكن حذفها ✅ |
| App activity (نقاط، إحصائيات لعب) | نعم — لوظائف التطبيق — يمكن حذفها ✅ |
| Device ID / Advertising ID | نعم — **تجمعها AdMob** للإعلانات (اختر "Advertising or marketing") |
| هل البيانات تُشارك مع أطراف ثالثة؟ | لا (Firebase وAdMob مزوّدو خدمة، مو مشاركة بيع) |
| هل يوفر التطبيق حذف الحساب؟ | **نعم** — من داخل التطبيق ✅ |

### باقي النماذج

- **Privacy policy**: حط رابط GitHub Pages من المرحلة 4.
- **Account deletion**: اختر "التطبيق يتيح حذف الحساب من الداخل" وحط نفس رابط
  صفحة الخصوصية (فيها شرح الحذف).
- **Ads**: نعم، التطبيق يعرض إعلانات.
- **Content rating**: عبّئ الاستبيان (لعبة أسئلة ثقافية — بدون عنف/قمار) → غالباً تصنيف 3+.
- **Target audience**: 13+ (لأن فيه حسابات وإعلانات).
- **App access**: اختر "كل الوظائف متاحة بدون شروط خاصة" وأضف ملاحظة أن
  وضع الضيف متاح بدون حساب (المراجع يكدر يدخل ضيف).

4. **أرسل للمراجعة** — عادة من يوم إلى أسبوع.

---

## ✅ قائمة الفحص الأخيرة قبل الضغط على "نشر"

- [ ] `ADMOB_TESTING = false` ومعرفات AdMob الحقيقية بالكود
- [ ] معرف تطبيق AdMob الحقيقي بـ `AndroidManifest.xml`
- [ ] قواعد Firestore منشورة من الـ Console
- [ ] صفحة الخصوصية شغالة على GitHub Pages
- [ ] الـ keystore محفوظ بمكانين آمنين
- [ ] جربت APK على موبايل حقيقي: دخول، لعب، شراء فئة، إعلان مكافأة، حذف حساب
- [ ] versionCode يزيد مع كل تحديث ترفعه (بـ `android/app/build.gradle`)

---

## ملاحظات للمستقبل (مو شرط للنشر الأول)

- **الدفع الحقيقي 💳**: أزرار الدولار مخفية حالياً. إذا تريد تفعيلها لاحقاً:
  اربط Google Play Billing، وبالكود خلّي `ENABLE_MONEY_BUY = true` وشيل
  `display:none` من `money-bundles-subs` و `money-bundles-gems`.
- **إعلان فتح التطبيق**: إضافة AdMob الحالية ما تدعم App Open — الربح من
  المكافأة والبيني كافي حالياً.
- **حماية أقوى للجواهر**: إذا كبرت اللعبة، انقل منح الجواهر لـ Cloud Functions.
