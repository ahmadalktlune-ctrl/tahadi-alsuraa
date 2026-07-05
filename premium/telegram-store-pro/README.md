# 🛒 Telegram Store Bot — PRO

بوت متجر تلكرام احترافي جاهز للبيع والتشغيل. / A production-ready Telegram store bot.

## المميزات / Features
- أقسام ومنتجات مع أسعار / Categories & products with prices
- سلة شراء لكل مستخدم / Per-user shopping cart
- إتمام الطلب وجمع بيانات العميل / Checkout with customer details
- إشعار الإدارة بكل طلب جديد / Admin notification on each order
- حفظ الطلبات في ملف `orders.json` / Persists orders to `orders.json`
- أوامر إدارية: `/orders` و `/stats` / Admin commands
- ملف إعدادات منفصل `config.py` / Separate config file

## التشغيل / Run
```bash
pip install -r requirements.txt
# عدّل config.py (التوكن، اسم المتجر، ADMIN_ID، المنتجات)
python bot.py
```

## الإعداد / Setup
1. أنشئ بوتاً من `@BotFather` وانسخ التوكن إلى `config.py`.
2. احصل على معرّفك الرقمي من `@userinfobot` وضعه في `ADMIN_ID`.
3. عدّل قائمة `PRODUCTS` بمنتجاتك وأسعارك.
4. شغّل البوت، وستصلك الطلبات على حسابك.

> 💡 لإبقائه شغّالاً 24/7 ارفعه على Railway / Render / VPS.
