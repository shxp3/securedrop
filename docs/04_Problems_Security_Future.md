# 4. ปัญหาที่อาจเจอ, มาตรการความปลอดภัย และแนวทางพัฒนาต่อ

## 4.1 ปัญหาที่อาจเจอและวิธีแก้ (Problems & Solutions)

| # | ปัญหา | สาเหตุ | วิธีแก้ |
|---|---|---|---|
| 1 | ESP32-CAM รีสตาร์ทเองขณะถ่ายภาพ (Brownout) | Current spike เกิน Buck Converter จ่ายไหว, สาย USB/jumper ต้านทานสูง | เพิ่ม capacitor 470-1000µF ที่ขา 5V/GND, ใช้สายไฟหน้าตัดใหญ่ขึ้น, แยก Buck Converter เฉพาะกล้อง |
| 2 | บาร์โค้ดอ่านไม่ติด/อ่านผิด | แสงสะท้อน, มุมสแกนไม่ตรง, GM65 ตั้งโหมดผิด (USB-HID แทน UART) | ตรวจสอบ DIP switch/คำสั่ง config ของ GM65 ให้เป็น UART mode, เพิ่มไฟส่องช่วย, ทำ retry 3 ครั้งก่อน reject |
| 3 | WiFi หลุดขณะกำลังส่งข้อมูล Firebase | สัญญาณอ่อน, router ไกลจากกล่องพัสดุ (ติดตั้งหน้าบ้าน) | Cache ข้อมูลใน SPIFFS/SD แล้ว retry queue เมื่อเน็ตกลับมา, ใช้เสาอากาศภายนอกสำหรับ ESP32-CAM |
| 4 | UV-C ค้างเปิดเกินเวลาเพราะโปรแกรมค้าง | Loop บั๊ก/deadlock ใน state machine | ใช้ Hardware Timer Interrupt แยกต่างหากตั้ง hard cutoff ที่ 5 นาที + Hardware Interlock ทางกายภาพกับ door sensor (ไม่พึ่งซอฟต์แวร์อย่างเดียว) |
| 5 | เซนเซอร์ HC-SR04 อ่านค่าคลาดเคลื่อนจากวัสดุพัสดุ (ดูดซับเสียง เช่น กล่องโฟม/ผ้า) | คุณสมบัติวัสดุสะท้อนเสียงอัลตราโซนิกต่าง ๆ กัน | ใช้ Sensor Fusion ร่วมกับ Load Cell (น้ำหนัก) เพื่อ cross-validate, เก็บค่าเฉลี่ยจากหลาย sample แทนค่าเดียว |
| 6 | False Alarm จากแมลง/ลมแรง/รถผ่าน (SW-420/MPU6050 ไวเกิน) | Threshold ตั้งต่ำเกินไป | ใช้ Debounce Time (300-500ms), ต้องตรวจพบเกิน threshold ต่อเนื่องหลาย sample ก่อน trigger จริง (Moving Average / Hysteresis) |
| 7 | Solenoid ค้าง ไม่ล็อก/ไม่ปลดล็อก | เพาเวอร์ไม่พอ, กลไกฝืด, Relay เสีย | เพิ่ม feedback sensor (limit switch) ยืนยันตำแหน่งจริงของโบลต์ ไม่ใช่เชื่อคำสั่งซอฟต์แวร์อย่างเดียว |
| 8 | Barcode ถูกใช้ซ้ำ (คนพยายาม scan tracking number เดิม) | ระบบไม่ตรวจ `used` flag | ตรวจสอบ `used == true` ก่อนปลดล็อกเสมอ + ใช้ Firebase Transaction (atomic update) ป้องกัน race condition กรณีสองอุปกรณ์ตรวจพร้อมกัน |
| 9 | Timeout ระหว่างรอคนส่งวางพัสดุ (ค้างสถานะ UNLOCKED) | คนส่งเดินจากไปโดยไม่ปิดประตู | ตั้ง timeout 60-90 วิ หากไม่มี event ปิดประตู ให้เตือนเสียงและกลับ state ปลอดภัย (auto-relock + แจ้งเตือน "unattended open door") |
| 10 | เวลาไม่ตรง (timestamp ผิด) | ESP32 ไม่มี RTC จริง อาศัย NTP | เรียก `configTime()` sync NTP ตอน boot ก่อนเริ่มทำงานจริง, ถ้า sync ไม่ได้ให้ fallback ใช้ millis() + เวลาที่ได้จาก Firebase server timestamp |
| 11 | รูปภาพไม่อัปโหลด (Storage error) | ขนาดไฟล์ใหญ่เกิน bandwidth, Token หมดอายุ | ลด resolution กล้องเป็น VGA/SVGA พอสำหรับหลักฐาน, ใช้ Token Refresh อัตโนมัติของ Firebase library |

## 4.2 มาตรการความปลอดภัย (Security Improvements)

### 4.2.1 ความปลอดภัยของระบบ IoT (Cybersecurity)

| มาตรการ | รายละเอียด |
|---|---|
| **TLS/HTTPS ทุกการเชื่อมต่อ** | Firebase และ Telegram API ใช้ HTTPS อยู่แล้ว ต้องตรวจสอบ Certificate Fingerprint ไม่ปิด SSL verification เพื่อความสะดวก |
| **ไม่ Hardcode Secret ในซอร์สโค้ดที่แชร์สาธารณะ** | แยกไฟล์ `secrets.h` ออกจาก `config.h`, ใส่ `secrets.h` ใน `.gitignore` |
| **Firebase Security Rules** | จำกัดสิทธิ์เขียน `theft_events`/`parcels` เฉพาะ device token ที่ auth แล้ว ไม่ให้ public write ได้ |
| **Rate Limiting การสแกนบาร์โค้ด** | จำกัดจำนวนครั้งสแกนผิดติดต่อกัน (เช่น 5 ครั้ง/5 นาที) ป้องกัน brute-force เดา tracking number |
| **Telegram Command Authentication** | คำสั่งฉุกเฉิน เช่น `/openbox` ต้องยืนยัน PIN หรือจำกัดเฉพาะ `chat_id` ที่ลงทะเบียนไว้เท่านั้น |
| **OTA Update ปลอดภัย** | ใช้ ArduinoOTA พร้อมรหัสผ่าน + ตรวจสอบลายเซ็นดิจิทัลของไฟล์ firmware ก่อนติดตั้ง |
| **Watchdog Timer (Software + Hardware)** | ป้องกันระบบค้างจาก DoS หรือบั๊ก แล้วรีสตาร์ทเข้าสู่สถานะปลอดภัย (Fail-Secure Locked) |

### 4.2.2 ความปลอดภัยทางกายภาพ (Physical Security)

| มาตรการ | รายละเอียด |
|---|---|
| **Fail-Secure Lock** | ไฟดับ = ล็อกอยู่ (ไม่ใช่ Fail-Safe ที่ปลดล็อกเมื่อไฟดับ) |
| **Tamper Switch เพิ่มเติม** | ติดสวิตช์ตรวจจับการเปิดฝาตัวเครื่อง (case tamper) แยกจาก door sensor หลัก |
| **สายไฟ/สาย Sensor ซ่อนภายในโครง** | ป้องกันการตัดสายจากภายนอกโดยตรง |
| **ตัวถังโลหะกันงัด** | เพิ่มความแข็งแรงทางกล ทนแรงงัด/เจาะ |
| **แบตเตอรี่สำรอง (UPS)** | ให้ระบบเฝ้าระวัง+ไซเรนทำงานต่อได้ 10-30 นาทีแม้ตัดไฟหลัก |

### 4.2.3 ความปลอดภัยจาก UV-C (Safety, ไม่ใช่ Security แต่สำคัญเทียบเท่า)

- **Hard Interlock ทางฮาร์ดแวร์**: ต่อ Magnetic Door Sensor อนุกรมกับวงจรไฟเลี้ยงจริงของ UV Relay ไม่ใช่พึ่งซอฟต์แวร์ตรวจสอบอย่างเดียว (Defense in Depth ตามหลัก Safety-Critical System)
- ติดฉลากเตือนอันตราย UV-C ที่ตัวกล่อง
- ออกแบบช่อง UV ให้ไม่มีแสงรั่วออกสู่ผู้ใช้แม้ประตูปิดไม่สนิท (ใช้ยางขอบกันแสงรั่ว)

## 4.3 แนวทางพัฒนาต่อยอด (Future Improvements)

| แนวทาง | รายละเอียด |
|---|---|
| **AI/Computer Vision บนขอบ (Edge AI)** | ใช้ ESP32-CAM + TensorFlow Lite Micro ตรวจจับใบหน้าคนส่งเทียบกับฐานข้อมูลคนส่งที่เคยมาส่ง (Courier Verification) เพิ่มความปลอดภัยอีกชั้น |
| **แอปมือถือเฉพาะ (Native App)** | แทนที่ Telegram Bot ด้วยแอป Flutter/React Native ที่เชื่อม Firebase โดยตรง มี UI ที่ดีกว่า |
| **รองรับพัสดุหลายชิ้นพร้อมกัน (Multi-Compartment)** | ออกแบบกล่องเป็นช่อง ๆ แยกล็อกอิสระ รองรับพัสดุจากหลายผู้ให้บริการพร้อมกัน |
| **Solar Power + Battery** | ทำให้ติดตั้งได้แม้ไม่มีปลั๊กไฟใกล้ตำแหน่งวางกล่อง |
| **Machine Learning สำหรับ Anomaly Detection** | เทรนโมเดลจากข้อมูล MPU6050/SW-420 จริงเพื่อแยกแยะ "สั่นจากลม" กับ "สั่นจากการงัด" แม่นยำกว่า threshold คงที่ |
| **Integration กับ Smart Home (Google Home/Home Assistant)** | แจ้งเตือนผ่านลำโพงอัจฉริยะ, เปิดกล้องวงจรปิดที่มีอยู่แล้วโดยอัตโนมัติ |
| **QR Code แทน/เสริม Barcode** | รองรับ tracking number ที่ซับซ้อนขึ้น หรือแนบข้อมูลเพิ่มเติม (courier ID, expected delivery window) |
| **ระบบยืนยันตัวตนคนส่งแบบ 2 ปัจจัย** | บาร์โค้ด + OTP ที่ส่งให้คนส่งผ่านแอปขนส่ง เพื่อป้องกันการปลอมบาร์โค้ด |
| **Dashboard วิเคราะห์ข้อมูล** | สร้าง Web Dashboard (เช่น ใช้ Firebase + Chart.js) แสดงสถิติการใช้งาน, ความถี่ false alarm, เวลาตอบสนองเฉลี่ย |
