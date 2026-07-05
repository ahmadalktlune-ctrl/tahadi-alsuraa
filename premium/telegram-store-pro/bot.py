# ==========================================================
#  🛒 Telegram Store Bot — PRO
#  متجر تلكرام احترافي: أقسام، سلة، إتمام طلب، إشعار الإدارة،
#  حفظ الطلبات، وأوامر إدارية.
#  ----------------------------------------------------------
#  التشغيل:
#    pip install python-telegram-bot
#    python bot.py
# ==========================================================

import json
import os
import logging
from datetime import datetime

from telegram import Update, InlineKeyboardButton, InlineKeyboardMarkup
from telegram.ext import (
    Application, CommandHandler, CallbackQueryHandler,
    MessageHandler, filters, ContextTypes,
)

import config

logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    level=logging.INFO,
)
logger = logging.getLogger("store-bot")

ORDERS_FILE = "orders.json"
CATEGORIES = list(config.PRODUCTS.keys())


# ----------------------- helpers -----------------------
def money(amount: int) -> str:
    return f"{amount:,} {config.CURRENCY}"


def products_of(ci: int):
    """returns list of (name, price) for a category index"""
    return list(config.PRODUCTS[CATEGORIES[ci]].items())


def load_orders():
    if os.path.exists(ORDERS_FILE):
        try:
            with open(ORDERS_FILE, encoding="utf-8") as f:
                return json.load(f)
        except (json.JSONDecodeError, OSError):
            return []
    return []


def save_order(order: dict):
    orders = load_orders()
    orders.append(order)
    with open(ORDERS_FILE, "w", encoding="utf-8") as f:
        json.dump(orders, f, ensure_ascii=False, indent=2)


def main_menu_keyboard():
    rows = [[InlineKeyboardButton(cat, callback_data=f"c|{i}")]
            for i, cat in enumerate(CATEGORIES)]
    rows.append([
        InlineKeyboardButton("🧺 سلّتي", callback_data="cart"),
        InlineKeyboardButton("✅ إتمام الطلب", callback_data="co"),
    ])
    return InlineKeyboardMarkup(rows)


def cart_total(cart):
    return sum(item["price"] for item in cart)


# ----------------------- handlers -----------------------
async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    context.user_data.setdefault("cart", [])
    text = config.WELCOME.format(store=config.STORE_NAME)
    if update.message:
        await update.message.reply_text(text, reply_markup=main_menu_keyboard())
    else:
        await update.callback_query.edit_message_text(text, reply_markup=main_menu_keyboard())


async def on_callback(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    data = query.data
    cart = context.user_data.setdefault("cart", [])

    if data == "menu":
        await start(update, context)

    elif data.startswith("c|"):
        ci = int(data.split("|")[1])
        buttons = [
            [InlineKeyboardButton(f"{name} — {money(price)}", callback_data=f"p|{ci}|{pi}")]
            for pi, (name, price) in enumerate(products_of(ci))
        ]
        buttons.append([InlineKeyboardButton("⬅️ رجوع", callback_data="menu")])
        await query.edit_message_text(
            f"📂 {CATEGORIES[ci]}\nاختر منتجاً لإضافته إلى السلة:",
            reply_markup=InlineKeyboardMarkup(buttons),
        )

    elif data.startswith("p|"):
        _, ci, pi = data.split("|")
        name, price = products_of(int(ci))[int(pi)]
        cart.append({"name": name, "price": price})
        await query.answer(f"✅ أُضيف: {name}", show_alert=False)
        await query.edit_message_text(
            f"✅ تمت إضافة «{name}» إلى سلّتك.\n🧺 عناصر السلة: {len(cart)}\n💰 الإجمالي: {money(cart_total(cart))}",
            reply_markup=main_menu_keyboard(),
        )

    elif data == "cart":
        if not cart:
            await query.edit_message_text("🧺 سلّتك فارغة.", reply_markup=main_menu_keyboard())
            return
        lines = [f"• {i['name']} — {money(i['price'])}" for i in cart]
        text = "🧺 سلّتك:\n" + "\n".join(lines) + f"\n\n💰 الإجمالي: {money(cart_total(cart))}"
        kb = InlineKeyboardMarkup([
            [InlineKeyboardButton("✅ إتمام الطلب", callback_data="co")],
            [InlineKeyboardButton("🗑️ تفريغ السلة", callback_data="clear")],
            [InlineKeyboardButton("⬅️ رجوع", callback_data="menu")],
        ])
        await query.edit_message_text(text, reply_markup=kb)

    elif data == "clear":
        cart.clear()
        await query.edit_message_text("🗑️ تم تفريغ السلة.", reply_markup=main_menu_keyboard())

    elif data == "co":
        if not cart:
            await query.edit_message_text("🧺 سلّتك فارغة، أضف منتجات أولاً.", reply_markup=main_menu_keyboard())
            return
        context.user_data["awaiting"] = "phone"
        await query.edit_message_text(
            f"📞 لإتمام الطلب أرسل رقم هاتفك وعنوانك في رسالة واحدة.\n\n"
            f"💰 إجمالي طلبك: {money(cart_total(cart))}"
        )


async def on_text(update: Update, context: ContextTypes.DEFAULT_TYPE):
    if context.user_data.get("awaiting") != "phone":
        return
    cart = context.user_data.get("cart", [])
    if not cart:
        context.user_data["awaiting"] = None
        return
    contact = update.message.text.strip()
    user = update.effective_user
    order = {
        "id": int(datetime.now().timestamp()),
        "date": datetime.now().strftime("%Y-%m-%d %H:%M"),
        "customer": user.full_name,
        "username": user.username or "",
        "contact": contact,
        "items": cart.copy(),
        "total": cart_total(cart),
    }
    save_order(order)

    # notify admin
    items_txt = "\n".join(f"• {i['name']} — {money(i['price'])}" for i in order["items"])
    admin_msg = (
        f"🔔 طلب جديد #{order['id']}\n"
        f"👤 {order['customer']} (@{order['username']})\n"
        f"📞 {order['contact']}\n\n{items_txt}\n\n💰 الإجمالي: {money(order['total'])}"
    )
    try:
        await context.bot.send_message(config.ADMIN_ID, admin_msg)
    except Exception as e:
        logger.warning("failed to notify admin: %s", e)

    context.user_data["awaiting"] = None
    context.user_data["cart"] = []
    await update.message.reply_text(
        f"✅ تم استلام طلبك #{order['id']} بنجاح!\nسنتواصل معك قريباً. شكراً لتسوّقك من {config.STORE_NAME} 🌟",
        reply_markup=main_menu_keyboard(),
    )


async def orders_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """أمر إداري: عرض آخر الطلبات"""
    if update.effective_user.id != config.ADMIN_ID:
        return
    orders = load_orders()
    if not orders:
        await update.message.reply_text("لا توجد طلبات بعد.")
        return
    last = orders[-10:]
    lines = [
        f"#{o['id']} — {o['customer']} — {money(o['total'])} — {o['date']}"
        for o in reversed(last)
    ]
    await update.message.reply_text("🧾 آخر الطلبات:\n" + "\n".join(lines))


async def stats_cmd(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """أمر إداري: إحصائيات"""
    if update.effective_user.id != config.ADMIN_ID:
        return
    orders = load_orders()
    revenue = sum(o["total"] for o in orders)
    await update.message.reply_text(
        f"📊 الإحصائيات:\n• عدد الطلبات: {len(orders)}\n• إجمالي المبيعات: {money(revenue)}"
    )


def main():
    if config.TOKEN.startswith("PASTE"):
        raise SystemExit("⚠️ ضع توكن البوت في config.py أولاً.")
    app = Application.builder().token(config.TOKEN).build()
    app.add_handler(CommandHandler("start", start))
    app.add_handler(CommandHandler("orders", orders_cmd))
    app.add_handler(CommandHandler("stats", stats_cmd))
    app.add_handler(CallbackQueryHandler(on_callback))
    app.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, on_text))
    logger.info("Store bot PRO is running...")
    app.run_polling()


if __name__ == "__main__":
    main()
