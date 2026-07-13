# 🧪 سيناريوهات اختبار قواعد الأمان — Rules Playground

اختبر هذه السيناريوهات بعد نشر `firestore.rules` من:
**Firebase Console ← Firestore Database ← Rules ← Rules Playground** (زر «تشغيل تجريبي/Playground» أعلى محرر القواعد).

## ⚙️ تحضير إلزامي قبل الاختبار

الـ Playground ياخذ قيمة الوثيقة **القديمة** (`resource`) من قاعدة البيانات الحقيقية.
فسوّي وثيقة اختبار حقيقية من تبويب Data بالمسار `users/test_uid_123` بهذه القيم:

```json
{
  "username": "لاعب تجريبي",
  "avatar": "🦁",
  "xp": 100, "games": 5, "wins": 2, "correct": 30, "wrong": 10,
  "joined": "2026-01-01", "history": [],
  "gems": 500,
  "unlockedCats": [],
  "welcomeGemsGiven": true,
  "dailyBonusAt": 0,
  "adDayStart": 0,
  "adDayCount": 0
}
```

وبكل سيناريو كتابة: فعّل **Authenticated** واختر Firebase UID = `test_uid_123`
(إلا إذا مكتوب غير ذلك). بالـ Playground، وثيقة الكتابة اللي تكتبها بالمحاكاة
هي **الوثيقة النهائية كاملة** — فانسخ وثيقة الاختبار كلها وغيّر الحقول المذكورة فقط.

> 💡 القيم الزمنية بالمللي ثانية. استخرج «الآن» من كونسول المتصفح بـ `Date.now()`.

---

## 1) كتابة gems مباشرة → ❌ لازم تنرفض

- العملية: `update` على `users/test_uid_123`
- غيّر: `gems: 5000` (والباقي كما هو)
- **النتيجة المتوقعة: Denied** — زيادة بلا مسار مكافأة.
- جرّب هم `gems: 501` (زيادة +1) و`gems: 1000000` → كلها **Denied**.

## 2) مكافأة إعلان صحيحة → ✅ تنقبل

- العملية: `update`
- غيّر: `gems: 550` (+50 بالضبط)، `adDayStart: <Date.now()>`، `adDayCount: 1`
- **النتيجة المتوقعة: Allowed** — أول إعلان يفتح نافذة جديدة.

## 3) مكافأة إعلان بعد تجاوز الحد اليومي → ❌ تنرفض

- حدّث وثيقة الاختبار من تبويب Data أولاً: `adDayCount: 10`، `adDayStart: <Date.now() قبل ساعة>` (مثلاً `Date.now() - 3600000`)
- العملية: `update`، غيّر: `gems: 550` (+50)، `adDayCount: 11`
- **النتيجة المتوقعة: Denied** — العداد تجاوز 10 بنفس النافذة.

## 4) تصفير عداد الإعلانات للتحايل → ❌ ينرفض

- (نفس وثيقة السيناريو 3: `adDayCount: 10`)
- العملية: `update`، غيّر: `adDayCount: 0` فقط (بدون تغيير gems)
- **النتيجة المتوقعة: Denied** — عدادات الإعلان ما تتغير إلا بمسار المكافأة نفسه.

## 5) شراء قسم بجواهر كافية → ✅ ينقبل

- رجّع وثيقة الاختبار: `gems: 500`، `unlockedCats: []`، والعدادات 0
- العملية: `update`
- غيّر: `gems: 400` و `unlockedCats: ["IQ_TEST"]` (سعر IQ = 100 بالضبط)
- **النتيجة المتوقعة: Allowed**
- جرّب هم: `gems: 50` و `unlockedCats: ["MESSI_RONALDO"]` (500-450) → **Allowed**

## 6) إضافة قسم بدون نقصان gems → ❌ ينرفض

- العملية: `update`، غيّر: `unlockedCats: ["SCIENCE"]` فقط (gems تبقى 500)
- **النتيجة المتوقعة: Denied**
- جرّب هم:
  - `unlockedCats: ["IQ_TEST"]` مع `gems: 450` (خصم 50 بدل 100) → **Denied**
  - `unlockedCats: ["IQ_TEST", "ANIME"]` مع `gems: 0` (قسمين بكتابة وحدة) → **Denied**
  - `unlockedCats: ["QQQ"]` مع أي خصم (قسم غير موجود) → **Denied**

## 7) قراءة iq_questions بدون فتح القسم → ❌ تنرفض

- العملية: `get` على `iq_questions/أي_معرف_موجود`
- Authenticated بـ `test_uid_123` والوثيقة مالته `unlockedCats: []`
- **النتيجة المتوقعة: Denied**
- بدون مصادقة (Unauthenticated) → **Denied** هم.

## 8) قراءة iq_questions بعد فتح القسم → ✅ تنقبل

- حدّث وثيقة الاختبار من Data: `unlockedCats: ["IQ_TEST"]`
- نفس قراءة السيناريو 7 → **النتيجة المتوقعة: Allowed**

## 9) حذف الحساب → ❌ ينرفض

- العملية: `delete` على `users/test_uid_123` (حتى وأنت Authenticated بنفس الـ UID)
- **النتيجة المتوقعة: Denied** — الحذف ممنوع نهائياً (يسد ثغرة إعادة هدية الترحيب).

## 10) إرجاع welcomeGemsGiven إلى false → ❌ ينرفض

- (الوثيقة عليها `welcomeGemsGiven: true`)
- العملية: `update`، غيّر: `welcomeGemsGiven: false` فقط
- **النتيجة المتوقعة: Denied**
- وبعدها محاولة `gems: 550` + `welcomeGemsGiven: true` (هدية ثانية والعلم أصلاً true) → **Denied**.

## 11) هدية الترحيب الشرعية → ✅ مرة وحدة بس

- حدّث وثيقة الاختبار من Data: `welcomeGemsGiven: false`، `gems: 0`
- العملية: `update`، غيّر: `gems: 50` و `welcomeGemsGiven: true`
- **النتيجة المتوقعة: Allowed** — وإذا أعدتها بعد ما صار العلم true → **Denied**.

## 12) المكافأة اليومية

- حدّث الوثيقة: `gems: 100`، `dailyBonusAt: 0`
- العملية: `update`، غيّر: `gems: 120` (+20) و `dailyBonusAt: <Date.now()>`
- **النتيجة المتوقعة: Allowed** (أول مرة — القيمة القديمة 0)
- بعدين حدّث الوثيقة: `dailyBonusAt: <Date.now() - 3600000>` (قبل ساعة) وأعد نفس الكتابة → **Denied** (ما مرت 20 ساعة)
- وجرّب `dailyBonusAt` جديدة بالمستقبل البعيد (مثلاً `Date.now() + 86400000`) مع +20 → **Denied** (القيمة الجديدة لازم تكون «الآن» تقريباً).

## 13) قراءة users

- `get` على `users/test_uid_123` **بدون** مصادقة → **Denied**
- نفسها Authenticated بأي UID (حتى مختلف) → **Allowed** (لوحة الصدارة تحتاجها)
- `update` على `users/test_uid_123` وأنت Authenticated بـ UID **مختلف** (مثلاً `other_uid`) → **Denied**.

## 14) إنشاء حساب جديد

- العملية: `create` على `users/new_uid_1` (Authenticated بـ `new_uid_1`)
- وثيقة نظيفة: `{"username":"جديد","avatar":"🦁","xp":0,"games":0,"wins":0,"correct":0,"wrong":0,"joined":"2026-07-13","history":[]}` → **Allowed**
- نفسها مع `"gems": 99999` → **Denied**
- نفسها مع `"unlockedCats": ["SCIENCE"]` → **Denied**
- نفسها مع `"welcomeGemsGiven": false, "gems": 0` → **Allowed**

## 15) بنك الأسئلة العام والرفض الافتراضي

- `get` على `questions/أي_معرف` بدون مصادقة → **Allowed** (مقصودة — الضيوف يلعبون)
- `create` على `questions/x` → **Denied**
- أي عملية على collection غير معرّفة (مثلاً `get` على `hacks/x`) → **Denied**

## 16) iq_stats (التقنين)

- `update` على `iq_stats/iq_normalization` بـ `count` يقفز +5 دفعة وحدة → **Denied**
- `update` بـ `count+1` لكن `sumX` يزيد 10000 دفعة وحدة → **Denied** (الحد 200 لكل نتيجة)
- `delete` عليها → **Denied**

---

## ✅ اختبار حي باللعبة نفسها (بعد نشر القواعد)

1. سجّل حساب جديد → توصلك هدية 50 + مكافأة يومية 20 (المجموع 70).
2. شاهد إعلان مكافأة → +50 وتشوف العداد (1/10).
3. اشترِ قسم IQ بـ100 → ينفتح والجواهر تنقص بالضبط.
4. العب گيم كامل واربح → الإحصائيات (xp/wins) تنحفظ طبيعي، وما أكو +20 فوز (انشالت).
5. ادخل كضيف → لوحة الصدارة تعرض «سجّل دخولك»، والأسئلة العادية تشتغل.
6. جرّب من كونسول المتصفح (وأنت مسجل):
   `fbDb.collection('users').doc(fbAuth.currentUser.uid).update({gems: 999999})`
   → لازم يطلع `permission-denied`.
