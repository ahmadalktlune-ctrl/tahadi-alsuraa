// ينسخ ملف اللعبة من جذر المشروع إلى www/index.html
// شغّله بـ: npm run copy-game   (أو تلقائياً ضمن npm run sync)
const fs = require('fs');
const path = require('path');

const SRC = path.join(__dirname, '..', 'gameSECURE_v26-1.html');
const DEST_DIR = path.join(__dirname, 'www');
const DEST = path.join(DEST_DIR, 'index.html');

if (!fs.existsSync(SRC)) {
  console.error('❌ ما لگيت ملف اللعبة:', SRC);
  process.exit(1);
}
fs.mkdirSync(DEST_DIR, { recursive: true });
fs.copyFileSync(SRC, DEST);
console.log('✅ انسخ ملف اللعبة إلى', DEST);
