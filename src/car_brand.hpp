#pragma once
namespace acc {
// ACC Server Admin Handbook 1.10.2, IX.3 (PC model IDs).
inline const char* carBrand(int model){switch(model){
 case 0:case 9:case 23:case 28:case 34:case 61:case 85:case 86:return "porsche";
 case 1:case 25:case 60:case 84:return "mercedes-benz";
 case 2:case 24:case 26:case 32:return "ferrari";
 case 3:case 19:case 31:case 52:case 80:return "audi";
 case 4:case 13:case 16:case 18:case 29:case 33:return "lamborghini";
 case 5:case 22:case 35:case 59:return "mclaren";
 case 6:case 10:return "nissan";
 case 7:case 27:case 30:case 53:return "bmw";
 case 8:case 11:return "bentley";
 case 12:case 20:case 51:return "aston-martin";
 case 14:return "jaguar";
 case 15:return "lexus";
 case 17:case 21:return "honda";
 case 36:return "ford";
 case 50:return "alpine";
 case 55:return "chevrolet";
 case 56:return "ginetta";
 case 57:case 82:return "ktm";
 case 58:case 83:return "maserati";
 default:return "";
}}
}
