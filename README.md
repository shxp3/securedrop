# 📦 Parcel Guardian AI
### กล่องพัสดุอัจฉริยะป้องกันการโจรกรรม พร้อมระบบยืนยันตัวตนพัสดุและฆ่าเชื้อ UV-C

โครงงาน STEAM ระดับมัธยมปลาย (ม.6) ที่ออกแบบในระดับ "ผลิตภัณฑ์เชิงพาณิชย์จริง" โดยใช้ฮาร์ดแวร์ราคาประหยัดที่หาซื้อได้ทั่วไป (ESP32 Ecosystem) แต่ยึดหลักวิศวกรรมฝังตัว (Embedded Systems Engineering) และสถาปัตยกรรม IoT ที่ถูกต้องตามหลักวิชาการ

---

## 1. ภาพรวมโครงการ (Executive Summary)

**Parcel Guardian AI** คือกล่องรับพัสดุอัจฉริยะที่แก้ปัญหา 3 เรื่องหลักที่ผู้บริโภคเจอในชีวิตจริง:

| ปัญหา | วิธีแก้ของ Parcel Guardian AI |
|---|---|
| พัสดุถูกขโมยหน้าบ้าน | ระบบล็อกอัตโนมัติ + เซนเซอร์ตรวจจับการงัด/ยก/เอียง + ไซเรน |
| ใครก็เปิดกล่องได้ ไม่มีการยืนยันตัวตนพัสดุ | สแกนบาร์โค้ด + ตรวจสอบกับฐานข้อมูลก่อนปลดล็อก |
| พัสดุอาจปนเปื้อนเชื้อโรค/ไวรัส | ฆ่าเชื้อด้วย UV-C อัตโนมัติหลังปิดประตู |
| เจ้าของไม่รู้ว่าพัสดุมาส่งแล้วหรือมีคนพยายามขโมย | แจ้งเตือนผ่าน Telegram Bot พร้อมรูปภาพหลักฐาน |

ระบบถูกออกแบบเป็น **Distributed Embedded System** ที่มี 3 โหนดไมโครคอนโทรลเลอร์ทำงานร่วมกัน (Main Controller + กล้อง 2 ตัว) สื่อสารกันผ่านโปรโตคอลที่กำหนดเอง และเชื่อมต่อ Cloud Backend (Firebase) เป็นแหล่งความจริงเดียว (Single Source of Truth)

## 2. เอกสารประกอบโครงการ

| ลำดับ | เอกสาร | เนื้อหา |
|---|---|---|
| 1 | [`docs/01_Architecture_and_Flow.md`](docs/01_Architecture_and_Flow.md) | สถาปัตยกรรมระบบ, System Flow, Communication Protocol, State Machine, Flowchart |
| 2 | [`docs/02_Hardware_and_Wiring.md`](docs/02_Hardware_and_Wiring.md) | Pin Assignment, Wiring Diagram, รายการฮาร์ดแวร์ + ข้อเสนอแนะปรับปรุง |
| 3 | [`docs/03_Database_and_Software_Structure.md`](docs/03_Database_and_Software_Structure.md) | โครงสร้างฐานข้อมูล Firebase, โครงสร้างโฟลเดอร์โปรเจกต์, Libraries ที่ใช้ |
| 4 | [`docs/04_Problems_Security_Future.md`](docs/04_Problems_Security_Future.md) | ปัญหาที่อาจเจอ+วิธีแก้, มาตรการความปลอดภัย, แนวทางพัฒนาต่อ |
| 5 | [`docs/05_Cost_and_Roadmap.md`](docs/05_Cost_and_Roadmap.md) | ประมาณการต้นทุน BOM, แผนพัฒนาโครงการ (Roadmap) |
| 6 | [`docs/06_Testing_and_Demo_Script.md`](docs/06_Testing_and_Demo_Script.md) | แผนทดสอบระบบ และสคริปต์สาธิตหน้ากรรมการ |

## 3. โครงสร้างโค้ด (Firmware)

```
firmware/
├── main_controller/     ← ESP32 DevKit V1 (สมองหลักของระบบ)
├── esp32cam_courier/    ← ESP32-CAM ตัวที่ 1 (ถ่ายรูปหน้าคนส่ง)
└── esp32cam_internal/   ← ESP32-CAM ตัวที่ 2 (ถ่ายรูปพัสดุในกล่อง)
```

ดูรายละเอียดการต่อวงจร ไลบรารีที่ต้องติดตั้ง และวิธี build ใน PlatformIO ได้ที่ [`docs/03_Database_and_Software_Structure.md`](docs/03_Database_and_Software_Structure.md)

## 4. Quick Start

1. ติดตั้ง [PlatformIO](https://platformio.org/) ใน VS Code
2. เปิดโฟลเดอร์ `firmware/main_controller/` เป็น PlatformIO Project
3. คัดลอก `include/secrets.h.example` เป็น `include/secrets.h` แล้วกรอก WiFi / Firebase / Telegram Token ของตัวเอง
4. ต่อวงจรตาม `docs/02_Hardware_and_Wiring.md`
5. Build & Upload firmware ทั้ง 3 บอร์ด
6. เปิด Serial Monitor (115200 baud) เพื่อดู log การทำงาน

---
*จัดทำเพื่อการแข่งขันโครงงาน STEAM ระดับมัธยมศึกษาตอนปลาย — ออกแบบในแนวทางวิศวกรรมฝังตัวและสถาปัตยกรรม IoT ระดับผลิตภัณฑ์จริง*
