#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 螢幕設定
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void scanI2CBus();
void readDeviceStatus();
void handleDeviceInterrupt();
void enableDS1307SQW();
void updateOLED(String deviceName, int count);

// 定義硬體腳位
#define I2C_SDA 21
#define I2C_SCL 22
#define INTERRUPT_PIN 4  // 假設感測器的中斷腳位連接至 GPIO 4

// 關鍵點：使用 volatile 確保變數在 ISR 與主程式間的同步
// 編譯器不會對其進行最佳化，每次讀取都會回到記憶體
volatile bool deviceEventTriggered = false;
volatile int interruptCount = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  while (!Serial);
  
    Serial.println("\n--- 韌體底層開發練習：I2C 掃描器與中斷驅動 ---");
    
    // 1. 初始化 I2C 通訊協議
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // 初始化 OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 分配失敗"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("System Initializing...");
    display.display();

    // 2. 執行 I2C 設備掃描 (模擬 BIOS POST 流程)
    scanI2CBus();
    
    // 3. 配置外部中斷 (External Interrupt)
    // 當腳位電位由高變低 (FALLING) 時觸發，模擬設備發送訊號
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleDeviceInterrupt, FALLING);
    enableDS1307SQW();
    Serial.println("系統初始化完成，等待中斷觸發...");
  }
  
  void loop() {
    if (deviceEventTriggered) {
      // 進入此區塊代表偵測到硬體事件
      Serial.print("偵測到硬體中斷！觸發次數: ");
      Serial.println(interruptCount);

      updateOLED("DS1307_SQW", interruptCount);
      // 模擬讀取設備暫存器 (Register Reading)
      // 在實際 BIOS 中，此處會讀取特定位址來判斷錯誤或狀態
      readDeviceStatus();
      
      // 重要：處理完後重置旗標
      deviceEventTriggered = false;
    }
    
    // 主迴圈保持非阻塞 (Non-blocking)，可以同時執行其他任務
    delay(10); // this speeds up the simulation
  }
  
  // --- 功能函式實作 ---
  
  // 中斷服務程式 (ISR) - 必須放在 RAM 中以提高執行速度
  void IRAM_ATTR handleDeviceInterrupt() {
      deviceEventTriggered = true;
      interruptCount++;
  }
  
  void enableDS1307SQW() {
      Wire.beginTransmission(0x68);
      Wire.write(0x07); // DS1307 的控制暫存器位址是 0x07
      // 寫入 0x10 代表開啟方波輸出 (SQWE = 1)，且頻率設定為 1Hz (RS1=0, RS0=0)
      Wire.write(0x10); 
      Wire.endTransmission();
      Serial.println("已發送 I2C 指令：開啟 DS1307 每秒方波 (SQW) 輸出...");
  }

  void scanI2CBus() {
    byte error, address;
    int nDevices = 0;
    
    Serial.println("正在掃描 I2C 總線位址 (1-127)...");

    for (address = 1; address < 127; address++) {
        // 握手流程 (Handshake)：
        // 發送起始訊號與位址位元 (Address << 1 | Write Bit)
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("發現設備於位址: 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" (ACK)");
            nDevices++;
        }
        else if (error == 4) {
            Serial.print("未知錯誤於位址: 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0) Serial.println("未發現任何 I2C 設備\n");
    else Serial.println("掃描完畢\n");
}

void readDeviceStatus() {
    // 模擬底層暫存器讀取邏輯
    // BIOS 工程師經常需要手動操作此流程來獲取硬體資訊
    Serial.println("正在透過 I2C 讀取設備內部暫存器狀態...");
}

void updateOLED(String deviceName, int count) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("--- Interrupt Log ---");
    display.println("");
    display.print("Device: ");
    display.println(deviceName);
    display.print("Count:  ");
    display.println(count);
    display.display(); // 真正將快取內容推送到螢幕上
}
