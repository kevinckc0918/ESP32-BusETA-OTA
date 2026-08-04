/*
 * HKBusETA v1.6.3 - Settings Menu Optimized & ETA Sort Fixed
 * Board: Waveshare ESP32-S3-Touch-LCD-4.3C
 * Base: official 12_lvgl_transplant/src
 * * 升級內容:
 * 1. 移除定時休眠：應要求退回 v1.6.2 基礎，移除休眠相關邏輯與 Web UI，保持全時顯示。
 * 2. 修復 ETA 排序：加入強制遞增排序邏輯 (Ascending Sort)，徹底解決循環線/尾班車出現「第二班車時間少於首班車」的 API 亂序問題。
 * 3. 系統診斷資訊：在 Web UI 及 ESP32 設定選單中保留「運行時間 (Uptime)」及「可用記憶體 (Free RAM)」。
 * 4. 完美繼承：無痛圖示對應、手勢滑動、自適應跑馬燈、純白天氣字體。
 */

#include "src/lvgl_port/lvgl_port.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <esp_system.h>
#include <ESPmDNS.h>

// ===================== 🌟 系統核心安全開關 =====================
#define USE_CHINESE 1              
#define USE_LARGE_CHINESE_FONT 1   
#define USE_WEATHER_IMG 0
#define USE_WEATHER_ICONS 1

// ===================== 🚨 警告圖示開關設定 (已全面開啟) =====================
#define HAS_ICON_TC1          1  // tc1.c 
#define HAS_ICON_TC3          1  // tc3.c 
#define HAS_ICON_TC8NE        1  // tc8ne.c
#define HAS_ICON_TC8NW        1  // tc8nw.c 
#define HAS_ICON_TC8SE        1  // tc8se.c 
#define HAS_ICON_TC8SW        1  // tc8sw.c 
#define HAS_ICON_TC9          1  // tc9.c 
#define HAS_ICON_TC10         1  // tc10.c 

#define HAS_ICON_RAINA        1  // raina.c 
#define HAS_ICON_RAINR        1  // rainr.c 
#define HAS_ICON_RAINB        1  // rainb.c 

#define HAS_ICON_TS           1  // ts.c (雷暴)
#define HAS_ICON_LANDSLIP     1  // landslip.c 
#define HAS_ICON_NTFL         1  // ntfl.c 

#define HAS_ICON_FIREY        1  // firey.c 
#define HAS_ICON_FIRER        1  // firer.c 

#define HAS_ICON_COLD         1  // cold.c 
#define HAS_ICON_VHOT         1  // vhot.c (酷熱)
#define HAS_ICON_FROST        1  // frost.c 
#define HAS_ICON_SMS          1  // sms.c (季候風)
#define HAS_ICON_TSUNAMI_WARN 1  // tsunami_warn.c 

// ===================== UI Font Theme =====================
#ifndef LV_FONT_MONTSERRAT_14
#define LV_FONT_MONTSERRAT_14 0
#endif
#ifndef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 0
#endif
#ifndef LV_FONT_MONTSERRAT_26
#define LV_FONT_MONTSERRAT_26 0
#endif
#ifndef LV_FONT_MONTSERRAT_40
#define LV_FONT_MONTSERRAT_40 0
#endif
#ifndef LV_FONT_MONTSERRAT_44
#define LV_FONT_MONTSERRAT_44 0
#endif

extern "C" {
    #if USE_CHINESE
        LV_FONT_DECLARE(lv_font_tc_24);
        #if USE_LARGE_CHINESE_FONT
            LV_FONT_DECLARE(lv_font_tc_36);
            #define FONT_DEST_LARGE    (&lv_font_tc_36)
        #else
            #define FONT_DEST_LARGE    (&lv_font_tc_24) 
        #endif
        #define FONT_DEST_SMALL    (&lv_font_tc_24)
        #define FONT_STOP_LARGE    (&lv_font_tc_24)
        #define FONT_STOP_SMALL    (&lv_font_tc_24)
        #define UI_FONT_HEADER     (&lv_font_tc_24)
    #endif

    #if USE_WEATHER_IMG
    LV_IMG_DECLARE(img_day_bg);
    #endif

    #if USE_WEATHER_ICONS
    LV_IMG_DECLARE(pic50); LV_IMG_DECLARE(pic51); LV_IMG_DECLARE(pic52); 
    LV_IMG_DECLARE(pic53); LV_IMG_DECLARE(pic54); LV_IMG_DECLARE(pic60);
    LV_IMG_DECLARE(pic61); LV_IMG_DECLARE(pic62); LV_IMG_DECLARE(pic63);
    LV_IMG_DECLARE(pic64); LV_IMG_DECLARE(pic65); LV_IMG_DECLARE(pic70);
    LV_IMG_DECLARE(pic71); LV_IMG_DECLARE(pic72); LV_IMG_DECLARE(pic73);
    LV_IMG_DECLARE(pic74); LV_IMG_DECLARE(pic75); LV_IMG_DECLARE(pic76);
    LV_IMG_DECLARE(pic77); LV_IMG_DECLARE(pic80); LV_IMG_DECLARE(pic81);
    LV_IMG_DECLARE(pic82); LV_IMG_DECLARE(pic83); LV_IMG_DECLARE(pic84);
    LV_IMG_DECLARE(pic85); LV_IMG_DECLARE(pic90); LV_IMG_DECLARE(pic91);
    LV_IMG_DECLARE(pic92); LV_IMG_DECLARE(pic93);

    #if HAS_ICON_TC1
        LV_IMG_DECLARE(tc1);
    #endif
    #if HAS_ICON_TC3
        LV_IMG_DECLARE(tc3);
    #endif
    #if HAS_ICON_TC8NE
        LV_IMG_DECLARE(tc8ne);
    #endif
    #if HAS_ICON_TC8NW
        LV_IMG_DECLARE(tc8nw);
    #endif
    #if HAS_ICON_TC8SE
        LV_IMG_DECLARE(tc8se);
    #endif
    #if HAS_ICON_TC8SW
        LV_IMG_DECLARE(tc8sw);
    #endif
    #if HAS_ICON_TC9
        LV_IMG_DECLARE(tc9);
    #endif
    #if HAS_ICON_TC10
        LV_IMG_DECLARE(tc10);
    #endif
    #if HAS_ICON_RAINA
        LV_IMG_DECLARE(raina);
    #endif
    #if HAS_ICON_RAINR
        LV_IMG_DECLARE(rainr);
    #endif
    #if HAS_ICON_RAINB
        LV_IMG_DECLARE(rainb);
    #endif
    #if HAS_ICON_TS
        LV_IMG_DECLARE(ts);
    #endif
    #if HAS_ICON_LANDSLIP
        LV_IMG_DECLARE(landslip);
    #endif
    #if HAS_ICON_NTFL
        LV_IMG_DECLARE(ntfl);
    #endif
    #if HAS_ICON_FIREY
        LV_IMG_DECLARE(firey);
    #endif
    #if HAS_ICON_FIRER
        LV_IMG_DECLARE(firer);
    #endif
    #if HAS_ICON_COLD
        LV_IMG_DECLARE(cold);
    #endif
    #if HAS_ICON_VHOT
        LV_IMG_DECLARE(vhot);
    #endif
    #if HAS_ICON_FROST
        LV_IMG_DECLARE(frost);
    #endif
    #if HAS_ICON_SMS
        LV_IMG_DECLARE(sms);
    #endif
    #if HAS_ICON_TSUNAMI_WARN
        LV_IMG_DECLARE(tsunami_warn);
    #endif
    #endif
}

#if LV_FONT_MONTSERRAT_44
#define UI_FONT_ROUTE      (&lv_font_montserrat_44)
#define UI_FONT_ETA_BIG    (&lv_font_montserrat_44)
#elif LV_FONT_MONTSERRAT_40
#define UI_FONT_ROUTE      (&lv_font_montserrat_40)
#define UI_FONT_ETA_BIG    (&lv_font_montserrat_40)
#else
#define UI_FONT_ROUTE      (&lv_font_montserrat_14)
#define UI_FONT_ETA_BIG    (&lv_font_montserrat_14)
#endif

#if LV_FONT_MONTSERRAT_26
#define UI_FONT_CLOCK      (&lv_font_montserrat_26)
#define UI_FONT_ETA_SMALL  (&lv_font_montserrat_26)
#elif LV_FONT_MONTSERRAT_20
#define UI_FONT_CLOCK      (&lv_font_montserrat_20)
#define UI_FONT_ETA_SMALL  (&lv_font_montserrat_20)
#else
#define UI_FONT_CLOCK      (&lv_font_montserrat_14)
#define UI_FONT_ETA_SMALL  (&lv_font_montserrat_14)
#endif

#if LV_FONT_MONTSERRAT_20
#define UI_FONT_SMALL      (&lv_font_montserrat_20)
#else
#define UI_FONT_SMALL      (&lv_font_montserrat_14)
#endif

#if USE_WEATHER_ICONS
const void* getWeatherIconSrc(int id) {
    switch(id) {
        case 50: return &pic50; case 51: return &pic51; case 52: return &pic52;
        case 53: return &pic53; case 54: return &pic54; case 60: return &pic60;
        case 61: return &pic61; case 62: return &pic62; case 63: return &pic63;
        case 64: return &pic64; case 65: return &pic65; case 70: return &pic70;
        case 71: return &pic71; case 72: return &pic72; case 73: return &pic73;
        case 74: return &pic74; case 75: return &pic75; case 76: return &pic76;
        case 77: return &pic77; case 80: return &pic80; case 81: return &pic81;
        case 82: return &pic82; case 83: return &pic83; case 84: return &pic84;
        case 85: return &pic85; case 90: return &pic90; case 91: return &pic91;
        case 92: return &pic92; case 93: return &pic93;
        default: return &pic50; 
    }
}
#endif

// ===================== Defaults =====================
const char *AP_SSID = "HKBusETA-Setup";
const char *AP_PASS = "";
const char *FIRMWARE_VERSION = "v1.6.3"; 

#define TOTAL_ROUTES 8         
#define MAX_CARDS_ON_SCREEN 4  

#define DEFAULT_REFRESH_MS 30000UL
#define DEFAULT_PAGE_MS 10000UL 

struct RouteConfig {
  String company;     
  String route;
  String bound;       
  String dir;         
  String serviceType; 
  String stopId;      
  String stopId2;     
  String stopNameEn;
  String stopNameTc;
};

struct EtaState {
  String eta1;
  String eta2;
  int min1;
  int min2;
  String destEn;
  String destTc;
};

RouteConfig routes[TOTAL_ROUTES];
EtaState etaStates[TOTAL_ROUTES];

Preferences prefs;
WebServer server(80);

String wifiSsid;
String wifiPass;
String uiTheme = "dark";
int currentBrightness = 100;   
bool showStopName = true;      
bool showBusPage = true;       
bool showWeatherPage = true;   
bool wifiReady = false;
bool apMode = false;

unsigned long refreshMs = DEFAULT_REFRESH_MS;
unsigned long pageIntervalMs = DEFAULT_PAGE_MS;
unsigned long lastFetchMs = 0;
unsigned long lastClockMs = 0;
unsigned long lastPageSwitchMs = 0;
unsigned long lastWeatherFetchMs = 0; 
int currentPage = 0;
int fetchRound = 0; 

volatile bool forceThemeUpdate = false; 
volatile bool needFullRefresh = false;

// 🌟 天氣與警告資訊
String currentTemp = "--";
String currentHum = "--";
String weatherWarning = ""; 
String displayWarning = ""; 
String currentRainfall = ""; 
int currentIconID = 50; 
String currentDescColor = "#FFFFFF ";
String currentDescText = "載入中";

const void* activeWarnIcons[3] = {NULL, NULL, NULL};
int activeWarnCount = 0;

String forecastWeek1 = "--";
String forecastTemp1 = "--°C - --°C";
int forecastIcon1 = 50;
String forecastColor1 = "#FFFFFF ";
String forecastDesc1 = "載入中";

String forecastWeek2 = "--";
String forecastTemp2 = "--°C - --°C";
int forecastIcon2 = 50;
String forecastColor2 = "#FFFFFF ";
String forecastDesc2 = "載入中";

// ===================== LVGL objects =====================
static lv_obj_t *headerObj = NULL;
static lv_obj_t *titleLabel = NULL;
static lv_obj_t *clockLabel = NULL;
static lv_obj_t *warningLabel = NULL; 
static lv_obj_t *ipLabel = NULL;      
static lv_obj_t *versionLabel = NULL; 

static lv_obj_t *headerTempLabel = NULL;
static lv_obj_t *headerWeatherIcon = NULL;
static lv_obj_t *wifiIconLabel = NULL; 

static lv_obj_t *cardObjs[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *badgeObjs[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *routeLabels[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *destLabels[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *stopLabels[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *eta1Labels[MAX_CARDS_ON_SCREEN] = {NULL};
static lv_obj_t *eta2Labels[MAX_CARDS_ON_SCREEN] = {NULL};

static lv_obj_t *weatherPanel = NULL;
static lv_obj_t *weatherImgObj = NULL; 
static lv_obj_t *weatherWarnBanner = NULL; 
static lv_obj_t *weatherDateLabel = NULL; 

static lv_obj_t *currWeatherBox = NULL;
static lv_obj_t *currTitleLabel = NULL;
static lv_obj_t *weatherIconObj = NULL;   
static lv_obj_t *weatherDescLabel = NULL; 
static lv_obj_t *weatherTempLabel = NULL;
static lv_obj_t *weatherHumLabel = NULL; 

static lv_obj_t *fc1Box = NULL;
static lv_obj_t *fc1TitleLabel = NULL;
static lv_obj_t *fc1IconObj = NULL;
static lv_obj_t *fc1DescLabel = NULL;
static lv_obj_t *fc1TempLabel = NULL;

static lv_obj_t *fc2Box = NULL;
static lv_obj_t *fc2TitleLabel = NULL;
static lv_obj_t *fc2IconObj = NULL;
static lv_obj_t *fc2DescLabel = NULL;
static lv_obj_t *fc2TempLabel = NULL;

static lv_obj_t *settingsBtn = NULL;
static lv_obj_t *settingsBtnLabel = NULL;
static lv_obj_t *settingsModal = NULL;
static lv_obj_t *sysInfoLabel = NULL; 
static lv_obj_t *themeLabel = NULL; 
static lv_obj_t *themeSwitch = NULL;  
static lv_obj_t *weatherSwitch = NULL;
static lv_obj_t *brightnessSlider = NULL;
static lv_obj_t *brightnessValLabel = NULL; 

static lv_obj_t *warnIconsScrl[3] = {NULL};   
static lv_obj_t *warnIconsBanner[3] = {NULL}; 

static void refreshDisplayAll();
static int getActiveRoutes(int* activeIdxs);

// ===================== Helpers =====================
static String htmlEscape(const String &s) {
  String r;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') r += "&amp;";
    else if (c == '<') r += "&lt;";
    else if (c == '>') r += "&gt;";
    else if (c == '"') r += "&quot;";
    else r += c;
  }
  return r;
}

static String nowDateTimeStr() {
  struct tm t;
  if (!getLocalTime(&t, 50)) return "----/--/-- --:--:--";
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
  return String(buf);
}

static String getUptimeString() {
    unsigned long sec = millis() / 1000;
    unsigned long d = sec / 86400;
    unsigned long h = (sec % 86400) / 3600;
    unsigned long m = (sec % 3600) / 60;
    unsigned long s = sec % 60;
    String uptime = "";
    if (d > 0) uptime += String(d) + "天 ";
    uptime += String(h) + "小時 " + String(m) + "分鐘 " + String(s) + "秒";
    return uptime;
}

static String getFormattedDateDay() {
    struct tm t;
    if (!getLocalTime(&t, 50)) return "----年--月--日";
    char buf[64];
    const char* days[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
    sprintf(buf, "%d年%d月%d日   %s", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, days[t.tm_wday]);
    return String(buf);
}

static int getCurrentHour() {
  struct tm t;
  if (!getLocalTime(&t, 50)) return 12; 
  return t.tm_hour;
}

static void safeSet(lv_obj_t *obj, const String &txt) {
  if (!obj) return;
  if (lvgl_port_lock(-1)) {
    if (strcmp(lv_label_get_text(obj), txt.c_str()) != 0) lv_label_set_text(obj, txt.c_str());
    lvgl_port_unlock();
  }
}

static int etaToMinutes(const String &eta) {
  if (eta.length() < 19) return -1;
  struct tm nowTm;
  if (!getLocalTime(&nowTm, 50)) return -1;

  struct tm etaTm = nowTm;
  etaTm.tm_year = eta.substring(0, 4).toInt() - 1900;
  etaTm.tm_mon  = eta.substring(5, 7).toInt() - 1;
  etaTm.tm_mday = eta.substring(8, 10).toInt();
  etaTm.tm_hour = eta.substring(11, 13).toInt();
  etaTm.tm_min  = eta.substring(14, 16).toInt();
  etaTm.tm_sec  = eta.substring(17, 19).toInt();

  time_t nowEpoch = mktime(&nowTm);
  time_t etaEpoch = mktime(&etaTm);
  int diff = (int)((etaEpoch - nowEpoch + 59) / 60);
  if (diff < 0) diff = 0;
  return diff;
}

static String minText(int m) {
  if (m < 0) return "-";
  return String(m);
}

static lv_color_t etaColor(int m) {
  bool isDark = (uiTheme != "kmb");
  if (m < 0) return isDark ? lv_color_hex(0x555555) : lv_color_hex(0xAAAAAA);     
  if (m <= 3) return isDark ? lv_color_hex(0xFF3B30) : lv_color_hex(0xCC0028);    
  if (m <= 10) return isDark ? lv_color_hex(0xFFCC00) : lv_color_hex(0xE67E22);   
  return isDark ? lv_color_hex(0x34C759) : lv_color_hex(0x2E7D32);                
}

static String httpGet(const String &url, int &code) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  code = -999;
  
  if (!http.begin(client, url)) return "";
  
  http.setTimeout(8000); 
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("User-Agent", "ESP32-Client/1.0");
  http.addHeader("Accept-Encoding", "identity");
  
  code = http.GET();
  String payload = http.getString();
  http.end();
  client.stop(); 
  return payload;
}

static String boundToDir(const String &bound) {
  return bound == "inbound" ? "I" : "O";
}

static void getWeatherInfoStr(int id, String &colorCode, String &descText) {
    colorCode = "#FFFFFF "; 
    descText = "天色明朗";
    switch(id) {
        case 50: descText = "陽光充沛"; break; 
        case 51: descText = "陽光時現"; break; 
        case 52: descText = "短暫陽光"; break;
        case 53: case 54: descText = "有驟雨"; break; 
        case 60: descText = "多雲"; break; 
        case 61: descText = "密雲"; break;
        case 62: descText = "微雨"; break;
        case 63: descText = "有雨"; break;
        case 64: descText = "大雨"; break;
        case 65: descText = "雷暴"; break; 
        case 70: case 71: case 72: case 73: case 74: case 75: descText = "天晴(晚)"; break;
        case 76: descText = "多雲(晚)"; break;
        case 77: descText = "天色明朗"; break;
        case 80: descText = "大風"; break;
        case 81: descText = "乾燥"; break;
        case 82: descText = "潮濕"; break;
        case 83: descText = "有霧"; break;
        case 84: descText = "薄霧"; break;
        case 85: descText = "煙霞"; break;
        case 90: descText = "酷熱"; break; 
        case 91: case 92: case 93: descText = "寒冷"; break;
    }
}

static String extractNestedValue(const String &payload, const String &parentKey, int startPos) {
    int parentPos = payload.indexOf(parentKey, startPos);
    if (parentPos > 0) {
        int valPos = payload.indexOf("\"value\":", parentPos);
        if (valPos > 0) {
            int endPos = payload.indexOf(",", valPos);
            if (endPos > valPos) {
                return payload.substring(valPos + 8, endPos);
            }
        }
    }
    return "--";
}

static String extractJsonString(const String &payload, const String &key, int startPos = 0) {
    int keyPos = payload.indexOf("\"" + key + "\"", startPos);
    if (keyPos < 0) return "";
    int colonPos = payload.indexOf(":", keyPos);
    if (colonPos < 0) return "";
    
    int commaPos = payload.indexOf(",", colonPos);
    int bracePos = payload.indexOf("}", colonPos);
    int endBound = payload.length();
    
    if (commaPos > 0 && bracePos > 0) {
        endBound = (commaPos < bracePos) ? commaPos : bracePos;
    } else if (commaPos > 0) {
        endBound = commaPos;
    } else if (bracePos > 0) {
        endBound = bracePos;
    }

    int quote1 = payload.indexOf("\"", colonPos);
    if (quote1 < 0 || quote1 > endBound) return ""; 
    
    int quote2 = payload.indexOf("\"", quote1 + 1);
    if (quote2 < 0 || quote2 > endBound) return "";
    
    return payload.substring(quote1 + 1, quote2);
}

// ===================== Config =====================
static void loadConfig() {
  prefs.begin("hkbuseta", false);
  wifiSsid = prefs.getString("wifi_ssid", "");
  wifiPass = prefs.getString("wifi_pass", "");
  uiTheme = prefs.getString("theme", "dark"); 
  currentBrightness = prefs.getInt("brightness", 100); 
  showBusPage = (prefs.getString("show_bus", "1") == "1");    
  showStopName = (prefs.getString("show_stop", "1") == "1");
  showWeatherPage = (prefs.getString("show_wth", "1") == "1");

  refreshMs = prefs.getUInt("refresh", DEFAULT_REFRESH_MS);
  if (refreshMs < 15000 || refreshMs > 60000) refreshMs = DEFAULT_REFRESH_MS;
  
  pageIntervalMs = prefs.getUInt("page_int", DEFAULT_PAGE_MS);
  if (pageIntervalMs < 5000 || pageIntervalMs > 60000) pageIntervalMs = DEFAULT_PAGE_MS;

  for (int i = 0; i < TOTAL_ROUTES; i++) {
    String p = "r" + String(i);
    routes[i].company = prefs.getString((p + "co").c_str(), "KMB");
    routes[i].route = prefs.getString((p + "route").c_str(), i == 0 ? "968" : "");
    routes[i].bound = prefs.getString((p + "bound").c_str(), "outbound");
    routes[i].dir = boundToDir(routes[i].bound);
    routes[i].serviceType = prefs.getString((p + "svc").c_str(), "1");
    routes[i].stopId = prefs.getString((p + "stop").c_str(), "");
    routes[i].stopId2 = prefs.getString((p + "stop2").c_str(), "");
    routes[i].stopNameEn = prefs.getString((p + "en").c_str(), "");
    routes[i].stopNameTc = prefs.getString((p + "tc").c_str(), "");
  }
}

static void saveRoutesFromWeb() {
  refreshMs = (unsigned long)server.arg("refresh").toInt() * 1000UL;
  if (refreshMs < 15000 || refreshMs > 60000) refreshMs = DEFAULT_REFRESH_MS;
  prefs.putUInt("refresh", refreshMs);

  pageIntervalMs = (unsigned long)server.arg("page_int").toInt() * 1000UL;
  if (pageIntervalMs < 5000 || pageIntervalMs > 60000) pageIntervalMs = DEFAULT_PAGE_MS;
  prefs.putUInt("page_int", pageIntervalMs);

  uiTheme = server.arg("theme");
  if (uiTheme.length() == 0) uiTheme = "dark";
  prefs.putString("theme", uiTheme); 

  currentBrightness = server.arg("brightness").toInt();
  if(currentBrightness < 10) currentBrightness = 10;
  if(currentBrightness > 100) currentBrightness = 100;
  prefs.putInt("brightness", currentBrightness);

  showBusPage = (server.arg("show_bus") == "1"); 
  prefs.putString("show_bus", showBusPage ? "1" : "0");

  showStopName = (server.arg("show_stop") == "1");
  prefs.putString("show_stop", showStopName ? "1" : "0");
  
  showWeatherPage = (server.arg("show_wth") == "1");
  prefs.putString("show_wth", showWeatherPage ? "1" : "0");

  vTaskDelay(pdMS_TO_TICKS(20)); 

  for (int i = 0; i < TOTAL_ROUTES; i++) {
    String p = "r" + String(i);
    routes[i].company = server.arg(p + "co"); if (routes[i].company.length() == 0) routes[i].company = "KMB";
    routes[i].route = server.arg(p + "route"); routes[i].route.trim(); routes[i].route.toUpperCase();
    routes[i].bound = server.arg(p + "bound");
    routes[i].dir = boundToDir(routes[i].bound);
    routes[i].serviceType = server.arg(p + "svc"); if (routes[i].serviceType.length() == 0) routes[i].serviceType = "1";
    
    String chosen = server.arg(p + "stop");
    int sep1 = chosen.indexOf('|');
    int sep2 = chosen.indexOf('|', sep1 + 1);
    if (sep1 > 0 && sep2 > sep1) {
      routes[i].stopId = chosen.substring(0, sep1);
      routes[i].stopNameEn = chosen.substring(sep1 + 1, sep2);
      routes[i].stopNameTc = chosen.substring(sep2 + 1);
    } else if (routes[i].route.length() == 0) {
      routes[i].stopId = "";
      routes[i].stopNameEn = "";
      routes[i].stopNameTc = "";
    }
    
    routes[i].stopId2 = server.arg(p + "stop2");
    if (routes[i].route.length() == 0) routes[i].stopId2 = "";

    prefs.putString((p + "co").c_str(), routes[i].company);
    prefs.putString((p + "route").c_str(), routes[i].route);
    prefs.putString((p + "bound").c_str(), routes[i].bound);
    prefs.putString((p + "svc").c_str(), routes[i].serviceType);
    prefs.putString((p + "stop").c_str(), routes[i].stopId);
    prefs.putString((p + "stop2").c_str(), routes[i].stopId2);
    prefs.putString((p + "en").c_str(), routes[i].stopNameEn);
    prefs.putString((p + "tc").c_str(), routes[i].stopNameTc);

    vTaskDelay(pdMS_TO_TICKS(30));
  }
}

// ===================== ESP32 UI Design & Gesture Events =====================

static void theme_switch_event_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) uiTheme = "dark";
    else uiTheme = "kmb";
    prefs.putString("theme", uiTheme);
    forceThemeUpdate = true; 
}

static void weather_switch_event_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    showWeatherPage = lv_obj_has_state(sw, LV_STATE_CHECKED);
    prefs.putString("show_wth", showWeatherPage ? "1" : "0");
    refreshDisplayAll();
}

static void screen_touch_event_cb(lv_event_t * e) {
    if(!lv_obj_has_flag(settingsModal, LV_OBJ_FLAG_HIDDEN)) return;

    lv_event_code_t code = lv_event_get_code(e);

    int activeIdxs[TOTAL_ROUTES];
    int activeCount = getActiveRoutes(activeIdxs);
    int itemsPerPage = (activeCount <= 3) ? 3 : 4;
    int busPages = 0;
    if (showBusPage) {
        busPages = (activeCount > 0) ? (activeCount + itemsPerPage - 1) / itemsPerPage : 1;
    }
    int totalPages = busPages + (showWeatherPage ? 1 : 0);
    if (totalPages <= 1) return;

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_LEFT) {
            currentPage = (currentPage + 1) % totalPages;
            lastPageSwitchMs = millis();
            refreshDisplayAll();
        } else if (dir == LV_DIR_RIGHT) {
            currentPage = (currentPage - 1 + totalPages) % totalPages;
            lastPageSwitchMs = millis();
            refreshDisplayAll();
        }
    }
}

static void settings_btn_click_cb(lv_event_t * e) {
    if(lv_obj_has_flag(settingsModal, LV_OBJ_FLAG_HIDDEN)) {
        
        if(sysInfoLabel) {
            String sysInfo = "運行時間: " + getUptimeString() + "  |  可用RAM: " + String(ESP.getFreeHeap() / 1024) + " KB";
            lv_label_set_text(sysInfoLabel, sysInfo.c_str());
        }

        if (uiTheme == "dark") lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
        else lv_obj_clear_state(themeSwitch, LV_STATE_CHECKED);

        if (showWeatherPage) lv_obj_add_state(weatherSwitch, LV_STATE_CHECKED);
        else lv_obj_clear_state(weatherSwitch, LV_STATE_CHECKED);

        lv_slider_set_value(brightnessSlider, currentBrightness, LV_ANIM_OFF);
        if (brightnessValLabel) lv_label_set_text_fmt(brightnessValLabel, "%d%%", currentBrightness);

        lv_obj_clear_flag(settingsModal, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(settingsModal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void brightness_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    currentBrightness = lv_slider_get_value(slider);
    uint8_t duty = 100 - currentBrightness; 
    IO_EXTENSION_Pwm_Output(duty);
    
    if (brightnessValLabel) {
        lv_label_set_text_fmt(brightnessValLabel, "%d%%", currentBrightness);
    }
}

static void brightness_slider_release_cb(lv_event_t * e) {
    prefs.putInt("brightness", currentBrightness);
}

static void applyFont(lv_obj_t *obj, const lv_font_t *font) {
  lv_obj_set_style_text_font(obj, font, 0);
}

static void applyThemeColors() {
  bool isDark = (uiTheme != "kmb");
  lv_color_t scrBg       = isDark ? lv_color_hex(0x0B0F19) : lv_color_hex(0xF4F6F9);
  lv_color_t headerBg    = isDark ? lv_color_hex(0x0B0F19) : lv_color_hex(0xCC0028); 
  lv_color_t headerText  = isDark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xFFFFFF); 
  lv_color_t cardBg      = isDark ? lv_color_hex(0x161B22) : lv_color_hex(0xFFFFFF);
  lv_color_t cardBorder  = isDark ? lv_color_hex(0x30363D) : lv_color_hex(0xDDDDDD);
  lv_color_t destColor   = isDark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
  lv_color_t stopColor   = isDark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x666666);
  lv_color_t marqueeColor = isDark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x444444); 

  lv_obj_set_style_bg_color(lv_scr_act(), scrBg, 0);
  lv_obj_set_style_bg_color(headerObj, headerBg, 0);
  lv_obj_set_style_bg_opa(headerObj, isDark ? 0 : LV_OPA_COVER, 0); 
  
  lv_obj_set_style_text_color(titleLabel, headerText, 0);
  lv_obj_set_style_text_color(clockLabel, headerText, 0);
  lv_obj_set_style_text_color(headerTempLabel, headerText, 0); 
  
  lv_obj_set_style_text_color(warningLabel, marqueeColor, 0); 
  lv_obj_set_style_text_color(ipLabel, stopColor, 0);
  lv_obj_set_style_text_color(versionLabel, stopColor, 0); 

  for (int i = 0; i < MAX_CARDS_ON_SCREEN; i++) {
    if (!cardObjs[i]) continue;
    lv_obj_set_style_bg_color(cardObjs[i], cardBg, 0);
    lv_obj_set_style_border_color(cardObjs[i], cardBorder, 0);
    lv_obj_set_style_text_color(destLabels[i], destColor, 0);
    lv_obj_set_style_text_color(stopLabels[i], stopColor, 0);
    lv_obj_set_style_text_color(eta2Labels[i], stopColor, 0);
  }
}

static void createWeatherBox(lv_obj_t* &box, lv_obj_t* &title, lv_obj_t* &icon, lv_obj_t* &desc, lv_obj_t* &temp, lv_obj_t** hum_ptr, int x_offset) {
    box = lv_obj_create(weatherPanel);
    lv_obj_set_size(box, 230, 220); 
    lv_obj_align(box, LV_ALIGN_TOP_LEFT, x_offset, 90); 
    lv_obj_set_style_bg_color(box, lv_color_hex(0x000000), 0); 
    lv_obj_set_style_bg_opa(box, 130, 0);                       
    lv_obj_set_style_radius(box, 16, 0);                      
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0); 
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(box, screen_touch_event_cb, LV_EVENT_GESTURE, NULL);

    title = lv_label_create(box);
    applyFont(title, FONT_STOP_SMALL);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_width(title, lv_pct(100)); 
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10); 

    icon = lv_img_create(box);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -35); 

    desc = lv_label_create(box);
    lv_label_set_recolor(desc, true);
    applyFont(desc, FONT_STOP_SMALL);
    lv_obj_set_width(desc, lv_pct(100)); 
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(desc, lv_color_white(), 0); 
    lv_obj_align(desc, LV_ALIGN_CENTER, 0, 15); 

    temp = lv_label_create(box);
    lv_label_set_recolor(temp, true);
    applyFont(temp, FONT_STOP_SMALL);
    lv_obj_set_width(temp, lv_pct(100)); 
    lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(temp, lv_color_white(), 0);
    lv_obj_align(temp, LV_ALIGN_CENTER, 0, 45); 

    if (hum_ptr != NULL) {
        *hum_ptr = lv_label_create(box);
        lv_label_set_recolor(*hum_ptr, true);
        applyFont(*hum_ptr, FONT_STOP_SMALL);
        lv_obj_set_width(*hum_ptr, lv_pct(100)); 
        lv_obj_set_style_text_align(*hum_ptr, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(*hum_ptr, lv_color_white(), 0); 
        lv_obj_align(*hum_ptr, LV_ALIGN_CENTER, 0, 75); 
    }
}

static void createBusScreen() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_add_event_cb(scr, screen_touch_event_cb, LV_EVENT_GESTURE, NULL);

  headerObj = lv_obj_create(scr);
  lv_obj_set_size(headerObj, 800, 50);
  lv_obj_align(headerObj, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_set_style_border_width(headerObj, 0, 0);
  lv_obj_clear_flag(headerObj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(headerObj, screen_touch_event_cb, LV_EVENT_GESTURE, NULL);

  titleLabel = lv_label_create(headerObj);
  lv_label_set_text(titleLabel, "Live Dashboard");
  applyFont(titleLabel, UI_FONT_HEADER);
  lv_obj_set_width(titleLabel, 280); 
  lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_DOT);
  lv_obj_align(titleLabel, LV_ALIGN_LEFT_MID, 15, 0);

  clockLabel = lv_label_create(headerObj);
  lv_label_set_text(clockLabel, "----/--/-- --:--:--");
  applyFont(clockLabel, UI_FONT_CLOCK);
  lv_obj_align(clockLabel, LV_ALIGN_RIGHT_MID, -260, 0); 

  headerWeatherIcon = lv_img_create(headerObj);
  lv_obj_align(headerWeatherIcon, LV_ALIGN_RIGHT_MID, -190, 0); 
  lv_obj_add_flag(headerWeatherIcon, LV_OBJ_FLAG_HIDDEN); 
  
  headerTempLabel = lv_label_create(headerObj);
  lv_label_set_text(headerTempLabel, "--°C");
  applyFont(headerTempLabel, UI_FONT_CLOCK);
  lv_obj_align(headerTempLabel, LV_ALIGN_RIGHT_MID, -110, 0); 

  wifiIconLabel = lv_label_create(headerObj);
  lv_label_set_text(wifiIconLabel, LV_SYMBOL_WIFI);
  applyFont(wifiIconLabel, UI_FONT_CLOCK);
  lv_obj_align(wifiIconLabel, LV_ALIGN_RIGHT_MID, -65, 0);
  lv_obj_set_style_text_color(wifiIconLabel, lv_color_hex(0xE74C3C), 0); 

  settingsBtn = lv_btn_create(headerObj);
  lv_obj_set_size(settingsBtn, 40, 40); 
  lv_obj_align(settingsBtn, LV_ALIGN_RIGHT_MID, -10, 0); 
  lv_obj_add_event_cb(settingsBtn, settings_btn_click_cb, LV_EVENT_CLICKED, NULL); 
  
  lv_obj_set_style_radius(settingsBtn, 8, 0); 
  lv_obj_set_style_bg_color(settingsBtn, lv_color_hex(0x555555), 0); 
  lv_obj_set_style_bg_color(settingsBtn, lv_color_hex(0x333333), LV_STATE_PRESSED); 
  lv_obj_set_style_border_width(settingsBtn, 1, 0);
  lv_obj_set_style_border_color(settingsBtn, lv_color_hex(0x777777), 0);
  
  settingsBtnLabel = lv_label_create(settingsBtn);
  lv_label_set_text(settingsBtnLabel, LV_SYMBOL_SETTINGS); 
  applyFont(settingsBtnLabel, UI_FONT_CLOCK); 
  lv_obj_center(settingsBtnLabel);
  lv_obj_set_style_text_color(settingsBtnLabel, lv_color_white(), 0);

  // --- Bus Page Cards ---
  for (int i = 0; i < MAX_CARDS_ON_SCREEN; i++) {
    cardObjs[i] = lv_obj_create(scr);
    lv_obj_set_style_border_width(cardObjs[i], 1, 0);
    lv_obj_set_style_radius(cardObjs[i], 16, 0); 
    lv_obj_clear_flag(cardObjs[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(cardObjs[i], screen_touch_event_cb, LV_EVENT_GESTURE, NULL);

    badgeObjs[i] = lv_obj_create(cardObjs[i]);
    lv_obj_set_style_border_width(badgeObjs[i], 0, 0);
    lv_obj_set_style_radius(badgeObjs[i], 12, 0);
    lv_obj_clear_flag(badgeObjs[i], LV_OBJ_FLAG_SCROLLABLE);

    routeLabels[i] = lv_label_create(badgeObjs[i]);
    lv_label_set_text(routeLabels[i], "");
    applyFont(routeLabels[i], UI_FONT_ROUTE);
    lv_obj_set_style_text_color(routeLabels[i], lv_color_white(), 0);
    lv_obj_center(routeLabels[i]);

    destLabels[i] = lv_label_create(cardObjs[i]);
    lv_label_set_text(destLabels[i], "Waiting...");
    lv_obj_set_width(destLabels[i], 490); 
    lv_label_set_long_mode(destLabels[i], LV_LABEL_LONG_DOT); 

    stopLabels[i] = lv_label_create(cardObjs[i]);
    lv_label_set_text(stopLabels[i], "");
    lv_obj_set_width(stopLabels[i], 490); 
    lv_label_set_long_mode(stopLabels[i], LV_LABEL_LONG_DOT); 

    eta1Labels[i] = lv_label_create(cardObjs[i]);
    lv_label_set_text(eta1Labels[i], "-");
    applyFont(eta1Labels[i], UI_FONT_ETA_BIG);
    lv_obj_set_style_text_align(eta1Labels[i], LV_TEXT_ALIGN_RIGHT, 0);

    eta2Labels[i] = lv_label_create(cardObjs[i]);
    lv_label_set_text(eta2Labels[i], "-");
    applyFont(eta2Labels[i], UI_FONT_ETA_SMALL);
    lv_obj_set_style_text_align(eta2Labels[i], LV_TEXT_ALIGN_RIGHT, 0);
  }

  // --- Weather Page Panel ---
  weatherPanel = lv_obj_create(scr);
  lv_obj_set_size(weatherPanel, 780, 360);
  lv_obj_align(weatherPanel, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_radius(weatherPanel, 20, 0);
  lv_obj_set_style_border_width(weatherPanel, 0, 0);
  lv_obj_set_style_pad_all(weatherPanel, 0, 0); 
  lv_obj_clear_flag(weatherPanel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_clip_corner(weatherPanel, true, 0); 
  lv_obj_add_flag(weatherPanel, LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_event_cb(weatherPanel, screen_touch_event_cb, LV_EVENT_GESTURE, NULL);

  weatherImgObj = lv_img_create(weatherPanel);
  lv_obj_align(weatherImgObj, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(weatherImgObj, LV_OBJ_FLAG_HIDDEN); 

  weatherWarnBanner = lv_label_create(weatherPanel);
  lv_label_set_text(weatherWarnBanner, "");
  applyFont(weatherWarnBanner, FONT_STOP_SMALL);
  lv_label_set_long_mode(weatherWarnBanner, LV_LABEL_LONG_SCROLL_CIRCULAR); 
  lv_obj_set_style_text_color(weatherWarnBanner, lv_color_white(), 0);           
  lv_obj_set_style_bg_color(weatherWarnBanner, lv_color_hex(0xE74C3C), 0);       
  lv_obj_set_style_bg_opa(weatherWarnBanner, LV_OPA_COVER, 0);                   
  lv_obj_set_style_radius(weatherWarnBanner, 8, 0);                             
  lv_obj_set_style_pad_left(weatherWarnBanner, 12, 0);                           
  lv_obj_set_style_pad_right(weatherWarnBanner, 12, 0);                          
  lv_obj_set_style_pad_top(weatherWarnBanner, 6, 0);                             
  lv_obj_set_style_pad_bottom(weatherWarnBanner, 6, 0);                          
  lv_obj_align(weatherWarnBanner, LV_ALIGN_TOP_MID, 0, 12); 
  lv_obj_add_flag(weatherWarnBanner, LV_OBJ_FLAG_HIDDEN);   

  for (int i=0; i<3; i++) {
      warnIconsBanner[i] = lv_img_create(weatherPanel);
      lv_obj_add_flag(warnIconsBanner[i], LV_OBJ_FLAG_HIDDEN);
  }

  weatherDateLabel = lv_label_create(weatherPanel);
  lv_label_set_text(weatherDateLabel, "----年--月--日   星期-");
  applyFont(weatherDateLabel, FONT_STOP_LARGE); 
  lv_obj_set_style_text_color(weatherDateLabel, lv_color_white(), 0);
  lv_obj_align(weatherDateLabel, LV_ALIGN_TOP_MID, 0, 35);

  createWeatherBox(currWeatherBox, currTitleLabel, weatherIconObj, weatherDescLabel, weatherTempLabel, &weatherHumLabel, 20);
  createWeatherBox(fc1Box, fc1TitleLabel, fc1IconObj, fc1DescLabel, fc1TempLabel, NULL, 275);
  createWeatherBox(fc2Box, fc2TitleLabel, fc2IconObj, fc2DescLabel, fc2TempLabel, NULL, 530);                     

  warningLabel = lv_label_create(scr);
  lv_label_set_recolor(warningLabel, true); 
  lv_label_set_text(warningLabel, ""); 
  applyFont(warningLabel, FONT_STOP_SMALL); 
  lv_label_set_long_mode(warningLabel, LV_LABEL_LONG_SCROLL_CIRCULAR); 
  lv_obj_align(warningLabel, LV_ALIGN_BOTTOM_LEFT, 20, -15); 

  for (int i=0; i<3; i++) {
      warnIconsScrl[i] = lv_img_create(scr);
      lv_obj_add_flag(warnIconsScrl[i], LV_OBJ_FLAG_HIDDEN);
  }

  ipLabel = lv_label_create(scr);
  lv_label_set_text(ipLabel, "IP: --");
  applyFont(ipLabel, UI_FONT_SMALL);
  lv_obj_align(ipLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -30); 

  versionLabel = lv_label_create(scr);
  lv_label_set_text(versionLabel, FIRMWARE_VERSION);
  applyFont(versionLabel, UI_FONT_SMALL);
  lv_obj_align(versionLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -10); 

  // ===================== 🌟 新版設定對話框 (Settings Modal) =====================
  settingsModal = lv_obj_create(scr);
  lv_obj_set_size(settingsModal, 540, 360); 
  lv_obj_center(settingsModal);
  lv_obj_set_style_bg_color(settingsModal, lv_color_hex(0x1C1C1C), 0);
  lv_obj_set_style_border_color(settingsModal, lv_color_hex(0x444444), 0);
  lv_obj_set_style_radius(settingsModal, 20, 0);
  lv_obj_add_flag(settingsModal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(settingsModal, LV_OBJ_FLAG_SCROLLABLE); 

  lv_obj_t *modalTitle = lv_label_create(settingsModal);
  lv_label_set_text(modalTitle, "系統設定"); 
  applyFont(modalTitle, FONT_STOP_LARGE);
  lv_obj_set_style_text_color(modalTitle, lv_color_white(), 0);
  lv_obj_align(modalTitle, LV_ALIGN_TOP_MID, 0, 10);

  sysInfoLabel = lv_label_create(settingsModal);
  lv_label_set_text(sysInfoLabel, "運行時間: 讀取中...");
  applyFont(sysInfoLabel, FONT_STOP_LARGE); 
  lv_obj_set_style_text_color(sysInfoLabel, lv_color_hex(0x95A5A6), 0);
  lv_obj_align(sysInfoLabel, LV_ALIGN_TOP_MID, 0, 50); 

  lv_obj_t *closeBtn = lv_btn_create(settingsModal);
  lv_obj_set_size(closeBtn, 50, 40);
  lv_obj_align(closeBtn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(closeBtn, lv_color_hex(0xE74C3C), 0);
  lv_obj_add_event_cb(closeBtn, settings_btn_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *closeLbl = lv_label_create(closeBtn);
  lv_label_set_text(closeLbl, "X");
  applyFont(closeLbl, UI_FONT_CLOCK); 
  lv_obj_center(closeLbl);

  themeLabel = lv_label_create(settingsModal);
  lv_label_set_text(themeLabel, "深色主題 (Dark Theme)");
  applyFont(themeLabel, FONT_STOP_SMALL);
  lv_obj_set_style_text_color(themeLabel, lv_color_white(), 0);
  lv_obj_align(themeLabel, LV_ALIGN_TOP_LEFT, 20, 100);

  themeSwitch = lv_switch_create(settingsModal);
  lv_obj_align(themeSwitch, LV_ALIGN_TOP_RIGHT, -20, 95);
  if (uiTheme == "dark") lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(themeSwitch, theme_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t * wthLabel = lv_label_create(settingsModal);
  lv_label_set_text(wthLabel, "顯示天氣概況頁面");
  applyFont(wthLabel, FONT_STOP_SMALL);
  lv_obj_set_style_text_color(wthLabel, lv_color_white(), 0);
  lv_obj_align(wthLabel, LV_ALIGN_TOP_LEFT, 20, 170);

  weatherSwitch = lv_switch_create(settingsModal);
  lv_obj_align(weatherSwitch, LV_ALIGN_TOP_RIGHT, -20, 165);
  if (showWeatherPage) lv_obj_add_state(weatherSwitch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(weatherSwitch, weather_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t * bLabel = lv_label_create(settingsModal);
  lv_label_set_text(bLabel, "螢幕亮度");
  applyFont(bLabel, FONT_STOP_SMALL);
  lv_obj_set_style_text_color(bLabel, lv_color_white(), 0);
  lv_obj_align(bLabel, LV_ALIGN_TOP_LEFT, 20, 240);

  brightnessValLabel = lv_label_create(settingsModal);
  lv_label_set_text_fmt(brightnessValLabel, "%d%%", currentBrightness);
  applyFont(brightnessValLabel, FONT_STOP_SMALL);
  lv_obj_set_style_text_color(brightnessValLabel, lv_color_white(), 0);
  lv_obj_align(brightnessValLabel, LV_ALIGN_TOP_RIGHT, -20, 240);

  brightnessSlider = lv_slider_create(settingsModal);
  lv_obj_set_size(brightnessSlider, 460, 20);
  lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, 290); 
  lv_slider_set_range(brightnessSlider, 10, 100);
  lv_slider_set_value(brightnessSlider, currentBrightness, LV_ANIM_OFF);
  lv_obj_add_event_cb(brightnessSlider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(brightnessSlider, brightness_slider_release_cb, LV_EVENT_RELEASED, NULL); 

  applyThemeColors();
}

static int getActiveRoutes(int* activeIdxs) {
  int count = 0;
  for (int i = 0; i < TOTAL_ROUTES; i++) {
    if (routes[i].route.length() > 0) {
      activeIdxs[count++] = i;
    }
  }
  return count;
}

static void updateWeatherUIColors() {
    if (!weatherPanel) return;
    int hour = getCurrentHour();
    
#if USE_WEATHER_IMG
    lv_obj_clear_flag(weatherImgObj, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(weatherImgObj, &img_day_bg); 
    
    if (hour >= 6 && hour < 18) {
        lv_obj_set_style_img_recolor_opa(weatherImgObj, 0, 0); 
    } else {
        lv_obj_set_style_img_recolor(weatherImgObj, lv_color_hex(0x000B18), 0);
        lv_obj_set_style_img_recolor_opa(weatherImgObj, 160, 0); 
    }
#else
    lv_color_t topColor = (hour >= 6 && hour < 18) ? lv_color_hex(0x4CA1AF) : lv_color_hex(0x0F2027);
    lv_color_t bottomColor = (hour >= 6 && hour < 18) ? lv_color_hex(0xC4E0E5) : lv_color_hex(0x203A43);
    lv_obj_set_style_bg_color(weatherPanel, topColor, 0);
    lv_obj_set_style_bg_grad_color(weatherPanel, bottomColor, 0);
    lv_obj_set_style_bg_grad_dir(weatherPanel, LV_GRAD_DIR_VER, 0);
#endif
}

static void refreshDisplayAll() {
  if (!lvgl_port_lock(-1)) return;
  applyThemeColors(); 
  
  if (themeSwitch != NULL) {
      bool isChecked = lv_obj_has_state(themeSwitch, LV_STATE_CHECKED);
      if (uiTheme == "dark" && !isChecked) lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
      else if (uiTheme != "dark" && isChecked) lv_obj_clear_state(themeSwitch, LV_STATE_CHECKED);
  }
  
  if (brightnessSlider != NULL) {
      lv_slider_set_value(brightnessSlider, currentBrightness, LV_ANIM_OFF);
      IO_EXTENSION_Pwm_Output(100 - currentBrightness);
  }

  String headTemp = currentTemp + (currentTemp != "--" ? " °C" : "");
  lv_label_set_text(headerTempLabel, headTemp.c_str());

  if (wifiIconLabel != NULL) {
      if (WiFi.status() == WL_CONNECTED) {
          lv_obj_set_style_text_color(wifiIconLabel, lv_color_hex(0x34C759), 0); 
      } else {
          lv_obj_set_style_text_color(wifiIconLabel, lv_color_hex(0xE74C3C), 0); 
      }
  }

  #if USE_WEATHER_ICONS
  if (currentIconID >= 50 && currentIconID <= 93) { 
      lv_img_set_src(headerWeatherIcon, getWeatherIconSrc(currentIconID));
      lv_obj_clear_flag(headerWeatherIcon, LV_OBJ_FLAG_HIDDEN);
  }
  #endif

  int activeIdxs[TOTAL_ROUTES];
  int activeCount = getActiveRoutes(activeIdxs);
  int itemsPerPage = (activeCount <= 3) ? 3 : 4;
  int busPages = (showBusPage && activeCount > 0) ? (activeCount + itemsPerPage - 1) / itemsPerPage : (showBusPage ? 1 : 0);
  int totalPages = busPages + (showWeatherPage ? 1 : 0);
  if (totalPages == 0) { busPages = 1; totalPages = 1; } 
  if (currentPage >= totalPages) currentPage = 0;

  bool isWeatherPageNow = (showWeatherPage && currentPage == busPages);
  bool isBusPageNow = (currentPage < busPages);

  int iconWidthOffset = 0;
  for (int i=0; i<3; i++) {
      if (i < activeWarnCount && activeWarnIcons[i] != NULL) {
          lv_img_set_src(warnIconsScrl[i], activeWarnIcons[i]);
          lv_obj_clear_flag(warnIconsScrl[i], LV_OBJ_FLAG_HIDDEN);
          lv_obj_align(warnIconsScrl[i], LV_ALIGN_BOTTOM_LEFT, 20 + i*60, -8); 
          iconWidthOffset += 60;
      } else {
          lv_obj_add_flag(warnIconsScrl[i], LV_OBJ_FLAG_HIDDEN);
      }
  }
  
  int labelStartX = 20 + iconWidthOffset + (activeWarnCount > 0 ? 10 : 0);
  lv_obj_align(warningLabel, LV_ALIGN_BOTTOM_LEFT, labelStartX, -15);
  lv_obj_set_width(warningLabel, 800 - labelStartX - 160); 

  if (isWeatherPageNow) {
      for (int i = 0; i < MAX_CARDS_ON_SCREEN; i++) lv_obj_add_flag(cardObjs[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(weatherPanel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(warningLabel, LV_OBJ_FLAG_HIDDEN); 
      
      updateWeatherUIColors(); 
      
      lv_label_set_text(weatherDateLabel, getFormattedDateDay().c_str());
      lv_label_set_text(currTitleLabel, "現在狀況");
      lv_label_set_text(weatherDescLabel, currentDescText.c_str());
      lv_label_set_text(weatherTempLabel, headTemp.c_str()); 
      
      if (weatherHumLabel != NULL) {
          String hText = "濕度: " + currentHum + (currentHum != "--" ? " %" : "");
          if (currentRainfall.length() > 0) {
              hText += "\n" + currentRainfall; 
              lv_obj_align(weatherHumLabel, LV_ALIGN_CENTER, 0, 85); 
          } else {
              lv_obj_align(weatherHumLabel, LV_ALIGN_CENTER, 0, 75); 
          }
          lv_label_set_text(weatherHumLabel, hText.c_str()); 
      }
      
      lv_label_set_text(fc1TitleLabel, forecastWeek1.c_str());
      lv_label_set_text(fc1DescLabel, forecastDesc1.c_str());
      lv_label_set_text(fc1TempLabel, forecastTemp1.c_str());
      lv_label_set_text(fc2TitleLabel, forecastWeek2.c_str());
      lv_label_set_text(fc2DescLabel, forecastDesc2.c_str());
      lv_label_set_text(fc2TempLabel, forecastTemp2.c_str());

      #if USE_WEATHER_ICONS
          lv_img_set_src(weatherIconObj, getWeatherIconSrc(currentIconID));
          lv_obj_clear_flag(weatherIconObj, LV_OBJ_FLAG_HIDDEN);
          lv_img_set_src(fc1IconObj, getWeatherIconSrc(forecastIcon1));
          lv_obj_clear_flag(fc1IconObj, LV_OBJ_FLAG_HIDDEN);
          lv_img_set_src(fc2IconObj, getWeatherIconSrc(forecastIcon2));
          lv_obj_clear_flag(fc2IconObj, LV_OBJ_FLAG_HIDDEN);
      #endif
      
      if (displayWarning.length() > 0) {
          lv_label_set_text(weatherWarnBanner, displayWarning.c_str());
          
          if (displayWarning.indexOf("黑色暴雨") >= 0 || displayWarning.indexOf("十號") >= 0 || displayWarning.indexOf("九號") >= 0) {
              lv_obj_set_style_bg_color(weatherWarnBanner, lv_color_hex(0x1C1C1C), 0); 
              lv_obj_set_style_text_color(weatherWarnBanner, lv_color_white(), 0);     
          } else if (displayWarning.indexOf("黃色暴雨") >= 0 || displayWarning.indexOf("三號") >= 0) {
              lv_obj_set_style_bg_color(weatherWarnBanner, lv_color_hex(0xF1C40F), 0); 
              lv_obj_set_style_text_color(weatherWarnBanner, lv_color_black(), 0);     
          } else {
              lv_obj_set_style_bg_color(weatherWarnBanner, lv_color_hex(0xE74C3C), 0); 
              lv_obj_set_style_text_color(weatherWarnBanner, lv_color_white(), 0);     
          }
          
          lv_obj_set_width(weatherWarnBanner, 750);
          lv_obj_set_style_pad_left(weatherWarnBanner, 15 + activeWarnCount * 60, 0); 
          lv_obj_clear_flag(weatherWarnBanner, LV_OBJ_FLAG_HIDDEN);
          lv_obj_align(weatherDateLabel, LV_ALIGN_TOP_MID, 0, 58);

          for (int i=0; i<3; i++) {
              if (i < activeWarnCount && activeWarnIcons[i] != NULL) {
                  lv_img_set_src(warnIconsBanner[i], activeWarnIcons[i]);
                  lv_obj_clear_flag(warnIconsBanner[i], LV_OBJ_FLAG_HIDDEN);
                  lv_obj_align(warnIconsBanner[i], LV_ALIGN_TOP_LEFT, 25 + i*60, 6); 
              } else {
                  lv_obj_add_flag(warnIconsBanner[i], LV_OBJ_FLAG_HIDDEN);
              }
          }
      } else {
          lv_obj_add_flag(weatherWarnBanner, LV_OBJ_FLAG_HIDDEN);
          lv_obj_align(weatherDateLabel, LV_ALIGN_TOP_MID, 0, 35);
          for (int i=0; i<3; i++) lv_obj_add_flag(warnIconsBanner[i], LV_OBJ_FLAG_HIDDEN);
      }

      lv_label_set_text(titleLabel, "香港天氣概況");

  } else if (isBusPageNow) {
      lv_obj_add_flag(weatherPanel, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(warningLabel, LV_OBJ_FLAG_HIDDEN); 
      for (int i=0; i<3; i++) lv_obj_add_flag(warnIconsBanner[i], LV_OBJ_FLAG_HIDDEN);

      if (activeCount == 0) {
          lv_label_set_text(titleLabel, "請使用手機設定巴士路線");
          for (int i = 0; i < MAX_CARDS_ON_SCREEN; i++) lv_obj_add_flag(cardObjs[i], LV_OBJ_FLAG_HIDDEN);
      } else {
          if (totalPages > 1) {
              lv_label_set_text(titleLabel, ("實時巴士報站 (" + String(currentPage + 1) + "/" + String(busPages) + ")").c_str());
          } else {
              lv_label_set_text(titleLabel, "實時巴士報站");
          }

          int cardH   = (activeCount <= 3) ? 105 : 80;
          int badgeH  = (activeCount <= 3) ? 75  : 55;
          int spacing = (activeCount <= 3) ? 20  : 10;
          int startY  = (activeCount <= 3) ? 75  : 65;
          int destY   = (activeCount <= 3) ? -15 : -12;
          int stopY   = (activeCount <= 3) ? 22  : 16;
          int etaX    = -25;
          int eta1Y   = (activeCount <= 3) ? -16 : -14;
          int eta2Y   = (activeCount <= 3) ? 28  : 24;

          int startDataIdx = currentPage * itemsPerPage;

          for (int i = 0; i < MAX_CARDS_ON_SCREEN; i++) {
            int dataIdx = startDataIdx + i;
            if (i >= itemsPerPage || dataIdx >= activeCount) {
              lv_obj_add_flag(cardObjs[i], LV_OBJ_FLAG_HIDDEN);
              continue;
            }
            
            lv_obj_clear_flag(cardObjs[i], LV_OBJ_FLAG_HIDDEN);
            int realIdx = activeIdxs[dataIdx];

            lv_obj_set_size(cardObjs[i], 780, cardH);
            lv_obj_align(cardObjs[i], LV_ALIGN_TOP_MID, 0, startY + i * (cardH + spacing));

            String co = routes[realIdx].company;
            lv_color_t badgeBg = lv_color_hex(0xCC0028); 
            lv_color_t badgeGrad = badgeBg;
            lv_grad_dir_t gradDir = LV_GRAD_DIR_NONE;

            if (co == "CTB") {
              badgeBg = lv_color_hex(0x0055A4); badgeGrad = badgeBg;
            } else if (co == "LWB") {
              badgeBg = lv_color_hex(0xE67E22); badgeGrad = badgeBg;
            } else if (co == "JOINT") {
              badgeBg = lv_color_hex(0xCC0028); badgeGrad = lv_color_hex(0x0055A4); gradDir = LV_GRAD_DIR_HOR;
            }
            
            lv_obj_set_style_bg_color(badgeObjs[i], badgeBg, 0);
            lv_obj_set_style_bg_grad_color(badgeObjs[i], badgeGrad, 0);
            lv_obj_set_style_bg_grad_dir(badgeObjs[i], gradDir, 0);

            lv_obj_set_size(badgeObjs[i], 145, badgeH);
            lv_obj_align(badgeObjs[i], LV_ALIGN_LEFT_MID, 0, 0);
            lv_obj_center(routeLabels[i]);

            if (!showStopName) {
                lv_obj_add_flag(stopLabels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(destLabels[i], LV_ALIGN_LEFT_MID, 160, 0);
                lv_obj_set_style_text_font(destLabels[i], FONT_DEST_LARGE, 0);
            } else {
                lv_obj_clear_flag(stopLabels[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(destLabels[i], LV_ALIGN_LEFT_MID, 160, destY);
                lv_obj_align(stopLabels[i], LV_ALIGN_LEFT_MID, 160, stopY);
                lv_obj_set_style_text_font(destLabels[i], (activeCount <= 3) ? FONT_DEST_LARGE : FONT_DEST_SMALL, 0);
                lv_obj_set_style_text_font(stopLabels[i], (activeCount <= 3) ? FONT_STOP_LARGE : FONT_STOP_SMALL, 0);
            }
            
            lv_obj_align(eta1Labels[i], LV_ALIGN_RIGHT_MID, etaX, eta1Y);
            lv_obj_align(eta2Labels[i], LV_ALIGN_RIGHT_MID, etaX, eta2Y);

            lv_label_set_text(routeLabels[i], routes[realIdx].route.c_str());
            String destText = etaStates[realIdx].destTc.length() > 0 ? "往: " + etaStates[realIdx].destTc : "等待數據...";
            lv_label_set_text(destLabels[i], destText.c_str());
            lv_label_set_text(stopLabels[i], routes[realIdx].stopNameTc.c_str());
            
            lv_label_set_text(eta1Labels[i], minText(etaStates[realIdx].min1).c_str());
            lv_label_set_text(eta2Labels[i], minText(etaStates[realIdx].min2).c_str());
            lv_obj_set_style_text_color(eta1Labels[i], etaColor(etaStates[realIdx].min1), 0);
          }
      }
  }
  
  lv_label_set_text(clockLabel, nowDateTimeStr().c_str());
  lv_label_set_text(warningLabel, weatherWarning.c_str());
  
  String ipTxt = "IP: ";
  if (wifiReady) ipTxt += WiFi.localIP().toString();
  else ipTxt += "Setup AP: " + String(AP_SSID); 
  lv_label_set_text(ipLabel, ipTxt.c_str());
  
  lvgl_port_unlock();
}

static void updateClockOnly() {
  safeSet(clockLabel, nowDateTimeStr());
}

static void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  int code;
  
  String url = "https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=rhrread&lang=tc";
  String payload = httpGet(url, code);
  if (code == 200 && payload.length() > 0) {
      int tBlock = payload.indexOf("\"香港天文台\"");
      if (tBlock > 0) {
          int vPos = payload.indexOf("\"value\":", tBlock);
          if (vPos > 0) {
              int endPos = payload.indexOf(",", vPos);
              if (endPos > 0) {
                  currentTemp = payload.substring(vPos + 8, endPos);
                  currentTemp.trim();
              }
          }
      }

      int hBlock = payload.indexOf("\"humidity\"");
      if (hBlock > 0) {
          int vPos = payload.indexOf("\"value\":", hBlock);
          if (vPos > 0) {
              int endPos = payload.indexOf(",", vPos);
              if (endPos > 0) {
                  currentHum = payload.substring(vPos + 8, endPos);
                  currentHum.trim();
              }
          }
      }

      int iconPos = payload.indexOf("\"icon\":[");
      if (iconPos > 0) {
          int iconEnd = payload.indexOf("]", iconPos);
          if (iconEnd > iconPos) {
              currentIconID = payload.substring(iconPos + 8, iconEnd).toInt();
              getWeatherInfoStr(currentIconID, currentDescColor, currentDescText);
          }
      }

      currentRainfall = "";
      int rainBlock = payload.indexOf("\"rainfall\"");
      if (rainBlock > 0) {
          float maxRain = 0.0;
          int searchPos = rainBlock;
          for(int i=0; i<30; i++) {
              int mP = payload.indexOf("\"max\":", searchPos);
              if(mP < 0 || mP > rainBlock + 3000) break; 
              int mE = payload.indexOf(",", mP);
              if(mE > mP) {
                  float v = payload.substring(mP+6, mE).toFloat();
                  if(v > maxRain) maxRain = v;
              }
              searchPos = mE;
          }
          if (maxRain > 0) {
              currentRainfall = "雨量: " + String(maxRain, 1) + " mm";
          }
      }

      weatherWarning = "";
      displayWarning = "";
      activeWarnCount = 0;
      for (int i=0; i<3; i++) activeWarnIcons[i] = NULL; 

      int w = payload.indexOf("\"warningMessage\":[");
      if (w > 0) {
          int wStart = payload.indexOf("[", w);
          int wEnd = payload.indexOf("]", wStart);
          if (wStart > 0 && wEnd > wStart) {
              String arr = payload.substring(wStart + 1, wEnd);
              String finalWarn = "";
              int p = 0;
              while (p < arr.length()) {
                  int q1 = arr.indexOf("\"", p);
                  if (q1 < 0) break;
                  int q2 = arr.indexOf("\"", q1 + 1);
                  if (q2 < 0) break;
                  String msg = arr.substring(q1 + 1, q2);
                  int cutP1 = msg.indexOf("，"); int cutP2 = msg.indexOf("！"); 
                  int cutP3 = msg.indexOf("。"); int cutP4 = msg.indexOf("現正生效");
                  int cutPos = msg.length();
                  if (cutP1 > 0 && cutP1 < cutPos) cutPos = cutP1;
                  if (cutP2 > 0 && cutP2 < cutPos) cutPos = cutP2;
                  if (cutP3 > 0 && cutP3 < cutPos) cutPos = cutP3;
                  if (cutP4 > 0 && cutP4 < cutPos) cutPos = cutP4;
                  msg = msg.substring(0, cutPos);
                  msg.replace("信號", ""); msg.trim();
                  
                  if (msg.length() > 0) {
                      if (finalWarn.length() > 0) finalWarn += " | ";
                      finalWarn += msg;
                      
                      const void* iconPtr = NULL;
                      #if USE_WEATHER_ICONS
                      #if HAS_ICON_TC1
                      if (msg.indexOf("一號") >= 0) iconPtr = &tc1;
                      #endif
                      #if HAS_ICON_TC3
                      else if (msg.indexOf("三號") >= 0) iconPtr = &tc3;
                      #endif
                      #if HAS_ICON_TC8NE
                      else if (msg.indexOf("八號") >= 0 && msg.indexOf("東北") >= 0) iconPtr = &tc8ne;
                      #endif
                      #if HAS_ICON_TC8NW
                      else if (msg.indexOf("八號") >= 0 && msg.indexOf("西北") >= 0) iconPtr = &tc8nw;
                      #endif
                      #if HAS_ICON_TC8SE
                      else if (msg.indexOf("八號") >= 0 && msg.indexOf("東南") >= 0) iconPtr = &tc8se;
                      #endif
                      #if HAS_ICON_TC8SW
                      else if (msg.indexOf("八號") >= 0 && msg.indexOf("西南") >= 0) iconPtr = &tc8sw;
                      #endif
                      #if HAS_ICON_TC9
                      else if (msg.indexOf("九號") >= 0) iconPtr = &tc9;
                      #endif
                      #if HAS_ICON_TC10
                      else if (msg.indexOf("十號") >= 0) iconPtr = &tc10;
                      #endif
                      #if HAS_ICON_RAINA
                      else if (msg.indexOf("黃色暴雨") >= 0) iconPtr = &raina;
                      #endif
                      #if HAS_ICON_RAINR
                      else if (msg.indexOf("紅色暴雨") >= 0) iconPtr = &rainr;
                      #endif
                      #if HAS_ICON_RAINB
                      else if (msg.indexOf("黑色暴雨") >= 0) iconPtr = &rainb;
                      #endif
                      #if HAS_ICON_TS
                      else if (msg.indexOf("雷暴") >= 0) iconPtr = &ts;
                      #endif
                      #if HAS_ICON_LANDSLIP
                      else if (msg.indexOf("山泥傾瀉") >= 0) iconPtr = &landslip;
                      #endif
                      #if HAS_ICON_NTFL
                      else if (msg.indexOf("水浸") >= 0) iconPtr = &ntfl;
                      #endif
                      #if HAS_ICON_FIREY
                      else if (msg.indexOf("黃色火災") >= 0) iconPtr = &firey;
                      #endif
                      #if HAS_ICON_FIRER
                      else if (msg.indexOf("紅色火災") >= 0) iconPtr = &firer;
                      #endif
                      #if HAS_ICON_COLD
                      else if (msg.indexOf("寒冷") >= 0) iconPtr = &cold;
                      #endif
                      #if HAS_ICON_VHOT
                      else if (msg.indexOf("酷熱") >= 0) iconPtr = &vhot;
                      #endif
                      #if HAS_ICON_FROST
                      else if (msg.indexOf("霜凍") >= 0) iconPtr = &frost;
                      #endif
                      #if HAS_ICON_SMS
                      else if (msg.indexOf("季候風") >= 0) iconPtr = &sms;
                      #endif
                      #if HAS_ICON_TSUNAMI_WARN
                      else if (msg.indexOf("海嘯") >= 0) iconPtr = &tsunami_warn;
                      #endif
                      
                      if (iconPtr != NULL && activeWarnCount < 3) {
                          activeWarnIcons[activeWarnCount++] = iconPtr;
                      }
                      #endif
                  }
                  p = q2 + 1;
              }
              if (finalWarn.length() > 0) {
                  weatherWarning = "[警告] " + finalWarn;
                  displayWarning = finalWarn; 
              }
          }
      }
  }

  String urlFnd = "https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=fnd&lang=tc";
  String payloadFnd = httpGet(urlFnd, code);
  if(code == 200 && payloadFnd.length() > 0) {
      int arrPos = payloadFnd.indexOf("\"weatherForecast\":[");
      if(arrPos > 0) {
          int p1 = payloadFnd.indexOf("\"forecastDate\"", arrPos);
          if(p1 > 0) {
              forecastWeek1 = extractJsonString(payloadFnd, "week", p1);
              String maxT1 = extractNestedValue(payloadFnd, "\"forecastMaxtemp\"", p1);
              String minT1 = extractNestedValue(payloadFnd, "\"forecastMintemp\"", p1);
              forecastTemp1 = minT1 + "°C - " + maxT1 + "°C";
              int iconP = payloadFnd.indexOf("\"ForecastIcon\":", p1);
              if(iconP > 0) {
                  forecastIcon1 = payloadFnd.substring(iconP+15, payloadFnd.indexOf(",", iconP)).toInt();
                  getWeatherInfoStr(forecastIcon1, forecastColor1, forecastDesc1);
              }
          }
          int p2 = payloadFnd.indexOf("\"forecastDate\"", p1 + 10);
          if(p2 > 0) {
              forecastWeek2 = extractJsonString(payloadFnd, "week", p2);
              String maxT2 = extractNestedValue(payloadFnd, "\"forecastMaxtemp\"", p2);
              String minT2 = extractNestedValue(payloadFnd, "\"forecastMintemp\"", p2);
              forecastTemp2 = minT2 + "°C - " + maxT2 + "°C";
              int iconP = payloadFnd.indexOf("\"ForecastIcon\":", p2);
              if(iconP > 0) {
                  forecastIcon2 = payloadFnd.substring(iconP+15, payloadFnd.indexOf(",", iconP)).toInt();
                  getWeatherInfoStr(forecastIcon2, forecastColor2, forecastDesc2);
              }
          }
      }
  }
}

// ===================== 巴士 API Fetching =====================
static bool autoPickFirstStopIfNeeded(int idx) {
  if (routes[idx].stopId.length() > 0) return true;
  
  int code;
  String co = routes[idx].company;
  if (co == "KMB" || co == "LWB" || co == "JOINT") {
    String url = String("https://data.etabus.gov.hk/v1/transport/kmb/route-stop/") + routes[idx].route + "/" + routes[idx].bound + "/" + routes[idx].serviceType;
    String payload = httpGet(url, code);
    if (code == 200 && payload.length() > 0) {
      String pt = "\"stop\":\""; int p = payload.indexOf(pt);
      if (p >= 0) {
        int e = payload.indexOf('"', p + pt.length());
        if (e >= 0) routes[idx].stopId = payload.substring(p + pt.length(), e);
      }
    }
  }
  
  if (co == "CTB" || co == "JOINT") {
    if ((co == "CTB" && routes[idx].stopId.length() == 0) || (co == "JOINT" && routes[idx].stopId2.length() == 0)) {
      String origBound = routes[idx].bound;
      String ctbBound = origBound;
      if (co == "JOINT") {
          ctbBound = (ctbBound == "outbound") ? "inbound" : "outbound";
      }

      String url = String("https://rt.data.gov.hk/v2/transport/citybus/route-stop/CTB/") + routes[idx].route + "/" + ctbBound;
      String payload = httpGet(url, code);
      
      if (co == "JOINT" && (code != 200 || payload.indexOf("\"data\":[]") >= 0 || payload.indexOf("\"data\": []") >= 0)) {
          ctbBound = origBound;
          url = String("https://rt.data.gov.hk/v2/transport/citybus/route-stop/CTB/") + routes[idx].route + "/" + ctbBound;
          payload = httpGet(url, code);
      }

      if (code == 200 && payload.length() > 0) {
        String pt = "\"stop\":\""; int p = payload.indexOf(pt);
        if (p >= 0) {
          int e = payload.indexOf('"', p + pt.length());
          if (e >= 0) {
            String sId = payload.substring(p + pt.length(), e);
            if (co == "CTB") routes[idx].stopId = sId; else routes[idx].stopId2 = sId;
          }
        }
      }
    }
  }
  return (routes[idx].stopId.length() > 0 || routes[idx].stopId2.length() > 0);
}

// 🌟 修復 2: 解決循環線時間倒序 (加入遞增排序邏輯)
static void fetchSingleAPI(int idx, String apiType, String sId, int &out_m1, int &out_m2, String &out_dTc, String &out_dEn) {
  out_m1 = -1; out_m2 = -1;
  if (sId.length() == 0) return;
  int code; String url;
  
  if (apiType == "KMB" || apiType == "LWB") {
    url = String("https://data.etabus.gov.hk/v1/transport/kmb/eta/") + sId + "/" + routes[idx].route + "/" + routes[idx].serviceType;
  } else {
    url = String("https://rt.data.gov.hk/v2/transport/citybus/eta/CTB/") + sId + "/" + routes[idx].route;
  }
  
  String payload = httpGet(url, code);
  
  if (code != 200 || payload.length() == 0) {
      out_dTc = "站:" + routes[idx].stopNameTc;
      return; 
  }

  String targetDir = routes[idx].dir; 
  String ctbTargetDir = routes[idx].bound; 
  
  if (apiType == "CTB" && routes[idx].company == "JOINT") {
      targetDir = (targetDir == "O") ? "I" : "O"; 
      ctbTargetDir = (ctbTargetDir == "outbound") ? "inbound" : "outbound";
  }

  int validEtas[10]; // 用作暫存收集到的有效 ETA 
  int etaCount = 0;
  String dTc = "", dEn = "";
  
  int dataPos = payload.indexOf("\"data\"");
  if(dataPos > 0) {
      int arrayStart = payload.indexOf("[", dataPos);
      if (arrayStart > 0) {
          int pos = arrayStart;
          while (pos < payload.length() && etaCount < 10) {
              int objStart = payload.indexOf("{", pos);
              if (objStart < 0) break;
              int objEnd = payload.indexOf("}", objStart);
              if (objEnd < 0) break;

              String objStr = payload.substring(objStart, objEnd + 1);
              pos = objEnd + 1;

              String dirVal = extractJsonString(objStr, "dir");
              
              bool dirMatch = false;
              if (apiType == "CTB" && routes[idx].company == "JOINT") {
                  dirMatch = (dirVal == targetDir || dirVal == ctbTargetDir || dirVal == routes[idx].dir || dirVal == routes[idx].bound || dirVal == "");
              } else {
                  dirMatch = (dirVal == targetDir || dirVal == ctbTargetDir || dirVal.indexOf(targetDir) >= 0 || dirVal == "");
              }
              
              if (dirMatch) {
                  if (dTc.length() == 0) dTc = extractJsonString(objStr, "dest_tc");
                  if (dEn.length() == 0) dEn = extractJsonString(objStr, "dest_en");

                  String etaStr = extractJsonString(objStr, "eta");
                  if (etaStr.length() >= 19) { 
                      int m = etaToMinutes(etaStr);
                      // 將有效的到達時間收集起來
                      if (m >= 0) {
                          validEtas[etaCount++] = m;
                      }
                  }
              }
          }
      }
  }

  // 🌟 強制遞增排序 (Bubble Sort)
  for (int i = 0; i < etaCount - 1; i++) {
      for (int j = 0; j < etaCount - i - 1; j++) {
          if (validEtas[j] > validEtas[j + 1]) {
              int temp = validEtas[j];
              validEtas[j] = validEtas[j + 1];
              validEtas[j + 1] = temp;
          }
      }
  }

  out_m1 = (etaCount > 0) ? validEtas[0] : -1;
  out_m2 = (etaCount > 1) ? validEtas[1] : -1;
  
  if(dTc.length() == 0) {
      if (routes[idx].stopNameTc.length() > 0) {
          dTc = routes[idx].stopNameTc;
      } else {
          dTc = "路線 " + routes[idx].route;
      }
  }
  if(out_dTc.length() == 0 && dTc.length() > 0) out_dTc = dTc;
}

static void fetchEtaRow(int idx) {
  if (routes[idx].route.length() == 0) return; 
  etaStates[idx].min1 = -1; etaStates[idx].min2 = -1;
  etaStates[idx].destTc = ""; etaStates[idx].destEn = "";
  if (!autoPickFirstStopIfNeeded(idx)) return;

  String co = routes[idx].company;
  if (co == "KMB" || co == "LWB") {
    fetchSingleAPI(idx, co, routes[idx].stopId, etaStates[idx].min1, etaStates[idx].min2, etaStates[idx].destTc, etaStates[idx].destEn);
  } else if (co == "CTB") {
    fetchSingleAPI(idx, co, routes[idx].stopId, etaStates[idx].min1, etaStates[idx].min2, etaStates[idx].destTc, etaStates[idx].destEn);
  } else if (co == "JOINT") {
    int k1, k2, c1, c2;
    fetchSingleAPI(idx, "KMB", routes[idx].stopId, k1, k2, etaStates[idx].destTc, etaStates[idx].destEn);
    fetchSingleAPI(idx, "CTB", routes[idx].stopId2, c1, c2, etaStates[idx].destTc, etaStates[idx].destEn);
    
    int valid[4]; int count = 0;
    if (k1 >= 0) valid[count++] = k1;
    if (k2 >= 0) valid[count++] = k2;
    if (c1 >= 0) valid[count++] = c1;
    if (c2 >= 0) valid[count++] = c2;
    
    // 聯營班次合併後排序
    for (int i=0; i<count-1; i++) {
        for (int j=0; j<count-i-1; j++) {
            if (valid[j] > valid[j+1]) {
                int temp = valid[j];
                valid[j] = valid[j+1];
                valid[j+1] = temp;
            }
        }
    }
    etaStates[idx].min1 = count > 0 ? valid[0] : -1;
    etaStates[idx].min2 = count > 1 ? valid[1] : -1;
  }
}

static void fetchAllEta() {
  if (WiFi.status() != WL_CONNECTED) return;
  fetchRound++;
  for (int i = 0; i < TOTAL_ROUTES; i++) {
      fetchEtaRow(i);
      vTaskDelay(pdMS_TO_TICKS(10)); 
  }
  refreshDisplayAll();
}

// ===================== Web UI 控制台 =====================
static String settingsPage() {
  String h = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'><title>香港巴士到站顯示器控制台</title>";
  h += "<style>";
  h += ":root{--primary:#B00020;--primary-dark:#8A0019;--bg:#F4F6F9;--card-bg:#FFFFFF;--text:#2C3E50;--border:#E1E8ED;--radius:14px}";
  h += "*{box-sizing:border-box} body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;margin:0;background:var(--bg);color:var(--text);line-height:1.5}";
  h += "header{background:linear-gradient(135deg, #B00020 0%, #8A0019 100%);color:#fff;padding:24px 20px;text-align:center;box-shadow:0 4px 12px rgba(176,0,32,0.15)}";
  h += "header h1{margin:0;font-size:22px;font-weight:700;letter-spacing:0.5px}";
  h += "header p{margin:6px 0 0;font-size:13px;opacity:0.85}";
  h += ".container{max-width:680px;margin:20px auto;padding:0 16px 40px}";
  h += ".card{background:var(--card-bg);border-radius:var(--radius);padding:20px;margin-bottom:20px;box-shadow:0 2px 8px rgba(0,0,0,0.04);border:1px solid var(--border)}";
  h += ".card-title{display:flex;align-items:center;gap:8px;font-size:17px;font-weight:700;color:var(--primary);margin:0 0 16px 0;padding-bottom:10px;border-bottom:2px solid #F0F3F6}";
  h += ".form-group{margin-bottom:16px}";
  h += ".form-group:last-child{margin-bottom:0}";
  h += "label{display:block;font-size:13px;font-weight:600;color:#5A6A75;margin-bottom:6px}";
  h += "input,select,button{width:100%;font-size:15px;padding:11px 14px;border:1px solid var(--border);border-radius:10px;background:#FAFAFC;color:var(--text);transition:all 0.2s}";
  h += "input:focus,select:focus{border-color:var(--primary);outline:none;background:#FFF;box-shadow:0 0 0 3px rgba(176,0,32,0.1)}";
  h += "button{background:var(--primary);color:#fff;border:none;font-weight:700;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;gap:6px}";
  h += "button:hover{background:var(--primary-dark);transform:translateY(-1px)}";
  h += "button:active{transform:translateY(0)}";
  h += ".btn-sec{background:#EBF0F5;color:#334E68;border:1px solid #D9E2EC;margin-top:4px;margin-bottom:12px}";
  h += ".btn-sec:hover{background:#D9E2EC}";
  h += ".btn-danger{background:#E74C3C;color:#fff;margin-top:10px}";
  h += ".btn-danger:hover{background:#C0392B}";
  h += ".btn-reboot{background:#3498DB;color:#fff;margin-top:10px}";
  h += ".btn-reboot:hover{background:#2980B9}";
  h += ".row{display:flex;gap:12px}";
  h += ".row > div{flex:1}";
  h += ".status-badge{display:inline-flex;align-items:center;gap:6px;padding:6px 12px;background:#E8F8F0;color:#27AE60;border-radius:20px;font-size:13px;font-weight:600;margin-bottom:12px}";
  h += ".route-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}";
  h += ".route-badge{background:var(--primary);color:#fff;font-size:12px;font-weight:700;padding:2px 8px;border-radius:6px}";
  h += "#status{text-align:center;font-size:13px;color:#27AE60;margin-bottom:16px;font-weight:600;padding:10px;background:#E8F8F0;border-radius:10px}";
  h += ".submit-btn-wrapper{position:sticky;bottom:16px;z-index:99}";
  h += ".btn-submit{padding:16px;font-size:17px;box-shadow:0 6px 20px rgba(176,0,32,0.3);border-radius:12px}";
  h += "</style>";

  h += "<script>";
  h += "const KMB_API='https://data.etabus.gov.hk/v1/transport/kmb';";
  h += "let stopsCache={};";
  h += "fetch(KMB_API+'/stop').then(r=>r.json()).then(d=>{";
  h += " d.data.forEach(s=>stopsCache[s.stop]=s);";
  h += " document.getElementById('status').innerText='✅ 系統就緒：已成功載入巴士站數據庫！';";
  h += "}).catch(e=>document.getElementById('status').innerText='⚠️ 無法連線至九巴數據庫。');";

  h += "function toggleJoint(i){";
  h += " let co = document.getElementById('r'+i+'co').value;";
  h += " let div2 = document.getElementById('r'+i+'stop2_div');";
  h += " let btn = document.getElementById('btn'+i);";
  h += " if(co==='JOINT'){ div2.style.display='block'; btn.innerHTML='🔍 搜尋九巴及城巴車站'; }";
  h += " else if(co==='CTB'){ div2.style.display='none'; btn.innerHTML='🔍 搜尋城巴車站'; }";
  h += " else { div2.style.display='none'; btn.innerHTML='🔍 搜尋車站'; }";
  h += "}";

  h += "async function loadStops(i){";
  h += " let co=document.getElementById('r'+i+'co').value;";
  h += " let r=document.getElementById('r'+i+'route').value.trim().toUpperCase();";
  h += " let b=document.getElementById('r'+i+'bound').value;";
  h += " let s=document.getElementById('r'+i+'svc').value || '1';";
  h += " let sel1=document.getElementById('r'+i+'stop');";
  h += " let sel2=document.getElementById('r'+i+'stop2');";
  h += " let btn=document.getElementById('btn'+i);";
  h += " if(!r){ alert('請先輸入路線號碼！'); return; }";
  h += " btn.innerText='⏳ 正在搜尋車站資料...';";
  
  h += " if(co==='KMB' || co==='LWB' || co==='JOINT'){";
  h += "  sel1.innerHTML='<option value=\"\">載入中...</option>';";
  h += "  try{";
  h += "   let res=await fetch(`${KMB_API}/route-stop/${r}/${b}/${s}`);";
  h += "   let json=await res.json();";
  h += "   let html='';";
  h += "   if(json.data){ json.data.forEach((item, idx)=>{";
  h += "    let obj=stopsCache[item.stop];";
  h += "    let en=obj?obj.name_en:item.stop;";
  h += "    let tc=obj?obj.name_tc:'';";
  h += "    html+=`<option value=\"${item.stop}|${en}|${tc}\">${idx+1}. ${tc} (${en})</option>`;";
  h += "   });}";
  h += "   sel1.innerHTML=html||'<option value=\"\">找不到任何車站</option>';";
  h += "  }catch(e){ sel1.innerHTML='<option value=\"\">網路連線失敗</option>'; }";
  h += " }";

  h += " if(co==='CTB' || co==='JOINT'){";
  h += "  let targetSel = (co==='CTB') ? sel1 : sel2;";
  h += "  let ctb_b = (co==='JOINT') ? (b==='outbound'?'inbound':'outbound') : b;";
  h += "  targetSel.innerHTML='<option value=\"\">載入中...</option>';";
  h += "  try{";
  h += "   let res=await fetch(`https://rt.data.gov.hk/v2/transport/citybus/route-stop/CTB/${r}/${ctb_b}`);";
  h += "   let json=await res.json();";
  
  h += "   if(co==='JOINT' && (!json.data || json.data.length===0)){";
  h += "      ctb_b = b;";
  h += "      res=await fetch(`https://rt.data.gov.hk/v2/transport/citybus/route-stop/CTB/${r}/${ctb_b}`);";
  h += "      json=await res.json();";
  h += "   }";

  h += "   let html='';";
  h += "   if(json.data && json.data.length>0){";
  h += "    let promises = json.data.map(st=>fetch(`https://rt.data.gov.hk/v2/transport/citybus/stop/${st.stop}`).then(r=>r.json()).catch(()=>null));";
  h += "    let stopDetails = await Promise.all(promises);";
  h += "    stopDetails.forEach((sd, idx)=>{";
  h += "     if(sd && sd.data){ let d=sd.data;";
  h += "      if(co==='CTB') html+=`<option value=\"${d.stop}|${d.name_en}|${d.name_tc}\">${idx+1}. ${d.name_tc} (${d.name_en})</option>`;";
  h += "      else html+=`<option value=\"${d.stop}\">${idx+1}. ${d.name_tc}</option>`;";
  h += "     }";
  h += "    });";
  h += "   }";
  h += "   targetSel.innerHTML=html||'<option value=\"\">找不到任何車站</option>';";
  h += "  }catch(e){ targetSel.innerHTML='<option value=\"\">網絡連線失敗</option>'; }";
  h += " }";
  
  h += " toggleJoint(i);";
  h += "}";
  h += "window.onload = () => { for(let i=0; i<" + String(TOTAL_ROUTES) + "; i++) toggleJoint(i); };";
  h += "</script>";

  h += "</head><body>";
  h += "<header>";
  h += "<h1>🚌 香港巴士到站顯示器</h1>";
  h += "<p>控制台與路線配置選單 (" + String(FIRMWARE_VERSION) + ")</p>";
  h += "</header>";

  h += "<div class='container'>";
  h += "<div id='status'>⏳ 正在下載全港車站資料庫...</div>";
  
  h += "<div class='card'>";
  h += "<div class='card-title'>📶 系統與網絡設定</div>";
  
  // 🌟 Web UI 增加運行時間顯示
  h += "<div class='form-group'>";
  h += "<label>系統運行時間</label>";
  h += "<div class='status-badge' style='background:#E3F2FD; color:#2980B9;'>⏱️ " + getUptimeString() + "</div>";
  h += "</div>";

  h += "<div class='form-group'>";
  h += "<label>目前網絡狀態</label>";
  h += "<div class='status-badge'>● " + (wifiReady ? String("已連線到 Wi-Fi (IP: ") + WiFi.localIP().toString() + ")" : String("AP 配網模式 (http://Bus-ETA.local)")) + "</div>";
  h += "</div>";
  h += "<div class='row'>";
  h += "<button type='button' class='btn-reboot' onclick='if(confirm(\"確定要重新啟動 ESP32 裝置嗎？\")) location.href=\"/reboot\"'>🔄 重新啟動裝置</button>";
  h += "<button type='button' class='btn-danger' onclick='if(confirm(\"確定要清除所有 WiFi 紀錄並重啟嗎？\")) location.href=\"/reset_wifi\"'>⚠️ 清除 Wi-Fi 紀錄</button>";
  h += "</div>";
  h += "</div>";

  h += "<form method='POST' action='/save'>";

  if(!wifiReady) {
      h += "<div class='card'>";
      h += "<div class='card-title'>🔗 連接新 Wi-Fi 網絡</div>";
      h += "<div class='form-group'><label>Wi-Fi 名稱 (SSID)</label><input name='wifi_ssid' placeholder='例如: Home_WiFi' value='" + htmlEscape(wifiSsid) + "'></div>";
      h += "<div class='form-group'><label>Wi-Fi 密碼</label><input name='wifi_pass' placeholder='請輸入無線網絡密碼' type='password' value='" + htmlEscape(wifiPass) + "'></div>";
      h += "</div>";
  } else {
      h += "<input type='hidden' name='wifi_ssid' value='" + htmlEscape(wifiSsid) + "'>";
      h += "<input type='hidden' name='wifi_pass' value='" + htmlEscape(wifiPass) + "'>";
  }

  h += "<div class='card'>";
  h += "<div class='card-title'>🎨 螢幕顯示與外觀風格</div>";
  h += "<div class='form-group'><label>外觀主題配色</label><select name='theme'>";
  h += "<option value='dark' " + String(uiTheme=="dark"?"selected":"") + ">深空黑魂 (Premium Dark)</option>";
  h += "<option value='kmb' " + String(uiTheme=="kmb"?"selected":"") + ">經典九巴紅 (Classic KMB Red)</option>";
  h += "</select></div>";
  
  h += "<div class='form-group'><label>螢幕背光亮度</label><div style='display:flex; gap:12px; align-items:center;'>";
  h += "<input type='range' name='brightness' min='10' max='100' value='" + String(currentBrightness) + "' style='flex:1' oninput='document.getElementById(\"br_val\").innerText=this.value+\"%\"'>";
  h += "<span id='br_val' style='font-weight:700; min-width:48px; text-align:right; color:var(--primary);'>" + String(currentBrightness) + "%</span></div></div>";

  h += "<div class='form-group'><label>顯示車站名稱</label><select name='show_stop'>";
  h += "<option value='1' " + String(showStopName ? "selected" : "") + ">顯示 (目的地 + 所在車站名稱)</option>";
  h += "<option value='0' " + String(!showStopName ? "selected" : "") + ">隱藏 (使用特大字體僅顯示目的地)</option>";
  h += "</select></div>";

  h += "<div class='form-group'><label>巴士到站資訊頁面</label><select name='show_bus'>";
  h += "<option value='1' " + String(showBusPage ? "selected" : "") + ">開啟 (輪播顯示巴士路線到站時間)</option>";
  h += "<option value='0' " + String(!showBusPage ? "selected" : "") + ">關閉 (隱藏巴士資訊頁面)</option>";
  h += "</select></div>";

  h += "<div class='form-group'><label>香港天文台天氣專頁</label><select name='show_wth'>";
  h += "<option value='1' " + String(showWeatherPage ? "selected" : "") + ">開啟 (定時切換至天氣與預報頁面)</option>";
  h += "<option value='0' " + String(!showWeatherPage ? "selected" : "") + ">關閉 (隱藏天氣專頁)</option>";
  h += "</select></div></div>";

  h += "<div class='card'><div class='card-title'>⚙️ 數據更新與翻頁頻率</div>";
  h += "<div class='form-group'><label>實時到站數據刷新頻率</label><select name='refresh'>";
  int secs = refreshMs / 1000;
  for (int v: {15,30,60}) h += String("<option value='") + v + "'" + (secs==v?" selected":"") + ">每 " + v + " 秒更新一次</option>";
  h += "</select></div>";
  
  h += "<div class='form-group'><label>自動輪播翻頁速度</label><select name='page_int'>";
  int psecs = pageIntervalMs / 1000;
  for (int v: {5,10,15,30}) h += String("<option value='") + v + "'" + (psecs==v?" selected":"") + ">每 " + v + " 秒切換下一頁</option>";
  h += "</select></div></div>";

  for (int i = 0; i < TOTAL_ROUTES; i++) {
    String p = "r" + String(i);
    h += "<div class='card'><div class='route-header'>";
    h += "<span style='font-size:16px; font-weight:700; color:#2C3E50;'>🚏 巴士路線 " + String(i+1) + "</span>";
    if (routes[i].route.length() > 0) h += "<span class='route-badge'>" + htmlEscape(routes[i].company) + " " + htmlEscape(routes[i].route) + "</span>";
    h += "</div>";
    
    h += "<div class='form-group'><label>專營巴士公司</label><select id='"+p+"co' name='"+p+"co' onchange='toggleJoint("+String(i)+")'>";
    h += "<option value='KMB'" + String(routes[i].company=="KMB"?" selected":"") + ">九巴 (KMB)</option>";
    h += "<option value='LWB'" + String(routes[i].company=="LWB"?" selected":"") + ">龍運 (LWB)</option>";
    h += "<option value='CTB'" + String(routes[i].company=="CTB"?" selected":"") + ">城巴 (Citybus)</option>";
    h += "<option value='JOINT'" + String(routes[i].company=="JOINT"?" selected":"") + ">聯營路線 (九巴 + 城巴)</option>";
    h += "</select></div>";

    h += "<div class='row'><div class='form-group'><label>路線號碼</label><input id='"+p+"route' name='"+p+"route' value='" + htmlEscape(routes[i].route) + "' placeholder='例: 968 (留空隱藏)'></div>";
    h += "<div class='form-group'><label>行車方向</label><select id='"+p+"bound' name='"+p+"bound'>";
    h += "<option value='outbound'" + String(routes[i].bound=="outbound"?" selected":"") + ">去程 (Outbound)</option>";
    h += "<option value='inbound'" + String(routes[i].bound=="inbound"?" selected":"") + ">回程 (Inbound)</option>";
    h += "</select></div></div>";

    h += "<div class='form-group'><label>服務班次類型</label><select id='"+p+"svc' name='"+p+"svc'>";
    h += "<option value='1' " + String(routes[i].serviceType=="1"?"selected":"") + ">常規服務班次</option>";
    h += "<option value='2' " + String(routes[i].serviceType!="1"?"selected":"") + ">特別服務班次</option>";
    h += "</select></div>";
    
    h += "<button type='button' class='btn-sec' id='btn"+String(i)+"' onclick='loadStops(" + String(i) + ")'>🔍 搜尋車站名稱</button>";
    
    h += "<div class='form-group'><label>指定上車車站</label><select id='"+p+"stop' name='"+p+"stop'>";
#if USE_CHINESE
    h += "<option value='" + htmlEscape(routes[i].stopId + "|" + routes[i].stopNameEn + "|" + routes[i].stopNameTc) + "'>目前設定: " + (routes[i].stopNameTc.length()>0 ? htmlEscape(routes[i].stopNameTc) : "尚未設定") + "</option>";
#else
    h += "<option value='" + htmlEscape(routes[i].stopId + "|" + routes[i].stopNameEn + "|" + routes[i].stopNameTc) + "'>目前設定: " + (routes[i].stopNameEn.length()>0 ? htmlEscape(routes[i].stopNameEn) : "Not Set") + "</option>";
#endif
    h += "</select></div>";
    
    h += "<div id='"+p+"stop2_div' style='display:none; margin-top:10px;' class='form-group'>";
    h += "<label style='color:#0055A4'>城巴車站對應 (聯營路線專用)</label><select id='"+p+"stop2' name='"+p+"stop2'>";
    h += "<option value='" + htmlEscape(routes[i].stopId2) + "'>目前城巴車站代碼: " + (routes[i].stopId2.length()>0 ? htmlEscape(routes[i].stopId2) : "無") + "</option>";
    h += "</select></div></div>";
  }

  h += "<div class='submit-btn-wrapper'><button type='submit' class='btn-submit'>💾 儲存所有設定並傳送至顯示器</button></div>";
  h += "</form></div></body></html>";
  return h;
}

static void startWebServer() {
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html; charset=utf-8", settingsPage()); });
  
  server.on("/reboot", HTTP_GET, [](){
      String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width'><style>body{font-family:sans-serif;text-align:center;margin-top:60px;color:#333;background:#F4F6F9} h1{color:#3498DB}</style></head><body><h1>🔄 正在重新啟動顯示器</h1><p>ESP32 核心正在重啟中，請稍候 8 秒將自動回到控制台...</p><script>setTimeout(()=>{location.href='/';}, 8000);</script></body></html>";
      server.send(200, "text/html; charset=utf-8", html);
      delay(1000);
      ESP.restart();
  });

  server.on("/reset_wifi", HTTP_GET, [](){ 
      prefs.putString("wifi_ssid", ""); 
      prefs.putString("wifi_pass", ""); 
      String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width'><style>body{font-family:sans-serif;text-align:center;margin-top:60px;color:#333;background:#F4F6F9} h1{color:#E74C3C}</style></head><body><h1>🔄 Wi-Fi 紀錄已完全清除</h1><p>裝置正在重啟，請使用手機連線至 「HKBusETA-Setup」 無線網絡進行配網。</p></body></html>";
      server.send(200, "text/html; charset=utf-8", html);
      delay(1000);
      ESP.restart();
  });

  server.on("/save", HTTP_POST, [](){
    wifiSsid = server.arg("wifi_ssid"); wifiPass = server.arg("wifi_pass");
    prefs.putString("wifi_ssid", wifiSsid); prefs.putString("wifi_pass", wifiPass);
    saveRoutesFromWeb();
    needFullRefresh = true; 
    String successHtml = "<!doctype html><html><head><meta name='viewport' content='width=device-width'><style>body{font-family:sans-serif;text-align:center;margin-top:60px;background:#F4F6F9;color:#333} h1{color:#27AE60} a{display:inline-block;padding:12px 28px;background:#B00020;color:#fff;text-decoration:none;border-radius:10px;font-weight:700;margin-top:20px}</style></head><body><h1>✅ 設定已成功更新</h1><p>最新資料已傳送至 ESP32，顯示器將會即時套用！</p><a href='/'>返回設定頁面</a></body></html>";
    server.send(200, "text/html; charset=utf-8", successHtml);
  });
  server.begin();
}

// ===================== WiFi =====================
static void startAP() {
  apMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  if (MDNS.begin("Bus-ETA")) {
      Serial.println("MDNS started at http://Bus-ETA.local");
  }
}

static void connectWiFi() {
  if (wifiSsid.length() == 0) {
    startAP();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(300); Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    Serial.print("WiFi IP: "); Serial.println(WiFi.localIP());
    if (MDNS.begin("Bus-ETA")) {
        Serial.println("MDNS started at http://Bus-ETA.local");
    }
    fetchWeather();
  } else {
    startAP();
  }
}

// ===================== Arduino Setup & Loop =====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("HKBusETA v1.6.3 - Settings Optimized & ETA Sort Fixed");

  static esp_lcd_panel_handle_t panel_handle = NULL;
  static esp_lcd_touch_handle_t tp_handle = NULL;
  DEV_I2C_Init();
  IO_EXTENSION_Init();
  tp_handle = touch_gt911_init(DEV_I2C_Get_Bus_Device());
  panel_handle = waveshare_esp32_s3_rgb_lcd_init();
  waveshare_rgb_lcd_bl_on();
  ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

  loadConfig();
  
  IO_EXTENSION_Pwm_Output(100 - currentBrightness);

  if (lvgl_port_lock(-1)) {
      createBusScreen();
      lvgl_port_unlock();
  }

  connectWiFi();
  startWebServer();

  if (wifiReady) {
    configTime(28800, 0, "pool.ntp.org", "stdtime.gov.hk");
  }
  refreshDisplayAll();
}

void loop() {
  server.handleClient();
  
  if (millis() - lastClockMs >= 1000) {
    lastClockMs = millis();
    updateClockOnly();
  }
  
  if (forceThemeUpdate) {
      forceThemeUpdate = false;
      refreshDisplayAll();
  }
  
  if (needFullRefresh) {
      needFullRefresh = false;
      currentPage = 0; 
      lastPageSwitchMs = millis();
      lastFetchMs = 0;           
  }
  
  if (millis() - lastPageSwitchMs >= pageIntervalMs) {
    lastPageSwitchMs = millis(); 
    
    int activeIdxs[TOTAL_ROUTES];
    int activeCount = getActiveRoutes(activeIdxs);
    int itemsPerPage = (activeCount <= 3) ? 3 : 4;
    
    int busPages = 0;
    if (showBusPage) {
        busPages = (activeCount > 0) ? (activeCount + itemsPerPage - 1) / itemsPerPage : 1;
    }
    
    int totalPages = busPages + (showWeatherPage ? 1 : 0);
    if (totalPages == 0) { busPages = 1; totalPages = 1; } 
    
    if (totalPages > 1) {
      currentPage = (currentPage + 1) % totalPages;
      refreshDisplayAll();
    }
  }

  if (wifiReady && (lastFetchMs == 0 || millis() - lastFetchMs >= refreshMs)) {
    lastFetchMs = millis();
    fetchAllEta();
  }

  if (wifiReady && (lastWeatherFetchMs == 0 || millis() - lastWeatherFetchMs >= 300000UL)) {
    lastWeatherFetchMs = millis();
    fetchWeather();
    refreshDisplayAll(); 
  }
  
  delay(10);
}